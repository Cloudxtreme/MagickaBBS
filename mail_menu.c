#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <jamlib/jam.h>
#include "bbs.h"

extern struct bbs_config conf; 

s_JamBase *open_jam_base(char *path) {
	int ret;
	s_JamBase *jb;
	
	ret = JAM_OpenMB((uchar *)path, &jb);
	
	if (ret != 0) {
		if (ret == JAM_IO_ERROR) {
			free(jb);
			ret = JAM_CreateMB((uchar *)path, 1, &jb);
			if (ret != 0) {
				free(jb);
				return NULL;
			}
		}
	}
	return jb;
}

char *editor(int socket, char *quote) {
	int lines = 0;
	char linebuffer[80];
	char prompt[12];
	int doquit = 0;
	char **content = NULL;
	int i;
	char *msg;
	int size = 0;
	
	s_putstring(socket, "\r\nMagicka Internal Editor, Type /S to Save, /A to abort and /? for help\r\n");
	s_putstring(socket, "-------------------------------------------------------------------------------");
	
	while(!doquit) {
		sprintf(prompt, "\r\n[%3d]: ", lines);
		s_putstring(socket, prompt);
		s_readstring(socket, linebuffer, 70);
		if (linebuffer[0] == '/') {
			if (toupper(linebuffer[1]) == 'S') {
				for (i=0;i<lines;i++) {
					size += strlen(content[i]) + 2;
				}
				
				msg = (char *)malloc(size);
				for (i=0;i<lines;i++) {
					strcat(msg, content[i]);
					strcat(msg, "\r");
					free(content[i]);
				}
				free(content);
				return msg;
			} else  if (toupper(linebuffer[1]) == 'A') {
				for (i=0;i<lines;i++) {
					free(content[i]);
				}
				if (lines > 0) {
					free(content);
				}
				return NULL;
			}
		} else {
			if (lines == 0) {
				content = (char **)malloc(sizeof(char *));
			} else {
				content = (char **)realloc(content, sizeof(char *) * (lines + 1));
			}
			
			content[lines] = strdup(linebuffer);
			
			lines++;
		}
	}
	return NULL;
}

