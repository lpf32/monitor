cmake_minimum_required(VERSION 3.7)
project(monitor)

set(CMAKE_CXX_STANDARD 11)

SET(LIBGIT2_DEFINITIONS ${PC_LIBGIT2_CFLAGS_OTHER})

FIND_PATH(LIBGIT2_INCLUDE_DIR NAMES git2.h
        HINTS
        ${PC_LIBGIT2_INCLUDEDIR}
        ${PC_LIBGIT2_INCLUDE_DIRS}
        )

FIND_LIBRARY(LIBGIT2_LIBRARIES NAMES git2
        HINTS
        ${PC_LIBGIT2_LIBDIR}
        ${PC_LIBGIT2_LIBRARY_DIRS}
        )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libgit2 DEFAULT_MSG LIBGIT2_LIBRARIES LIBGIT2_INCLUDE_DIR)

MARK_AS_ADVANCED(LIBGIT2_INCLUDE_DIR LIBGIT2_LIBRARIES)

find_library(LIBGIT2_LIBRARIES NAMES git2)

include_directories(${PROJECT_SOURCE_DIR})


FIND_PATH(LIBRAVEN_INCLUDE_DIR NAMES raven.h)

find_library(LIBRAVEN_LIBRARIES  NAMES ravenpp)


include_directories(libinotifytools/src)

add_subdirectory(libinotifytools/src)

set(SOURCE_FILES main.cpp gittest.cpp sentry_test.cpp)
add_executable(defences ${SOURCE_FILES})

add_executable(gittest gittest.cpp)
target_link_libraries(gittest git2)

add_subdirectory(diff)
include_directories(diff)

add_executable(sentry_test sentry_test.cpp)
target_link_libraries(sentry_test ravenpp boost_iostreams z)