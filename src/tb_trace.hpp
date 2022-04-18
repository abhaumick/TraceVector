/**
 * @file tb_trace.hpp
 * @author Abhishek Bhaumick (abhaumic@purdue.edu)
 * @brief 
 * @version 0.1
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#ifndef TB_TRACE_HPP
#define TB_TRACE_HPP

#include <cstddef>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include "warp_trace.hpp"

#ifndef dim3
typedef struct {
  unsigned x, y, z;
} dim3;
#endif

template <typename T>
class tb_trace {
public:
  tb_trace();
  // tb_trace(const tb_trace<T>& t);
  ~tb_trace();

  constexpr dim3 id(void) const { return _id; }
  constexpr size_t size(void) const { return _size; }

  int init(const std::string& file_path, size_t file_offset);

  int parse_tb();

  warp_trace<T>& get_warp(unsigned warp_id) const { return warps[warp_id]; }

  std::vector <warp_trace <T>> warps;
  
protected:
  std::set <int> _warp_set;

  std::string file_path;
  size_t file_offset;
  std::ifstream* file_handle;
  size_t tb_file_start;
  size_t tb_file_end;

  size_t _page_size;

  int map_tb_to_file(std::ifstream & handle);

private:
  dim3 _id;
  size_t _size;             /// Number of warps
  bool _init_done;
};

template <typename T>
tb_trace<T>::tb_trace() :
  _size(0),
  _init_done(false)
{
  _page_size = TRACE_PAGE_SIZE;
  file_handle = new std::ifstream;
}

// template <typename T>
// tb_trace<T>::tb_trace(const tb_trace<T>& t) {
//   std::copy(t.warps(), _warps);
// }

template <typename T>
tb_trace<T>::~tb_trace() {
  warps.clear();
  _warp_set.clear();
  if (file_handle->is_open()) {
    file_handle->close();
  }
  delete file_handle;
}

template <typename T>
int tb_trace<T>::init(const std::string& file_path, size_t file_offset) {
  if (file_handle->is_open()) {
    file_handle->close();
  }
  file_handle->open(file_path.c_str());

  if (! file_handle->is_open()) {
    std::cout << "Unable to open file " << file_path.c_str() << " @ " 
      << std::filesystem::current_path() << "\n";
    return -1;
  }
  else {
    // Seek to file_offset
    file_handle->seekg(file_offset, file_handle->beg);

    // Map File
    auto retVal = this->map_tb_to_file(* file_handle);

    //  Init the warps created
    for (auto& warp : warps) {
      warp.init(* file_handle);
    }
  }
  return 0;
}


template <typename T>
int tb_trace<T>::map_tb_to_file(std::ifstream & handle) {
  bool start_of_tb_stream_found = false;
  unsigned instr_idx;
  unsigned warp_id;
  unsigned num_instrs;
  size_t line_start = 0;
  size_t line_end = 0;
  warps.clear();

  std::stringstream ss;
  std::string line, word1, word2, word3, word4;

  while (! handle.eof()) {
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
          tb_file_start = line_start;
        } 
        else {
          assert(0 && "Parsing error: thread block start before "  
            "the previous one finishes");
          tb_file_end = line_end;
        }
      } 
      else if (word1 == "#END_TB") {
        assert(start_of_tb_stream_found);
        tb_file_end = line_end;
        break;  // end of TB stream
      } 
      else if (word1 == "thread" && word2 == "block") {
        //  TB Dimensions
        assert(start_of_tb_stream_found);
        sscanf_s(line.c_str(), "thread block = %d,%d,%d", &_id.x,
          &_id.y, &_id.z);
      } 
      else if (word1 == "warp") {
        // start of new warp stream
        assert(start_of_tb_stream_found);
        sscanf_s(line.c_str(), "warp = %d", &warp_id);
      }
      else if (word1 == "insts") {
        assert(start_of_tb_stream_found);
        sscanf_s(line.c_str(), "insts = %d", &num_instrs);
        // Check warp already exists
        if (warp_id >= warps.size()) {
          warps.emplace_back(warp_id, num_instrs);
          ++ _size;
        }
        else {
          assert(0 && "Warp already exists in TB");
        }
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
          if (instr_idx % _page_size == 0) {
            auto page_idx = (size_t) (instr_idx / _page_size);
            warps[warp_id].page_map[page_idx] = line_start;
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


#endif  // TB_TRACE_HPP