void read_message(int socket, struct user_record *user, int mailno) {
	s_JamBase *jb;
	s_JamBaseHeader jbh;
	s_JamMsgHeader jmh;
	s_JamSubPacket* jsp;
	s_JamSubfield jsf;
	char buffer[256];
	int z;
	
	char *subject = NULL;
	char *from = NULL;
	char *to = NULL;	
	char *body = NULL;
	int lines = 0;
	
	
	jb = open_jam_base(conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
	if (!jb) {
		printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
		return;
	}

	z = JAM_ReadMsgHeader(jb, mailno, &jmh, &jsp);
	if (z != 0) {
		return;
	}

	for (z=0;z<jsp->NumFields;z++) {
		if (jsp->Fields[z]->LoID == JAMSFLD_SUBJECT) {
			subject = (char *)malloc(jsp->Fields[z]->DatLen + 1);
			memset(subject, 0, jsp->Fields[z]->DatLen + 1);
			memcpy(subject, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
		}
		if (jsp->Fields[z]->LoID == JAMSFLD_SENDERNAME) {
			from = (char *)malloc(jsp->Fields[z]->DatLen + 1);
			memset(from, 0, jsp->Fields[z]->DatLen + 1);
			memcpy(from, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
		}				
		if (jsp->Fields[z]->LoID == JAMSFLD_RECVRNAME) {
			to = (char *)malloc(jsp->Fields[z]->DatLen + 1);
			memset(to, 0, jsp->Fields[z]->DatLen + 1);
			memcpy(to, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
		}						
	}
	sprintf(buffer, "\e[2JFrom    : %s\r\n", from);
	s_putstring(socket, buffer);
	sprintf(buffer, "To      : %s\r\n", to);
	s_putstring(socket, buffer);
	sprintf(buffer, "Subject : %s\r\n", subject);
	s_putstring(socket, buffer);
	s_putstring(socket, "-------------------------------------------------------------------------------\r\n");

	body = (char *)malloc(jmh.TxtLen);
	
	JAM_ReadMsgText(jb, jmh.TxtOffset, jmh.TxtLen, (uchar *)body);

	JAM_CloseMB(jb);
	
	for (z=0;z<jmh.TxtLen;z++) {
		if (body[z] == '\r') {
			s_putstring(socket, "\r\n");
			lines++;
			if (lines == 18) {
				s_putstring(socket, "Press a key to continue...\r\n");
				s_getc(socket);
				lines = 0;
			}
		} else {
			s_putchar(socket, body[z]);
		}
	}

	free(body);

	if (from != NULL) {
		free(from);
	}
	
	if (to != NULL) {
		free(to);
	}
	
	if (subject != NULL) {
		free(subject);
	}
	
}

int mail_menu(int socket, struct user_record *user) {
	int doquit = 0;
	int domail = 0;
	char c;
	char prompt[128];
	char buffer[256];
	int i;
	int j;
	int z;
	
	s_JamBase *jb;
	s_JamBaseHeader jbh;
	s_JamMsgHeader jmh;
	s_JamSubPacket* jsp;
	s_JamSubfield jsf;
	
	struct tm msg_date;
	
	char *subject;
	char *from;
	char *to;
	
	char *msg;
	int closed;
	
	while (!domail) {
		s_displayansi(socket, "mailmenu");
		
		
		sprintf(prompt, "\r\nConf: (%d) %s\r\nArea: (%d) %s\r\nTL: %dm :> ", user->cur_mail_conf, conf.mail_conferences[user->cur_mail_conf]->name, user->cur_mail_area, conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->name, user->timeleft);
		s_putstring(socket, prompt);
		
		c = s_getc(socket);
		
		switch(tolower(c)) {
			case 'p':
				{
					s_putstring(socket, "\r\nTO: ");
					s_readstring(socket, buffer, 16);
					
					if (strlen(buffer) == 0) {
						strcpy(buffer, "ALL");
					}
					
					if (conf.mail_conferences[user->cur_mail_conf]->networked == 0 && strcasecmp(buffer, "ALL") != 0) {
						if (check_user(buffer)) {
							s_putstring(socket, "\r\n\r\nInvalid Username\r\n");
							break;
						} 
					}
					
					to = strdup(buffer);
					s_putstring(socket, "\r\nSUBJECT: ");
					s_readstring(socket, buffer, 25);
					subject = strdup(buffer);
					
					// post a message
					msg = editor(socket, NULL);
					
					if (msg != NULL) {
						jb = open_jam_base(conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
						if (!jb) {
							printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
							free(msg);
							free(to);
							free(subject);
							break;
						}
						
						JAM_ClearMsgHeader( &jmh );
						jmh.DateWritten = time(NULL);
						
						jsp = JAM_NewSubPacket();
						jsf.LoID   = JAMSFLD_SENDERNAME;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(user->loginname);
						jsf.Buffer = (uchar *)user->loginname;
						JAM_PutSubfield(jsp, &jsf);

						jsf.LoID   = JAMSFLD_RECVRNAME;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(to);
						jsf.Buffer = (uchar *)to;
						JAM_PutSubfield(jsp, &jsf);
						
						jsf.LoID   = JAMSFLD_SUBJECT;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(subject);
						jsf.Buffer = (uchar *)subject;
						JAM_PutSubfield(jsp, &jsf);
						
						
						while (1) {
							z = JAM_LockMB(jb, 100);
							if (z == 0) {
								break;
							} else if (z == JAM_LOCK_FAILED) {
								sleep(1);
							} else {
								free(msg);
								free(to);
								free(subject);
								printf("Failed to lock msg base!\n");
								break;
							}
						}
						if (z != 0) {
							JAM_CloseMB(jb);
							break;
						}
						
						if (JAM_AddMessage(jb, &jmh, jsp, (uchar *)msg, strlen(msg))) {
							printf("Failed to add message\n");
						}
							
						JAM_UnlockMB(jb);
						
						JAM_DelSubPacket(jsp);
						free(msg);
						JAM_CloseMB(jb);
					}
					free(to);
					free(subject);
				}
				break;
			case 'l':
				{
					s_putstring(socket, "\r\n");
					// list mail in message base
					jb = open_jam_base(conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
					if (!jb) {
						printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
						break;
					} else {
						JAM_ReadMBHeader(jb, &jbh);
						
						sprintf(buffer, "Start at message [0-%d] ? ", jbh.ActiveMsgs - 1);
						s_putstring(socket, buffer);
						
						s_readstring(socket, buffer, 6);
						i = atoi(buffer);
						closed = 0;
						s_putstring(socket, "\r\n");
						for (j=i;j<jbh.ActiveMsgs;j++) {
							z = JAM_ReadMsgHeader(jb, j, &jmh, &jsp);
							if (z != 0) {
								continue;
							}
							
							if (jmh.Attribute & MSG_DELETED) {
								JAM_DelSubPacket(jsp);
								continue;
							}

							if (jmh.Attribute & MSG_NODISP) {
								JAM_DelSubPacket(jsp);
								continue;
							}
							subject = NULL;
							from = NULL;
							to = NULL;
							
							for (z=0;z<jsp->NumFields;z++) {
								if (jsp->Fields[z]->LoID == JAMSFLD_SUBJECT) {
									subject = (char *)malloc(jsp->Fields[z]->DatLen + 1);
									memset(subject, 0, jsp->Fields[z]->DatLen + 1);
									memcpy(subject, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
								}
								if (jsp->Fields[z]->LoID == JAMSFLD_SENDERNAME) {
									from = (char *)malloc(jsp->Fields[z]->DatLen + 1);
									memset(from, 0, jsp->Fields[z]->DatLen + 1);
									memcpy(from, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
								}				
								if (jsp->Fields[z]->LoID == JAMSFLD_RECVRNAME) {
									to = (char *)malloc(jsp->Fields[z]->DatLen + 1);
									memset(to, 0, jsp->Fields[z]->DatLen + 1);
									memcpy(to, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
								}				
												
							}
							
							localtime_r((time_t *)&jmh.DateWritten, &msg_date);
							
							sprintf(buffer, "[%4d] %-25s %-15s %-15s %02d:%02d %02d-%02d-%02d\r\n", j, subject, from, to, msg_date.tm_hour, msg_date.tm_min, msg_date.tm_mday, msg_date.tm_mon + 1, msg_date.tm_year - 100);
							s_putstring(socket, buffer);
							JAM_DelSubPacket(jsp);
							if (subject != NULL) {
								free(subject);
							}
							if (from != NULL) {
								free(from);
							}
							if (to != NULL) {
								free(to);
							}
							
							if ((j - i) != 0 && (j - i) % 22 == 0) {
								sprintf(buffer, "(#) Read Message # (Q) Quit (ENTER) Continue\r\n");
								s_putstring(socket, buffer);
								s_readstring(socket, buffer, 6);
								
								if (tolower(buffer[0]) == 'q') {
									break;
								} else if (strlen(buffer) > 0) {
									z = atoi(buffer);
									if (z >= 0 && z <= jbh.ActiveMsgs) {
										JAM_CloseMB(jb);
										closed = 1;
										read_message(socket, user, z);
										break;
									}
								}
							}
							
						}
						sprintf(buffer, "(#) Read Message # (Q) Quit (ENTER) Continue\r\n");
						s_putstring(socket, buffer);
						s_readstring(socket, buffer, 6);
								
						if (tolower(buffer[0]) == 'q') {
							break;
						} else if (strlen(buffer) > 0) {
							z = atoi(buffer);
							if (z >= 0 && z <= jbh.ActiveMsgs) {
								JAM_CloseMB(jb);
								closed = 1;
								read_message(socket, user, z);
								break;
							}
						}						
					}
					if (closed == 0) {
						JAM_CloseMB(jb);
					}
				}
				break;
			case 'c':
				{
					s_putstring(socket, "\r\n\r\nMail Conferences:\r\n\r\n");
					for (i=0;i<conf.mail_conference_count;i++) {
						sprintf(buffer, "  %d. %s\r\n", i, conf.mail_conferences[i]->name);
						s_putstring(socket, buffer);
						
						if (i != 0 && i % 22 == 0) {
							s_putstring(socket, "Press any key to continue...\r\n");
							c = s_getc(socket);
						}
					}
					s_putstring(socket, "Enter the conference number: ");
					s_readstring(socket, buffer, 5);
					if (tolower(buffer[0]) != 'q') {
						j = atoi(buffer);
						if (j < 0 || j >= conf.mail_conference_count) {
							s_putstring(socket, "\r\nInvalid conference number!\r\n");
						} else {
							s_putstring(socket, "\r\n");
							user->cur_mail_conf = j;
							user->cur_mail_area = 0;
						}
					}
				}
				break;
			case 'a':
				{
					s_putstring(socket, "\r\n\r\nMail Areas:\r\n\r\n");
					for (i=0;i<conf.mail_conferences[user->cur_mail_conf]->mail_area_count;i++) {
						sprintf(buffer, "  %d. %s\r\n", i, conf.mail_conferences[user->cur_mail_conf]->mail_areas[i]->name);
						s_putstring(socket, buffer);
						
						if (i != 0 && i % 22 == 0) {
							s_putstring(socket, "Press any key to continue...\r\n");
							c = s_getc(socket);
						}
					}
					s_putstring(socket, "Enter the area number: ");
					s_readstring(socket, buffer, 5);
					if (tolower(buffer[0]) != 'q') {
						j = atoi(buffer);
						if (j < 0 || j >= conf.mail_conferences[user->cur_mail_conf]->mail_area_count) {
							s_putstring(socket, "\r\nInvalid area number!\r\n");
						} else {
							s_putstring(socket, "\r\n");
							user->cur_mail_area = j;
						}
					}
				}
				break;				
			case 'q':
				{
					domail = 1;
				}
				break;
			case 'g':
				{
					s_putstring(socket, "\r\nAre you sure you want to log off? (Y/N)");
					c = s_getc(socket);
					if (tolower(c) == 'y') {
						domail = 1;
						doquit = 1;
					}
				}
				break;
			
		}
	}
	
	return doquit;
}
