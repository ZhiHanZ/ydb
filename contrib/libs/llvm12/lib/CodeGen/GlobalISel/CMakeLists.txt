
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(lib-CodeGen-GlobalISel)
target_include_directories(lib-CodeGen-GlobalISel PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel
)
target_link_libraries(lib-CodeGen-GlobalISel PUBLIC
  contrib-libs-cxxsupp
  contrib-libs-llvm12
  libs-llvm12-include
  llvm12-lib-Analysis
  llvm12-lib-CodeGen
  lib-CodeGen-SelectionDAG
  llvm12-lib-IR
  llvm12-lib-MC
  llvm12-lib-Support
  llvm12-lib-Target
  lib-Transforms-Utils
)
target_sources(lib-CodeGen-GlobalISel PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/CSEInfo.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/CSEMIRBuilder.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/CallLowering.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/Combiner.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/CombinerHelper.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/GISelChangeObserver.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/GISelKnownBits.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/GlobalISel.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/IRTranslator.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/InlineAsmLowering.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/InstructionSelect.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/InstructionSelector.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/LegalityPredicates.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/LegalizeMutations.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/Legalizer.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/LegalizerHelper.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/LegalizerInfo.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/Localizer.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/LostDebugLocObserver.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/MachineIRBuilder.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/RegBankSelect.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/RegisterBank.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/RegisterBankInfo.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/CodeGen/GlobalISel/Utils.cpp
)
