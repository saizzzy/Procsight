cmake_minimum_required(VERSION 3.20)

project(ProcSight VERSION 1.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep build artifacts under build/bin and build/lib.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

find_package(Curses REQUIRED)

set(PROCSIGHT_SOURCES
    src/main.cpp
    src/ProcessList.cpp
    src/SystemStats.cpp
    src/App.cpp
    src/Renderer.cpp
    src/Theme.cpp
    src/NcursesSession.cpp
)

add_executable(procsight ${PROCSIGHT_SOURCES})

target_include_directories(procsight PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${CURSES_INCLUDE_DIRS}
)

target_link_libraries(procsight PRIVATE
    ${CURSES_LIBRARIES}
    ncursesw
)

target_compile_features(procsight PRIVATE cxx_std_17)