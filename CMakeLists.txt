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
# enable TSL for ixwebsocket so wss:// works
set(USE_TLS 1 CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ixwebsocket)

add_executable(sim
    apps/main.cpp
    src/OrderBook.cpp
    src/BitfinexFeed.cpp
)

target_include_directories(sim PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Most common:
target_link_libraries(sim PRIVATE ixwebsocket nlohmann_json::nlohmann_json)

# If you get "Target not found: ixwebsocket", change the above to:
# target_link_libraries(sim PRIVATE ixwebsocket::ixwebsocket nlohmann_json::nlohmann_json)