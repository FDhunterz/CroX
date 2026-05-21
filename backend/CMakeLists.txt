add_library(inputflow_core STATIC
  src/app/inputflow_app.cpp
  src/app/inputflow_app_profiles.cpp
  src/config/json_mapping_parser.cpp
  src/config/mapping_combo.cpp
  src/config/mapping_validator.cpp
  src/config/profile_manifest.cpp
  src/joystick/joystick_mapping_engine.cpp
  src/joystick/joystick_provider_factory.cpp
  src/input/input_provider_factory.cpp
  src/util/key_code_table.cpp
  src/platform/platform_detect.cpp
)

if(NOT DEFINED INPUTFLOW_ROOT)
  set(INPUTFLOW_ROOT "${CMAKE_SOURCE_DIR}")
endif()

target_include_directories(inputflow_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${INPUTFLOW_ROOT}/shared/include
)

target_link_libraries(inputflow_core PUBLIC nlohmann_json::nlohmann_json)

if(WIN32)
  target_sources(inputflow_core PRIVATE
    src/platform/win/input_provider_win.cpp
    src/platform/win/joystick_provider_win.cpp
  )
  target_link_libraries(inputflow_core PRIVATE user32 xinput)
elseif(APPLE)
  target_sources(inputflow_core PRIVATE
    src/platform/mac/input_provider_mac.mm
    src/platform/mac/joystick_provider_mac.mm
    src/platform/mac/joystick_hid_exclusive_mac.mm
  )
  find_library(GAMECONTROLLER GameController REQUIRED)
  find_library(IOKIT IOKit REQUIRED)
  find_library(COREFOUNDATION CoreFoundation REQUIRED)
  find_library(CARBON Carbon REQUIRED)
  find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
  target_link_libraries(inputflow_core PRIVATE
    ${GAMECONTROLLER}
    ${IOKIT}
    ${COREFOUNDATION}
    ${CARBON}
    ${APPLICATION_SERVICES}
  )
  set_source_files_properties(
    src/platform/mac/input_provider_mac.mm
    src/platform/mac/joystick_provider_mac.mm
    src/platform/mac/joystick_hid_exclusive_mac.mm
    PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
  )
endif()

target_compile_definitions(inputflow_core PRIVATE
  $<$<BOOL:${WIN32}>:INPUTFLOW_PLATFORM_WINDOWS>
  $<$<BOOL:${APPLE}>:INPUTFLOW_PLATFORM_MACOS>
)
