cmake_minimum_required(VERSION 3.1)

project(RISCV-Simulator)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-O0 -g -Wall")

# yyx 
set(CMAKE_BUILD_TYPE Debug)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(nlohmann_json 3.11.3 REQUIRED)


add_executable(
    Simulator 
    src/MainCPU.cpp 
    src/MemoryManager.cpp 
    src/Simulator.cpp 
    src/Tomasulo.cpp
)

target_link_libraries(Simulator PRIVATE nlohmann_json::nlohmann_json)