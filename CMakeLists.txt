cmake_minimum_required(VERSION 3.20)
project(InputFlow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(INPUTFLOW_BUILD_NATIVE "Build N-API native addon" ON)
option(INPUTFLOW_BUILD_TESTS "Build unit tests" OFF)

# nlohmann/json via FetchContent
include(FetchContent)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

add_subdirectory(backend)

# Native addon: build from native/ with npm run build (cmake-js).
# Root CMake only builds inputflow_core unless you integrate manually.
if(INPUTFLOW_BUILD_NATIVE AND CMAKE_JS_INC)
  message(STATUS "INPUTFLOW_BUILD_NATIVE with CMAKE_JS_INC — use native/ npm run build instead")
endif()

if(INPUTFLOW_BUILD_TESTS)
  enable_testing()
  add_subdirectory(backend/tests)
endif()
