cmake_minimum_required(VERSION 3.10)
project(gs2test VERSION 1.0)

set(CMAKE_DEBUG_POSTFIX _d)

set(BIN_DIR "bin" CACHE STRING "Binary output directory")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set up output ditrectories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/${BIN_DIR})

# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
	string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/lib )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/lib )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/${BIN_DIR} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

link_directories(${PROJECT_SOURCE_DIR}/lib)

if(WIN32 OR APPLE)
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
	set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
endif()

# Windows: use win_flex/win_bison from chocolatey (do NOT build winflexbison submodule)
if(WIN32)
    # Try MSYS2 bison/flex first (newer versions), then fall back to winflexbison
    find_program(WIN_FLEX flex PATHS "C:/msys64/usr/bin" "C:/msys64/mingw64/bin" NO_DEFAULT_PATH)
    find_program(WIN_BISON bison PATHS "C:/msys64/usr/bin" "C:/msys64/mingw64/bin" NO_DEFAULT_PATH)

    # If not found in MSYS2, try winflexbison names (chocolatey)
    if(NOT WIN_FLEX)
        find_program(WIN_FLEX win_flex)
    endif()
    if(NOT WIN_BISON)
        find_program(WIN_BISON win_bison)
    endif()

    # Detect if we're using winflexbison or MSYS2 flex
    # MSYS2 flex doesn't support --wincompat flag
    get_filename_component(FLEX_NAME ${WIN_FLEX} NAME_WE)
    if(FLEX_NAME STREQUAL "win_flex")
        set(FLEX_FLAGS "--wincompat")
    else()
        set(FLEX_FLAGS "")
    endif()

    set(GENERATOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generator)
    set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    set(PARSER_Y ${GENERATOR_DIR}/gs2parser.y)
    set(SCANNER_L ${GENERATOR_DIR}/gs2scanner.l)

    set(PARSER_CC ${GENERATED_DIR}/gs2parser.tab.cc)
    set(PARSER_HH ${GENERATED_DIR}/gs2parser.tab.hh)
    set(PARSER_H  ${GENERATED_DIR}/gs2parser.tab.h)
    set(LEX_CC    ${GENERATED_DIR}/lex.yy.cc)
    set(LEX_H     ${GENERATED_DIR}/lex.yy.h)

    # Set executable variables for consistency with Unix code
    set(BISON_EXECUTABLE ${WIN_BISON})
    set(FLEX_EXECUTABLE ${WIN_FLEX})

    # Generate parser: produce .cc and (if produced) .hh, then copy .hh -> .h if present
    add_custom_command(
      OUTPUT ${PARSER_CC} ${PARSER_H}
      COMMAND "${BISON_EXECUTABLE}" "-d" "-o" "${PARSER_CC}" "${PARSER_Y}"
      # conditional copy on Windows using cmd; if hh was produced copy to .h
      COMMAND "${CMAKE_COMMAND}" "-E" "echo" "Running bison..."
      COMMAND "cmd" "/c" "if exist \"${PARSER_HH}\" copy /Y \"${PARSER_HH}\" \"${PARSER_H}\" >nul 2>&1 || echo No .hh produced, skipping copy."
      DEPENDS ${PARSER_Y}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating parser from ${PARSER_Y}"
    )

    # Generate scanner
    if(FLEX_FLAGS)
        add_custom_command(
          OUTPUT ${LEX_CC} ${LEX_H}
          COMMAND "${FLEX_EXECUTABLE}" ${FLEX_FLAGS} "-o${LEX_CC}" "--header-file=${LEX_H}" "${SCANNER_L}"
          DEPENDS ${SCANNER_L}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMENT "Generating scanner from ${SCANNER_L}"
        )
    else()
        add_custom_command(
          OUTPUT ${LEX_CC} ${LEX_H}
          COMMAND "${FLEX_EXECUTABLE}" "-o" "${LEX_CC}" "--header-file=${LEX_H}" "${SCANNER_L}"
          DEPENDS ${SCANNER_L}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMENT "Generating scanner from ${SCANNER_L}"
        )
    endif()

    # Top-level target to ensure generated files exist before building targets that use them
    add_custom_target(gen_flex_bison_files DEPENDS ${PARSER_CC} ${PARSER_H} ${LEX_CC} ${LEX_H})

    # Expose variables used later in SOURCES_ALL
    set(BISON_GS2Parser_INPUT ${PARSER_Y})
    set(FLEX_GS2Scanner_INPUT ${SCANNER_L})
    set(BISON_GS2Parser_OUTPUTS ${PARSER_CC} ${PARSER_H})
    set(FLEX_GS2Scanner_OUTPUTS ${LEX_CC} ${LEX_H})
else()
	find_package(BISON 3.0 REQUIRED)
	find_package(FLEX REQUIRED)
	BISON_TARGET(GS2Parser generator/gs2parser.y ${CMAKE_CURRENT_BINARY_DIR}/gs2parser.tab.cc)
	FLEX_TARGET(GS2Scanner generator/gs2scanner.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.h COMPILE_FLAGS "${FLEX_FLAGS}")
	ADD_FLEX_BISON_DEPENDENCY(GS2Scanner GS2Parser)
