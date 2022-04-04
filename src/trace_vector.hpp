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
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <map>

enum class page_type_t{
  PAGE_RAW,
  PAGE_STRING
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


  trace_vector() {
    size = 10;
    page_size = 4;
    this->page_buffer.reserve(size);
  };

  ~trace_vector() {
    this->delete_page_buffer();
  }

  int init(const std::string& filePath, size_t file_offset = 0);


protected:


  int setBackingFile(const std::string & file_path);
  int setBackingFile(const std::ifstream & file_handle);

  int insert_page(int insert_loc, size_t vector_start, 
    std::ifstream & handle, size_t file_offset = 0);
  int remove_page(int loc);
  int delete_page_buffer();


private:
  bool is_initialized;
  std::ifstream * file_stream;
  size_t size;
  int MRI;
  int LRI;
  int page_size;
  std::vector <page_entry> page_buffer;
  std::map <int, page_entry*> page_map;
};



template <typename T> 
int trace_vector<T>::init(const std::string& file_path, size_t file_offset) {
  
  std::ifstream file_handle;
  file_handle.open(file_path.c_str());

  if (! file_handle.is_open()) {
    std::cout << "Unable to open file " << file_path.c_str() << " @ " 
      << std::filesystem::current_path() << "\n";
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
int trace_vector<T>::insert_page(int loc, size_t vector_start, 
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
int trace_vector<T>::remove_page(int loc) {
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
  for (auto idx = 0; idx < this->page_buffer.size(); ++ idx) {
    remove_page(idx);
  }
  return 0;
}


#endif // TRACE_VECTOR_HPP
