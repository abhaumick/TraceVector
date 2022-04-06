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


  std::cout << "\n\n Test map_file \n";
  trace_vector<int> tv;
  std::ifstream f;
  tv.init("D:/Work/Purdue/Research/TraceVector/traces/kernel-1.traceg");
  
  std::cout << tv.at(0) << " @ " << 0 << " \n";
  std::cout << tv.at(1) << " @ " << 1 << " \n";


  return 0;
}
