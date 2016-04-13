#ifndef __BBS_H__
#define __BBS_H__

#include <time.h>
#include "lua/lua.h"
#include "lua/lauxlib.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#define VERSION_STR "dev"

#define NETWORK_FIDO 1
#define NETWORK_WWIV 2

#define TYPE_LOCAL_AREA    0
#define TYPE_NETMAIL_AREA  1
#define TYPE_ECHOMAIL_AREA 2

struct fido_addr {
	unsigned short zone;
	unsigned short net;
	unsigned short node;
	unsigned short point;
};

struct last10_callers {
	char name[17];
	char location[33];
	time_t time;
}__attribute__((packed));

struct text_file {
	char *name;
	char *path;
};

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
	int type;
};

struct mail_conference {
	char *name;
	char *path;
	char *tagline;
	int networked;
	int nettype;
	int realnames;
	int sec_level;
	int mail_area_count;
	struct mail_area **mail_areas;
	struct fido_addr *fidoaddr;
	int wwivnode;
};

struct file_sub {
	char *name;
	char *database;
	char *upload_path;
	int upload_sec_level;
	int download_sec_level;
};

struct file_directory {
	char *name;
	char *path;
	int sec_level;
	int file_sub_count;
	struct file_sub **file_subs;
};

struct bbs_config {
	char *bbs_name;
	char *sysop_name;
	
	char *ansi_path;
	char *bbs_path;
	char *log_path;
	char *script_path;
	
	char *default_tagline;
	
	char *irc_server;
	int irc_port;
	char *irc_channel;
	
	char *external_editor_cmd;
	int external_editor_stdio;
	
	int nodes;
	int newuserlvl;
	int mail_conference_count;
	struct mail_conference **mail_conferences;
	int door_count;
	struct door_config **doors;
	int file_directory_count;
	struct file_directory **file_directories;
	int text_file_count;
	struct text_file **text_files;
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

extern void dolog(char *fmt, ...);
extern void runbbs(int sock, char *config, char *ipaddress);
extern struct fido_addr *parse_fido_addr(const char *str);
extern void s_putchar(int socket, char c);
extern void s_putstring(int socket, char *c);
extern void s_displayansi_p(int socket, char *file);
extern void s_displayansi(int socket, char *file);
extern char s_getchar(int socket);
extern void s_readpass(int socket, char *buffer, int max);
extern void s_readstring(int socket, char *buffer, int max);
extern char s_getc(int socket);
extern void disconnect(int socket);
extern void display_info(int socket);
extern void display_last10_callers(int socket, struct user_record *user);

extern int save_user(struct user_record *user);
extern int check_user(char *loginname);
extern struct user_record *new_user(int socket);
extern struct user_record *check_user_pass(int socket, char *loginname, char *password);
extern void list_users(int socket, struct user_record *user);

extern void main_menu(int socket, struct user_record *user);

extern void mail_scan(int socket, struct user_record *user);
extern int mail_menu(int socket, struct user_record *user);
extern char *editor(int socket, struct user_record *user, char *quote, char *from);
extern char *external_editor(int socket, struct user_record *user, char *to, char *from, char *quote, char *qfrom, char *subject, int email);

extern int door_menu(int socket, struct user_record *user);
extern void rundoor(int socket, struct user_record *user, char *cmd, int stdio);

extern void bbs_list(int socket, struct user_record *user);

extern void chat_system(int sock, struct user_record *user);

extern int mail_getemailcount(struct user_record *user);
extern void send_email(int socket, struct user_record *user);
extern void list_emails(int socket, struct user_record *user);

extern int file_menu(int socket, struct user_record *user);

extern void settings_menu(int sock, struct user_record *user);

extern void lua_push_cfunctions(lua_State *L);
#endif
