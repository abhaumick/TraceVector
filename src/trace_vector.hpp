/**
 * @file trace_vector.hpp
 * @author Abhishek Bhaumick (abhaumic@purdue.edu)
 * @brief 
 * @version 0.1
 * @date 2022-03-31
 * 
 * 
 */

#pragma once

#ifndef TRACE_VECTOR_HPP
#define TRACE_VECTOR_HPP

#include <cstddef>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
// #include <filesystem>
#include <string>
#include <map>
#include <set>

#include "warp_trace.hpp"

enum class page_type_t{
  PAGE_RAW,
  PAGE_STRING
};

enum class line_type {
  EMPTY,
  TB_START,
  TB_DIM,
  TB_END,
  WARP_ID,
  WARP_INSTS,
  INSTR
};




class tb_trace_old {
public:
  unsigned tb_id_x;
  unsigned tb_id_y;
  unsigned tb_id_z;
  std::vector <warp_trace <std::string>*> warps;
  size_t file_start;
  size_t file_end;
};

  /*
    From Linux Page Table Entry format
    _PAGE_PRESENT	  Page is resident in memory and not swapped out
    _PAGE_PROTNONE  Page is resident but not accessable
    _PAGE_RW        Set if the page may be written to
    _PAGE_USER      Set if the page is accessible from user space
    _PAGE_DIRTY     Set if the page is written to
    _PAGE_ACCESSED  Set if the page is accessed
  */
  class page_entry {
  public:
    ~page_entry() {
      if (page_present)
        delete page_ptr;
    }

    size_t page_size;
    page_type_t page_type;
    std::vector<std::string> * page_ptr;
    // Indices
    size_t vector_start;
    size_t vector_end;
    size_t file_start;
    size_t file_end;
    // Flags
    bool page_present;
    bool page_rw;
    bool page_dirty;
    bool page_accessed;
  };


template <class T>
class trace_vector {
public:


  trace_vector() :
    size(10),
    page_size(4) {
    size = 10;
    page_size = 4;
    this->page_buffer.reserve(size);
  };

  ~trace_vector() {
    this->delete_page_buffer();
    // if (this->file_stream->is_open()) {
    //   this->file_stream->close();
    // }
  }

  int init(const std::string& filePath, size_t file_offset = 0);
  std::string& at(size_t index);

  int map_tb_to_file(std::ifstream & handle);

  tb_trace_old& get_tb_trace(void) {
    return tb_map;
  }

protected:


  int setBackingFile(const std::string & file_path);
  int setBackingFile(const std::ifstream & file_handle);


  int insert_page(unsigned insert_loc, size_t vector_start, 
    std::ifstream & handle, size_t file_offset = 0);
  
  int remove_page(unsigned loc);
  int delete_page_buffer();



private:
  bool is_initialized;
  std::ifstream * file_stream;
  size_t size;
  int MRI;
  int LRI;
  int page_size;
  std::vector <page_entry> page_buffer;
  tb_trace_old tb_map;
};



template <typename T> 
int trace_vector<T>::init(const std::string& file_path, size_t file_offset) {
  
  std::ifstream file_handle;
  file_handle.open(file_path.c_str());

  if (! file_handle.is_open()) {
    std::cout << "Unable to open file " << file_path.c_str() << " @ " ;
      // << std::filesystem::current_path() << "\n";
    return -1;
  }
  else {
    // Seek to file_offset
    file_handle.seekg(file_offset, file_handle.beg);
    file_stream = & file_handle;

    // Create page_buffer
    for (auto idx = 0; idx < this->size; ++ idx) {
      this->page_buffer.emplace_back();
    }

    // Map File
    this->map_tb_to_file(file_handle);

    file_handle.seekg(0);

    // Read in first pages into page_buffer
    size_t vector_start = 0;
    for (auto idx = 0; idx < this->size; ++ idx) {
      auto retVal = insert_page(idx, vector_start, file_handle);
      vector_start = this->page_buffer[idx].vector_end + 1;
    }
  }
  return 0;
}


template <typename T>
int trace_vector<T>::insert_page(unsigned loc, size_t vector_start, 
  std::ifstream & handle, size_t file_offset) {
  // Create new empty page
  std::vector <std::string> * new_page = new std::vector <std::string>;
  new_page->reserve(this->page_size);
  
  // Temp buffer 
  std::string line;
  int line_count = 0;

  if (file_offset != 0) {
    handle.seekg(file_offset, handle.beg);
  }
  auto file_start = handle.tellg();
  
  for (auto idx = 0; idx < this->page_size; ++ idx) {
    if (! handle.eof()) {
      // Replace with parser
      std::getline(handle, line);
      ++ line_count;

      new_page->emplace_back(line);
    }
  }
  auto file_end = handle.tellg();
  
  // Populate Page Entry
  page_entry & pte = this->page_buffer[loc];
  if (pte.page_present) {
    remove_page(loc);
  }
  pte.file_start    = file_start;
  pte.file_end      = file_end;
  pte.vector_start  = vector_start;
  pte.vector_end    = line_count - 1;
  pte.page_accessed = false;
  pte.page_dirty    = false;
  pte.page_present  = true;
  pte.page_ptr      = new_page;
  pte.page_rw       = false;
  pte.page_size     = line_count;
  pte.page_type     = page_type_t::PAGE_STRING;


  this->MRI = loc;

  return 0;
}

