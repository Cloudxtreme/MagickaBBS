#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include "inih/ini.h"
#include "bbs.h"

int mynode;
struct bbs_config conf;

struct user_record *gUser;
int gSocket;

int usertimeout;

struct fido_addr *parse_fido_addr(const char *str) {
	struct fido_addr *ret = (struct fido_addr *)malloc(sizeof(struct fido_addr));
	int c;
	int state = 0;
	
	ret->zone = 0;
	ret->net = 0;
	ret->node = 0;
	ret->point = 0;
	
	for (c=0;c<strlen(str);c++) {
		switch(str[c]) {
			case ':':
				state = 1;
				break;
			case '/':
				state = 2;
				break;
			case '.':
				state = 3;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				{
					switch (state) {
						case 0:
							ret->zone = ret->zone * 10 + (str[c] - '0');
							break;
						case 1:
							ret->net = ret->net * 10 + (str[c] - '0');
							break;
						case 2:
							ret->node = ret->node * 10 + (str[c] - '0');
							break;
						case 3:
							ret->point = ret->point * 10 + (str[c] - '0');
							break;
					}
				}
				break;
			default:
				free(ret);
				return NULL;
		}
	}
	return ret;
}


void timer_handler(int signum) {
	if (signum == SIGALRM) {
		if (gUser != NULL) {
			gUser->timeleft--;
			
			if (gUser->timeleft <= 0) {
				s_putstring(gSocket, "\r\n\r\nSorry, you're out of time today..\r\n");
				disconnect(gSocket);
			}
			

		} 
		usertimeout--;
		if (usertimeout <= 0) {
			s_putstring(gSocket, "\r\n\r\nTimeout waiting for input..\r\n");
			disconnect(gSocket);
		}		
	}
}
static int door_config_handler(void* user, const char* section, const char* name,
                   const char* value)
{
	struct bbs_config *conf = (struct bbs_config *)user;
	int i;
	
	for (i=0;i<conf->door_count;i++) {
		if (strcasecmp(conf->doors[i]->name, section) == 0) {
			// found it
			if (strcasecmp(name, "key") == 0) {
				conf->doors[i]->key = value[0];
			} else if (strcasecmp(name, "command") == 0) {
				conf->doors[i]->command = strdup(value);
			} else if (strcasecmp(name, "stdio") == 0) {
				if (strcasecmp(value, "true") == 0) {
					conf->doors[i]->stdio = 1;
				} else {
					conf->doors[i]->stdio = 0;
				}
			}
			return 1;
		}
	}
	
	if (conf->door_count == 0) {
		conf->doors = (struct door_config **)malloc(sizeof(struct door_config *));
	} else {
		conf->doors = (struct door_config **)realloc(conf->doors, sizeof(struct door_config *) * (conf->door_count + 1));
	}
	
	conf->doors[conf->door_count] = (struct door_config *)malloc(sizeof(struct door_config));
	
	conf->doors[conf->door_count]->name = strdup(section);

	if (strcasecmp(name, "key") == 0) {
		conf->doors[conf->door_count]->key = value[0];
	} else if (strcasecmp(name, "command") == 0) {
		conf->doors[conf->door_count]->command = strdup(value);
	} else if (strcasecmp(name, "stdio") == 0) {
		if (strcasecmp(value, "true") == 0) {
			conf->doors[conf->door_count]->stdio = 1;
		} else {
			conf->doors[conf->door_count]->stdio = 0;
		}
	}
	conf->door_count++;
	
	return 1;
}

