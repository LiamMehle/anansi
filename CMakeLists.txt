cmake_minimum_required(VERSION 3.24)
include(FetchContent)
project(raven C)

set (sources main.c)
set (srcdir src)
set (CMAKE_C_STANDARD 23)
set (CMAKE_C_FLAGS "-Os -Wall -Wextra")
set (CMAKE_BUILD_DIRECTORY "build")
set (OUTPUT_PATH "bin")

FetchContent_Declare(zlib
    GIT_REPOSITORY "https://github.com/madler/zlib.git"
    GIT_TAG         v1.3.1
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(zlib)

# find_package(ZLIB REQUIRED)

list (TRANSFORM sources PREPEND "src/")

add_executable(${PROJECT_NAME} ${sources})
# target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
target_link_libraries(${PROJECT_NAME} PRIVATE zlibstatic)
install(TARGETS ${PROJECT_NAME} DESTINATION ${OUTPUT_PATH})