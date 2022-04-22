cmake_minimum_required(VERSION 3.5)
message(STATUS "CMake version: ${CMAKE_VERSION}")

# Project name and a few useful settings. Other commands can pick up the results
project(
  TraceVector
  VERSION 0.1
  DESCRIPTION "A storage backed limited memory footprint container"
  LANGUAGES CXX)
  
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_INSTALL_PREFIX ".")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib")


# enable C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -g3")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -g3")

if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/*.c
	${CMAKE_SOURCE_DIR}/src/*.cpp)
	
# Add header files
file(GLOB_RECURSE HEADER_FILES 
  ${CMAKE_SOURCE_DIR}/include/*.h
	${CMAKE_SOURCE_DIR}/include/*.hpp
	${CMAKE_SOURCE_DIR}/src/*.h
	${CMAKE_SOURCE_DIR}/src/*.hpp)

# Add include folders
include_directories(${CMAKE_SOURCE_DIR}/include)

# Executables
add_executable(test "main.cpp" ${HEADER_FILES} ${SOURCE_FILES})

