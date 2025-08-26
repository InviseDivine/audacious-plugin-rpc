cmake_minimum_required(VERSION 3.10)
project(audacious-rpc)

# General options
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Finding Audacious
include(FindPkgConfig)
PKG_SEARCH_MODULE(AUDACIOUS REQUIRED audacious>=3.9)
add_definitions(${AUDACIOUS_CFLAGS})

# Building discord-rpc
option(BUILD_EXAMPLES "Build examples for discord-rpc" OFF)

# Creating RPC plugin
add_library(audacious-plugin-rpc SHARED 
audacious-plugin-rpc.cc
)

add_subdirectory(libs/discord-rpc)
add_subdirectory(libs/cpr)

target_include_directories(audacious-plugin-rpc PRIVATE "${CMAKE_SOURCE_DIR}/libs/discord-rpc/include")
target_link_libraries(audacious-plugin-rpc discord-rpc cpr)

# Installing RPC plugin
PKG_GET_VARIABLE(AUDACIOUS_PLUGIN_DIR audacious plugin_dir)
set(INSTALL_DIR "${AUDACIOUS_PLUGIN_DIR}/General")

install(TARGETS audacious-plugin-rpc LIBRARY DESTINATION ${INSTALL_DIR})