static int file_sub_handler(void* user, const char* section, const char* name,
                   const char* value)
{
	struct file_directory *fd = (struct file_directory *)user;
	int i;
	
	if (strcasecmp(section, "main") == 0) {
		if (strcasecmp(name, "visible sec level")) {
			fd->sec_level = atoi(value);
		}
	} else {
		// check if it's partially filled in
		for (i=0;i<fd->file_sub_count;i++) {
			if (strcasecmp(fd->file_subs[i]->name, section) == 0) {
				if (strcasecmp(name, "upload sec level") == 0) {
					fd->file_subs[i]->upload_sec_level = atoi(value);
				} else if (strcasecmp(name, "download sec level") == 0) {
					fd->file_subs[i]->download_sec_level = atoi(value);
				} else if (strcasecmp(name, "database") == 0) {
					fd->file_subs[i]->database = strdup(value);
				}
				return 1;
			}
		}
		if (fd->file_sub_count == 0) {
			fd->file_subs = (struct file_sub **)malloc(sizeof(struct file_sub *));
		} else {
			fd->file_subs = (struct file_sub **)realloc(fd->file_subs, sizeof(struct file_sub *) * (fd->file_sub_count + 1));
		}
		
		fd->file_subs[fd->file_sub_count] = (struct file_sub *)malloc(sizeof(struct file_sub));
		
		fd->file_subs[fd->file_sub_count]->name = strdup(section);
		if (strcasecmp(name, "upload sec level") == 0) {
			fd->file_subs[fd->file_sub_count]->upload_sec_level = atoi(value);
		} else if (strcasecmp(name, "download sec level") == 0) {
			fd->file_subs[fd->file_sub_count]->download_sec_level = atoi(value);
		} else if (strcasecmp(name, "database") == 0) {
			fd->file_subs[fd->file_sub_count]->database = strdup(value);
		}
		fd->file_sub_count++;
	}
	return 1;
}


