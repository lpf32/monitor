set(LIBSRC
        inotifytools.c
        redblack.c
        )


add_library(libinotify ${LIBSRC})


add_executable(example example.cpp)
target_link_libraries(example libinotify)