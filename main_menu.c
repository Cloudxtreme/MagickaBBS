#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include "bbs.h"
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

extern struct bbs_config conf; 

void main_menu(int socket, struct user_record *user) {
	int doquit = 0;
	char c;
	char prompt[128];
	char buffer[256];
	int i;
	struct stat s;
	int do_internal_menu = 0;
	char *lRet;
	lua_State *L;
	int result;
	
	if (conf.script_path != NULL) {
		sprintf(buffer, "%s/mainmenu.lua", conf.script_path);
		if (stat(buffer, &s) == 0) {
			L = luaL_newstate();
			luaL_openlibs(L);
			lua_push_cfunctions(L);
			luaL_loadfile(L, buffer);
			do_internal_menu = 0;
			result = lua_pcall(L, 0, 1, 0);
			if (result) {
				fprintf(stderr, "Failed to run script: %s\n", lua_tostring(L, -1));
				do_internal_menu = 1;
			}
		} else {
			do_internal_menu = 1;
		}
	} else {
		do_internal_menu = 1;
	}
	
	while (!doquit) {
		
		if (do_internal_menu == 1) {
			s_displayansi(socket, "mainmenu");
		
		
			sprintf(prompt, "\r\n\e[0mTL: %dm :> ", user->timeleft);
			s_putstring(socket, prompt);
		
			c = s_getc(socket);
		} else {
			lua_getglobal(L, "menu");
			result = lua_pcall(L, 0, 1, 0);
			if (result) {
				fprintf(stderr, "Failed to run script: %s\n", lua_tostring(L, -1));
				do_internal_menu = 1;
				lua_close(L);
				continue;
			}
			lRet = (char *)lua_tostring(L, -1);
			lua_pop(L, 1);
			c = lRet[0];
		}
		
		switch(tolower(c)) {
			case 'o':
				{
					automessage_write(socket, user);
				}
				break;
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
	if (do_internal_menu == 0) {
		lua_close(L);
	}
}
