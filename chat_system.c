#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "bbs.h"

extern struct bbs_config conf;
extern int mynode;

static char **screenbuffer;
static int chat_socket;
static int line_at;
static int row_at;
static char sbuf[512];
extern struct user_record gUser;

void scroll_up() {
	int y;

	for (y=1;y<23;y++) {
		strcpy(screenbuffer[y-1], screenbuffer[y]);

	}
	memset(screenbuffer[22], 0, 81);
	row_at = 0;
}

void raw(char *fmt, ...) {

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sbuf, 512, fmt, ap);
    va_end(ap);
    write(chat_socket, sbuf, strlen(sbuf));
}

int hostname_to_ip(char * hostname , char* ip) {
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
         
	if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
		// get the host info
        return 1;
    }
 
	addr_list = (struct in_addr **) he->h_addr_list;
     
	for(i = 0; addr_list[i] != NULL; i++) {
		strcpy(ip , inet_ntoa(*addr_list[i]) );
		return 0;
	}
     
    return 1;
}
void append_screenbuffer(char *buffer) {
	int z;
	
	for (z=0;z<strlen(buffer);z++) {
		if (row_at == 80) {
			if (line_at == 22) {
				scroll_up();
				row_at = 0;
			} else {
				row_at = 0;
				line_at++;
			}
		}
		
		screenbuffer[line_at][row_at] = buffer[z];
		row_at++;
		screenbuffer[line_at][row_at] = '\0';
	}
	if (line_at == 22) {
		scroll_up();
	}
	if (line_at < 22) {
		line_at++;
	}	
	
	row_at = 0;
}

void chat_system(int sock, struct user_record *user) {
	struct sockaddr_in servaddr;
    fd_set fds;
    int t;
    int ret;
    char inputbuffer[80];
    int inputbuffer_at = 0;
    int len;
    char c;
    char buffer2[256];
    char buffer[513];
    char buffer3[513];
    char outputbuffer[513];
    int buffer_at = 0;
	int do_update = 1;
	int i;
	int o;
	int l;
	int j;
	int z;
	int z2;
	char *usr;
	char *cmd;
	char *where;
	char *message;
	char *pos;
	char *sep;
	char *target;
    memset(inputbuffer, 0, 80);
    if (conf.irc_server == NULL) {
		s_putstring(sock, "\r\nSorry, Chat is not supported on this system.\r\n");
		return;
	}
	row_at = 0;
	line_at = 0;
	s_putstring(sock, "\e[2J");
	
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(conf.irc_port);
    
    
    if ( (chat_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return;
    }
    if (inet_pton(AF_INET, conf.irc_server, &servaddr.sin_addr) != 0) {
        hostname_to_ip(conf.irc_server, buffer);
        if (!inet_pton(AF_INET, conf.irc_server, &servaddr.sin_addr)) {
			return;
		}
    }
    if (connect(chat_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0 ) {
        return;
    }

	raw("USER %s 0 0 :%s\r\n", user->loginname, user->loginname);
	raw("NICK %s\r\n", user->loginname);
	raw("JOIN %s\r\n", conf.irc_channel);
	
	memset(buffer, 0, 513);
	
	screenbuffer = (char **)malloc(sizeof(char *) * 23);  
    for (i=0;i<23;i++) {
		screenbuffer[i] = (char *)malloc(81);
		memset(screenbuffer[i], 0, 81);
	}
	
	while (1) {
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		FD_SET(chat_socket, &fds);
		
		if (chat_socket > sock) {
			t = chat_socket + 1;
		} else {
			t = sock + 1;
		}
		
		ret = select(t, &fds, NULL, NULL, NULL);
		
		if (ret > 0) {
			if (FD_ISSET(sock, &fds)) {
				len = read(sock, &c, 1);
				if (len == 0) {
					raw("QUIT\r\n");
					disconnect(sock);
				}
				
				if (c == '\r') {
					if (inputbuffer[0] == '/') {
						if (strcasecmp(&inputbuffer[1], "quit") == 0) {
							raw("QUIT\r\n");
							for (i=0;i<22;i++) {
								free(screenbuffer[i]);
							}
							free(screenbuffer);
							return;
						}
					} else {
						raw("PRIVMSG %s :%s\r\n", conf.irc_channel, inputbuffer);
						sprintf(buffer2, "%s: %s", user->loginname, inputbuffer);
						append_screenbuffer(buffer2);
						do_update = 1;
					}
					memset(inputbuffer, 0, 80);
					inputbuffer_at = 0;
				} else if (c != '\n') {
					if (c == '\b' || c == 127) {
						if (inputbuffer_at > 0) {
							inputbuffer_at--;
							inputbuffer[inputbuffer_at] = '\0';
							do_update = 2;
						}
					} else if (inputbuffer_at < 79) {
						inputbuffer[inputbuffer_at++] = c;
						do_update = 2;
					}
				}
			} 
			if (FD_ISSET(chat_socket, &fds)) {
				len = read(chat_socket, &c, 1);
				if (len == 0) {
					s_putstring(sock, "\r\n\r\n\r\nLost connection to chat server!\r\n");
					for (i=0;i<22;i++) {
						free(screenbuffer[i]);
					}
					free(screenbuffer);					
					return;
				}
				
				if (c == '\r' || buffer_at == 512) {	
					if (!strncmp(buffer, "PING", 4)) {
						buffer[1] = 'O';
						raw(buffer);
					} else if (buffer[0] == ':') {
						usr = cmd = where = message = NULL;
						for (j=1;j<buffer_at;j++) {
							if (buffer[j] == ' ') {
								usr = &buffer[1];
								buffer[j] = '\0';
								cmd = &buffer[j+1];
								break;
							}
						}

						for (;j<buffer_at;j++) {
							if (buffer[j] == ' ') {
								message = &buffer[j+1];
								buffer[j] = '\0';
								break;
							}
						}


						if (!strncmp(cmd, "PRIVMSG", 7) || !strncmp(cmd, "NOTICE", 6)) {				
							for (j=0;j<strlen(message);j++) {
								if (message[j] == ' ') {
									where = message;
									message[j] = '\0';
									message = &message[j+2];
									break;
								}
							}
							if ((sep = strchr(usr, '!')) != NULL) usr[sep - usr] = '\0';
							if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!') target = where; else target = usr;
							if (!strncmp(cmd, "PRIVMSG", 7)) {
								if (strcmp(target, conf.irc_channel) == 0) {
									sprintf(outputbuffer, "%s: %s", usr, message);
								} 
								append_screenbuffer(outputbuffer);
								do_update = 1;
							} 
						}
					}
					
					memset(buffer, 0, 513);
					buffer_at = 0;
				} else if (c != '\n') {
					buffer[buffer_at] = c;
					buffer_at++;
				}
			}
		}
		if (do_update == 1) {
			s_putstring(sock, "\e[2J");		
			for (i=0;i<=line_at;i++) {
				sprintf(buffer2, "%s\r\n", screenbuffer[i]);
				s_putstring(sock, buffer2);
			}
			for (i=line_at+1;i<22;i++) {
				s_putstring(sock, "\r\n");
			}
			s_putstring(sock, "\e[1;45;37m Type /Quit to Exit\e[K\e[0m\r\n");
			if (inputbuffer_at > 0) {
				s_putstring(sock, inputbuffer);
			} 
			do_update = 0;
		} else if (do_update == 2) {
			sprintf(buffer2, "\e[24;1f%s\e[K", inputbuffer);
			s_putstring(sock, buffer2);
		}
	}
}
