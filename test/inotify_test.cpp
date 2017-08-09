#include <stdio.h>
#include <string.h>
#include <inotifytools/inotifytools.h>
#include <inotifytools/inotify.h>
#include <sys/select.h>
#include <cstdlib>
#include <iostream>

#define GIT_PATH " https://zhangpanpan:zxcvbnm1@git.kuainiujinke.com/zhangpanpan/encryptor.git"

void init()
{
    if ( !inotifytools_initialize()
         || !inotifytools_watch_recursively( "/tmp/tmpintf", IN_ALL_EVENTS ) ) {
        fprintf(stderr, "%s\n", strerror( inotifytools_error() ) );
        exit(EXIT_FAILURE);
    }
}

int main() {
    // initialize and watch the entire directory tree from the current working
    // directory downwards for all events
    init();

    // set time format to 24 hour time, HH:MM:SS
    inotifytools_set_printf_timefmt( "%T" );

    static fd_set read_fds;
    int rc;
    FD_ZERO(&read_fds);
    FD_SET(inotify_fd, &read_fds);

    rc = select(inotify_fd + 1, &read_fds, NULL, NULL, NULL);

    // Output all events as "<timestamp> <path> <events>"
    struct inotify_event * event = inotifytools_next_event( -1 );
//    while ( event ) {
//        inotifytools_printf( event, "%T %w%f %e\n" );
//        event = inotifytools_next_event( -1 );
//    }
    while (event) {
        char *pathname = inotifytools_filename_from_wd(event->wd);
        std::cout << pathname;
        if (event->len > 0)
            std::cout << event->name;
        char *event_str = inotifytools_event_to_str_sep(event->mask, ',');
        std::cout << " " << event_str;
        std::cout << std::endl;
        event = inotifytools_next_event( -1 );
    }
}

