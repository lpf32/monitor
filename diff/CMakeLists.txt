set(DIFF_SOURCE
        config.h
        system.h
        analyze.c
        cmp.c
        cmpbuf.c
        context.c
        dir.c
        ed.c
#        error.c
        fnmatch.c
        getopt.c
        getopt1.c
        ifdef.c
        io.c
        normal.c
        regex.c
        side.c
        util.c
        version.c
        xmalloc.c
)

add_library(diff2 ${DIFF_SOURCE})

add_executable(cmp cmp.c)
target_link_libraries(cmp diff2)

add_executable(diff diff.c)
target_link_libraries(diff diff2)

add_executable(diff3 diff3.c)
target_link_libraries(diff3 diff2)

add_executable(sdiff sdiff.c)
target_link_libraries(sdiff diff2)

