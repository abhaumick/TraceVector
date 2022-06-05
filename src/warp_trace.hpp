


#pragma once

#ifndef WARP_TRACE_HPP
#define WARP_TRACE_HPP

#include <cstddef>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>

// #define TRACE_WARN_OUTOFBOUNDS_ENABLE

#define TRACE_PAGE_SIZE ((10))
#define TRACE_PAGE_BUFFER_SIZE ((10))

#ifndef _dim3
typedef struct {
  unsigned x, y, z;
} _dim3;
#endif

template <typename T>
class warp_trace {
public:
  typedef typename std::vector <T> page_type;
  typedef uint64_t addr_type;

  typedef struct __attribute__((packed)) {
    unsigned _id;
    addr_type size;
  } bytes_header;

  warp_trace (unsigned w_id, unsigned n_instr, addr_type page_size = 0);
  ~warp_trace();

  /**
   * @brief 
   * 
   * @param index 
   * @return T& reference to T object at `index` location
   */
  T& at(unsigned index);
  T& operator[] (unsigned index);

  constexpr unsigned id(void) const { return _id; }
  constexpr addr_type size(void) const { return _size; }
  void set_tb_id(_dim3 tb_id) { _tb_id = tb_id; }
  std::string get_id(void) const;

  int init (std::ifstream& handle);
  int init (const std::string& handle, size_t file_offset);

  int to_bytes(std::vector <unsigned char>& v) const;
  
  template <typename T2> 
  friend int pack(T2 value, std::vector <unsigned char>& v);

  friend std::ostream& operator<<(std::ostream& o, const warp_trace<T>& wt);

  unsigned _page_size;
  unsigned _max_pages;
  unsigned _buffer_size;
  std::map <addr_type, addr_type> page_map;
  std::set <addr_type> page_set;
  std::vector <addr_type> tag_array;
  std::ifstream* file_handle;
  std::string file_path;
  std::vector <page_type> page_buffer;
  unsigned LRU, MRI;
  unsigned trace_version;
  bool private_file_handle;

protected:
  unsigned _id;
  _dim3 _tb_id;
  addr_type _size;

  int fetch_page(int page_num, int index);
  int evict_page(int index);
  void update_lru(int index);

};

template <typename T>
warp_trace<T>::warp_trace (unsigned w_id, unsigned n_instr, addr_type page_size) :
  _id(w_id),
  _size(n_instr),
  _page_size(page_size),
  LRU(0),
  private_file_handle(false),
  file_handle(nullptr)
{
  if (page_size == 0)
    _page_size = TRACE_PAGE_SIZE;
  _max_pages = (n_instr + _page_size - 1) / _page_size; 
  _buffer_size = std::min(_max_pages, (unsigned) TRACE_PAGE_BUFFER_SIZE);
}

template <typename T>
warp_trace<T>::~warp_trace () {
  if (private_file_handle) {
    if (file_handle != nullptr) {
      if (file_handle->is_open()) {
        file_handle->close();
      }
      delete file_handle;
    }
  }
  page_map.clear();
  page_buffer.clear();
  page_set.clear();
  file_handle = nullptr;
}


template <typename T2> 
inline int pack(T2 value, std::vector <unsigned char>& v) {
  for (auto i = 0U; i < sizeof(T2); ++ i) {
    v.push_back((unsigned char) (value & 0x00FF));
    value = value >> 8;
  }
  return 0;
}

template <typename T>
std::string warp_trace<T>::get_id(void) const {
  std::string str;
  std::stringstream ss(str);
  ss << " tb " << _tb_id.x << "," << _tb_id.y << "," << _tb_id.z << " ";
  ss << " w " << _id << " ";
  return ss.str();
}

/**
 * @brief 
 * 
 * @tparam T 
 * @param handle 
 * @return int 
 */
template <typename T>
int warp_trace<T>::init(std::ifstream& handle) {
  file_handle = &handle;

  page_buffer.reserve(_buffer_size);
  tag_array.resize(_buffer_size);
  for (auto page_id = 0U; page_id < _buffer_size; ++ page_id) {
    page_buffer.emplace_back();
    page_buffer[page_id].reserve(_page_size);
    auto retVal = fetch_page(page_id, page_id);
    assert((retVal == 0) && "Trace Page Fetch Failed");
  }
  return 0;
}

