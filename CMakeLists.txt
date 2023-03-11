cmake_minimum_required(VERSION 3.11)
project(pattern-matching)

option(INSOURCEBUILD OFF)
if((${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR}) AND NOT
                                                              ${INSOURCEBUILD})
  message(
    FATAL_ERROR
      "In-source building disabled. Provide -DINSOURCEBUILD option ifyou are sure about it."
  )
endif()

set(CMAKE_CXX_STANDARD 20)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
endif()

# Possibly support windows and MSVC compiler
if(MSVC)
  add_compile_options(/W3)
  add_definitions("/EHsc")
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(LINT OFF)

if(${LINT})
  find_program(CLANG_TIDY_EXECUTABLE "clang-tidy")

  set(CLANG_TIDY_COMMAND
      "${CLANG_TIDY_EXECUTABLE}"
      "-checks=-*,modernize-*,-modernize-use-trailing-return-type*,readability-*,portability-*,bugprone-*"
  )

  function(target_enable_linter TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_CLANG_TIDY
                                                    "${CLANG_TIDY_COMMAND}")
  endfunction()
else()
  function(target_enable_linter TARGET_NAME)
    # Do nothing
  endfunction()
endif()

option(OPTIMIZE_FOR_NATIVE "Build with -march=native" ON)
if(OPTIMIZE_FOR_NATIVE)
  include(CheckCXXCompilerFlag)

  check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
  if(COMPILER_SUPPORTS_MARCH_NATIVE)
    message(STATUS "Compiling with -march=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
  else()
    message(WARNING "Compiler does not support -march=native")
  endif()
endif()

# Add flags for GNU sampling profiler gprof in Debug build and RelWithDebInfo
option(PROFILE OFF)
if(PROFILE)
  add_compile_options(-pg)
  add_link_options(-pg)
endif()

option(SANITIZE OFF)
if(SANITIZE)
  add_compile_options(-fsanitize=undefined,address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=undefined,address -fno-omit-frame-pointer)
endif()

add_subdirectory(3rd-party/OpenCL-Headers EXCLUDE_FROM_ALL)
add_subdirectory(3rd-party/OpenCL-ICD-Loader EXCLUDE_FROM_ALL)
add_subdirectory(3rd-party/OpenCL-CLHPP EXCLUDE_FROM_ALL)
add_subdirectory(3rd-party/graphs EXCLUDE_FROM_ALL)

add_library(popl INTERFACE)
target_include_directories(popl INTERFACE 3rd-party/popl)

find_package(
  Python3
  COMPONENTS Interpreter
  REQUIRED)

set(kernel2hpp ${CMAKE_CURRENT_SOURCE_DIR}/scripts/kernel2hpp.py)
set(KERNEL_HPP_DIR ${CMAKE_CURRENT_BINARY_DIR}/kernelhpp/kernelhpp)
set(KERNEL_HPP_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/kernelhpp)

function(add_opencl_program TARGET_NAME INPUT_FILES OPENCL_VERSION)
  add_executable(${TARGET_NAME} ${INPUT_FILES})
  target_link_libraries(${TARGET_NAME} PUBLIC OpenCL::OpenCL OpenCL::Headers
                                              OpenCL::HeadersCpp popl)
  target_compile_definitions(
    ${TARGET_NAME} PUBLIC CL_HPP_TARGET_OPENCL_VERSION=${OPENCL_VERSION}
                          CL_TARGET_OPENCL_VERSION=${OPENCL_VERSION})
  target_include_directories(${TARGET_NAME} PUBLIC include
                                                   ${KERNEL_HPP_INCLUDE})
  target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
endfunction()

function(add_kernel TARGET_NAME INPUT_FILE)
  add_custom_command(
    OUTPUT ${KERNEL_HPP_DIR}/${TARGET_NAME}.hpp
    COMMAND Python3::Interpreter ${kernel2hpp} -i ${INPUT_FILE} -o
            ${KERNEL_HPP_DIR}/${TARGET_NAME}.hpp
    MAIN_DEPENDENCY ${INPUT_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  add_custom_target(${TARGET_NAME} ALL
                    DEPENDS ${KERNEL_HPP_DIR}/${TARGET_NAME}.hpp)
  set(${TARGET_NAME}_OUTPUTS
      ${KERNEL_HPP_DIR}/${TARGET_NAME}.hpp
      PARENT_SCOPE)
endfunction()

find_program(BASH_PROGRAM bash)

add_opencl_program(oclinfo src/oclinfo.cc 220)
target_enable_linter(oclinfo)

add_opencl_program(ahocorasick src/ahocorasick.cc 220)
target_enable_linter(ahocorasick)
target_link_libraries(ahocorasick PUBLIC graphs)
