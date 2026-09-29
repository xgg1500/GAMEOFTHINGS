cmake_minimum_required(VERSION 3.16)
project(NFSHardcorePursuit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(pursuit_sim ${SOURCES})

# Platform-specific optimization flags
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(pursuit_sim PRIVATE -O3 -Wall -Wextra)
endif()
