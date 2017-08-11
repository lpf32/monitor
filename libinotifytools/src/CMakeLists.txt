set(LIBSRC
        inotifytools.c
        redblack.c
        )


add_library(libinotify ${LIBSRC})


add_executable(example example.c)
target_link_libraries(example libinotify)