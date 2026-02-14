cmake_minimum_required(VERSION 3.14)
project(AnimalProj)   # Replace with your project name

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR}/include)
include(FetchContent)

set(ENTT_VERSION "3.16.0")

# Try to find EnTT on the system
message(STATUS "Detecting system EnTT")
find_package(EnTT ${ENTT_VERSION} QUIET)

if (EnTT_FOUND)
  message(STATUS "EnTT ${EnTT_VERSION}")
else ()
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
else ()
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

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

# Add EnTT
target_include_directories(${PROJECT_NAME} PRIVATE ${entt_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMENT "Copying resources to output directory"
  COMMAND ${CMAKE_COMMAND} -E remove_directory
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
)
