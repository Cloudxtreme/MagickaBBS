#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/wait.h>
#include "bbs.h"

extern struct bbs_config conf;
extern int mynode;

int write_door32sys(int socket, struct user_record *user) {
	struct stat s;
	char buffer[256];
	FILE *fptr;
	char *ptr;
	int i;
	
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
	
	// create dorinfo1.def
	
	sprintf(buffer, "%s/node%d", conf.bbs_path, mynode);
	
	if (stat(buffer, &s) != 0) {
		mkdir(buffer, 0755);
	}
	
	sprintf(buffer, "%s/node%d/dorinfo1.def", conf.bbs_path, mynode);
	
	fptr = fopen(buffer, "w");
	
	if (!fptr) {
		printf("Unable to open %s for writing!\n", buffer);
		return 1;
	}
	
	strcpy(buffer, conf.sysop_name);
	
	ptr = NULL;
	
	for (i=0;i<strlen(buffer);i++) {
		if (buffer[i] == ' ') {
			ptr = buffer[i+1];
			buffer[i] = '\0';
			break;
		}
	}
	
	fprintf(fptr, "%s\n", conf.bbs_name); // telnet type
	fprintf(fptr, "%s\n", buffer);
	if (ptr != NULL) {
		fprintf(fptr, "%s\n", ptr);
	} else {
		fprintf(fptr, "\n");
	}
	fprintf(fptr, "1\n"); // com port
	fprintf(fptr, "38400\n");
	fprintf(fptr, "0\n");
	fprintf(fptr, "%s\n", user->firstname);
	fprintf(fptr, "%s\n", user->lastname);
	fprintf(fptr, "%s\n", user->location);
	fprintf(fptr, "1\n");
	fprintf(fptr, "30\n");
	fprintf(fptr, "%d\n", user->timeleft);
	fprintf(fptr, "-1\n");

	
	fclose(fptr);
	
	
	return 0;
}

void rundoor(int socket, struct user_record *user, char *cmd, int stdio) {
	char buffer[256];
	int pid;
	char *arguments[4];
	int ret;
	char c;
	int len;
	int status;
	
	if (write_door32sys(socket, user) != 0) {
		return;
	}
	
	if (stdio) {
		
		arguments[0] = strdup(basename(cmd));
		sprintf(buffer, "%d", mynode);
		arguments[1] = strdup(buffer);
		sprintf(buffer, "%d", socket);
		arguments[2] = strdup(buffer);
		arguments[3] = NULL;
		
		pid = fork();
		if (pid < 0) {
			return;
		} else if (pid == 0) {
			// forked process
			dup2(socket, 0);
			dup2(socket, 1);
			execvp(cmd, arguments);
		} else {
			waitpid(pid, &status, 0);
		}
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
				{	
					s_putstring(socket, "\r\nAre you sure you want to log off? (Y/N)");
					c = s_getc(socket);
					if (tolower(c) == 'y') {
						doquit = 1;
						dodoors = 1;
					}
				}
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
