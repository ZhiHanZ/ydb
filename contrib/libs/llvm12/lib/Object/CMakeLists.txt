
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(llvm12-lib-Object)
target_include_directories(llvm12-lib-Object PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object
)
target_link_libraries(llvm12-lib-Object PUBLIC
  contrib-libs-cxxsupp
  contrib-libs-llvm12
  libs-llvm12-include
  llvm12-lib-BinaryFormat
  lib-Bitcode-Reader
  llvm12-lib-IR
  llvm12-lib-MC
  lib-MC-MCParser
  llvm12-lib-Support
  lib-TextAPI-MachO
)
target_sources(llvm12-lib-Object PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/Archive.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/ArchiveWriter.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/Binary.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/COFFImportFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/COFFModuleDefinition.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/COFFObjectFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/Decompressor.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/ELF.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/ELFObjectFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/Error.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/IRObjectFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/IRSymtab.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/MachOObjectFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/MachOUniversal.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/MachOUniversalWriter.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/Minidump.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/ModuleSymbolTable.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/Object.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/ObjectFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/RecordStreamer.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/RelocationResolver.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/SymbolSize.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/SymbolicFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/TapiFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/TapiUniversal.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/WasmObjectFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/WindowsMachineFlag.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/WindowsResource.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Object/XCOFFObjectFile.cpp
)
