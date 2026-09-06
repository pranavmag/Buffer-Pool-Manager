cmake_minimum_required(VERSION 3.10)

project(BufferPool)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    main.cpp
    src/buffer_pool_manager.cpp
    src/clock_replacer.cpp
    src/disk_manager.cpp
    src/page_table.cpp
)

add_executable(buffer_pool ${SOURCES})

target_include_directories(buffer_pool PRIVATE ${CMAKE_SOURCE_DIR}/include)