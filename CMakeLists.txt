cmake_minimum_required(VERSION 3.0)

project(mathjit)

include_directories("include")

set(SOURCES "src/main.cpp")

## Find ASMJIT
find_path(ASMJIT_INCLUDE_DIR "asmjit/asmjit.h")
find_library(ASMJIT_LIB asmjit)

if(${ASMJIT_INCLUDE_DIR} MATCHES "NOTFOUND" OR ${ASMJIT_LIB} MATCHES "NOTFOUND")
    message(SEND_ERROR "asmjit not found")
else()
    message(STATUS "Found asmjit include dir ${ASMJIT_INCLUDE_DIR}")
    message(STATUS "Found asmjit lib ${ASMJIT_LIB}")
endif()
# ----

## Find Boost.Spirit
find_path(BOOST_SPIRIT_INCLUDE_DIRS "boost/spirit.hpp")
if(${BOOST_SPIRIT_INCLUDE_DIRS} MATCHES "NOTFOUND")
    message(SEND_ERROR "Boost.Spirit not found")
else()
    message(STATUS "Found Boost.Spirit include dir ${BOOST_SPIRIT_INCLUDE_DIRS}")
endif()
# ----

## Static runtime
if(${CMAKE_CXX_COMPILER} MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

include_directories(${ASMJIT_INCLUDE_DIR})
include_directories(${BOOST_SPIRIT_INCLUDE_DIRS})

add_executable(mathjit ${SOURCES})
target_link_libraries(mathjit PUBLIC ${ASMJIT_LIB})