static int mail_area_handler(void* user, const char* section, const char* name,
                   const char* value)
{
	struct mail_conference *mc = (struct mail_conference *)user;
	int i;
	
	if (strcasecmp(section, "main") == 0) {
		if (strcasecmp(name, "visible sec level")) {
			mc->sec_level = atoi(value);
		} else if (strcasecmp(name, "networked")) {
			if (strcasecmp(value, "true") == 0) {
				mc->networked = 1;
			} else {
				mc->networked = 0;
			}
		} else if (strcasecmp(name, "real names")) {
			if (strcasecmp(value, "true") == 0) {
				mc->realnames = 1;
			} else {
				mc->realnames = 0;
			}
		}
	} else if (strcasecmp(section, "network") == 0) {
		if (strcasecmp(name, "type") == 0) {
			if (strcasecmp(value, "fido") == 0) {
				mc->nettype = NETWORK_FIDO;
			}
		} else if (strcasecmp(name, "fido node") == 0) {
			mc->fidoaddr = parse_fido_addr(value);
		}
	} else {
		// check if it's partially filled in
		for (i=0;i<mc->mail_area_count;i++) {
			if (strcasecmp(mc->mail_areas[i]->name, section) == 0) {
				if (strcasecmp(name, "read sec level") == 0) {
					mc->mail_areas[i]->read_sec_level = atoi(value);
				} else if (strcasecmp(name, "write sec level") == 0) {
					mc->mail_areas[i]->write_sec_level = atoi(value);
				} else if (strcasecmp(name, "path") == 0) {
					mc->mail_areas[i]->path = strdup(value);
				} else if (strcasecmp(name, "type") == 0) {
					if (strcasecmp(value, "local") == 0) {
						mc->mail_areas[i]->type = TYPE_LOCAL_AREA;
					} else if (strcasecmp(value, "echo") == 0) {
						mc->mail_areas[i]->type = TYPE_ECHOMAIL_AREA;
					} else if (strcasecmp(value, "netmail") == 0) {
						mc->mail_areas[i]->type = TYPE_NETMAIL_AREA;
					}
				}
				return 1;
			}
		}
		if (mc->mail_area_count == 0) {
			mc->mail_areas = (struct mail_area **)malloc(sizeof(struct mail_area *));
		} else {
			mc->mail_areas = (struct mail_area **)realloc(mc->mail_areas, sizeof(struct mail_area *) * (mc->mail_area_count + 1));
		}
		
		mc->mail_areas[mc->mail_area_count] = (struct mail_area *)malloc(sizeof(struct mail_area));
		
		mc->mail_areas[mc->mail_area_count]->name = strdup(section);
		if (strcasecmp(name, "read sec level") == 0) {
			mc->mail_areas[mc->mail_area_count]->read_sec_level = atoi(value);
		} else if (strcasecmp(name, "write sec level") == 0) {
			mc->mail_areas[mc->mail_area_count]->write_sec_level = atoi(value);
		} else if (strcasecmp(name, "path") == 0) {
			mc->mail_areas[mc->mail_area_count]->path = strdup(value);
		} else if (strcasecmp(name, "type") == 0) {
			if (strcasecmp(value, "local") == 0) {
				mc->mail_areas[mc->mail_area_count]->type = TYPE_LOCAL_AREA;
			} else if (strcasecmp(value, "echo") == 0) {
				mc->mail_areas[mc->mail_area_count]->type = TYPE_ECHOMAIL_AREA;
			} else if (strcasecmp(value, "netmail") == 0) {
				mc->mail_areas[mc->mail_area_count]->type = TYPE_NETMAIL_AREA;
			}
		}
		mc->mail_area_count++;
	}
	return 1;
}

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
	struct bbs_config *conf = (struct bbs_config *)user;
	
	if (strcasecmp(section, "main") == 0) {
		if (strcasecmp(name, "bbs name") == 0) {
			conf->bbs_name = strdup(value);
		} else if (strcasecmp(name, "sysop name") == 0) {
			conf->sysop_name = strdup(value);
		} else if (strcasecmp(name, "nodes") == 0) {
			conf->nodes = atoi(value);
		} else if (strcasecmp(name, "new user level") == 0) {
			conf->newuserlvl = atoi(value);
		} else if (strcasecmp(name, "irc server") == 0) {
			conf->irc_server = strdup(value);
		} else if (strcasecmp(name, "irc port") == 0) {
			conf->irc_port = atoi(value);
		} else if (strcasecmp(name, "irc channel") == 0) {
			conf->irc_channel = strdup(value);
		} 
	} else if (strcasecmp(section, "paths") == 0){
		if (strcasecmp(name, "ansi path") == 0) {
			conf->ansi_path = strdup(value);
		} else if (strcasecmp(name, "bbs path") == 0) {
			conf->bbs_path = strdup(value);
		} else if (strcasecmp(name, "email path") == 0) {
			conf->email_path = strdup(value);
		}
	} else if (strcasecmp(section, "mail conferences") == 0) {
		if (conf->mail_conference_count == 0) {
			conf->mail_conferences = (struct mail_conference **)malloc(sizeof(struct mail_conference *));
		} else {
			conf->mail_conferences = (struct mail_conference **)realloc(conf->mail_conferences, sizeof(struct mail_conference *) * (conf->mail_conference_count + 1));
		}
		
		conf->mail_conferences[conf->mail_conference_count] = (struct mail_conference *)malloc(sizeof(struct mail_conference));
		conf->mail_conferences[conf->mail_conference_count]->name = strdup(name);
		conf->mail_conferences[conf->mail_conference_count]->path = strdup(value);
		conf->mail_conferences[conf->mail_conference_count]->mail_area_count = 0;
		conf->mail_conference_count++;
	} else if (strcasecmp(section, "file directories") == 0) {
		if (conf->file_directory_count == 0) {
			conf->file_directories = (struct file_directory **)malloc(sizeof(struct file_directory *));
		} else {
			conf->file_directories = (struct file_directory **)realloc(conf->file_directories, sizeof(struct file_directory *) * (conf->file_directory_count + 1));
		}
		
		conf->file_directories[conf->file_directory_count] = (struct file_directory *)malloc(sizeof(struct file_directory));
		conf->file_directories[conf->file_directory_count]->name = strdup(name);
		conf->file_directories[conf->file_directory_count]->path = strdup(value);
		conf->file_directories[conf->file_directory_count]->file_sub_count = 0;
		conf->file_directory_count++;
	} else if (strcasecmp(section, "text files") == 0) {
		if (conf->text_file_count == 0) {
			conf->text_files = (struct text_file **)malloc(sizeof(struct text_file *));
		} else {
			conf->text_files = (struct text_file **)realloc(conf->text_files, sizeof(struct text_file *) * (conf->text_file_count + 1));
		}
		
		conf->text_files[conf->text_file_count] = (struct text_file *)malloc(sizeof(struct text_file));
		conf->text_files[conf->text_file_count]->name = strdup(name);
		conf->text_files[conf->text_file_count]->path = strdup(value);
		conf->text_file_count++;
		
	}
	
	return 1;
}

