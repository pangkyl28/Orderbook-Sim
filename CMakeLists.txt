cmake_minimum_required(VERSION 3.16)
project(orderbook_sim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sim 
    apps/main.cpp
    src/OrderBook.cpp
)

target_include_directories(sim PRIVATE ${CMAKE_SOURCE_DIR}/include)