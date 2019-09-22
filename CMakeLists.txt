cmake_minimum_required (VERSION 3.8)

project(sg20_graphgen)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option (SG20GG_COLORED_OUTPUT "Produce ANSI-colored output." TRUE)
if (${COLORED_OUTPUT})
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
     add_compile_options (-fdiagnostics-color=always)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
     add_compile_options (-fcolor-diagnostics)
  endif ()
endif (${COLORED_OUTPUT})

option(SG20GG_USE_LLD 0)
if (SG20GG_USE_LLD)
  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
  else()
    message(FATAL_ERROR "Compiler does not support lld as linker")
  endif()
endif()

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.45.0 COMPONENTS graph) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
endif()

include_directories(
  include/
  external/HtmlBuilder/include/
  external/abseil-cpp/
  external/yaml-cpp/include/
)

option(YAML_CPP_BUILD_TESTS OFF)
add_subdirectory(external/yaml-cpp)

add_subdirectory(external/abseil-cpp)
add_subdirectory(src)
