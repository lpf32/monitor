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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <signal.h>
#include <syslog.h>
#include <pwd.h>

/* Change this to whatever your daemon is called */
#define DAEMON_NAME "monitor"

/* Change this to the user under which to run */
#define RUN_AS_USER "daemon"


FILE *log;
char command_buf[4096];
FILE *rc_file;
char command[4096];
char sentry_buf[4096];
static time_t ticks;

char GIT_REMOTE[1024];
char GIT_PATH[1024];
char WEB_PATH[1024];

struct {
    char *name;
    monitor_cb fn;
} commands[] = {
        {"creat_action", creat_action},
        {"del_action", del_action},
        {"diff_action", diff_action},
        { NULL, NULL}
};

struct {
    char name[1024];
    char git_remote[1024];
    char git_path[1024];
    char web_path[1024];
}Entry[24];

static int entry_index = -1;

static void child_handler(int signum)
{
    switch(signum) {
        case SIGALRM: exit(EXIT_FAILURE); break;
        case SIGUSR1: exit(EXIT_SUCCESS); break;
        case SIGCHLD: exit(EXIT_FAILURE); break;
    }
}

int socket_INIT()
{
    int sockfd, n;
    struct sockaddr_in servaddr;
    int yes = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        sys_error("bind error", errno);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) < 0) {
        sys_error("listen error", errno);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void init()
{
    parse_config("/etc/monitor/conf.cfg");
    log_INIT();
//    skeleton_daemon("/data/" DAEMON_NAME);
    inotify_INIT();

}


void inotify_INIT()
{
    if (!inotifytools_initialize())
    {
        syslog(LOG_ERR, "inotify_error: %s\n", strerror(inotifytools_error()));
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i <= entry_index; i++)
    {
        if (!inotifytools_watch_recursively(Entry[i].web_path, IN_ALL_EVENTS))
        {
            syslog(LOG_ERR, "inotify_error: %s %s\n", strerror(inotifytools_error()), Entry[i].web_path);
            exit(EXIT_FAILURE);
        }
    }
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
//        rc = remove_directory(path);
//        if (rc < 0) {
//            perror("rmdir");
//            exit(EXIT_FAILURE);
//        }
        return 1;
    }
    else if (errno != ENOENT)
    {
        return -1;
    }

    return 0;

}

int handle_action(struct inotify_event * event, char *action)
{
    char *pathname = NULL;
    char *eventname = NULL;
    char filename[4096];
    char git_filename[4096];
    int rc;
    int i;

    if (event->len > 0)
        eventname = event->name;


    pathname = inotifytools_filename_from_wd(event->wd);

    if (eventname != NULL)
        snprintf(filename, sizeof(filename), "%s%s", pathname, eventname);
    else
        snprintf(filename, sizeof(filename), "%s", pathname);

    for (i = 0; i <= entry_index; i++)
    {
        if (strcmp(Entry[i].web_path, pathname) == 0)
            break;
    }
    snprintf(git_filename, sizeof(git_filename), "%s%s", Entry[i].git_path, filename + strlen(pathname));
    printf("%s\n", git_filename);

    for (i = 0; commands[i].name != NULL; ++i) {
        if (!strcmp(action, commands[i].name)) {
            rc = commands[i].fn(git_filename, filename, event);
            syslog(LOG_NOTICE, "%s status: %d", action, rc);
        }
    }
}

int creat_action(char* git_filename, char * filename, struct inotify_event * event)
{
    int rc = 0, rst = 0;
    char buf[128];
    int mode_diff = 0;

    struct stat path_stat;

    rc = stat(git_filename, &path_stat);

    if (!rc) {
        rc = access(git_filename, F_OK) != -1 ? 1 : 0;
        mode_diff = S_ISDIR(path_stat.st_mode) ^ !!(event->mask & IN_ISDIR);
    }

    if (rc < 0 || !rc || mode_diff) {
        snprintf(command, sizeof command, "rm -rf %s 2>&1", filename);

        bzero(command_buf, sizeof command_buf);
        rc_file = popen(command, "r");
        rst = fread(command_buf, sizeof command_buf, 1, rc_file);
        rst = fclose(rc_file);

        snprintf(buf, sizeof buf, "%s was created", filename);
//        send_sentry(buf, command_buf);
        action_log(buf);
//        if (rc != 0) {
//            sys_error("git clone error", rc);
//        }
    }
    return rst;
}


int del_action(char* git_filename, char * filename, struct inotify_event * event)
{
    int rc = 0, rst = 0;
    char buf[128];

    bzero(command_buf, sizeof command_buf);


    rc = access( git_filename, F_OK ) != -1? 1: 0;

    if (rc == 1) {

        snprintf(command, sizeof command, "cp -rf %s %s 2>&1", git_filename, filename);

        rc_file = popen(command, "r");
        rst = fread(command_buf, sizeof command_buf, 1, rc_file);
        rst = fclose(rc_file);

        snprintf(buf, sizeof buf, "%s was deleted", filename);
//        send_sentry(buf, command_buf);
        action_log(buf);

    }

    return rst;
}


