cmake_minimum_required(VERSION 3.10)
project(MoshpitSimulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)
set(SOURCES 
    src/mosher.cpp 
    src/mosher_initialization.cpp 
    src/vector.cpp
)

add_executable(moshpit_sim main.cpp ${SOURCES})