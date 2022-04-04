/**
 * @file main.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-03-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>
#include "src/trace_vector.hpp"

int testPageBuffer();

int main(int argc, char ** argv) {
  std::cout << "Hello TraceVector!\n";

  // std::cout << "Test Page Buffer \n";
  // testPageBuffer();
  

  std::cout << "Test trace_vector \n";
  trace_vector<int> tv;
  tv.init("D:/Work/Purdue/Research/TraceVector/trace.log");
  auto& retVal = tv.at(1);
  std::cout << retVal << " @ " << 1 << " \n";
  retVal.append("hh");
  std::cout << tv.at(0) << " @ " << 0 << " \n";
  std::cout << tv.at(1) << " @ " << 1 << " \n";

  std::cout << "\n\n Test map_file \n";
  trace_vector<int> tv2;
  std::ifstream f;
  f.open("D:/Work/Purdue/Research/TraceVector/trace.log");
  tv2.map_file(f);
  tv2.map_file(f);
  tv2.map_file(f);

  return 0;
}

class classX{
  int value;
public :
  static int count;
  classX(int v = 0) : value{v} {
    ++ count;
  }
  ~classX() {
    -- count;
  }
  int getValue() {
    return value;
  }
};

int classX::count = 0;

class xEntry{
public:
  ~xEntry() {
    if (page_present)
      delete page_ptr;
  }

  size_t page_size;
  page_type_t page_type;
  std::vector<classX> * page_ptr;
  bool page_present;
  bool page_rw;
  bool page_dirty;
  bool page_accessed;
};

int testPageBuffer() {
  std::vector <xEntry *> page_buffer;

  // Populate
  for (auto i = 0; i < 10; ++ i) {
    std::vector <classX> * v = new std::vector <classX>;
    v->emplace_back(i);

    xEntry * pte = new xEntry;
    pte->page_accessed  = false;
    pte->page_dirty     = false;
    pte->page_present   = true;
    pte->page_ptr       = v;
    pte->page_rw        = false;
    pte->page_size      = 10;
    pte->page_type      = page_type_t::PAGE_STRING;

    page_buffer.push_back(pte);
  }

  for (auto i = 0; i < 10; ++ i) {
    // Check pte
    xEntry * pte = page_buffer[i];
    if (pte == nullptr)
      return 1;
    else
      delete page_buffer[i];
    page_buffer[i] = nullptr;
  }
  return 0;
}