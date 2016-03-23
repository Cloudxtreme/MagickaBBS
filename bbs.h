#ifndef __BBS_H__
#define __BBS_H__

#include <time.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_STR "dev"

struct last10_callers {
	char name[17];
	char location[33];
	time_t time;
}__attribute__((packed));

struct door_config {
	char *name;
	char key;
	char *command;
	int stdio;
};

struct mail_area {
	char *name;
	char *path;
	int read_sec_level;
	int write_sec_level;
};

struct mail_conference {
	char *name;
	char *path;
	int networked;
	int realnames;
	int sec_level;
	int mail_area_count;
	struct mail_area **mail_areas;
};

struct bbs_config {
	char *bbs_name;
	char *sysop_name;
	
	char *ansi_path;
	char *bbs_path;
	char *email_path;
	
	int nodes;
	int newuserlvl;
	int mail_conference_count;
	struct mail_conference **mail_conferences;
	int door_count;
	struct door_config **doors;
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
	int cur_mail_conf;
	int cur_mail_area;
	int cur_file_dir;
	int cur_file_sub;
	int timeson;
};


extern void runbbs(int sock, char *config);

extern void s_putchar(int socket, char c);
extern void s_putstring(int socket, char *c);
extern void s_displayansi(int socket, char *file);
extern char s_getchar(int socket);
extern void s_readstring(int socket, char *buffer, int max);
extern char s_getc(int socket);
extern void disconnect(int socket);
extern void display_info(int socket);
extern void display_last10_callers(int socket, struct user_record *user, int record);

extern int save_user(struct user_record *user);
extern int check_user(char *loginname);
extern struct user_record *new_user(int socket);
extern struct user_record *check_user_pass(int socket, char *loginname, char *password);
extern void list_users(int socket, struct user_record *user);

extern void main_menu(int socket, struct user_record *user);

extern int mail_getemailcount(struct user_record *user);
extern int mail_menu(int socket, struct user_record *user);

extern int door_menu(int socket, struct user_record *user);
#endif
