#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "bbs.h"

extern struct bbs_config conf; 

void main_menu(int socket, struct user_record *user) {
	int doquit = 0;
	char c;
	char prompt[128];
	
	while (!doquit) {
		s_displayansi(socket, "mainmenu");
		
		
		sprintf(prompt, "TL: %dm :> ", user->timeleft);
		s_putstring(socket, prompt);
		
		c = s_getc(socket);
		
		switch(tolower(c)) {
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
