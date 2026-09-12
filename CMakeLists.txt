cmake_minimum_required(VERSION 3.10)

project(BufferPool)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    main.cpp
    src/buffer/buffer_pool_manager.cpp
    src/buffer/clock_replacer.cpp
    src/storage/disk_manager.cpp
    src/buffer/page_table.cpp
    tests/buffer_pool_tests.cpp 
    tests/buffer_pool_concurrency_tests.cpp
)

add_executable(buffer_pool ${SOURCES})

target_include_directories(buffer_pool PRIVATE ${CMAKE_SOURCE_DIR}/include)

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if (ENABLE_TSAN)
    target_compile_options(buffer_pool PRIVATE
        -fsanitize=thread
        -g
    )

    target_link_options(buffer_pool PRIVATE
        -fsanitize=thread
    )
endif()

