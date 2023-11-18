cmake_minimum_required(VERSION 3.6)

project(HLFixes)

set(FUNCHOOK_BUILD_SHARED OFF CACHE BOOL "build shared library" FORCE)
set(FUNCHOOK_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(FUNCHOOK_INSTALL OFF CACHE BOOL "Install Funchook" FORCE)
set(CMAKE_C_FLAGS "-m32")
set(CMAKE_CXX_FLAGS "-m32")

include_directories(HLFixes funchook/include)
add_subdirectory(funchook)

find_package(PkgConfig REQUIRED)
pkg_search_module(LIBELF REQUIRED libelf)

add_library(HLFixes SHARED main.cpp utils.cpp)
target_link_libraries(HLFixes funchook-static ${LIBELF_LIBRARIES})
set_target_properties(HLFixes PROPERTIES PREFIX "")