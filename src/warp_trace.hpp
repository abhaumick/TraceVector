


#pragma once

#ifndef WARP_TRACE_HPP
#define WARP_TRACE_HPP

#include <cstddef>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <set>

// #define TRACE_WARN_OUTOFBOUNDS_ENABLE

#define TRACE_PAGE_SIZE ((10))
#define TRACE_PAGE_BUFFER_SIZE ((10))

template <typename T>
class warp_trace {
public:
  typedef typename std::vector <T> page_type;
  typedef uint64_t addr_type;

  typedef struct __attribute__((packed)) {
    unsigned _id;
    addr_type size;
  } bytes_header;

  warp_trace (unsigned w_id, unsigned n_instr);
  ~warp_trace();

  /**
   * @brief 
   * 
   * @param index 
   * @return T& reference to T object at `index` location
   */
  T& at(int index);
  T& operator[] (int index);

  constexpr unsigned id(void) const { return _id; }
  constexpr addr_type size(void) const { return _size; }

  int init (std::ifstream& handle);

  int to_bytes(std::vector <unsigned char>& v) const;
  
  template <typename T2> 
  friend int pack(T2 value, std::vector <unsigned char>& v);

  friend std::ostream& operator<<(std::ostream& o, const warp_trace<T>& wt);

  unsigned page_size;
  unsigned max_pages;
  unsigned buffer_size;
  std::map <addr_type, addr_type> page_map;
  std::set <addr_type> page_set;
  std::vector <addr_type> tag_array;
  std::ifstream* file_handle;
  std::vector <page_type> page_buffer;
  int LRU, MRI;

protected:
  unsigned _id;
  addr_type _size;

  int fetch_page(int page_num, int index);
  int evict_page(int index);
  void update_lru(int index);

};

template <typename T>
warp_trace<T>::warp_trace (unsigned w_id, unsigned n_instr) :
  _id(w_id),
  _size(n_instr),
  LRU(0)
{
  page_size = TRACE_PAGE_SIZE;
  max_pages = (n_instr + TRACE_PAGE_SIZE - 1) / TRACE_PAGE_SIZE; 
  buffer_size = std::min(max_pages, (unsigned) TRACE_PAGE_BUFFER_SIZE);
}

template <typename T>
warp_trace<T>::~warp_trace () {
  page_map.clear();
  page_buffer.clear();
  page_set.clear();
  file_handle = nullptr;
}


template <typename T2> 
inline int pack(T2 value, std::vector <unsigned char>& v) {
  #pragma unroll
  for (auto i = 0; i < sizeof(T2); ++ i) {
    v.push_back((unsigned char) (value & 0x00FF));
    value = value >> 8;
  }
  return 0;
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

  page_buffer.reserve(buffer_size);
  tag_array.resize(buffer_size);
  for (auto page_id = 0; page_id < buffer_size; ++ page_id) {
    page_buffer.emplace_back();
    auto retVal = fetch_page(page_id, page_id);
    assert((retVal == 0) && "Trace Page Fetch Failed");
  }
  return 0;
}

template <typename T>
T& warp_trace<T>::at(int index) {
  assert((index < _size) && "Warp Trace Index OutOfBounds");
  int at_tag = index / page_size;
  int at_offset = index % page_size;

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
T& warp_trace<T>::operator[] (int index) {
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

  auto fetch_size = std::min((addr_type) page_size, _size - (page_tag - 1) * page_size);
  for (auto i = 0; i < page_size; ++ i) {
    page.emplace_back();
    std::getline(*file_handle, page[i]);
  }
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
  LRU = (LRU + 1) % buffer_size;
}

template <typename T>
int warp_trace<T>::to_bytes(std::vector <unsigned char>& v) const {
  auto estimated_size = (8 + 2 * page_map.size()) * sizeof(addr_type);
  std::cout << "estimated_size = " << estimated_size << "\n";
  v.reserve(v.size() + estimated_size);

  pack(_id, v);
  pack(_size, v);
  pack(page_size, v);
  pack(max_pages, v);
  pack(buffer_size, v);
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
  os << wt.buffer_size << " pages of size " << wt.buffer_size;
  os << " x " << typeid(T).name() << "\n";
  os << "Page Map: \n";
  for (auto idx = 0; idx < wt.buffer_size; ++ idx) {
    os << " " << idx << " -> " << wt.tag_array[idx];
    if (idx == wt.LRU) os << " <- LRU ";
    os << " , ";
  }
  os << std::endl;
  return os;
}


#endif  // WARP_TRACE_HPP
