
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(ycontainer-internal-absl_hashtablez_sampler)
target_include_directories(ycontainer-internal-absl_hashtablez_sampler PUBLIC
  ${CMAKE_SOURCE_DIR}/contrib/restricted/abseil-cpp-tstring
)
target_include_directories(ycontainer-internal-absl_hashtablez_sampler PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/restricted/abseil-cpp-tstring
)
target_link_libraries(ycontainer-internal-absl_hashtablez_sampler PUBLIC
  contrib-libs-cxxsupp
  yutil
  yabseil-cpp-tstring-y_absl-base
  ybase-internal-low_level_alloc
  ybase-internal-raw_logging
  ybase-internal-spinlock_wait
  ybase-internal-throw_delegate
  yabsl-base-log_severity
  yabseil-cpp-tstring-y_absl-debugging
  yabsl-debugging-stacktrace
  yabsl-debugging-symbolize
  yabseil-cpp-tstring-y_absl-demangle
  yabseil-cpp-tstring-y_absl-numeric
  yprofiling-internal-exponential_biased
  yabseil-cpp-tstring-y_absl-strings
  ystrings-internal-absl_strings_internal
  yabseil-cpp-tstring-y_absl-synchronization
  yabsl-synchronization-internal
  yabseil-cpp-tstring-y_absl-time
  yabsl-time-civil_time
  yabsl-time-time_zone
)
target_sources(ycontainer-internal-absl_hashtablez_sampler PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/restricted/abseil-cpp-tstring/y_absl/container/internal/hashtablez_sampler.cc
  ${CMAKE_SOURCE_DIR}/contrib/restricted/abseil-cpp-tstring/y_absl/container/internal/hashtablez_sampler_force_weak_definition.cc
)
