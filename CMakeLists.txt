cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()

# The version here is the single source for the executable and release package.
project(ChojemaInputLens VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Platform-independent logic, shared by every platform.
add_library(InputLensCore STATIC src/core/input_tracker.cpp)
target_include_directories(InputLensCore PUBLIC src/core)

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    target_compile_options(InputLensCore PRIVATE -Wall -Wextra -Wpedantic)
    add_executable(ChojemaInputLens MACOSX_BUNDLE
        src/mac/main.mm src/mac/key_labels.mm src/mac/overlay.mm
        assets/icon/ChojemaInputLens.icns LICENSE src/mac/INSTALL.md
    )
    set_source_files_properties(assets/icon/ChojemaInputLens.icns LICENSE src/mac/INSTALL.md
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_compile_options(ChojemaInputLens PRIVATE -fobjc-arc -Wall -Wextra -Wpedantic)
    target_link_libraries(ChojemaInputLens PRIVATE InputLensCore "-framework Cocoa" "-framework Carbon")
    set_target_properties(ChojemaInputLens PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Chojema InputLens"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/mac/Info.plist.in"
    )
elseif(WIN32)
enable_language(RC)
configure_file(src/win/version.h.in generated/version.h @ONLY)
target_compile_options(InputLensCore PRIVATE /utf-8 /W4 /permissive- /GR-)

add_executable(ChojemaInputLens WIN32
    src/win/main.cpp
    src/win/key_labels.cpp
    src/win/overlay.cpp
    src/win/app.rc
    src/win/app.manifest
)

target_include_directories(ChojemaInputLens PRIVATE
    src/win
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_compile_definitions(ChojemaInputLens PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
)
target_compile_options(ChojemaInputLens PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:/utf-8 /W4 /permissive- /GR->
)
target_link_libraries(ChojemaInputLens PRIVATE user32 gdi32 gdiplus shell32 shcore advapi32 InputLensCore)
set_property(TARGET ChojemaInputLens PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
    message(FATAL_ERROR "Chojema InputLens supports Windows and macOS.")
endif()
