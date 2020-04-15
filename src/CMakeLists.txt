
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
#add_compile_options(-Wall -Wextra)

#set(CMAKE_BUILD_TYPE "Debug")
message(STATUS "Found LLVM: ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
#set(LLVM_LIBS ${LLVM_INCLUDE_DIRS}/../lib)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

#add_definitions(${LLVM_DEFINITIONS})
#add_definitions(${CLANG_DEFINITIONS})

add_subdirectory(AST)

add_library(fsm SHARED
        FSM/FSM.cpp
        FSM/StateFSM.cpp
        )

add_library(smt SHARED
        SMT/BinarySMT.cpp
        SMT/VariableSMT.cpp
        SMT/VersionedVariable.cpp
        )

add_executable(ESCA main.cpp)


target_link_libraries(ESCA
        LLVM
        ast
        fsm
        smt
        clangAST
        clangLex
        clangBasic
        clangFrontend
        clangTooling
        clangParse
        clangDriver
        )
