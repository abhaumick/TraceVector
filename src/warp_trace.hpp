


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

#define TRACE_PAGE_SIZE ((4))
#define TRACE_PAGE_BUFFER_SIZE ((10))

template <typename T>
class warp_trace {
public:
  typedef typename std::vector <T> page_type;

  warp_trace (unsigned w_id, unsigned n_instr) :
    warp_id(w_id),
    num_instrs(n_instr)
  {
    page_size = TRACE_PAGE_SIZE;
    max_pages = (n_instr + TRACE_PAGE_SIZE - 1) / TRACE_PAGE_SIZE; 
    buffer_size = std::min(max_pages, (unsigned) TRACE_PAGE_BUFFER_SIZE);
  }

  ~warp_trace () {
    page_map.clear();
    page_buffer.clear();
    page_avail.clear();
    file_handle = nullptr;
  }

  int init (std::ifstream& handle)
  {
    file_handle = &handle;

    page_buffer.reserve(buffer_size);
    for (auto i = 0; i < buffer_size; ++ i) {
      page_buffer.emplace_back();
      fetch_page(i, i);
    }
    return 0;
  }

  int fetch_page(int page_num, int index);
  int evict_page(int index);

  friend std::ostream& operator<<(std::ostream& o, const warp_trace<T>& wt);

  unsigned warp_id;
  unsigned num_instrs;
  unsigned page_size;
  unsigned max_pages;
  unsigned buffer_size;
  std::map <size_t, size_t> page_map;
  std::set <size_t> page_avail;
  std::ifstream* file_handle;
  std::vector <page_type> page_buffer;
  int LRU, MRI;

  T& at(int index);
};

template <typename T>
T& warp_trace<T>::at(int index) {
  int at_page = index / page_size;
  int at_offset = index % page_size;

  auto search_idx = page_avail.find(at_page);
  if (search_idx != page_avail.end()) {
    // In Buffer
    auto page_idx = std::distance(search_idx, page_avail.begin());
    return page_buffer[page_idx][at_offset];
  }

  std::string a;
  return a;
}

template <typename T>
int warp_trace<T>::fetch_page(int page_num, int index) {
  // Check if already present
  auto search_idx = page_avail.find(page_num);
  if ( search_idx != page_avail.end() ) {
    return -(std::distance(page_avail.begin(), search_idx));
  }

  // Fetch page from file
  auto& page = page_buffer[index]; 
  page.clear();
  auto file_loc = page_map[page_num];
  file_handle->seekg(file_loc);

  for (auto i = 0; i < TRACE_PAGE_SIZE; ++ i) {
    page.emplace_back();
    std::getline(*file_handle, page[i]);
  }
  
  // Update page_avail set
  page_avail.insert(page_num);
  MRI = index;
  return 0;
}


#endif  // WARP_TRACE_HPP
