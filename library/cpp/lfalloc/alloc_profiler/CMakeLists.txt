
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(cpp-lfalloc-alloc_profiler)
target_link_libraries(cpp-lfalloc-alloc_profiler PUBLIC
  contrib-libs-cxxsupp
  yutil
  cpp-lfalloc-dbg_info
  library-cpp-cache
)
target_sources(cpp-lfalloc-alloc_profiler PRIVATE
  ${CMAKE_SOURCE_DIR}/library/cpp/lfalloc/alloc_profiler/profiler.cpp
  ${CMAKE_SOURCE_DIR}/library/cpp/lfalloc/alloc_profiler/stackcollect.cpp
)