endif()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	src
)

# -- BEGIN GENERATED FILE HANDLING --
# Collect any generated file variables and mark headers as GENERATED so CMake
# won't require them to exist at configure time. Also extract generated .c/.cc/.cpp files
# to add to the source list.
set(GENERATED_C_SOURCES)

set(_gen_list)
if(DEFINED BISON_GS2Parser_OUTPUTS)
    list(APPEND _gen_list ${BISON_GS2Parser_OUTPUTS})
endif()
if(DEFINED FLEX_GS2Scanner_OUTPUTS)
    list(APPEND _gen_list ${FLEX_GS2Scanner_OUTPUTS})
endif()

foreach(_f IN LISTS _gen_list)
    # Mark headers as GENERATED (so missing headers don't break configure)
    if(_f MATCHES "\\.h$" OR _f MATCHES "\\.hh$")
        set_source_files_properties(${_f} PROPERTIES GENERATED TRUE)
    endif()

    # Add only generated C/C++ source files to final sources list
    if(_f MATCHES "\\.c$" OR _f MATCHES "\\.cc$" OR _f MATCHES "\\.cpp$" OR _f MATCHES "\\.cxx$")
        list(APPEND GENERATED_C_SOURCES ${_f})
    endif()
endforeach()
# -- END GENERATED FILE HANDLING --

set(SOURCES_ALL
	src/ast/ast.cpp
	src/encoding/buffer.cpp
	src/visitors/GS2CompilerVisitor.cpp
	src/GS2BuiltInFunctions.cpp
	src/GS2Bytecode.cpp
	src/GS2Context.cpp
	src/Parser.cpp
	src/c_interface.cpp

	src/ast/ast.h
	src/ast/astvisitor.h
	src/ast/astnodevisitor.h
	src/ast/expressiontypes.h
	src/encoding/buffer.h
	src/encoding/graalencoding.h
	src/utils/EventHandler.h
	src/exceptions/GS2CompilerError.h
	src/utils/ContextThreadPool.h
	src/visitors/FunctionInspectVisitor.h
	src/visitors/GS2CompilerVisitor.h
	src/visitors/GS2SourceVisitor.h
	src/CompilerThreadJob.h
	src/GS2BuiltInFunctions.h
	src/GS2Bytecode.h
	src/GS2Context.h
	src/opcodes.h
	src/Parser.h

	${BISON_GS2Parser_INPUT}
	${FLEX_GS2Scanner_INPUT}
	${GENERATED_C_SOURCES})

add_subdirectory(${PROJECT_SOURCE_DIR}/dependencies/fmtlib EXCLUDE_FROM_ALL)

if (DEFINED EMSCRIPTEN)
	add_executable(gs2test ${SOURCES_ALL} src/js_interface.cpp)
	set_target_properties(gs2test PROPERTIES LINK_FLAGS "--embind-emit-tsd gs2test.d.ts -s ENVIRONMENT=web -s DYNAMIC_EXECUTION=0 -s SINGLE_FILE=1 -s MODULARIZE -s 'EXPORT_NAME=GS2Compiler' --bind")
else()
	add_executable(gs2test ${SOURCES_ALL} src/main.cpp)
endif()

if (STATIC)
	add_library(gs2compiler STATIC ${SOURCES_ALL})
else()
	add_library(gs2compiler SHARED ${SOURCES_ALL})
endif()

if(WIN32 AND MINGW)
	target_compile_options(gs2compiler PRIVATE "-fno-rtti")
endif()

set_property(TARGET gs2test PROPERTY CXX_STANDARD 20)
set_property(TARGET gs2compiler PROPERTY CXX_STANDARD 20)

target_link_libraries(gs2test fmt::fmt)
target_link_libraries(gs2compiler fmt::fmt)

# Add dependencies for Windows to ensure generated files exist
if(WIN32)
    add_dependencies(gs2test gen_flex_bison_files)
    add_dependencies(gs2compiler gen_flex_bison_files)
endif()

set(GS2COMPILER_INCLUDE_DIRECTORY
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_BINARY_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src"
	PARENT_SCOPE)

# Python bindings with pybind11
option(BUILD_PYTHON "Build Python bindings" OFF)
if(BUILD_PYTHON)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(gs2compiler_py bindings/python/module.cpp ${SOURCES_ALL})
    target_link_libraries(gs2compiler_py PRIVATE fmt::fmt)
    set_target_properties(gs2compiler_py PROPERTIES
        OUTPUT_NAME gs2compiler
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gs2compiler
    )
    install(TARGETS gs2compiler_py LIBRARY DESTINATION .)

    # Add dependency for Windows to ensure generated files exist
    if(WIN32)
        add_dependencies(gs2compiler_py gen_flex_bison_files)
    endif()
endif()
