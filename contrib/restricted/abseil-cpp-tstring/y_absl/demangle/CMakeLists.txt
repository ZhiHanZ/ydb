
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(yabseil-cpp-tstring-y_absl-demangle)
target_include_directories(yabseil-cpp-tstring-y_absl-demangle PUBLIC
  ${CMAKE_SOURCE_DIR}/contrib/restricted/abseil-cpp-tstring
)
target_include_directories(yabseil-cpp-tstring-y_absl-demangle PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/restricted/abseil-cpp-tstring
)
target_link_libraries(yabseil-cpp-tstring-y_absl-demangle PUBLIC
  contrib-libs-cxxsupp
  yutil
  yabseil-cpp-tstring-y_absl-base
  ybase-internal-raw_logging
  ybase-internal-spinlock_wait
  yabsl-base-log_severity
)
target_sources(yabseil-cpp-tstring-y_absl-demangle PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/restricted/abseil-cpp-tstring/y_absl/debugging/internal/demangle.cc
)
