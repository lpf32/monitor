#include <stdio.h>
#include <string.h>
#include <inotifytools/inotifytools.h>
#include <inotifytools/inotify.h>
#include <sys/select.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include "monitor.h"
#include "common.h"
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

/*void abort_handler(int signum)
{

}

void signal_handler()
{
    struct sigaction new_action;
    new_action.sa_handler = abort_handler;
//    sigemptyset(&new_action.sa_mask);

//    sigaction(SIGABRT, &new_action, NULL);
    signal(SIGABRT, SIG_IGN);
}*/

int socket_INIT()
{
    int sockfd, n;
    struct sockaddr_in servaddr;
    int yes = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void init()
{
    inotify_INIT();
    diff_INIT();
}

void diff_INIT()
{

}

void inotify_INIT()
{
    if ( !inotifytools_initialize()
         || !inotifytools_watch_recursively(WEB_PATH, IN_ALL_EVENTS ) ) {
        fprintf(stderr, "%s\n", strerror( inotifytools_error() ) );
        exit(EXIT_FAILURE);
    }
}

void git_INIT()
{
    char *argv[3];
    argv[0] = malloc(100);
    argv[1] = malloc(100);
    argv[2] = malloc(100);
    argv[3] = malloc(100);

    argv[1] = "clone";
    argv[2] = GIT_REMOTE;
    argv[3] = GIT_PATH;
    dir_exists(GIT_PATH);

    git_libgit2_init();
    git_warpper(4, argv);
}

int remove_directory(const char *path)
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d)
    {
        struct dirent *p;

        r = 0;

        while (!r && (p=readdir(d)))
        {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
            {
                continue;
            }

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        r2 = remove_directory(buf);
                    }
                    else
                    {
                        r2 = unlink(buf);
                    }
                }

                free(buf);
            }

            r = r2;
        }

        closedir(d);
    }

    if (!r)
    {
        r = rmdir(path);
    }

    return r;
}

int dir_exists(char *path) {
    DIR *dir = opendir(path);
    int rc;
    if (dir)
    {
        rc = remove_directory(path);
        if (rc < 0) {
            perror("rmdir");
            exit(EXIT_FAILURE);
        }
    }
    else if (errno != ENOENT)
    {
        perror("opendir");
        exit(EXIT_FAILURE);

    }

}

int handle_diff(struct inotify_event * event)
{
    char * pathname = NULL;
    char * eventname = NULL;
    char filename[4096];
    char git_filename[4096];
    char command[4096];

    if (event->len > 0)
        eventname = event->name;
    else
        return 0;

    pathname = inotifytools_filename_from_wd( event->wd );

    snprintf(filename, sizeof(filename), "%s%s", pathname, eventname);

    snprintf(git_filename, sizeof(git_filename), "%s%s", GIT_PATH, filename+strlen(WEB_PATH));


    printf("diff filename: %s\n", filename);
    printf("git filename: %s\n", git_filename);

    char *argv[3];
    argv[0] = malloc(100);
    argv[1] = malloc(100);
    argv[2] = malloc(100);

    /*argv[1] = "/data/www/web/cipher.sql";
    argv[2] = "/data/www/web1/cipher.sql";*/
//    sleep(1);
//    diff(3, argv);

//    pid_t child;
//    child = fork();
    snprintf(command, sizeof(command), "/home/zhang/CLionProjects/monitor/diff_command.sh %s %s", git_filename, filename);
    system(command);

//    system("python /home/zhang/CLionProjects/monitor/sentry_report.py");

}

int get_line(int sock,char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}



int main() {
    // initialize and watch the entire directory tree from the current working
    // directory downwards for all events

    int listenfd, clifd, maxfd;
    static fd_set read_fds;
    struct pollfd fds[2];
    char buf[4096];
    int rc;
    int n;
    init();
    {
        char *argv[4];
        argv[0] = malloc(100);
        argv[1] = malloc(100);
        argv[2] = malloc(100);
        argv[3] = malloc(100);

        argv[1] = "clone";
        argv[2] = GIT_REMOTE;
        argv[3] = GIT_PATH;
        dir_exists(GIT_PATH);

        git_libgit2_init();
        git_warpper(4, argv);
    }

    listenfd = socket_INIT();

    fds[0].fd = listenfd;
    fds[0].events = POLLRDNORM | POLLIN;

    fds[1].fd = inotify_fd;
    fds[1].events = POLLRDNORM | POLLIN;


    for (;;) {
        rc = poll(fds, 2, -1);

        if (rc < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (fds[1].revents & POLLIN) {
            // Output all events as "<timestamp> <path> <events>"
            struct inotify_event *event = inotifytools_next_event(-1);
            while (event) {
                inotifytools_printf(event, "%T %w%f %e\n");
                switch (event->mask) {
                    case IN_ACCESS:
                    case IN_OPEN:
                    case IN_CLOSE_NOWRITE:
                    case IN_CLOSE:
                    case IN_ATTRIB:
                    case IN_CREATE:
                        break;
                    case IN_CLOSE_WRITE:
                    case IN_DELETE:
                    case IN_DELETE_SELF:
                    case IN_MODIFY:
                        handle_diff(event);
                        break;
                }
                event = inotifytools_next_event(-1);
            }
        } else {
            /*n = get_line(listenfd, buf, sizeof(buf));
            if (n > 0) {
                printf("%s\n", buf);
            }*/
            clifd = accept(listenfd, NULL, NULL);
            read(clifd, buf, sizeof(buf));
            printf("%s\n", buf);
            close(clifd);
        }
    }

    git_libgit2_shutdown();
    return 0;
}