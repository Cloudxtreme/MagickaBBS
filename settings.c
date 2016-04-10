#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "bbs.h"

void settings_menu(int sock, struct user_record *user) {
	char buffer[256];
	int dosettings = 0;
	char c;
	
	while (!dosettings) {
		s_putstring(sock, "\e[1;32mYour Settings\r\n");
		s_putstring(sock, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");
		s_putstring(sock, "\e[0;36mP. \e[1;37mPassword (\e[1;33mNot Shown\e[1;37m)\r\n");
		sprintf(buffer, "\e[0;36mL. \e[1;37mLocation (\e[1;33m%s\e[1;37m)\r\n", user->location);
		s_putstring(sock, buffer);
		s_putstring(sock, "\e[0;36mQ. \e[1;37mQuit to Main Menu\r\n");
		s_putstring(sock, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");
		
		c = s_getc(sock);
		
		switch(tolower(c)) {
			case 'p':
				{
					s_putstring(sock, "\r\nEnter your current password: ");
					s_readpass(sock, buffer, 16);
					if (strcmp(buffer, user->password) == 0) {
						s_putstring(sock, "\r\nEnter your new password (8 chars min): ");
						s_readstring(sock, buffer, 16);
						if (strlen(buffer) >= 8) {
							free(user->password);
							user->password = (char *)malloc(strlen(buffer) + 1);
							strcpy(user->password, buffer);
							save_user(user);
							s_putstring(sock, "\r\nPassword Changed!\r\n");
						} else {
							s_putstring(sock, "\r\nPassword too short!\r\n");
						}
					} else {
						s_putstring(sock, "\r\nPassword Incorrect!\r\n");
					}
				}
				break;
			case 'l':
				{
					s_putstring(sock, "\r\nEnter your new location: ");
					s_readstring(sock, buffer, 32);
					free(user->location);
					user->location = (char *)malloc(strlen(buffer) + 1);
					strcpy(user->location, buffer);
					save_user(user);
				}
				break;
			case 'q':
				dosettings = 1;
				break;
		}
	}
}
