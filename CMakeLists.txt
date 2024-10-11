cmake_minimum_required(VERSION 3.16)
project(AHL LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include_directories("${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}")
add_executable(assembler src/main.c
src/fileFunctions.c
src/assembler.c
src/ht.c
)

set_property(TARGET assembler PROPERTY C_STANDARD 11)

#add_custom_target(testInput
#    COMMAND assembler "/asmFiles/testFile.asm" "/asmFiles/output.hex"
#    DEPENDS assembler
#    COMMENT "Running CommandLineArgsExample with arguments arg1, arg2, arg3"
#)

