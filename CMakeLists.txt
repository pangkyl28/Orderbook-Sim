cmake_minimum_required(VERSION 3.16)
project(orderbook_sim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# ixwebsocket
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.5
)
set(USE_TLS 1 CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ixwebsocket)

# core library
add_library(orderbook_core
    src/OrderBook.cpp
)

target_include_directories(orderbook_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# simulator
add_executable(sim
    apps/main.cpp
    src/BitfinexFeed.cpp
)

target_link_libraries(sim PRIVATE
    orderbook_core
    ixwebsocket
    nlohmann_json::nlohmann_json
)

# googletest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(orderbook_tests
    tests/test_orderbook.cpp
)

target_link_libraries(orderbook_tests PRIVATE
    orderbook_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(orderbook_tests)