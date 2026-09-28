cmake_minimum_required(VERSION 3.16)

project(CrushASCII VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(CrushASCII
    src/main.cpp
    src/Image.cpp
    src/ASCIIConverter.cpp
    src/Renderer.cpp
)

target_include_directories(CrushASCII PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(CrushASCII PRIVATE /W4 /EHsc /utf-8)
    target_compile_definitions(CrushASCII PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(CrushASCII PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(CrushASCII PROPERTIES
    OUTPUT_NAME CrushASCII
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

install(TARGETS CrushASCII RUNTIME DESTINATION bin)
