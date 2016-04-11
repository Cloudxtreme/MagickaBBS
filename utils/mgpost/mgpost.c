#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "../../jamlib/jam.h"

s_JamBase *open_jam_base(char *path) {
	int ret;
	s_JamBase *jb;
	
	ret = JAM_OpenMB((char *)path, &jb);
	
	if (ret != 0) {
		if (ret == JAM_IO_ERROR) {
			free(jb);
			ret = JAM_CreateMB((char *)path, 1, &jb);
			if (ret != 0) {
				free(jb);
				return NULL;
			}
		}
	}
	return jb;
}

int main(int argc, char **argv) {
	char buffer[256];
	char *body;
	char *subject;
	char *from;
	FILE *fptr;
	int len;
	int totlen;
	time_t thetime;
	int z;
	
	s_JamBase *jb;
	s_JamMsgHeader jmh;
	s_JamSubPacket* jsp;
	s_JamSubfield jsf;
	
	if (argc < 5) {
		printf("Usage:\n");
		printf("%s filename jambase from subject\n", argv[0]);
		exit(1);
	}
	fptr = fopen(argv[1], "r");
	
	if (!fptr) {
		printf("Unable to open %s\n", argv[1]);
		exit(1);
	}
	body = NULL;
	totlen = 0;
	
	len = fread(buffer, 1, 256, fptr);
	while (len > 0) {
		totlen += len;
		if (body == NULL) {
			body = (char *)malloc(totlen + 1);
		} else {
			body = (char *)realloc(body, totlen + 1);
		}
		memcpy(&body[totlen - len], buffer, len);
		body[totlen] = '\0';
		len = fread(buffer, 1, 256, fptr);
	}
	
	fclose(fptr);
	
	jb = open_jam_base(argv[2]);
	if (!jb) {
		printf("Unable to open JAM base %s\n", argv[2]);
		exit(1);
	}
	thetime = time(NULL);
	
	JAM_ClearMsgHeader( &jmh );
	jmh.DateWritten = thetime;
										
	jmh.Attribute |= MSG_TYPELOCAL;
			
	jsp = JAM_NewSubPacket();
	jsf.LoID   = JAMSFLD_SENDERNAME;
	jsf.HiID   = 0;
	jsf.DatLen = strlen(argv[3]);
	jsf.Buffer = (char *)argv[3];
	JAM_PutSubfield(jsp, &jsf);

	jsf.LoID   = JAMSFLD_RECVRNAME;
	jsf.HiID   = 0;
	jsf.DatLen = 3;
	jsf.Buffer = "ALL";
	JAM_PutSubfield(jsp, &jsf);
										
	jsf.LoID   = JAMSFLD_SUBJECT;
	jsf.HiID   = 0;
	jsf.DatLen = strlen(argv[4]);
	jsf.Buffer = (char *)argv[4];
	JAM_PutSubfield(jsp, &jsf);
			
	while (1) {
		z = JAM_LockMB(jb, 100);
		if (z == 0) {
			break;
		} else if (z == JAM_LOCK_FAILED) {
			sleep(1);
		} else {
			printf("Failed to lock msg base!\n");		
			break;
		}
	}
	if (z == 0) {
		if (JAM_AddMessage(jb, &jmh, jsp, (char *)body, strlen(body))) {
			printf("Failed to add message\n");
		}
												
		JAM_UnlockMB(jb);
		JAM_DelSubPacket(jsp);
	}
	JAM_CloseMB(jb);
	
	return 0;
}
