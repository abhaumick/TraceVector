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
#include <algorithm>

#include "src/trace_vector.hpp"
#include "src/tb_trace.hpp"
#include "src/warp_trace.hpp"

const unsigned numTB = 10;
const unsigned sample_size = 100;

const auto testSize = 1000000; 

int testWarpTrace();
int testTbTrace();
int testGpuTrace();
int testToBytes();

int main(int argc, char ** argv) {
  std::cout << "Hello TraceVector!\n";

  std::cout << "\n\n Test map_file \n";
  // testWarpTrace();

  std::cout << "\n\n Test tb_trace \n";
  // testTbTrace();

  std::cout << "\n\n Test gpu tb_trace - " << numTB << " TBs \n";
  testGpuTrace();

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

  for (auto i = 0U; i < w0->size(); ++ i) {
    std::cout << w0->at(i) << "\n";
  }

  for (auto i = 0U; i < w0->size(); ++ i) {
    std::cout << (*w0)[i] << "\n";
  }

  std::cout << *w0;

  if (f.is_open()) {
    f.close();
  }
  return 0;
}

int testTbTrace(void) {

  std::vector <tb_trace <std::string>> tb;
  tb.emplace_back();
  tb[0].init("trace.log", 0);

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
  tb.init("trace.log", 0);

  std::vector <unsigned char> v;
  tb.warps[4].to_bytes(v);

  std::cout << "Bytes Size = " << v.size() << "\n";
  for (auto byte : v) {
    std::cout << std::hex << (int)byte << " ";
  }
  std::cout << std::dec << "\n";

  return 0;
}

int testGpuTrace(void) {

  std::vector <tb_trace <std::string>> tb;
  size_t offset = 0;
  tb.reserve(numTB);
  for (auto idx = 0U; idx < numTB; ++ idx) {
    tb.emplace_back();
    tb[idx].init("trace.log", offset);
    offset = tb[idx].get_file_end();
    // std::cout << idx << " Done @ " << offset << "\n";
  }

  // Generate uniform random sample
  std::random_device rand_device;
  std::mt19937_64 mersenne_engine(rand_device());
  
  std::uniform_int_distribution <unsigned> dist_tb(0, numTB - 1);
  auto tb_indexes = std::vector <unsigned> (testSize);
  auto tb_gen = [&dist_tb, &mersenne_engine](){ return dist_tb(mersenne_engine); };
  std::generate(tb_indexes.begin(), tb_indexes.end(), tb_gen);

  auto warp_indexes = std::vector <unsigned> (testSize);
  std::uniform_int_distribution <unsigned> dist_warp(0, 10000);
  auto warp_gen = [&dist_warp, &mersenne_engine](){ return dist_warp(mersenne_engine); };
  std::generate(warp_indexes.begin(), warp_indexes.end(), warp_gen);

  auto instr_indexes = std::vector <unsigned> (testSize);
  std::uniform_int_distribution <unsigned> dist_instr(0, 10000);
  auto instr_gen = [&dist_instr, &mersenne_engine](){ return dist_instr(mersenne_engine); };
  std::generate(instr_indexes.begin(), instr_indexes.end(), instr_gen);

  size_t correct_count = 0;
  for (auto i = 0; i < testSize; ++ i) {
    auto tb_idx = tb_indexes[i];
    auto num_warps = tb[tb_idx].size();
    auto warp_idx = warp_indexes[i] % num_warps;
    auto num_instr = tb[tb_idx].warps[warp_idx].size();
    auto instr_idx = instr_indexes[i] % num_instr;
    // std::cout << tb_idx << ", " << warp_idx << ", " << instr_idx ;
    
    auto paged_instr = tb[tb_idx].warps[warp_idx][instr_idx];
    
    std::string string_obj;
    std::stringstream ss;
    ss.str(string_obj);
    ss << tb_idx << ", " << warp_idx << ", " << instr_idx << " ";

    if (paged_instr.find(ss.str(), 0) != std::string::npos)
      ++ correct_count;

    // std::cout << ss.str() << "  " << paged_instr << " " 
    //   << (paged_instr.find(ss.str(), 0) != std::string::npos) << " \n";
  }

  std::cout << "Correctly returned " << correct_count << " / " 
    << testSize << " instruction strings" ;

  return 0;
}

