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
  f.open("D:/Work/Purdue/Research/TraceVector/traces/kernel-1.traceg");
  // tv.init("D:/Work/Purdue/Research/TraceVector/traces/kernel-1.traceg");
  tv.map_tb_to_file(f);
  auto w0 = tv.get_tb_trace().warps[0];
  w0->init(f);

  for (auto i = 0; i < 45; ++ i) {
    std::cout << w0->at(i) << "\n";
  }


  if (f.is_open()) {
    f.close();
  }
  return 0;
}