int diff_action(char* git_filename, char * filename, struct inotify_event * event)
{
    char buf[256];
    int rc = 0;

    bzero(command_buf, sizeof command_buf);
    snprintf(buf, sizeof buf, "%s was MODIFY", filename);
    action_log(buf);

    snprintf(command, sizeof(command), "diff_command.sh %s %s", git_filename, filename);
    rc = system(command);
    return rc;
//    syslog(LOG_NOTICE, "diff rc %d", rc);

}

static int find_sep(const char *message, int size, char sep)
{
    int i;
    for (i = 0; i < size; i++)
    {
        if (*(message+i) == sep)
        {
            return i;
        }
    }

    return i;
}

char *trimwhitespace(char *str)
{
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

static int parse_config(char *filename) {
    FILE *f;
    char buf[1024];
    char *pos = NULL;
    char *begin;

    int in_entry = 0;
    int cnt = 0;
    if ((f = fopen(filename, "r")) == NULL) {
        perror("fopen error");
    }
    while (fgets(buf, sizeof buf, f) != NULL)
    {
        begin = trimwhitespace(buf);
        if (strlen(begin) == 0)
            continue;

        if ((pos = strchr(begin, '[')) == NULL)
        {
            if (in_entry == 0)
                continue;
        }
        else
        {
            entry_index++;

            *begin++ = '\0';
            pos = strchr(begin, ']');
            if (pos == NULL)
            {
                perror("config format error");
                exit(1);
            }

            *pos = '\0';
            memcpy(Entry[entry_index].name, begin, strlen(begin)+1);
            in_entry = 1;
            printf("GIT NAME: %s\n", Entry[entry_index].name);
            continue;
        }


        if ((pos = strchr(begin, '\n')) != NULL)
            *pos = '\0';

        pos = strchr(begin, '=');
        if (pos == NULL)
        {
            perror("config format error");
            exit(-1);
        }
        *pos++ = '\0';
        if (strcmp(begin, "GIT_REMOTE") == 0) {
            memcpy(Entry[entry_index].git_remote, pos, strlen(pos));
            printf("GIT_REMOTE: %s\n", Entry[entry_index].git_remote);
        } else if (strcmp(begin, "GIT_PATH") == 0) {
            memcpy(Entry[entry_index].git_path, pos, strlen(pos));
            printf("GIT_PATH: %s\n", Entry[entry_index].git_path);
        } else if (strcmp(begin, "WEB_PATH") == 0) {
            memcpy(Entry[entry_index].web_path, pos, strlen(pos));
            printf("WEB_PATH: %s\n", Entry[entry_index].web_path);
        }
    }

    if (entry_index == -1)
    {
        perror("config format error");
        exit(-1);
    }

    return 0;
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

void log_INIT()
{
//    log = fopen(LOG_PATH, "a");
//    if (log == NULL) {
//        perror("fopen error");
//        sys_error("fopen error", errno);
//    }
    openlog( DAEMON_NAME, LOG_PID, LOG_LOCAL5 );
    syslog( LOG_INFO, "starting" );
}



int sys_error(char *message, int errnum)
{
    ticks = time(NULL);

    snprintf(sentry_buf, sizeof sentry_buf, "[%.24s] %s: %s: %s\n", ctime(&ticks), message ,strerror(errnum), command_buf);

    send_sentry(message, sentry_buf);
    syslog(LOG_ERR, sentry_buf);
    exit(EXIT_FAILURE);
}

int action_log(char *message)
{
    static time_t ticks;
    ticks = time(NULL);

    snprintf(sentry_buf, sizeof sentry_buf, "[%.24s] %s: %s\n", ctime(&ticks), message, command_buf);

    send_sentry(message, sentry_buf);
    syslog(LOG_ERR, sentry_buf);
}

int send_sentry(char *message, char *content)
{
    snprintf(command, sizeof command, "sentry_report.py %s", message);
    rc_file = popen(command, "w");
    fwrite(content, strlen(content), 1, rc_file);
    fclose(rc_file);
}

int git_fetch(char *tag)
{
    int rc;

    snprintf(command, sizeof command, "cd %s && git checkout master && git pull && git checkout %s 2>&1",
             GIT_PATH, tag);
    rc_file = popen(command, "r");
    rc = fread(command_buf, sizeof command_buf, 1, rc_file);
    rc = fclose(rc_file);

    if (rc != 0) {
//        sys_error("git clone error", rc);
        action_log("git clone error");
        return 1;
    }
    return 0;
}

int find_space(char *message, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        if (isspace(*(message+i)))
        {
            return i;
        }
    }

    return i;
}

static void skeleton_daemon(const char *lockfile)
{
    pid_t pid, sid, parent;
    int lfp = -1;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Create the lock file as the current user */
    if ( lockfile && lockfile[0] ) {
        lfp = open(lockfile,O_RDWR|O_CREAT,0640);
        if ( lfp < 0 ) {
            fprintf( stderr, "unable to create lock file %s, code=%d (%s)",
                    lockfile, errno, strerror(errno) );
            exit(EXIT_FAILURE);
        }
    }

    /* Drop user if there is one, and we were run as root */
//    if ( getuid() == 0 || geteuid() == 0 ) {
//        struct passwd *pw = getpwnam(RUN_AS_USER);
//        if ( pw ) {
//            syslog( LOG_NOTICE, "setting user to " RUN_AS_USER );
//            setuid( pw->pw_uid );
//        }
//    }

    /* Trap signals that we expect to recieve */
    signal(SIGCHLD,child_handler);
    signal(SIGUSR1,child_handler);
    signal(SIGALRM,child_handler);

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        syslog( LOG_ERR, "unable to fork daemon, code=%d (%s)",
                errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {

        /* Wait for confirmation from the child via SIGTERM or SIGCHLD, or
           for two seconds to elapse (SIGALRM).  pause() should not return. */
        alarm(2);
        pause();

        exit(EXIT_FAILURE);
    }

    /* At this point we are executing as the child process */
    parent = getppid();

    /* Cancel certain signals */
    signal(SIGCHLD,SIG_DFL); /* A child process dies */
    signal(SIGTSTP,SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
    signal(SIGTERM,SIG_DFL); /* Die on SIGTERM */

    /* Change the file mode mask */
    umask(002);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        syslog( LOG_ERR, "unable to create a new session, code %d (%s)",
                errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        syslog( LOG_ERR, "unable to change directory to %s, code %d (%s)",
                "/", errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);

    /* Tell the parent process that we are A-okay */
    kill( parent, SIGUSR1 );
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
    nfds_t fdsize = 2;

    init();
    listenfd = socket_INIT();

    for (int i = 0; i <= entry_index; i++)
    {
        rc = dir_exists(Entry[i].git_path);
        if (rc == 1)
        {
//            rc = remove_directory(Entry[i].git_path);
            snprintf(command, sizeof command, "rm -rf %s 2>&1", Entry[i].git_path);
            rc_file = popen(command, "r");
            rc = fread(command_buf, sizeof command_buf, 1, rc_file);
            rc = pclose(rc_file);
            if (rc < 0)
            {
                sys_error("remove directory error", errno);
            }
        }

        snprintf(command, sizeof command, "git clone %s %s 2>&1", Entry[i].git_remote, Entry[i].git_path);
        rc_file = popen(command, "r");
        rc = fread(command_buf, sizeof command_buf, 1, rc_file);
        rc = pclose(rc_file);
        if (rc != 0)
        {
            sys_error("git clone error", errno);
        }
    }

    fds[0].fd = listenfd;
    fds[0].events = POLLRDNORM | POLLIN;

    fds[1].fd = inotify_fd;
    fds[1].events = POLLRDNORM | POLLIN;


    for (;;) {
        rc = poll(fds, fdsize, -1);

        if (rc < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (rc == 0) {
            continue;
        }

        if (fds[1].revents & POLLIN)
        {
            // Output all events as "<timestamp> <path> <events>"
            struct inotify_event *event = inotifytools_next_event(-1);
            while (event) {
                inotifytools_printf(event, "%T %w%f %e\n");

                if (event->mask & IN_ACCESS
                    || event->mask & IN_OPEN
                    || event->mask & IN_CLOSE_NOWRITE
                    || event->mask & IN_CLOSE
                    || event->mask & IN_ATTRIB)
                {
                } else if (event->mask & IN_CREATE) {
                    handle_action(event, "creat_action");
                } else if (event->mask & IN_DELETE || event->mask & IN_DELETE_SELF) {
                    handle_action(event, "del_action");
                } else if (event->mask & IN_MODIFY) {
                    handle_action(event, "diff_action");
                }
                event = inotifytools_next_event(-1);
            }
        } else if (fds[0].revents & POLLIN){
            clifd = accept(listenfd, NULL, NULL);
            n = get_line(clifd, buf, sizeof(buf));
            if (n < 0)
                continue;

            buf[n-1] = '\0';
            if (strcmp(buf, "GIT") != 0) {
                continue;
            }

            n = get_line(clifd, buf, sizeof(buf));
            if (n < 0)
                continue;
            buf[n-1] = '\0';


            if (strcmp(buf, "flush") == 0) {

                n = get_line(clifd, buf, sizeof(buf));
                buf[n-1] = '\0';

                rc = find_space(buf, n); // rc 为　index

                if (rc == n) {
                    memcpy(command_buf, buf, strlen(buf));
                    command_buf[strlen(buf)] = '\0';
                    sys_error("socket message format error", 234234);
                }

                buf[rc] = '\0';

                rc = git_fetch(buf+rc+1);

                //暂停　ｍｏｎｉｔｏｒ　ｗｅｂl
                if (rc == 0)
                {
                    fdsize = 1;
                    write(clifd, "OK\r\n", 2);
                }
                else
                {
                    write(clifd, "ERROR\r\n", 2);
                }


            }
            else if (strcmp(buf, "monitor") == 0)
            {
                fdsize = 2;
                inotify_INIT();
                fds[1].fd = inotify_fd;
                fds[1].events = POLLRDNORM | POLLIN;

                write(clifd, "OK\r\n", 2);
            }
            close(clifd);
        }
    }

    return 0;
}