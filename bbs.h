#ifndef __BBS_H__
#define __BBS_H__

#include <time.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_STR "dev"

struct bbs_config {
	char *bbs_name;
	char *sysop_name;
	
	char *ansi_path;
	char *bbs_path;
	int nodes;
	int newuserlvl;
	
};

struct sec_level_t {
	int timeperday;
};

struct user_record {
	int id;
	char *loginname;
	char *password;
	char *firstname;
	char *lastname;
	char *email;
	char *location;
	int sec_level;
	struct sec_level_t *sec_info;
	time_t laston;
	int timeleft;
};


extern void runbbs(int sock, char *config);

extern void s_putchar(int socket, char c);
extern void s_putstring(int socket, char *c);
extern void s_displayansi(int socket, char *file);
extern char s_getchar(int socket);
extern void s_readstring(int socket, char *buffer, int max);
extern char s_getc(int socket);
extern void disconnect(int socket);

extern int save_user(struct user_record *user);
extern int check_user(char *loginname);
extern struct user_record *new_user(int socket);
extern struct user_record *check_user_pass(int socket, char *loginname, char *password);


extern void main_menu(int socket, struct user_record *user);
#endif
