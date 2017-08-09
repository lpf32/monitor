#ifndef MONITOR_MONITOR_H
#define MONITOR_MONITOR_H

#include "common.h"

#define GIT_REMOTE "https://zhangpanpan:zxcvbnm1@git.kuainiujinke.com/zhangpanpan/encryptor.git"
#define GIT_PATH "/data/www/web"
#define WEB_PATH "/data/www/web1"

#define PORT 9000

void init();
void git_INIT();
void diff_INIT();
void inotify_INIT();

int git_warpper(int argc, char *argv[]);
int dir_exists(char *path);
int remove_directory(const char *path);
void signal_handler();
int handle_diff(struct inotify_event * event);

int diff(int argc, char *argv[]);
int socket_INIT();
int get_line(int, char*, int);
#endif //MONITOR_MONITOR_H
