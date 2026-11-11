cmake_minimum_required(VERSION 3.20)
project(RX7FB85 VERSION 1.1.0 LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(platform)

add_library(RX7FB85 SHARED src/RX7FB85.cpp)
target_link_libraries(RX7FB85 PRIVATE fh2mod_api)

set_target_properties(RX7FB85 PROPERTIES
    OUTPUT_NAME RX7FB85
    PREFIX ""
)

if(NOT MSVC)
    target_link_options(RX7FB85 PRIVATE -static)
endif()