template <typename T>
int warp_trace<T>::init(const std::string& file_path, size_t file_offset) {
  private_file_handle = true;
  file_handle = new std::ifstream();
  file_handle->open(file_path.c_str());

  if (! file_handle->is_open()) {
    std::cout << "Unable to open file " << file_path.c_str() << " @ "
      << file_offset << " \n";
      // << std::filesystem::current_path() << "\n";
    return -1;
  }

  if (file_handle->is_open()) {
    this->file_path = file_path;
    // Seek to file_offset
    file_handle->seekg(file_offset, file_handle->beg);

    page_buffer.reserve(_buffer_size);
    tag_array.resize(_buffer_size);
    for (auto page_id = 0U; page_id < _buffer_size; ++ page_id) {
      page_buffer.emplace_back();
      page_buffer[page_id].reserve(_page_size);
      auto retVal = fetch_page(page_id, page_id);
      assert((retVal == 0) && "Trace Page Fetch Failed");
    }
    return 0;
  }
  return -1;
}

template <typename T>
T& warp_trace<T>::at(unsigned index) {
  assert((index < _size) && "Warp Trace Index OutOfBounds");
  uint64_t at_tag = index / _page_size;
  uint64_t at_offset = index % _page_size;

  auto search_idx = page_set.find(at_tag);
  if (search_idx != page_set.end()) {
    // In Buffer
    auto buffer_idx = 0;
    for (auto tag : tag_array) {
      if (tag == at_tag) {
        return page_buffer[buffer_idx][at_offset];
      }
      ++ buffer_idx;
    }
    assert(0 && "Found in PageSet but not in TagArray");
  }

  #ifdef TRACE_WARN_OUTOFBOUNDS_ENABLE
    std::cout << "OutOfBounds \n";
  #endif

  // Remove LRU
  auto buffer_idx = LRU;
  evict_page(LRU);

  // Fetch page
  fetch_page(at_tag, buffer_idx);

  update_lru(buffer_idx);
  // New page will always be at buffer_idx
  return page_buffer[buffer_idx][at_offset];
}

template <typename T>
T& warp_trace<T>::operator[] (unsigned index) {
  return at(index);
}

template <typename T>
int warp_trace<T>::fetch_page(int page_tag, int index) {
  // Check if already present
  auto search_idx = page_set.find(page_tag);
  if ( search_idx != page_set.end() ) {
    return -(std::distance(page_set.begin(), search_idx));
  }

  // Fetch page from file
  auto& page = page_buffer[index]; 
  page.clear();
  auto file_loc = page_map[page_tag];
  file_handle->seekg(file_loc, std::ios::beg);

  auto fetch_size = std::min((addr_type) _page_size, _size - (page_tag * _page_size));
  for (auto i = 0U; i < fetch_size; ++ i) {
    page.emplace_back();
    std::getline(*file_handle, page[i]);
    if (page[i].empty()) {
      // assert(0 && "getline empty line");
      if(file_handle->fail() || file_handle->bad()) {
        std::cout << " ERROR !!! \n\n\n" << std::flush;
        // page[i].clear();
        // file_handle->clear();
        // i = 0;
      }
    }
  }

  // Check full page empty
  bool empty_page = true;
  for (auto line : page_buffer[index]) {
    if (! line.empty())
      empty_page = false;
  }
  if (empty_page)
    std::cout << "Empty PAGE !!!! \n\n\n" << std::flush;
  
  // Update page_set, LRU
  page_set.insert(page_tag); 
  tag_array[index] = page_tag;
  MRI = index;
  return 0;
}

template <typename T>
int warp_trace<T>::evict_page(int index) {
  auto page_tag = tag_array[index]; 
  page_set.erase(page_tag);
  return 0;
}

template <typename T>
void warp_trace<T>::update_lru(int index) {
  LRU = (LRU + 1) % _buffer_size;
}

template <typename T>
int warp_trace<T>::to_bytes(std::vector <unsigned char>& v) const {
  auto estimated_size = (8 + 2 * page_map.size()) * sizeof(addr_type);
  std::cout << "estimated_size = " << estimated_size << "\n";
  v.reserve(v.size() + estimated_size);

  pack(_id, v);
  pack(_size, v);
  pack(_page_size, v);
  pack(_max_pages, v);
  pack(_buffer_size, v);
  pack(LRU, v);
  pack(MRI, v);

  for(auto& item : page_map) {
    pack(item.first, v);
    pack(item.second, v);
  }

  // Only for complete serialization
  // for(auto& item : tag_array) {
  //   pack(item,v);
  // }
  return 0;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, warp_trace<T>& wt) {
  os << "Warp " << wt.id() << " with " << wt.size() << " instructions. \n";
  os << wt._buffer_size << " pages of size " << wt._buffer_size;
  os << " x " << typeid(T).name() << "\n";
  os << "Page Map: \n";
  for (auto idx = 0U; idx < wt._buffer_size; ++ idx) {
    os << " " << idx << " -> " << wt.tag_array[idx];
    if (idx == wt.LRU) os << " <- LRU ";
    os << " , ";
  }
  os << std::endl;
  return os;
}


#endif  // WARP_TRACE_HPP
