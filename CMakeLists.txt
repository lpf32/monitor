cmake_minimum_required(VERSION 3.7)
project(monitor)

set(CMAKE_C_STANDARD 99)

include_directories(${PROJECT_SOURCE_DIR})

include_directories(libinotifytools/src)
add_subdirectory(libinotifytools/src)

add_subdirectory(test)

add_executable(monitor monitor.c)
target_link_libraries(monitor libinotify)
