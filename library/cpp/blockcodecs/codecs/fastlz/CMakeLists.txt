
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(blockcodecs-codecs-fastlz INTERFACE)
target_link_libraries(blockcodecs-codecs-fastlz INTERFACE
  contrib-libs-cxxsupp
  yutil
  contrib-libs-fastlz
  cpp-blockcodecs-core
)

add_global_library_for(blockcodecs-codecs-fastlz.global blockcodecs-codecs-fastlz)
target_link_libraries(blockcodecs-codecs-fastlz.global PUBLIC
  contrib-libs-cxxsupp
  yutil
  contrib-libs-fastlz
  cpp-blockcodecs-core
)
target_sources(blockcodecs-codecs-fastlz.global PRIVATE
  ${CMAKE_SOURCE_DIR}/library/cpp/blockcodecs/codecs/fastlz/fastlz.cpp
)
