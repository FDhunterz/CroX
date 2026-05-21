{
  "targets": [
    {
      "target_name": "inputflow_native",
      "sources": [
        "src/bridge.cpp",
        "src/engine_wrapper.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../backend/include",
        "../shared/include"
      ],
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "conditions": [
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "11.0"
          },
          "link_settings": {
            "libraries": [
              "-framework CoreFoundation",
              "-framework Carbon",
              "-framework ApplicationServices"
            ]
          }
        }],
        ["OS=='win'", {
          "libraries": ["-luser32"]
        }]
      ]
    }
  ]
}
