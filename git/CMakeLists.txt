set(GIT_SOURCE
        common.c
        clone.c
        fetch.c
        git2.c
        index-pack.c
        ls-remote.c
)

add_library(libgit ${GIT_SOURCE})