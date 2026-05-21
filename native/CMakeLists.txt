cmake_minimum_required(VERSION 3.20)

# Standalone CMake project for cmake-js (run from native/: npm run build)
# Do not use -d .. ; cmake-js sets CMAKE_JS_INC / CMAKE_JS_LIB automatically.

project(inputflow_native VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INPUTFLOW_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

# --- Node / N-API headers (cmake-js) ---
if(NOT CMAKE_JS_INC)
  message(FATAL_ERROR
    "CMAKE_JS_INC not set. Build with:\n"
    "  cd native && npm install && npm run build\n"
    "Or: npx cmake-js compile -r electron -v 28.3.3 -T inputflow_native")
endif()

set(NODE_ADDON_API_DIR "${CMAKE_CURRENT_LIST_DIR}/node_modules/node-addon-api")
if(NOT EXISTS "${NODE_ADDON_API_DIR}/napi.h")
  message(FATAL_ERROR "node-addon-api not found. Run: cd native && npm install")
endif()

message(STATUS "CMAKE_JS_INC=${CMAKE_JS_INC}")
message(STATUS "NODE_ADDON_API_DIR=${NODE_ADDON_API_DIR}")

# --- nlohmann/json ---
include(FetchContent)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

# --- Backend core ---
add_subdirectory("${INPUTFLOW_ROOT}/backend" "${CMAKE_BINARY_DIR}/inputflow_backend")

# --- N-API addon ---
add_library(inputflow_native SHARED
  src/bridge.cpp
  src/engine_wrapper.cpp
)

target_include_directories(inputflow_native PRIVATE
  ${CMAKE_JS_INC}
  ${NODE_ADDON_API_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/src
  ${INPUTFLOW_ROOT}/backend/include
  ${INPUTFLOW_ROOT}/shared/include
)

target_link_libraries(inputflow_native PRIVATE
  inputflow_core
  ${CMAKE_JS_LIB}
)

target_compile_definitions(inputflow_native PRIVATE
  NAPI_DISABLE_CPP_EXCEPTIONS
  NODE_ADDON_API_DISABLE_DEPRECATED
  $<$<BOOL:${WIN32}>:INPUTFLOW_PLATFORM_WINDOWS>
  $<$<BOOL:${APPLE}>:INPUTFLOW_PLATFORM_MACOS>
)

set_target_properties(inputflow_native PROPERTIES
  PREFIX ""
  SUFFIX ".node"
  CXX_STANDARD 20
  POSITION_INDEPENDENT_CODE ON
)

if(APPLE)
  target_link_options(inputflow_native PRIVATE -undefined dynamic_lookup)
  find_library(GAMECONTROLLER GameController REQUIRED)
  target_link_libraries(inputflow_native PRIVATE ${GAMECONTROLLER})
elseif(WIN32)
  target_link_libraries(inputflow_native PRIVATE user32)
endif()
