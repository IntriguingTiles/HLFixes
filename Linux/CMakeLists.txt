cmake_minimum_required(VERSION 3.6)

project(HLFixes)

set(FUNCHOOK_BUILD_SHARED OFF CACHE BOOL "build shared library" FORCE)
set(FUNCHOOK_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(FUNCHOOK_INSTALL OFF CACHE BOOL "Install Funchook" FORCE)
set(CMAKE_C_FLAGS "-m32")
set(CMAKE_CXX_FLAGS "-m32")

include_directories(HLFixes funchook/include)
add_subdirectory(funchook)

# find_package(PkgConfig REQUIRED)
# pkg_search_module(LIBELF REQUIRED libelf)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(HLFixes SHARED main.cpp utils.cpp)
target_link_libraries(HLFixes funchook-static elf)
set_target_properties(HLFixes PROPERTIES PREFIX "")