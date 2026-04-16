cmake_minimum_required(VERSION 3.14)
project(AnimalProj)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR}/include)
include(FetchContent)

set(ENTT_VERSION "3.16.0")

# TODO: make this only run in debug configuration
add_compile_options(-g -O0)

if (MINGW)
    add_compile_options(-Wa,-mbig-obj)
endif()

# Try to find EnTT on the system
message(STATUS "Detecting system EnTT")
find_package(EnTT ${ENTT_VERSION} QUIET)

if (EnTT_FOUND)
  message(STATUS "EnTT ${EnTT_VERSION}")
else()
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

# Try to find SFML on the system
message(STATUS "Detecting system SFML")
find_package(SFML ${SFML_MIN_VERSION} COMPONENTS Graphics Window System Audio QUIET)

if (SFML_FOUND)
  message(STATUS "SFML ${SFML_VERSION}")
else()
  message(STATUS "No SFML, fetching from Github...")

  set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
  # Network not needed
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

# Fetch yaml-cpp
message(STATUS "Detecting system yaml-cpp")
find_package(yaml-cpp ${YAML_CPP_VERSION} QUIET)

if (yaml-cpp_FOUND)
  message(STATUS "yaml-cpp ${yaml-cpp_VERSION}")
else()
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

# Fetch reflect-cpp
message(STATUS "Detecting system reflect-cpp")
find_package(reflectcpp ${REFLECT_CPP_VERSION} QUIET)

if (reflectcpp_FOUND)
  message(STATUS "reflectcpp ${reflect-cpp_VERSION}")
else()
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

target_include_directories(${PROJECT_NAME} PRIVATE ${entt_SOURCE_DIR}/src ${boost-pfr_SOURCE_DIR}/include)

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

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
# TODO: make this only run in debug configuration
target_compile_options(${PROJECT_NAME} PRIVATE -DAPROJ_DEBUG)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMENT "Copying resources to output directory"
  COMMAND ${CMAKE_COMMAND} -E remove_directory
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
)
