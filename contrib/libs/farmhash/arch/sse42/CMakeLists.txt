
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(farmhash-arch-sse42)
target_compile_options(farmhash-arch-sse42 PRIVATE
  -msse4.2
)
target_link_libraries(farmhash-arch-sse42 PUBLIC
  contrib-libs-cxxsupp
  yutil
)
target_sources(farmhash-arch-sse42 PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/farmhash/farmhashsa.cc
  ${CMAKE_SOURCE_DIR}/contrib/libs/farmhash/farmhashte.cc
)
