#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
			case 'a':
				{
					if (conf.text_file_count > 0) {
						
						while(1) {
							s_putstring(socket, "\r\n\e[1;32mText Files Collection\r\n");
							s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");
							
							for (i=0;i<conf.text_file_count;i++) {
								sprintf(buffer, "\e[1;30m[\e[1;34m%3d\e[1;30m] \e[1;37m%s\r\n", i, conf.text_files[i]->name);
								s_putstring(socket, buffer);
							}
							s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");
							s_putstring(socket, "Enter the number of a text file to display or Q to quit: ");
							s_readstring(socket, buffer, 4);
							if (tolower(buffer[0]) != 'q') {
								i = atoi(buffer);
								if (i >= 0 && i < conf.text_file_count) {
									s_putstring(socket, "\r\n");
									s_displayansi_p(socket, conf.text_files[i]->path);
									s_putstring(socket, "Press any key to continue...");
									s_getc(socket);
									s_putstring(socket, "\r\n");
								}
							} else {
								break;
							}
						}
					} else {
						s_putstring(socket, "\r\nSorry, there are no text files to display\r\n");
						s_putstring(socket, "Press any key to continue...\r\n");
						s_getc(socket);
					}
				}
				break;
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
						sprintf(buffer, "\e[0mPress any key to continue...\r\n");
						s_putstring(socket, buffer);
						s_getc(socket);
						i++;
						sprintf(buffer, "%s/bulletin%d.ans", conf.ansi_path, i);
					}					
				}
				break;
			case '1':
				{
					display_last10_callers(socket, user);
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
			case 't':
				{
					doquit = file_menu(socket, user);
				}
				break;
			case 's':
				{
					settings_menu(socket, user);
				}
				break;
		}
	}
}
