#ifndef MONITOR_MONITOR_H
#define MONITOR_MONITOR_H

//#include "common.h"
#include <stdio.h>

#define GIT_REMOTE "git@git.kuainiujinke.com:zhangpanpan/test_.git"
#define GIT_PATH "/data/www/web"
#define WEB_PATH "/data/www/web1"

#define LOG_PATH "/data/log/monitor/monitor.log"
#define WORK_PATH "/home/zhang/CLionProjects/monitor"

#define PORT 9000

void init();
void diff_INIT();
void inotify_INIT();
void log_INIT();

int git_warpper(int argc, char *argv[]);
int dir_exists(char *path);
int remove_directory(const char *path);

int handle_diff(struct inotify_event * event);
int handle_del(struct inotify_event * event);
int handle_creat(struct inotify_event * event);

int socket_INIT();
int get_line(int, char*, int);
int sys_error(char *message, int errnum);
int send_sentry(char *message, char *content);

void git_fetch(char *tag);
int find_space(char *message, int size);

#endif //MONITOR_MONITOR_H
