cmake_minimum_required(VERSION 3.15)
project(compressor C)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_C_STANDARD 23)
set(CMAKE_CXX_FLAGS "-Os -Wall -Wextra")
set(CMAKE_C_FLAGS   "-Os -Wall -Wextra -std=c23")

set(CMAKE_BUILD_DIRECTORY "build")
set(OUTPUT_PATH "build/bin")

# find_package(ZLIB REQUIRED)
if(NOT ZLIB_LIBRARY)
    set(ZLIB_LIBRARY "libz.a")
endif()
if(NOT ZLIB_INCLUDE_DIR)
    set(ZLIB_INCLUDE_DIR "/mingw64/include")
endif()

include_directories("/mingw64/include")
link_directories("/mingw64/lib")

add_executable(${PROJECT_NAME} src/main.c)
# target_link_libraries(${PROJECT_NAME} ${ZLIB_LIBRARY})
install(TARGETS ${PROJECT_NAME} DESTINATION ${OUTPUT_PATH})