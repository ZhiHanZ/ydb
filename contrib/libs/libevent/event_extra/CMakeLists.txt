
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(libs-libevent-event_extra)
target_compile_options(libs-libevent-event_extra PRIVATE
  -DHAVE_CONFIG_H
  -DEVENT__HAVE_STRLCPY=1
)
target_include_directories(libs-libevent-event_extra PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/libevent
  ${CMAKE_SOURCE_DIR}/contrib/libs/libevent/include
)
target_link_libraries(libs-libevent-event_extra PUBLIC
  contrib-libs-libc_compat
)
target_sources(libs-libevent-event_extra PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/libevent/evdns.c
  ${CMAKE_SOURCE_DIR}/contrib/libs/libevent/event_tagging.c
  ${CMAKE_SOURCE_DIR}/contrib/libs/libevent/evrpc.c
  ${CMAKE_SOURCE_DIR}/contrib/libs/libevent/http.c
)
