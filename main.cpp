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
#include <random>

#include "src/trace_vector.hpp"
#include "src/tb_trace.hpp"
#include "src/warp_trace.hpp"

int testWarpTrace();
int testTbTrace();
int testToBytes();

int main(int argc, char ** argv) {
  std::cout << "Hello TraceVector!\n";

  std::cout << "\n\n Test map_file \n";
  // testWarpTrace();

  std::cout << "\n\n Test tb_trace \n";
  // testTbTrace();

  std::cout << "\n\n Test Serialization \n ";
  testToBytes();

  return 0;
}


int testWarpTrace(void) {
  trace_vector<int> tv;
  std::ifstream f;
  f.open("D:/Work/Purdue/Research/TraceVector/traces/kernel-1.traceg");
  // tv.init("D:/Work/Purdue/Research/TraceVector/traces/kernel-1.traceg");
  tv.map_tb_to_file(f);
  auto& w0 = tv.get_tb_trace().warps[0];
  w0->init(f);

  for (auto i = 0; i < w0->size(); ++ i) {
    std::cout << w0->at(i) << "\n";
  }

  for (auto i = 0; i < w0->size(); ++ i) {
    std::cout << (*w0)[i] << "\n";
  }

  std::cout << *w0;

  if (f.is_open()) {
    f.close();
  }
  return 0;
}

int testTbTrace(void) {

  const unsigned sample_size = 100;

  std::vector <tb_trace <std::string>> tb;
  tb.emplace_back();
  tb[0].init("D:/Work/Purdue/Research/TraceVector/traces/kernel-1.traceg", 0);

  std::cout << "# Warps = " << tb[0].size() << "\n" ;

  // Generate uniform random sample
  std::random_device rand_device;
  std::mt19937_64 mersenne_engine(rand_device());
  std::uniform_int_distribution <unsigned> dist1(0, tb[0].size()-1);
  auto rand_gen1 = [&dist1, &mersenne_engine](){ 
    return dist1(mersenne_engine);
  };

  auto warp_ids = std::vector <unsigned> (sample_size);
  std::generate(warp_ids.begin(), warp_ids.end(), rand_gen1);

  std::uniform_int_distribution <unsigned> dist2(0, sample_size);
  auto rand_gen2 = [&dist2, &mersenne_engine](){ 
    return dist2(mersenne_engine);
  };
  auto instr_ids = std::vector <unsigned> (sample_size);
  std::generate(instr_ids.begin(), instr_ids.end(), rand_gen2);

  auto instr_iter = instr_ids.begin();
  for (auto& warp_id : warp_ids) {
    auto instr = *instr_iter % tb[0].warps[warp_id].size();
    std::cout << "Warp " << warp_id << " Instr " << instr << " ";
    std::cout << tb[0].warps[warp_id][instr] << "\n";
    ++ instr_iter;
    if (instr_iter == instr_ids.end())
      break;
  }

  return 0;
}

int testToBytes(void) {
  tb_trace <std::string> tb;
  tb.init("D:/Work/Purdue/Research/TraceVector/traces/kernel-1.traceg", 0);

  std::vector <unsigned char> v;
  tb.warps[4].to_bytes(v);

  std::cout << "Bytes Size = " << v.size() << "\n";
  for (auto byte : v) {
    std::cout << std::hex << (int)byte << " ";
  }
  std::cout << std::dec << "\n";

  std::cout << 

  return 0;
}

