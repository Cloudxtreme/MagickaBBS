#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include "bbs.h"

extern struct bbs_config conf;
extern int mynode;

int write_door32sys(int socket, struct user_record *user) {
	struct stat s;
	char buffer[256];
	FILE *fptr;
	
	sprintf(buffer, "%s/node%d", conf.bbs_path, mynode);
	
	if (stat(buffer, &s) != 0) {
		mkdir(buffer, 0755);
	}
	
	sprintf(buffer, "%s/node%d/door32.sys", conf.bbs_path, mynode);
	
	fptr = fopen(buffer, "w");
	
	if (!fptr) {
		printf("Unable to open %s for writing!\n", buffer);
		return 1;
	}
	
	fprintf(fptr, "2\n"); // telnet type
	fprintf(fptr, "%d\n", socket); // socket
	fprintf(fptr, "38400\n"); // baudrate
	fprintf(fptr, "Magicka %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
	fprintf(fptr, "%d\n", user->id);
	fprintf(fptr, "%s %s\n", user->firstname, user->lastname);
	fprintf(fptr, "%s\n", user->loginname);
	fprintf(fptr, "%d\n", user->sec_level);
	fprintf(fptr, "%d\n", user->timeleft);
	fprintf(fptr, "1\n"); // ansi emulation = 1
	fprintf(fptr, "%d\n", mynode);
	
	fclose(fptr);
	return 0;
}

void rundoor(int socket, struct user_record *user, char *cmd, int stdio) {
	char buffer[256];
	if (write_door32sys(socket, user) != 0) {
		return;
	}
	
	if (stdio) {
	} else {
		sprintf(buffer, "%s %d %d", cmd, mynode, socket);
		system(buffer);
	}
}

int door_menu(int socket, struct user_record *user) {
	int doquit = 0;
	int dodoors = 0;
	char prompt[128];
	int i;
	char c;
	while (!dodoors) {
		s_displayansi(socket, "doors");
		
		sprintf(prompt, "\r\nTL: %dm :> ", user->timeleft);
		s_putstring(socket, prompt);
		
		c = s_getc(socket);
		
		switch(tolower(c)) {
			case 'q':
				dodoors = 1;
				break;
			case 'g':
				doquit = 1;
				dodoors = 1;
				break;
			default:
				{
					for (i=0;i<conf.door_count;i++) {
						if (tolower(c) == tolower(conf.doors[i]->key)) {
							rundoor(socket, user, conf.doors[i]->command, conf.doors[i]->stdio);
							break;
						}
					}
				}
				break;
		}
	}
	
	return doquit;
}
