
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(cpp-messagebus-config)
target_link_libraries(cpp-messagebus-config PUBLIC
  contrib-libs-cxxsupp
  yutil
  library-cpp-getopt
  cpp-deprecated-enum_codegen
)
target_sources(cpp-messagebus-config PRIVATE
  ${CMAKE_SOURCE_DIR}/library/cpp/messagebus/config/netaddr.cpp
  ${CMAKE_SOURCE_DIR}/library/cpp/messagebus/config/session_config.cpp
)
