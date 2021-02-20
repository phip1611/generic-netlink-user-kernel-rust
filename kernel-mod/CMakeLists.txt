# CMake is only used in order for Clion IDE to work (auto completion etc). It is not required
# for building the kernel module. To build it, just execute `$ make`.

cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project("Generic Netlink Foobar Example Linux Kernel Module" VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find kernel headers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/.cmake")
find_package(KernelHeaders REQUIRED)

# find MODULE_LICENSE("GPL"), MODULE_AUTHOR() etc.
# thanks to "merseyviking" from stack overflow
add_definitions(-D__KERNEL__ -DMODULE)

# this is needed in order for CLion IDE to provide syntax highlightning
# this is independent from the actual kernel object that is built
add_executable(dummy
        # add all *.h and *.c files here that # CLion should cover
        gnl_foobar_xmpl.c
)

# CLion IDE will find symbols from <linux/*>
target_include_directories("dummy" PRIVATE ${KERNELHEADERS_INCLUDE_DIRS})
target_include_directories("dummy" PRIVATE ../include)