void s_putchar(int socket, char c) {
	write(socket, &c, 1);
}

void s_putstring(int socket, char *c) {
	write(socket, c, strlen(c));
}

void s_displayansi_p(int socket, char *file) {
	FILE *fptr;
	char c;
	
	fptr = fopen(file, "r");
	if (!fptr) {
		return;
	}
	c = fgetc(fptr);
	while (!feof(fptr)) {
		s_putchar(socket, c);
		c = fgetc(fptr);
	}
	fclose(fptr);
}


void s_displayansi(int socket, char *file) {
	FILE *fptr;
	char c;
	
	char buffer[256];
	
	sprintf(buffer, "%s/%s.ans", conf.ansi_path, file);
	
	fptr = fopen(buffer, "r");
	if (!fptr) {
		return;
	}
	c = fgetc(fptr);
	while (!feof(fptr)) {
		s_putchar(socket, c);
		c = fgetc(fptr);
	}
	fclose(fptr);
}

char s_getchar(int socket) {
	unsigned char c;
	int len;

	do {

		len = read(socket, &c, 1);

		if (len == 0) {
			disconnect(socket);
		}
			
		while (c == 255) {
			len = read(socket, &c, 1);
			if (len == 0) {
				disconnect(socket);
			}
			len = read(socket, &c, 1);
			if (len == 0) {
				disconnect(socket);
			}
			len = read(socket, &c, 1);
			if (len == 0) {
				disconnect(socket);
			}		
		}


		
		if (c == '\r') {
			if (len == 0) {
				disconnect(socket);
			}
		}
	} while (c == '\n');
	usertimeout = 10;
	return (char)c;
}

char s_getc(int socket) {
	char c = s_getchar(socket);

	s_putchar(socket, c);
	return (char)c;
}

void s_readstring(int socket, char *buffer, int max) {
	int i;
	char c;
	
	memset(buffer, 0, max);
	
	for (i=0;i<max;i++) {
		c = s_getchar(socket);
		if ((c == '\b' || c == 127) && i > 0) {
			buffer[i-1] = '\0';
			i -= 2;
			s_putstring(socket, "\e[D \e[D");
			continue;
		}
		
		if (c == '\n' || c == '\r') {
			return;
		}
		s_putchar(socket, c);
		buffer[i] = c;
		buffer[i+1] = '\0';
	}
}

void s_readpass(int socket, char *buffer, int max) {
	int i;
	char c;
	
	for (i=0;i<max;i++) {
		c = s_getchar(socket);

		if ((c == '\b' || c == 127) && i > 0) {
			buffer[i-1] = '\0';
			i-=2;
			s_putstring(socket, "\e[D \e[D");
			continue;
		}

		if (c == '\n' || c == '\r') {
			return;
		}
		s_putchar(socket, '*');
		buffer[i] = c;
		buffer[i+1] = '\0';
	}
}

void disconnect(int socket) {
	char buffer[256];
	if (gUser != NULL) {
		save_user(gUser);
	}
	sprintf(buffer, "%s/nodeinuse.%d", conf.bbs_path, mynode);
	remove(buffer);
	close(socket);
	exit(0);
}

