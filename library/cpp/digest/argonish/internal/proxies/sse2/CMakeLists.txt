
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(internal-proxies-sse2)
target_link_libraries(internal-proxies-sse2 PUBLIC
  contrib-libs-cxxsupp
  internal-proxies-macro
  argonish-internal-argon2
  argonish-internal-blake2b
)
target_sources(internal-proxies-sse2 PRIVATE
  ${CMAKE_SOURCE_DIR}/library/cpp/digest/argonish/internal/proxies/sse2/proxy_sse2.cpp
)
set_property(
  SOURCE
  ${CMAKE_SOURCE_DIR}/library/cpp/digest/argonish/internal/proxies/sse2/proxy_sse2.cpp
  APPEND
  PROPERTY
  COMPILE_OPTIONS
  -msse2
)
