cmake_minimum_required(VERSION 3.18)
project(MemRangeX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_library(IBVERBS ibverbs REQUIRED)
find_library(RDMACM rdmacm REQUIRED)
find_library(NUMA numa REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCE_FILES
    src/common/*.h
    src/common/*.cpp
    src/rdma/*.h
    src/rdma/*.cpp
    src/numa_concurrency/*.h
    src/numa_concurrency/*.cpp
    src/hssi_index/*.h
    src/hssi_index/*.cpp
    src/load_collector/*.h
    src/load_collector/*.cpp
    src/offload_engine/*.h
    src/offload_engine/*.cpp
    src/cache_system/*.h
    src/cache_system/*.cpp
    src/segment_manager/*.h
    src/segment_manager/*.cpp
    src/fault_tolerance/*.h
    src/fault_tolerance/*.cpp
    src/metadata/*.h
    src/metadata/*.cpp
    src/thread_pool/*.h
    src/thread_pool/*.cpp
    src/ycsb_driver/*.h
    src/ycsb_driver/*.cpp
)

add_executable(MemRangeX main.cpp ${PROJECT_SOURCE_FILES})

target_include_directories(MemRangeX PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/rdma
    ${PROJECT_SOURCE_DIR}/src/numa_concurrency
    ${PROJECT_SOURCE_DIR}/src/hssi_index
    ${PROJECT_SOURCE_DIR}/src/load_collector
    ${PROJECT_SOURCE_DIR}/src/offload_engine
    ${PROJECT_SOURCE_DIR}/src/cache_system
    ${PROJECT_SOURCE_DIR}/src/segment_manager
    ${PROJECT_SOURCE_DIR}/src/fault_tolerance
    ${PROJECT_SOURCE_DIR}/src/metadata
    ${PROJECT_SOURCE_DIR}/src/thread_pool
    ${PROJECT_SOURCE_DIR}/src/ycsb_driver
)

target_compile_options(MemRangeX PRIVATE -O3 -Wall -Wextra -Wpedantic -pthread)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mrtm" COMPILER_SUPPORTS_MRTM)
if(COMPILER_SUPPORTS_MRTM)
    target_compile_options(MemRangeX PRIVATE -mrtm)
endif()

target_link_libraries(MemRangeX
    Threads::Threads
    ${IBVERBS}
    ${RDMACM}
    ${NUMA}
    pthread
    dl
    rt
)

file(COPY config.ini run_local.sh run_cluster_4node.sh stop_cluster.sh clear_log.sh DESTINATION ${CMAKE_BINARY_DIR})