void display_last10_callers(int socket, struct user_record *user, int record) {
	struct last10_callers callers[10];
	struct last10_callers new_entry;
	int i,z,j;
	char buffer[256];
	struct tm l10_time;
	FILE *fptr = fopen("last10.dat", "rb");
	
	s_putstring(socket, "\r\n\e[1;37mLast 10 callers:\r\n");
	s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\r\n");
	
	if (fptr != NULL) {
		
		for (i=0;i<10;i++) {
			if (fread(&callers[i], sizeof(struct last10_callers), 1, fptr) < 1) {
				break;
			}
		}
		
		fclose(fptr);
	} else {
		i = 0;
	}
	
	for (z=0;z<i;z++) {
		localtime_r(&callers[z].time, &l10_time);
		sprintf(buffer, "\e[1;37m%-16s \e[1;36m%-32s \e[1;32m%02d:%02d %02d-%02d-%02d\e[0m\r\n", callers[z].name, callers[z].location, l10_time.tm_hour, l10_time.tm_min, l10_time.tm_mday, l10_time.tm_mon + 1, l10_time.tm_year - 100);
		s_putstring(socket, buffer);
	}
	s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");
	if (strcasecmp(conf.sysop_name, user->loginname) != 0 && record) {
		memset(&new_entry, 0, sizeof(struct last10_callers));
		strcpy(new_entry.name, user->loginname);
		strcpy(new_entry.location, user->location);
		new_entry.time = time(NULL);
		
		if (i == 10) {
			j = 1;
		} else {
			j = 0;
		}
		fptr = fopen("last10.dat", "wb");
		for (;j<i;j++) {
			fwrite(&callers[j], sizeof(struct last10_callers), 1, fptr);
		}
		fwrite(&new_entry, sizeof(struct last10_callers), 1, fptr);
		fclose(fptr);
	}
	sprintf(buffer, "Press any key to continue...\r\n");
	s_putstring(socket, buffer);
	s_getc(socket);	
}

