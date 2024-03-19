cmake_minimum_required(VERSION 3.10)
project(ECE141-Archive)

set(CMAKE_CXX_STANDARD 17)
find_package(ZLIB REQUIRED)

include_directories(.)

add_executable(archive
        Archive.cpp
        Archive.hpp
        main.cpp
        Testable.hpp
        Testing.hpp
        Timer.hpp
        Tracker.hpp
        ChunkManager.cpp
        )
target_link_libraries(archive PRIVATE ${ZLIB_LIBRARIES})
target_include_directories(archive PRIVATE ${ZLIB_INCLUDE_DIRS})