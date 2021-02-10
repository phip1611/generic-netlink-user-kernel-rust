# CMakeLists for Jetbrains CLion IDE support
# this way I get auto completion

# regular "Makefile" is also fine and works for development via CLI

cmake_minimum_required(VERSION 3.0.0)
project (generic-netlink-user-kernel-rust)

add_executable(user-libnl user-libnl.c)
add_executable(user-pure user-pure.c)

target_link_libraries("user-libnl" PRIVATE nl-3 nl-genl-3)

include_directories(/usr/include/libnl3)
include_directories(../include)
