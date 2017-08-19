#ifndef MONITOR_MONITOR_H
#define MONITOR_MONITOR_H

//#include "common.h"
#include <stdio.h>

//#define GIT_REMOTE "git@git.kuainiujinke.com:zhangpanpan/test_.git"
//#define GIT_PATH "/data/www/web"
//#define WEB_PATH "/data/www/web1"


typedef int (*monitor_cb)(char* git_filename, char *filename, struct inotify_event *event);

#define PORT 9090

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
int action_log(char *message);

int git_fetch(char *tag);
int find_space(char *message, int size);

int creat_action(char*, char *, struct inotify_event *);
int del_action(char*, char *, struct inotify_event *);
int diff_action(char*, char *, struct inotify_event *);


static void skeleton_daemon(const char *lockfile);

static int parse_config(char *filename);

static int find_sep(const char *message, int size, char sep);

char *trimwhitespace(char *str);
#endif //MONITOR_MONITOR_H