template <typename T>
int trace_vector<T>::remove_page(unsigned loc) {
  //  Check buffer_size
  if (this->page_buffer.size() >= loc) {
    // Check pte
    page_entry & pte = this->page_buffer[loc];
    if (pte.page_present) {
      delete pte.page_ptr;      //  Deallocate page
      pte.page_size = 0;
      pte.page_present = false;
    }
  }
  return 0;
}

template <typename T>
int trace_vector<T>::delete_page_buffer() {
  for (auto idx = 0U; idx < this->page_buffer.size(); ++ idx) {
    remove_page(idx);
  }
  return 0;
}

template <typename T>
std::string& trace_vector<T>::at(size_t index) {
  // Check if present in buffer
  for (auto& pte : this->page_buffer) {
    if (index >= pte.vector_start && index <= pte.vector_end) {
      auto vector_offset = index - pte.vector_start;
      auto& page_vector = *(pte.page_ptr);
      return page_vector[vector_offset];
    }
  }
  // Not Found
  
    // Translate to file_offset
    auto line_offset = index;
    auto page_id = index / page_size;  
    // Find victim page

  std::string * a = new std::string;
  return *a;
}


template <typename T>
int trace_vector<T>::map_tb_to_file(std::ifstream & handle) {
  bool start_of_tb_stream_found = false;
  unsigned instr_idx;
  unsigned warp_id;
  unsigned num_instrs;
  size_t line_start = 0;
  size_t line_end = 0;
  tb_trace_old &t = this->tb_map;
  t.warps.clear();

  std::stringstream ss;
  std::string line, word1, word2, word3, word4;

  while (! handle.eof()) {
    // line_start = handle.tellg();
    std::getline(handle, line);
    line_end = line_end + line.size() + 1;
    ss.clear();

    if (line.length() == 0) {
      line_start = line_start + 1;
      continue;
    }
    else {
      ss.str(line);
      ss >> word1 >> word2 >> word3 >> word4;
      if (word1 == "#BEGIN_TB") {
        if (!start_of_tb_stream_found) {
          start_of_tb_stream_found = true;
          t.file_start = line_start;
          // std::cout << " TB Start \n";
        } 
        else {
          assert(0 && "Parsing error: thread block start before "  
            "the previous one finishes");
          t.file_end = line_end;
        }
      } 
      else if (word1 == "#END_TB") {
        assert(start_of_tb_stream_found);
        t.file_end = line_end;
        // std::cout << line << " TB End \n";
        break;  // end of TB stream
      } 
      else if (word1 == "thread" && word2 == "block") {
        assert(start_of_tb_stream_found);
        // std::cout << "tb dim \n";
        #ifdef __linux__
          sscanf(line.c_str(), "thread block = %d,%d,%d", &t.tb_id_x, &t.tb_id_y, &t.tb_id_z);
        #elif _WIN32
          sscanf_s(line.c_str(), "thread block = %d,%d,%d", &t.tb_id_x, &t.tb_id_y, &t.tb_id_z);
        #endif

      } 
      else if (word1 == "warp") {
        // the start of new warp stream
        assert(start_of_tb_stream_found);
        #ifdef __linux__
          sscanf(line.c_str(), "warp = %d", &warp_id);
        #elif _WIN32
          sscanf_s(line.c_str(), "warp = %d", &warp_id);
        #endif
      }
      else if (word1 == "insts") {
        assert(start_of_tb_stream_found);
        #ifdef __linux__
          sscanf(line.c_str(), "insts = %d", &num_instrs);
        #elif _WIN32
          sscanf_s(line.c_str(), "insts = %d", &num_instrs);
        #endif
        // Check warp already exists
        if (warp_id >= t.warps.size()) {
          t.warps.push_back(new warp_trace<std::string>(warp_id, num_instrs));
        }
        else {
          assert(0 && "Warp already exists in TB");
        }
        // std::cout << line << std::endl;
        instr_idx = 0;
      } 
      else {
        if (word1[0] == '#' || word1[0] == '-') {
          // Ignore Config / Info line
          ;
        }
        else {
          assert(start_of_tb_stream_found);
          // Insert in page map IF page boundary
          if (instr_idx % this->page_size == 0) {
            auto page_idx = instr_idx / page_size;
            t.warps[warp_id]->page_map[page_idx] = line_start;
          }
          instr_idx ++;
        }
      }
    }

    line_start = line_start + line.size() + 1;
  }
  
  // std::copy(this->tb_map, t);
  return 0;
}




#endif // TRACE_VECTOR_HPP