void display_info(int socket) {
	char buffer[256];
	struct utsname name;
	int mailwaiting;
	
	mailwaiting = mail_getemailcount(gUser);
	
	uname(&name);
	
	sprintf(buffer, "\r\n\r\n\e[1;37mSystem Information\r\n");
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;30m----------------------------------------------\r\n");
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;32mBBS Name    : \e[1;37m%s\r\n", conf.bbs_name);
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;32mSysOp Name  : \e[1;37m%s\r\n", conf.sysop_name);
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;32mNode        : \e[1;37m%d\r\n", mynode);
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;32mBBS Version : \e[1;37mMagicka %d.%d (%s)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_STR);
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;32mSystem      : \e[1;37m%s (%s)\r\n", name.sysname, name.machine);
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;30m----------------------------------------------\e[0m\r\n");
	s_putstring(socket, buffer);
	
	sprintf(buffer, "Press any key to continue...\r\n");
	s_putstring(socket, buffer);
	s_getc(socket);
}

void runbbs(int socket, char *config_path) {
	char buffer[256];
	char password[17];

	struct stat s;
	FILE *nodefile;
	int i;
	char iac_echo[] = {255, 251, 1, '\0'};
	char iac_sga[] = {255, 251, 3, '\0'};
	struct user_record *user;
	struct tm thetime;
	struct tm oldtime;
	time_t now;
	struct itimerval itime;
	struct sigaction sa;
	
	
	write(socket, iac_echo, 3);
	write(socket, iac_sga, 3);

	

	sprintf(buffer, "Magicka BBS v%d.%d (%s), Loading...\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_STR);
	s_putstring(socket, buffer);

	conf.mail_conference_count = 0;
	conf.door_count = 0;
	conf.file_directory_count = 0;
	conf.irc_server = NULL;
	conf.irc_port = 6667;
	conf.text_file_count = 0;
	
	// Load BBS data
	if (ini_parse(config_path, handler, &conf) <0) {
		printf("Unable to load configuration ini (%s)!\n", config_path);
		exit(-1);
	}	
	// Load mail Areas
	for (i=0;i<conf.mail_conference_count;i++) {
		if (ini_parse(conf.mail_conferences[i]->path, mail_area_handler, conf.mail_conferences[i]) <0) {
			printf("Unable to load configuration ini (%s)!\n", conf.mail_conferences[i]->path);
			exit(-1);
		}			
	}
	// Load file Subs
	for (i=0;i<conf.file_directory_count;i++) {
		if (ini_parse(conf.file_directories[i]->path, file_sub_handler, conf.file_directories[i]) <0) {
			printf("Unable to load configuration ini (%s)!\n", conf.file_directories[i]->path);
			exit(-1);
		}			
	}
		
	if (ini_parse("config/doors.ini", door_config_handler, &conf) <0) {
		printf("Unable to load configuration ini (doors.ini)!\n");
		exit(-1);
	}	
	
	// find out which node we are
	mynode = 0;
	for (i=1;i<=conf.nodes;i++) {
		sprintf(buffer, "%s/nodeinuse.%d", conf.bbs_path, i);
		if (stat(buffer, &s) != 0) {
			mynode = i;
			nodefile = fopen(buffer, "w");
			if (!nodefile) {
				printf("Error opening nodefile!\n");
				close(socket);
				exit(1);
			}
			
			fputs("UNKNOWN", nodefile);
			fclose(nodefile);
			
			break;
		}
	}
	
	if (mynode == 0) {
		s_putstring(socket, "Sorry, all nodes are in use. Please try later\r\n");
		close(socket);
		exit(1);
	}
	gUser = NULL;
	gSocket = socket;
	usertimeout = 10;
	
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	sa.sa_flags = SA_RESTART;
	sigaction (SIGALRM, &sa, 0);
	
	itime.it_interval.tv_sec = 60;
	itime.it_interval.tv_usec = 0;
	itime.it_value.tv_sec = 60;
	itime.it_value.tv_usec = 0;
	
	setitimer (ITIMER_REAL, &itime, 0);
	
	s_displayansi(socket, "issue");
	

	
	s_putstring(socket, "\e[0mEnter your Login Name or NEW to create an account\r\n");
	s_putstring(socket, "Login:> ");
	
	s_readstring(socket, buffer, 25);

	if (strcasecmp(buffer, "new") == 0) {
		user = new_user(socket);
	} else {
		s_putstring(socket, "\r\nPassword:> ");
		s_readpass(socket, password, 16);
		user = check_user_pass(socket, buffer, password);
		if (user == NULL) {
			s_putstring(socket, "\r\nIncorrect Login.\r\n");
			disconnect(socket);
		}
		
		for (i=1;i<=conf.nodes;i++) {
			sprintf(buffer, "%s/nodeinuse.%d", conf.bbs_path, i);
			if (stat(buffer, &s) == 0) {
				nodefile = fopen(buffer, "r");
				if (!nodefile) {
					printf("Error opening nodefile!\n");
					disconnect(socket);
				}
				fgets(buffer, 256, nodefile);
					
				if (strcasecmp(user->loginname, buffer) == 0) {
					fclose(nodefile);
					s_putstring(socket, "\r\nYou are already logged in.\r\n");
					disconnect(socket);
				} 
				fclose(nodefile);
			}
		}
	}
	
	sprintf(buffer, "%s/nodeinuse.%d", conf.bbs_path, mynode);
	nodefile = fopen(buffer, "w");
	if (!nodefile) {
		printf("Error opening nodefile!\n");
		close(socket);
		exit(1);
	}
			
	fputs(user->loginname, nodefile);
	fclose(nodefile);	
	
	// do post-login
	// check time left
	now = time(NULL);
	localtime_r(&now, &thetime);
	localtime_r(&user->laston, &oldtime);
	
	if (thetime.tm_mday != oldtime.tm_mday || thetime.tm_mon != oldtime.tm_mon || thetime.tm_year != oldtime.tm_year) {
		user->timeleft = user->sec_info->timeperday;
		user->laston = now;
		save_user(user);
	}		
	gUser = user;
	user->timeson++;
	// bulletins
	i = 0;
	sprintf(buffer, "%s/bulletin%d.ans", conf.ansi_path, i);
	
	while (stat(buffer, &s) == 0) {
		sprintf(buffer, "bulletin%d", i);
		s_displayansi(socket, buffer);
		sprintf(buffer, "Press any key to continue...\r\n");
		s_putstring(socket, buffer);
		s_getc(socket);
		i++;
		sprintf(buffer, "%s/bulletin%d.ans", conf.ansi_path, i);
	}
	
	// external login cmd
	
	// display info
	display_info(socket);
	
	display_last10_callers(socket, user, 1);
	
	// check email
	i = mail_getemailcount(user);
	if (i > 0) {
		sprintf(buffer, "\r\nYou have %d e-mail(s) in your inbox.\r\n", i);
		s_putstring(socket, buffer);
	} else {
		s_putstring(socket, "\r\nYou have no e-mail.\r\n");
	}
	
	mail_scan(socket, user);
	
	// main menu
	main_menu(socket, user);
	
	s_displayansi(socket, "goodbye");
	
	disconnect(socket);
}
