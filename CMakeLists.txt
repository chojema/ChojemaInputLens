cmake_minimum_required(VERSION 3.25)

# The version here is the single source for the executable and release package.
project(ChojemaInputLens VERSION 0.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(INPUTLENS_METRICS "Write event counters to a file for performance checks" OFF)

configure_file(src/win/version.h.in generated/version.h @ONLY)

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
    $<$<BOOL:${INPUTLENS_METRICS}>:INPUTLENS_METRICS>
)
target_compile_options(ChojemaInputLens PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:/utf-8 /W4 /permissive- /GR->
)
target_link_libraries(ChojemaInputLens PRIVATE user32 gdi32 gdiplus shell32 shcore advapi32)
set_property(TARGET ChojemaInputLens PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
