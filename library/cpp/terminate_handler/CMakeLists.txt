
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(library-cpp-terminate_handler)
target_link_libraries(library-cpp-terminate_handler PUBLIC
  contrib-libs-cxxsupp
  yutil
)
target_sources(library-cpp-terminate_handler PRIVATE
  ${CMAKE_SOURCE_DIR}/library/cpp/terminate_handler/segv_handler.cpp
)

add_global_library_for(library-cpp-terminate_handler.global library-cpp-terminate_handler)
target_link_libraries(library-cpp-terminate_handler.global PUBLIC
  contrib-libs-cxxsupp
  yutil
)
target_sources(library-cpp-terminate_handler.global PRIVATE
  ${CMAKE_SOURCE_DIR}/library/cpp/terminate_handler/terminate_handler.cpp
)
