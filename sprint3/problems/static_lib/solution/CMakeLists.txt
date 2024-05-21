cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
#find_package(Catch2 REQUIRED)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(game_server_lib
	src/http_server.cpp
	src/model.cpp
	src/logger.cpp
	src/boost_json.cpp
	src/json_loader.cpp
	src/request_handler.cpp
	src/game.cpp
	src/player_tokens.cpp
	src/config.cpp
	src/json_serialization.cpp
	src/loot_generator.cpp
)

target_include_directories(game_server_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server_lib PUBLIC Threads::Threads CONAN_PKG::boost)

add_executable(game_server
	src/main.cpp
)

#target_include_directories(game_server PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE game_server_lib)


include(CTest)
#include(Catch)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)

add_executable(game_server_test
	tests/loot_generator_tests.cpp
)

#target_include_directories(game_server_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" CONAN_PKG::boost CONAN_PKG::catch2)
target_link_libraries(game_server_test PRIVATE CONAN_PKG::catch2 game_server_lib)

#catch_discover_tests(game_server_tests)