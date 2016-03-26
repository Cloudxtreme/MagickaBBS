#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "bbs.h"

extern struct bbs_config conf; 

void main_menu(int socket, struct user_record *user) {
	int doquit = 0;
	char c;
	char prompt[128];
	char buffer[256];
	int i;
	struct stat s;
	while (!doquit) {
		s_displayansi(socket, "mainmenu");
		
		
		sprintf(prompt, "\r\n\e[0mTL: %dm :> ", user->timeleft);
		s_putstring(socket, prompt);
		
		c = s_getc(socket);
		
		switch(tolower(c)) {
			case 'c':
				{
					chat_system(socket, user);
				}
				break;
			case 'l':
				{
					bbs_list(socket, user);
				}
				break;
			case 'u':
				{
					list_users(socket, user);
				}
				break;
			case 'b':
				{
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
				}
				break;
			case '1':
				{
					display_last10_callers(socket, user, 0);
				}
				break;
			case 'd':
				{
					doquit = door_menu(socket, user);
				}
				break;
			case 'm':
				{
					doquit = mail_menu(socket, user);
				}
				break;
			case 'g':
				{
					s_putstring(socket, "\r\nAre you sure you want to log off? (Y/N)");
					c = s_getc(socket);
					if (tolower(c) == 'y') {
						doquit = 1;
					}
				}
				break;
			
		}
	}
}
