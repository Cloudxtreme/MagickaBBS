#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include "bbs.h"

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


int main(int argc, char **argv) {
	int socket_desc, client_sock, c, *new_sock;
	int pid;
	struct sockaddr_in server, client;
	int port;
	struct sigaction sa;
	    
	if (argc < 3) {
		printf("Usage ./magicka config/bbs.ini port\n");
		exit(1);
	}
	
	sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
	
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		printf("Couldn't create socket..\n");
		return 1;
	}
	port = atoi(argv[2]);
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	
	if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Bind Failed, Error\n");
		return 1;
	}
	
	listen(socket_desc, 3);
	
	c = sizeof(struct sockaddr_in);
	
	while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c))) {
		pid = fork();
		
		if (pid < 0) {
			perror("Error on fork\n");
			return 1;
		}
		
		if (pid == 0) {
			close(socket_desc);
			runbbs(client_sock, argv[1], strdup(inet_ntoa(client.sin_addr)));
			
			exit(0);
		} else {
			close(client_sock);
		}
	}
}
