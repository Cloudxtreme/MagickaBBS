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

char *editor(int socket, char *quote, char *from) {
	int lines = 0;
	char buffer[256];
	char linebuffer[80];
	char prompt[12];
	int doquit = 0;
	char **content = NULL;
	int i;
	char *msg;
	int size = 0;
	int quotelines = 0;
	char **quotecontent;
	int lineat=0;
	int qfrom,qto;
	int z;
	
	if (quote != NULL) {
		for (i=0;i<strlen(quote);i++) {
			
			if (quote[i] == '\r' || lineat == 75) {
				if (quotelines == 0) {
					quotecontent = (char **)malloc(sizeof(char *));
				} else {
					quotecontent = (char **)realloc(quotecontent, sizeof(char *) * (quotelines + 1));
				}
				
				quotecontent[quotelines] = (char *)malloc(strlen(linebuffer) + 4);
				sprintf(quotecontent[quotelines], "%c> %s", from[0], linebuffer);
				quotelines++;
				lineat = 0;
				linebuffer[0] = '\0';
			} else {
				linebuffer[lineat++] = quote[i];
				linebuffer[lineat] = '\0';
			}
		}
	}
	s_putstring(socket, "\r\n\e[1;32mMagicka Internal Editor, Type \e[1;37m/S \e[1;32mto save, \e[1;37m/A \e[1;32mto abort and \e[1;37m/? \e[1;32mfor help\r\n");
	s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m");
	
	while(!doquit) {
		sprintf(prompt, "\r\n\e[1;30m[\e[1;34m%3d\e[1;30m]: \e[0m", lines);
		s_putstring(socket, prompt);
		s_readstring(socket, linebuffer, 70);
		if (linebuffer[0] == '/') {
			if (toupper(linebuffer[1]) == 'S') {
				for (i=0;i<lines;i++) {
					size += strlen(content[i]) + 1;
				}
				size ++;
				msg = (char *)malloc(size);
				memset(msg, 0, size);
				for (i=0;i<lines;i++) {
					strcat(msg, content[i]);
					strcat(msg, "\r");
					free(content[i]);
				}
				free(content);
				if (quote != NULL) {
					for (i=0;i<quotelines;i++) {
						free(quotecontent[i]);
					}
					free(quotecontent);
				}				
				return msg;
			} else  if (toupper(linebuffer[1]) == 'A') {
				for (i=0;i<lines;i++) {
					free(content[i]);
				}
				if (lines > 0) {
					free(content);
				}
				
				if (quote != NULL) {
					for (i=0;i<quotelines;i++) {
						free(quotecontent[i]);
					}
					free(quotecontent);
				}
				
				return NULL;
			} else if (toupper(linebuffer[1]) == 'Q') {
				if (quote == NULL) {
					s_putstring(socket, "\r\nNo message to quote!\r\n");
				} else {
					s_putstring(socket, "\r\n");
					for (i=0;i<quotelines;i++) {
						sprintf(buffer, "\r\n\e[1;30m[\e[1;34m%3d\e[1;30m]: \e[0m%s", i, quotecontent[i]);
						s_putstring(socket, buffer);
					}
					
					s_putstring(socket, "\r\nQuote from Line: ");
					s_readstring(socket, buffer, 5);
					qfrom = atoi(buffer);
					s_putstring(socket, "\r\nQuote to Line: ");
					s_readstring(socket, buffer, 5);
					qto = atoi(buffer);
					s_putstring(socket, "\r\n");
					
					if (qto > quotelines) {
						qto = quotelines;
					}
					if (qfrom < 0) {
						qfrom = 0;
					}
					if (qfrom > qto) {
						s_putstring(socket, "Quoting Cancelled\r\n");
					}
					
					for (i=qfrom;i<=qto;i++) {
						if (lines == 0) {
							content = (char **)malloc(sizeof(char *));
						} else {
							content = (char **)realloc(content, sizeof(char *) * (lines + 1));
						}
						
						content[lines] = strdup(quotecontent[i]);
						lines++;
					}
					
					s_putstring(socket, "\r\n\e[1;32mMagicka Internal Editor, Type \e[1;37m/S \e[1;32mto save, \e[1;37m/A \e[1;32mto abort and \e[1;37m/? \e[1;32mfor help\r\n");
					s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m");

					for (i=0;i<lines;i++) {
						sprintf(buffer, "\r\n\e[1;30m[\e[1;34m%3d\e[1;30m]: \e[0m%s", i, content[i]);
						s_putstring(socket, buffer);
					}
				}
			} else if (toupper(linebuffer[1]) == 'L') {
				s_putstring(socket, "\r\n\e[1;32mMagicka Internal Editor, Type \e[1;37m/S \e[1;32mto save, \e[1;37m/A \e[1;32mto abort and \e[1;37m/? \e[1;32mfor help\r\n");
				s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m");

				for (i=0;i<lines;i++) {
					sprintf(buffer, "\r\n\e[1;30m[\e[1;34m%3d\e[1;30m]: \e[0m%s", i, content[i]);
					s_putstring(socket, buffer);
				}				
			} else if (linebuffer[1] == '?') {
				s_putstring(socket, "\e[1;33m\r\nHELP\r\n");
				s_putstring(socket, "/S - Save Message\r\n");
				s_putstring(socket, "/A - Abort Message\r\n");
				s_putstring(socket, "/Q - Quote Message\r\n");
				s_putstring(socket, "/E - Edit (Rewrite) Line\r\n");
				s_putstring(socket, "/D - Delete Line\r\n");
				s_putstring(socket, "/I - Insert Line\r\n");
				s_putstring(socket, "/L - Relist Message\r\n\e[0m");
			} else if (toupper(linebuffer[1]) == 'D') {
				s_putstring(socket, "\r\nWhich line do you want to delete? ");
				s_readstring(socket, buffer, 6);
				if (strlen(buffer) == 0) {
					s_putstring(socket, "\r\nAborted...\r\n");
				} else {
					z = atoi(buffer);
					if (z < 0 || z >= lines) {
						s_putstring(socket, "\r\nAborted...\r\n");
					} else {
						for (i=z;i<lines-1;i++) {
							free(content[i]);
							content[i] = strdup(content[i+1]);
						}
						free(content[i]);
						lines--;
						content = (char **)realloc(content, sizeof(char *) * lines);
					}
				}
			} else if (toupper(linebuffer[1]) == 'E') {
				s_putstring(socket, "\r\nWhich line do you want to edit? ");
				s_readstring(socket, buffer, 6);
				if (strlen(buffer) == 0) {
					s_putstring(socket, "\r\nAborted...\r\n");
				} else {
					z = atoi(buffer);
					if (z < 0 || z >= lines) {
						s_putstring(socket, "\r\nAborted...\r\n");
					} else {
						sprintf(buffer, "\r\n\e[1;30m[\e[1;34m%3d\e[1;30m]: \e[0m%s", z, content[z]);
						s_putstring(socket, buffer);						
						sprintf(buffer, "\r\n\e[1;30m[\e[1;34m%3d\e[1;30m]: \e[0m", z);
						s_putstring(socket, buffer);
						s_readstring(socket, linebuffer, 70);
						free(content[z]);
						content[z] = strdup(linebuffer);	
					}
				}
			} else if (toupper(linebuffer[1]) == 'I') {
				s_putstring(socket, "\r\nInsert before which line? ");
				s_readstring(socket, buffer, 6);
				if (strlen(buffer) == 0) {
					s_putstring(socket, "\r\nAborted...\r\n");
				} else {
					z = atoi(buffer);
					if (z < 0 || z >= lines) {
						s_putstring(socket, "\r\nAborted...\r\n");
					} else {
						sprintf(buffer, "\r\n\e[1;30m[\e[1;34m%3d\e[1;30m]: \e[0m", z);
						s_putstring(socket, buffer);
						s_readstring(socket, linebuffer, 70);
						lines++;
						content = (char **)realloc(content, sizeof(char *) * lines);
						
						for (i=lines;i>z;i--) {
							content[i] = content[i-1];
						}
						
						content[z] = strdup(linebuffer);
					}
				}
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
	if (quote != NULL) {
		for (i=0;i<quotelines;i++) {
			free(quotecontent[i]);
		}
		free(quotecontent);
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
	struct tm msg_date;
	
	char *subject = NULL;
	char *from = NULL;
	char *to = NULL;	
	char *body = NULL;
	int lines = 0;
	char c;
	char *replybody;
	struct fido_addr *from_addr = NULL;
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
		if (jsp->Fields[z]->LoID == JAMSFLD_OADDRESS) {
			memset(buffer, 0, jsp->Fields[z]->DatLen + 1);
			memcpy(buffer, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
			from_addr = parse_fido_addr(buffer);
		}							
	}
	
	
	if (from_addr != NULL) {
		sprintf(buffer, "\e[2J\e[1;32mFrom    : \e[1;37m%s (%d:%d/%d.%d)\r\n", from, from_addr->zone, from_addr->net, from_addr->node, from_addr->point);
	} else {
		sprintf(buffer, "\e[2J\e[1;32mFrom    : \e[1;37m%s\r\n", from);
	}
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;32mTo      : \e[1;37m%s\r\n", to);
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;32mSubject : \e[1;37m%s\r\n", subject);
	s_putstring(socket, buffer);
	localtime_r((time_t *)&jmh.DateWritten, &msg_date);
	sprintf(buffer, "\e[1;32mDate    : \e[1;37m%s", asctime(&msg_date));
	buffer[strlen(buffer) - 1] = '\0';
	strcat(buffer, "\r\n");
	s_putstring(socket, buffer);
	s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");

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

	s_putstring(socket, "Press R to reply or any other key to quit...\r\n");
	
	c = s_getc(socket);
	
	if (tolower(c) == 'r') {
		if (user->sec_level < conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->write_sec_level) {
			s_putstring(socket, "\r\nSorry, you are not allowed to post in this area\r\n");
		} else {
			if (subject != NULL) {
				sprintf(buffer, "RE: %s", subject);
				free(subject);
			}
			subject = (char *)malloc(strlen(buffer) + 1);
			strcpy(subject, buffer);
			if (from != NULL) {
				strcpy(buffer, from);
				free(from);
			}
			if (conf.mail_conferences[user->cur_mail_conf]->realnames == 0) {
				from = (char *)malloc(strlen(user->loginname) + 1);
				strcpy(from, user->loginname);
			} else {
				from = (char *)malloc(strlen(user->firstname) + strlen(user->lastname) + 2);
				sprintf(from, "%s %s", user->firstname, user->lastname);
			}
			if (to != NULL) {
				free(to);
			}
			to = (char *)malloc(strlen(buffer) + 1);
			strcpy(to, buffer);
			replybody = editor(socket, body, to);
			if (replybody != NULL) {
				jb = open_jam_base(conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
				if (!jb) {
					printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
					free(replybody);
					free(body);
					free(subject);
					free(to);
					free(from);
					if (from_addr != NULL) {
						free(from_addr);
					}
					return;
				}
							
				JAM_ClearMsgHeader( &jmh );
				jmh.DateWritten = time(NULL);
				jmh.Attribute |= MSG_LOCAL;
							
				jsp = JAM_NewSubPacket();
				jsf.LoID   = JAMSFLD_SENDERNAME;
				jsf.HiID   = 0;
				jsf.DatLen = strlen(from);
				jsf.Buffer = (uchar *)from;
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
				
				if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_ECHOMAIL_AREA) {
					jmh.Attribute |= MSG_TYPEECHO;
					
					if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
						sprintf(buffer, "%d:%d/%d.%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
													conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
													conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
													conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
						jsf.LoID   = JAMSFLD_OADDRESS;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(buffer);
						jsf.Buffer = (uchar *)buffer;
						JAM_PutSubfield(jsp, &jsf);
					}
				} else if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_NETMAIL_AREA) {
					jmh.Attribute |= MSG_TYPENET;
					jmh.Attribute |= MSG_KILLSENT;
					if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
						sprintf(buffer, "%d:%d/%d.%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
													conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
													conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
													conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
						jsf.LoID   = JAMSFLD_OADDRESS;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(buffer);
						jsf.Buffer = (uchar *)buffer;
						JAM_PutSubfield(jsp, &jsf);
						
						if (from_addr != NULL) {
						
							sprintf(buffer, "%d:%d/%d.%d", from_addr->zone,
														from_addr->net,
														from_addr->node,
														from_addr->point);
							jsf.LoID   = JAMSFLD_DADDRESS;
							jsf.HiID   = 0;
							jsf.DatLen = strlen(buffer);
							jsf.Buffer = (uchar *)buffer;
							JAM_PutSubfield(jsp, &jsf);						
						}
					}					
				}
							
				while (1) {
					z = JAM_LockMB(jb, 100);
					if (z == 0) {
						break;
					} else if (z == JAM_LOCK_FAILED) {
						sleep(1);
					} else {
						free(replybody);
						free(body);
						free(subject);
						free(to);
						free(from);
						printf("Failed to lock msg base!\n");
						if (from_addr != NULL) {
							free(from_addr);
						}						
						return;
					}
				}
				if (JAM_AddMessage(jb, &jmh, jsp, (uchar *)replybody, strlen(replybody))) {
					printf("Failed to add message\n");
				}
								
				JAM_UnlockMB(jb);
							
				JAM_DelSubPacket(jsp);
				free(replybody);
				JAM_CloseMB(jb);
			}
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
	if (from_addr != NULL) {
		free(from_addr);
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
	char *body;
	char *replybody;
	
	
	char *msg;
	int closed;
	ulong jam_crc;
	unsigned int lastmsg,currmsg;
	int lines;
	struct fido_addr *from_addr;
	
	while (!domail) {
		s_displayansi(socket, "mailmenu");
		
		
		sprintf(prompt, "\r\nConf: (%d) %s\r\nArea: (%d) %s\r\nTL: %dm :> ", user->cur_mail_conf, conf.mail_conferences[user->cur_mail_conf]->name, user->cur_mail_area, conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->name, user->timeleft);
		s_putstring(socket, prompt);
		
		c = s_getc(socket);
		
		switch(tolower(c)) {
			case 'p':
				{
					if (user->sec_level < conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->write_sec_level) {
						s_putstring(socket, "\r\nSorry, you are not allowed to post in this area\r\n");
						break;
					}
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
					if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_NETMAIL_AREA) {
						s_putstring(socket, "\r\nADDR: ");
						s_readstring(socket, buffer, 32);
						from_addr = parse_fido_addr(buffer);
						if (!from_addr) {
							s_putstring(socket, "\r\n\r\nInvalid Address\r\n");
							break;							
						} else {
							sprintf(buffer, "\r\nMailing to %d:%d/%d.%d\r\n", from_addr->zone, from_addr->net, from_addr->node, from_addr->point);
						}
					}
					to = strdup(buffer);
					s_putstring(socket, "\r\nSUBJECT: ");
					s_readstring(socket, buffer, 25);
					subject = strdup(buffer);
					
					// post a message
					msg = editor(socket, NULL, NULL);
					
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
						jmh.Attribute |= MSG_LOCAL;
						if (conf.mail_conferences[user->cur_mail_conf]->realnames == 0) {
							strcpy(buffer, user->loginname);
						} else {
							sprintf(buffer, "%s %s", user->firstname, user->lastname);
						}
						
						jsp = JAM_NewSubPacket();
						jsf.LoID   = JAMSFLD_SENDERNAME;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(buffer);
						jsf.Buffer = (uchar *)buffer;
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
						
						if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_ECHOMAIL_AREA) {
							jmh.Attribute |= MSG_TYPEECHO;
							
							if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
								sprintf(buffer, "%d:%d/%d.%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
								jsf.LoID   = JAMSFLD_OADDRESS;
								jsf.HiID   = 0;
								jsf.DatLen = strlen(buffer);
								jsf.Buffer = (uchar *)buffer;
								JAM_PutSubfield(jsp, &jsf);
							}
						} else if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_NETMAIL_AREA) {
							jmh.Attribute |= MSG_TYPENET;
							jmh.Attribute |= MSG_KILLSENT;
							if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
								sprintf(buffer, "%d:%d/%d.%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
								jsf.LoID   = JAMSFLD_OADDRESS;
								jsf.HiID   = 0;
								jsf.DatLen = strlen(buffer);
								jsf.Buffer = (uchar *)buffer;
								JAM_PutSubfield(jsp, &jsf);
								
								if (from_addr != NULL) {
								
									sprintf(buffer, "%d:%d/%d.%d", from_addr->zone,
																from_addr->net,
																from_addr->node,
																from_addr->point);
									jsf.LoID   = JAMSFLD_DADDRESS;
									jsf.HiID   = 0;
									jsf.DatLen = strlen(buffer);
									jsf.Buffer = (uchar *)buffer;
									JAM_PutSubfield(jsp, &jsf);						
								}
							}					
						}
						
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
						if (jbh.ActiveMsgs > 0) {
							sprintf(buffer, "Start at message [0-%d] ? ", jbh.ActiveMsgs - 1);
							s_putstring(socket, buffer);
							
							s_readstring(socket, buffer, 6);
							i = atoi(buffer);
							closed = 0;
							s_putstring(socket, "\e[2J\e[1;37;44m[MSG#] Subject                   From            To              Date          \r\n\e[0m");
							for (j=i;j<jbh.ActiveMsgs;j++) {
								z = JAM_ReadMsgHeader(jb, j, &jmh, &jsp);
								if (z != 0) {
									printf("Failed to read msg header: %d Erro %d\n", z, JAM_Errno(jb));
									continue;
								}
								
								if (jmh.Attribute & MSG_DELETED) {
									printf("Deleted MSG\n");
									JAM_DelSubPacket(jsp);
									continue;
								}

								if (jmh.Attribute & MSG_NODISP) {
									printf("No Display\n");
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
								
								sprintf(buffer, "\e[1;30m[\e[1;34m%4d\e[1;30m] \e[1;37m%-25s \e[1;32m%-15s \e[1;33m%-15s \e[1;35m%02d:%02d %02d-%02d-%02d\e[0m\r\n", j, subject, from, to, msg_date.tm_hour, msg_date.tm_min, msg_date.tm_mday, msg_date.tm_mon + 1, msg_date.tm_year - 100);
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
						} else {
							s_putstring(socket, "\r\nThere is no mail in this area\r\n");
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
						if (conf.mail_conferences[i]->sec_level <= user->sec_level) {
							sprintf(buffer, "  %d. %s\r\n", i, conf.mail_conferences[i]->name);
							s_putstring(socket, buffer);
						}
						if (i != 0 && i % 22 == 0) {
							s_putstring(socket, "Press any key to continue...\r\n");
							c = s_getc(socket);
						}
					}
					s_putstring(socket, "Enter the conference number: ");
					s_readstring(socket, buffer, 5);
					if (tolower(buffer[0]) != 'q') {
						j = atoi(buffer);
						if (j < 0 || j >= conf.mail_conference_count || conf.mail_conferences[j]->sec_level > user->sec_level) {
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
						if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[i]->read_sec_level <= user->sec_level) {
							sprintf(buffer, "  %d. %s\r\n", i, conf.mail_conferences[user->cur_mail_conf]->mail_areas[i]->name);
							s_putstring(socket, buffer);
						}
						if (i != 0 && i % 22 == 0) {
							s_putstring(socket, "Press any key to continue...\r\n");
							c = s_getc(socket);
						}
					}
					s_putstring(socket, "Enter the area number: ");
					s_readstring(socket, buffer, 5);
					if (tolower(buffer[0]) != 'q') {
						j = atoi(buffer);
						if (j < 0 || j >= conf.mail_conferences[user->cur_mail_conf]->mail_area_count || conf.mail_conferences[user->cur_mail_conf]->mail_areas[j]->read_sec_level > user->sec_level) {
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
			case 'e':
				{
					s_putstring(socket, "\r\nTO: ");
					s_readstring(socket, buffer, 16);
					
					if (strlen(buffer) == 0) {
						s_putstring(socket, "\r\nAborted\r\n");
						break;
					}
					if (check_user(buffer)) {
						s_putstring(socket, "\r\n\r\nInvalid Username\r\n");
						break;
					}
					
					to = strdup(buffer);
					s_putstring(socket, "\r\nSUBJECT: ");
					s_readstring(socket, buffer, 25);
					subject = strdup(buffer);
					
					// post a message
					msg = editor(socket, NULL, NULL);
					
					if (msg != NULL) {
						jb = open_jam_base(conf.email_path);
						if (!jb) {
							printf("Error opening JAM base.. %s\n", conf.email_path);
							free(msg);
							free(to);
							free(subject);
							break;
						}
						
						JAM_ClearMsgHeader( &jmh );
						jmh.DateWritten = time(NULL);
						jmh.Attribute |= MSG_PRIVATE;
						
						strcpy(buffer, user->loginname);	
						
						jsp = JAM_NewSubPacket();
						jsf.LoID   = JAMSFLD_SENDERNAME;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(buffer);
						jsf.Buffer = (uchar *)buffer;
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
			case 'r':
				{
					// Read your email...
					s_putstring(socket, "\r\n");
					// list mail in message base
					jb = open_jam_base(conf.email_path);
					if (!jb) {
						printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
						break;
					} else {
						jam_crc = JAM_Crc32((uchar *)user->loginname, strlen(user->loginname));
						lastmsg = 0;
						while (JAM_FindUser(jb, jam_crc, lastmsg, &currmsg) == 0) {
							if (JAM_ReadMsgHeader(jb, currmsg, &jmh, &jsp) != 0) {
								lastmsg = currmsg + 1;
								continue;
							}
							
							if (jmh.Attribute & MSG_DELETED) {
								JAM_DelSubPacket(jsp);
								lastmsg = currmsg + 1;
								continue;
							}

							if (jmh.Attribute & MSG_NODISP) {
								JAM_DelSubPacket(jsp);
								lastmsg = currmsg + 1;
								continue;
							}
							subject = NULL;
							from = NULL;
							
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
							}					
							sprintf(buffer, "[%3d]. %s From %s\r\n", currmsg, subject, from);
							if (subject != NULL) {
								free(subject);
							}
							if (from != NULL) {
								free(from);
							}
							s_putstring(socket, buffer);
							JAM_DelSubPacket(jsp);
							lastmsg = currmsg + 1;
						}
						
						if (lastmsg == 0) {
							s_putstring(socket, "\r\nYou have no mail...\r\n");
							JAM_CloseMB(jb);
						} else {
							s_putstring(socket, "(#) View Message\r\n");
							s_readstring(socket, buffer, 5);
							currmsg = atoi(buffer);
							
							if (JAM_ReadMsgHeader(jb, currmsg, &jmh, &jsp) != 0) {
								JAM_CloseMB(jb);
								break;
							}
							
							if (jmh.Attribute & MSG_DELETED) {
								JAM_DelSubPacket(jsp);
								JAM_CloseMB(jb);
								break;
							}

							if (jmh.Attribute & MSG_NODISP) {
								JAM_DelSubPacket(jsp);
								JAM_CloseMB(jb);
								break;
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
							
							if (strcasecmp(from, user->loginname) == 0) {
								sprintf(buffer, "\e[2J\e[1;32mFrom    : \e[1;37m%s\r\n", from);
								s_putstring(socket, buffer);
								sprintf(buffer, "\e[1;32mTo      : \e[1;37m%s\r\n", to);
								s_putstring(socket, buffer);
								sprintf(buffer, "\e[1;32mSubject : \e[1;37m%s\r\n", subject);
								s_putstring(socket, buffer);
								localtime_r((time_t *)&jmh.DateWritten, &msg_date);
								sprintf(buffer, "\e[1;32mDate    : \e[1;37m%s", asctime(&msg_date));
								buffer[strlen(buffer) - 1] = '\0';
								strcat(buffer, "\r\n");
								s_putstring(socket, buffer);
								s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");
								body = (char *)malloc(jmh.TxtLen);
	
								JAM_ReadMsgText(jb, jmh.TxtOffset, jmh.TxtLen, (uchar *)body);

								JAM_CloseMB(jb);
								
								lines = 0;
								
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

								s_putstring(socket, "Press R to reply, D to delete or any other key to quit...\r\n");
	
								c = s_getc(socket);
	
								if (tolower(c) == 'r') {
									if (subject != NULL) {
										sprintf(buffer, "RE: %s", subject);
										free(subject);
									}
									subject = (char *)malloc(strlen(buffer) + 1);
									strcpy(subject, buffer);
									if (from != NULL) {
										strcpy(buffer, from);
										free(from);
									}
									
									from = (char *)malloc(strlen(user->loginname) + 1);
									strcpy(from, user->loginname);
									
									if (to != NULL) {
										free(to);
									}
									to = (char *)malloc(strlen(buffer) + 1);
									strcpy(to, buffer);
									
									replybody = editor(socket, body, to);
									
									if (replybody != NULL) {
										jb = open_jam_base(conf.email_path);
										if (!jb) {
											printf("Error opening JAM base.. %s\n", conf.email_path);
											free(replybody);
											free(body);
											free(subject);
											free(to);
											free(from);
											break;
										}
													
										JAM_ClearMsgHeader( &jmh );
										jmh.DateWritten = time(NULL);
										jmh.Attribute |= MSG_PRIVATE;
										
										jsp = JAM_NewSubPacket();
										jsf.LoID   = JAMSFLD_SENDERNAME;
										jsf.HiID   = 0;
										jsf.DatLen = strlen(from);
										jsf.Buffer = (uchar *)from;
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
													
										free(body);
										free(subject);
										free(to);
										free(from);
																									
										while (1) {
											z = JAM_LockMB(jb, 100);
											if (z == 0) {
												break;
											} else if (z == JAM_LOCK_FAILED) {
												sleep(1);
											} else {
												free(replybody);
												printf("Failed to lock msg base!\n");
												break;
											}
										}
										
										if (z != 0) {
											break;
										}
										
										if (JAM_AddMessage(jb, &jmh, jsp, (uchar *)replybody, strlen(replybody))) {
											printf("Failed to add message\n");
										}
														
										JAM_UnlockMB(jb);
													
										JAM_DelSubPacket(jsp);
										free(replybody);
										JAM_CloseMB(jb);
									}
								} else if (tolower(c) == 'd') {
									jb = open_jam_base(conf.email_path);
									if (!jb) {
										printf("Error opening JAM base.. %s\n", conf.email_path);
										free(body);
										free(subject);
										free(to);
										free(from);
										break;
									}
																				
									while (1) {
										z = JAM_LockMB(jb, 100);
										if (z == 0) {
											break;
										} else if (z == JAM_LOCK_FAILED) {
											sleep(1);
										} else {
											free(body);
											free(subject);
											free(to);
											free(from);
											printf("Failed to lock msg base!\n");
											break;
										}
									}
									if (z != 0) {
										break;
									}
									
									JAM_DeleteMessage(jb, currmsg);
									JAM_UnlockMB(jb);
									JAM_CloseMB(jb);
									free(body);
									free(subject);
									free(to);
									free(from);
								}

							} else {
								s_putstring(socket, "\r\nInvalid E-Mail\r\n");
								JAM_CloseMB(jb);
							}
						}
					}
				}
				break;
		}
	}
	
	return doquit;
}

int mail_getemailcount(struct user_record *user) {
	s_JamBase *jb;
	s_JamBaseHeader jbh;
	s_JamMsgHeader jmh;
	s_JamSubPacket* jsp;
	s_JamSubfield jsf;
	ulong jam_crc;
	ulong lastmsg, currmsg;
	
	int msg_count = 0;
	
	jb = open_jam_base(conf.email_path);
	if (!jb) {
		printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
	} else {
		jam_crc = JAM_Crc32((uchar *)user->loginname, strlen(user->loginname));
		lastmsg = 0;
		while (JAM_FindUser(jb, jam_crc, lastmsg, &currmsg) == 0) {
			if (JAM_ReadMsgHeader(jb, currmsg, &jmh, &jsp) != 0) {
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
			JAM_DelSubPacket(jsp);
			msg_count++;
			lastmsg = currmsg + 1;
		}
		JAM_CloseMB(jb);
	}
	return msg_count;
}
