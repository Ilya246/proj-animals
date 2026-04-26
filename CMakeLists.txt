cmake_minimum_required(VERSION 3.14)
project(AnimalProj)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR}/include)
include(FetchContent)

# Use -g for everything
add_compile_options(-g)

set(ENTT_VERSION "3.16.0")
message(STATUS "Detecting system EnTT")
find_package(EnTT ${ENTT_VERSION} QUIET)
if(NOT EnTT_FOUND)
  message(STATUS "No EnTT, fetching from Github...")
  FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v${ENTT_VERSION}
    GIT_SHALLOW    1
  )
  FetchContent_MakeAvailable(EnTT)
endif()

set(SFML_MIN_VERSION "3.0.2")
message(STATUS "Detecting system SFML")
find_package(SFML ${SFML_MIN_VERSION} COMPONENTS Graphics Window System Audio QUIET)
if(NOT SFML_FOUND)
  message(STATUS "No SFML, fetching from Github...")
  set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_NETWORK OFF)

  FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        ${SFML_VERSION}
    GIT_SHALLOW    1
  )
  FetchContent_MakeAvailable(SFML)
endif()

set(YAML_CPP_VERSION "0.9.0")
message(STATUS "Detecting system yaml-cpp")
find_package(yaml-cpp ${YAML_CPP_VERSION} QUIET)
if(NOT yaml-cpp_FOUND)
  message(STATUS "No yaml-cpp, fetching from Github...")
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        yaml-cpp-${YAML_CPP_VERSION}
    GIT_SHALLOW    1
  )
  FetchContent_MakeAvailable(yaml-cpp)
endif()

set(REFLECT_CPP_VERSION "0.24.0")
message(STATUS "Detecting system reflect-cpp")
find_package(reflectcpp ${REFLECT_CPP_VERSION} QUIET)
if(NOT reflectcpp_FOUND)
  message(STATUS "No reflectcpp, fetching from Github...")
  set(REFLECTCPP_YAML ON CACHE BOOL "" FORCE)
  set(REFLECTCPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(REFLECTCPP_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
  set(REFLECTCPP_USE_BUNDLED_DEPENDENCIES ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
      reflectcpp
      GIT_REPOSITORY https://github.com/getml/reflect-cpp.git
      GIT_TAG        v${REFLECT_CPP_VERSION}
      GIT_SHALLOW    1
      GIT_SUBMODULES "" # don't get them
  )
  FetchContent_MakeAvailable(reflectcpp)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include ${entt_SOURCE_DIR}/src)

# disable warnings for reflectcpp
if(DEFINED reflectcpp_SOURCE_DIR)
    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${reflectcpp_SOURCE_DIR}/include)
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    yaml-cpp
    reflectcpp
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:-g -O0>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:APROJ_DEBUG>
)

target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Release>:-O3 -flto=auto>
)

if(MINGW)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wa,-mbig-obj)
endif()

# Warning levels
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy resources
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMENT "Copying resources to output directory"
  COMMAND ${CMAKE_COMMAND} -E remove_directory
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
)
