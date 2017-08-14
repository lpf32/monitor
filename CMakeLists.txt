cmake_minimum_required(VERSION 2.7)
project(monitor)

set(CMAKE_C_STANDARD 99)

include_directories(${PROJECT_SOURCE_DIR})

include_directories(libinotifytools/src)
add_subdirectory(libinotifytools/src)

add_subdirectory(test)

add_executable(monitor monitor.c)
target_link_libraries(monitor libinotify)

file(GLOB SHELL "*.sh" "*.py")
message(STATUS ${SHELL})

file(GLOB CONF "conf/conf.cfg")
message(STATUS ${CONF})

install(TARGETS monitor DESTINATION /usr/local/bin)
install(FILES ${SHELL} DESTINATION /usr/local/bin
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ  WORLD_EXECUTE
        )

install(FILES ${CONF} DESTINATION /etc/monitor)