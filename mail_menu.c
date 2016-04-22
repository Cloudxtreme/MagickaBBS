#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "jamlib/jam.h"
#include "bbs.h"
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

extern struct bbs_config conf; 
extern int mynode;
struct jam_msg {
	int msg_no;
	s_JamMsgHeader *msg_h;
	char *from;
	char *to;
	char *subject;
	char *oaddress;
	char *daddress;
	char *msgid;
	char *replyid;
};

struct msg_headers {
	struct jam_msg **msgs;
	int msg_count;
};

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
		} else {
			free(jb);
			printf("Got %d\n", ret);
			return NULL;
		}
	}
	return jb;
}

void free_message_headers(struct msg_headers *msghs) {
	int i;
	
	for (i=0;i<msghs->msg_count;i++) {
		free(msghs->msgs[i]->msg_h);
		if (msghs->msgs[i]->from != NULL) {
			free(msghs->msgs[i]->from);
		}
		if (msghs->msgs[i]->to != NULL) {
			free(msghs->msgs[i]->to);
		}
		if (msghs->msgs[i]->from != NULL) {
			free(msghs->msgs[i]->subject);
		}
		if (msghs->msgs[i]->oaddress != NULL) {
			free(msghs->msgs[i]->oaddress);
		}
		if (msghs->msgs[i]->daddress != NULL) {
			free(msghs->msgs[i]->daddress);
		}
		if (msghs->msgs[i]->msgid != NULL) {
			free(msghs->msgs[i]->msgid);
		}
		if (msghs->msgs[i]->replyid != NULL) {
			free(msghs->msgs[i]->replyid);
		}
	}
	free(msghs->msgs);
	free(msghs);
}

struct msg_headers *read_message_headers(int msgconf, int msgarea, struct user_record *user) {
	s_JamBase *jb;
	s_JamBaseHeader jbh;
	s_JamMsgHeader jmh;
	s_JamSubPacket* jsp;	
	struct jam_msg *jamm;
	char *wwiv_addressee;
	int to_us;
	int i;
	int z;
	int j;
	int k;
	
	struct fido_addr *dest;
	struct msg_headers *msghs = NULL;

	jb = open_jam_base(conf.mail_conferences[msgconf]->mail_areas[msgarea]->path);
	if (!jb) {
		printf("Error opening JAM base.. %s\n", conf.mail_conferences[msgconf]->mail_areas[msgarea]->path);
		return NULL;
	}

	JAM_ReadMBHeader(jb, &jbh);

	if (jbh.ActiveMsgs > 0) {
		msghs = (struct msg_headers *)malloc(sizeof(struct msg_headers));
		msghs->msg_count = 0;
		k = 0;
		for (i=0;k < jbh.ActiveMsgs;i++) {

			memset(&jmh, 0, sizeof(s_JamMsgHeader));
			z = JAM_ReadMsgHeader(jb, i, &jmh, &jsp);						
			if (z != 0) {
				printf("Failed to read msg header: %d Erro %d\n", z, JAM_Errno(jb));
				continue;
			}
								
			if (jmh.Attribute & MSG_DELETED) {
				JAM_DelSubPacket(jsp);
				continue;
			}

			jamm = (struct jam_msg *)malloc(sizeof(struct jam_msg));
			
			jamm->msg_no = i;
			jamm->msg_h = (s_JamMsgHeader *)malloc(sizeof(s_JamMsgHeader));
			memcpy(jamm->msg_h, &jmh, sizeof(s_JamMsgHeader));
			jamm->from = NULL;
			jamm->to = NULL;
			jamm->subject = NULL;
			jamm->oaddress = NULL;
			jamm->daddress = NULL;
			jamm->msgid = NULL;
			jamm->replyid = NULL;
				
			for (z=0;z<jsp->NumFields;z++) {
				if (jsp->Fields[z]->LoID == JAMSFLD_SUBJECT) {
					jamm->subject = (char *)malloc(jsp->Fields[z]->DatLen + 1);
					memset(jamm->subject, 0, jsp->Fields[z]->DatLen + 1);
					memcpy(jamm->subject, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
				}
				if (jsp->Fields[z]->LoID == JAMSFLD_SENDERNAME) {
					jamm->from = (char *)malloc(jsp->Fields[z]->DatLen + 1);
					memset(jamm->from, 0, jsp->Fields[z]->DatLen + 1);
					memcpy(jamm->from, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
				}				
				if (jsp->Fields[z]->LoID == JAMSFLD_RECVRNAME) {
					jamm->to = (char *)malloc(jsp->Fields[z]->DatLen + 1);
					memset(jamm->to, 0, jsp->Fields[z]->DatLen + 1);
					memcpy(jamm->to, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
				}				
				if (jsp->Fields[z]->LoID == JAMSFLD_DADDRESS) {
					jamm->daddress = (char *)malloc(jsp->Fields[z]->DatLen + 1);
					memset(jamm->daddress, 0, jsp->Fields[z]->DatLen + 1);
					memcpy(jamm->daddress, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
				}	
				if (jsp->Fields[z]->LoID == JAMSFLD_OADDRESS) {
					jamm->oaddress = (char *)malloc(jsp->Fields[z]->DatLen + 1);
					memset(jamm->oaddress, 0, jsp->Fields[z]->DatLen + 1);
					memcpy(jamm->oaddress, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
				}	
				if (jsp->Fields[z]->LoID == JAMSFLD_MSGID) {
					jamm->msgid = (char *)malloc(jsp->Fields[z]->DatLen + 1);
					memset(jamm->msgid, 0, jsp->Fields[z]->DatLen + 1);
					memcpy(jamm->msgid, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
				}	
				if (jsp->Fields[z]->LoID == JAMSFLD_REPLYID) {
					jamm->replyid = (char *)malloc(jsp->Fields[z]->DatLen + 1);
					memset(jamm->replyid, 0, jsp->Fields[z]->DatLen + 1);
					memcpy(jamm->replyid, jsp->Fields[z]->Buffer, jsp->Fields[z]->DatLen);
				}									
			}
			JAM_DelSubPacket(jsp);

			if (jmh.Attribute & MSG_PRIVATE) {
				wwiv_addressee = strdup(jamm->to);
				for (j=0;j<strlen(jamm->to);j++) {
					if (wwiv_addressee[j] == ' ') {
						wwiv_addressee[j] = '\0';
						break;
					}
				}
									
				if (conf.mail_conferences[msgconf]->nettype == NETWORK_WWIV) {
					if (conf.mail_conferences[msgconf]->wwivnode == atoi(jamm->daddress)) {
						to_us = 1;
					} else {
						to_us = 0;
					}
				} else if (conf.mail_conferences[msgconf]->nettype == NETWORK_FIDO) {
					dest = parse_fido_addr(jamm->daddress);
					if (conf.mail_conferences[msgconf]->fidoaddr->zone == dest->zone &&
						conf.mail_conferences[msgconf]->fidoaddr->net == dest->net &&
						conf.mail_conferences[msgconf]->fidoaddr->node == dest->node &&
						conf.mail_conferences[msgconf]->fidoaddr->point == dest->point) {
										
						to_us = 1;
					} else {
						to_us = 0;
					}
					free(dest);
				}
				
									
				if (!(strcasecmp(wwiv_addressee, user->loginname) == 0) || ((strcasecmp(jamm->from, user->loginname) == 0) && to_us)) {							
					
					if (jamm->subject != NULL) {
						free(jamm->subject);
					}
					if (jamm->from != NULL) {
						free(jamm->from);
					}
					if (jamm->to != NULL) {
						free(jamm->to);
					}
					if (jamm->oaddress != NULL) {
						free(jamm->oaddress);
					}
					if (jamm->daddress != NULL) {
						free(jamm->daddress);
					}
					if (jamm->msgid != NULL) {
						free(jamm->msgid);
					}
					if (jamm->replyid != NULL) {
						free(jamm->replyid);
					}
					free(wwiv_addressee);
					free(jamm->msg_h);
					free(jamm);
					k++;
					continue;
				}
				free(wwiv_addressee);
			}
								
			if (msghs->msg_count == 0) {
				msghs->msgs = (struct jam_msg **)malloc(sizeof(struct jam_msg *));
			} else {
				msghs->msgs = (struct jam_msg **)realloc(msghs->msgs, sizeof(struct jam_msg *) * (msghs->msg_count + 1));
			}
			
			msghs->msgs[msghs->msg_count] = jamm;
			msghs->msg_count++;
			k++;						
		}
		
	} else {
		JAM_CloseMB(jb);
		return NULL;
	}
	JAM_CloseMB(jb);
	return msghs;
}

char *external_editor(int socket, struct user_record *user, char *to, char *from, char *quote, char *qfrom, char *subject, int email) {
	char c;
	FILE *fptr;
	char *body = NULL;
	char buffer[256];
	char buffer2[256];
	int len;
	int totlen;
	char *body2 = NULL;
	char *tagline;
	int i;
	int j;
	struct stat s;
	
	if (conf.external_editor_cmd != NULL) {
		s_putstring(socket, "\r\nUse external editor? (Y/N) ");
		c = s_getc(socket);
		if (tolower(c) == 'y') {
			
			sprintf(buffer, "%s/node%d", conf.bbs_path, mynode);
	
			if (stat(buffer, &s) != 0) {
				mkdir(buffer, 0755);
			}
			
			sprintf(buffer, "%s/node%d/MSGTMP", conf.bbs_path, mynode);
			
			if (stat(buffer, &s) == 0) {
				remove(buffer);
			}
			
			// write msgtemp
			if (quote != NULL) {
				fptr = fopen(buffer, "w");
				for (i=0;i<strlen(quote);i++) {
					if (quote[i] == '\r') {
						fprintf(fptr, "\r\n");
					} else if (quote[i] == 0x1) {
						continue;
					} else if (quote[i] == '\e' && quote[i + 1] == '[') {
						while (strchr("ABCDEFGHIGJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", quote[i]) == NULL)
							i++;
					} else {
						fprintf(fptr, "%c", quote[i]);
					}
				}
				fclose(fptr);
			}
			sprintf(buffer, "%s/node%d/MSGINF", conf.bbs_path, mynode);
			fptr = fopen(buffer, "w");
			fprintf(fptr, "%s\r\n", user->loginname);
			if (qfrom != NULL) {
				fprintf(fptr, "%s\r\n", qfrom);
			} else {
				fprintf(fptr, "%s\r\n", to);
			}
			fprintf(fptr, "%s\r\n", subject);
			fprintf(fptr, "0\r\n");
			if (email == 1) {
				fprintf(fptr, "E-Mail\r\n");
				fprintf(fptr, "YES\r\n");
			} else {
				fprintf(fptr, "%s\r\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->name);
				if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_NETMAIL_AREA){
					fprintf(fptr, "YES\r\n");
				} else {
					fprintf(fptr, "NO\r\n");
				}
			}
			fclose(fptr);
			
			rundoor(socket, user, conf.external_editor_cmd, conf.external_editor_stdio);
			
			// readin msgtmp
			sprintf(buffer, "%s/node%d/MSGTMP", conf.bbs_path, mynode);
			fptr = fopen(buffer, "r");
			if (!fptr) {
				return NULL;
			}
	
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
			
			if (email == 1) {	
				tagline = conf.default_tagline;
			} else {
				if (conf.mail_conferences[user->cur_mail_conf]->tagline != NULL) {
					tagline = conf.mail_conferences[user->cur_mail_conf]->tagline;
				} else {
					tagline = conf.default_tagline;
				}
			}
			
			if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
				if (conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point == 0) {
					snprintf(buffer, 256, "\r--- MagickaBBS v%d.%d (%s)\r * Origin: %s (%d:%d/%d)\r", VERSION_MAJOR, VERSION_MINOR, VERSION_STR, tagline, conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																																						  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																																						  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node);
				} else {
					snprintf(buffer, 256, "\r--- MagickaBBS v%d.%d (%s)\r * Origin: %s (%d:%d/%d.%d)\r", VERSION_MAJOR, VERSION_MINOR, VERSION_STR, tagline, conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																																						  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																																						  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
																																						  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
				}
			} else {
				snprintf(buffer, 256, "\r--- MagickaBBS v%d.%d (%s)\r * Origin: %s \r", VERSION_MAJOR, VERSION_MINOR, VERSION_STR, tagline);
			}
			body2 = (char *)malloc(totlen + 2 + strlen(buffer));
			
			j = 0;
			
			for (i=0;i<totlen;i++) {
				if (body[i] == '\n') {
					continue;
				} else if (body[i] == '\0') {
					continue;
				}
				body2[j++] = body[i];
				body2[j] = '\0';
			}
			
			
			strcat(body2, buffer);
			
			free(body);
			
			return body2;
		}
	}
	return editor(socket, user, quote, qfrom);
}

char *editor(int socket, struct user_record *user, char *quote, char *from) {
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
	char *tagline;

	char next_line_buffer[80];

	memset(next_line_buffer, 0, 80);

	if (quote != NULL) {
		//wrap(quote, 65);
		for (i=0;i<strlen(quote);i++) {
			if (quote[i] == '\r' || lineat == 67) {
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
				if (quote[i] != '\r') {
					i--;
				}
			} else {
				linebuffer[lineat++] = quote[i];
				linebuffer[lineat] = '\0';
			}
		}
	}
	s_putstring(socket, "\r\n\e[1;32mMagicka Internal Editor, Type \e[1;37m/S \e[1;32mto save, \e[1;37m/A \e[1;32mto abort and \e[1;37m/? \e[1;32mfor help\r\n");
	s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m");
	
	while(!doquit) {
		sprintf(prompt, "\r\n\e[1;30m[\e[1;34m%3d\e[1;30m]: \e[0m%s", lines, next_line_buffer);
		s_putstring(socket, prompt);
		strcpy(linebuffer, next_line_buffer);
		s_readstring(socket, &linebuffer[strlen(next_line_buffer)], 70 - strlen(next_line_buffer));
		memset(next_line_buffer, 0, 70);

		if (strlen(linebuffer) == 70 && linebuffer[69] != ' ') {
			for (i=strlen(linebuffer) - 1;i > 15;i--) {
				if (linebuffer[i] == ' ') {
					linebuffer[i] = '\0';
					strcpy(next_line_buffer, &linebuffer[i+1]);
					sprintf(prompt, "\e[%dD\e[0K", 70 - i);
					s_putstring(socket, prompt);
					break;
				}
			}
		}
		
		if (linebuffer[0] == '/' && strlen(linebuffer) == 2) {
			if (toupper(linebuffer[1]) == 'S') {
				for (i=0;i<lines;i++) {
					size += strlen(content[i]) + 1;
				}
				size ++;
				
				if (conf.mail_conferences[user->cur_mail_conf]->tagline != NULL) {
					tagline = conf.mail_conferences[user->cur_mail_conf]->tagline;
				} else {
					tagline = conf.default_tagline;
				}
				
				if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
					if (conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point == 0) {
						snprintf(buffer, 256, "\r--- MagickaBBS v%d.%d (%s)\r * Origin: %s (%d:%d/%d)\r", VERSION_MAJOR, VERSION_MINOR, VERSION_STR, tagline, conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																																							  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																																							  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node);
					} else {
						snprintf(buffer, 256, "\r--- MagickaBBS v%d.%d (%s)\r * Origin: %s (%d:%d/%d.%d)\r", VERSION_MAJOR, VERSION_MINOR, VERSION_STR, tagline, conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																																							  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																																							  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
																																							  conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
					}
				} else {
					snprintf(buffer, 256, "\r--- MagickaBBS v%d.%d (%s)\r * Origin: %s \r", VERSION_MAJOR, VERSION_MINOR, VERSION_STR, tagline);
				}				
				size += 2;
				size += strlen(buffer);
				
				msg = (char *)malloc(size);
				memset(msg, 0, size);
				for (i=0;i<lines;i++) {
					strcat(msg, content[i]);
					strcat(msg, "\r");
					free(content[i]);
				}
				
				strcat(msg, buffer);
				
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

void read_message(int socket, struct user_record *user, struct msg_headers *msghs, int mailno) {
	s_JamBase *jb;
	s_JamBaseHeader jbh;
	s_JamMsgHeader jmh;
	s_JamSubPacket* jsp;
	s_JamSubfield jsf;
	s_JamLastRead jlr;
	
	
	char buffer[256];
	int z, z2;
	struct tm msg_date;
	
	char *subject = NULL;
	char *from = NULL;
	char *to = NULL;	
	char *body = NULL;
	char *body2 = NULL;
	int lines = 0;
	char c;
	char *replybody;
	struct fido_addr *from_addr = NULL;
	struct fido_addr *dest;
	int wwiv_to = 0;
	int i;
	char *wwiv_addressee;
	char *dest_addr;
	int to_us;
	char *msgid = NULL;
	char timestr[17];
	int doquit = 0;
	int skip_line = 0;
	int chars = 0;
	int ansi;
	
	jb = open_jam_base(conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
	if (!jb) {
		printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
		return;
	}
	
	while (!doquit) {
		
		if (JAM_ReadLastRead(jb, user->id, &jlr) == JAM_NO_USER) {
			jlr.UserCRC = JAM_Crc32(user->loginname, strlen(user->loginname));
			jlr.UserID = user->id;
			jlr.HighReadMsg = msghs->msgs[mailno]->msg_no;
		}
		
		jlr.LastReadMsg = mailno;
		if (jlr.HighReadMsg < msghs->msgs[mailno]->msg_no) {
			jlr.HighReadMsg = msghs->msgs[mailno]->msg_no;
		}
		
		if (msghs->msgs[mailno]->oaddress != NULL && conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
			from_addr = parse_fido_addr(msghs->msgs[mailno]->oaddress);
			sprintf(buffer, "\e[2J\e[1;32mFrom    : \e[1;37m%s (%d:%d/%d.%d)\r\n", msghs->msgs[mailno]->from, from_addr->zone, from_addr->net, from_addr->node, from_addr->point);
			free(from_addr);
		} else {
			sprintf(buffer, "\e[2J\e[1;32mFrom    : \e[1;37m%s\r\n", msghs->msgs[mailno]->from);
		}
		s_putstring(socket, buffer);
		sprintf(buffer, "\e[1;32mTo      : \e[1;37m%s\r\n", msghs->msgs[mailno]->to);
		s_putstring(socket, buffer);
		sprintf(buffer, "\e[1;32mSubject : \e[1;37m%s\r\n", msghs->msgs[mailno]->subject);
		s_putstring(socket, buffer);
		localtime_r((time_t *)&msghs->msgs[mailno]->msg_h->DateWritten, &msg_date);
		sprintf(buffer, "\e[1;32mDate    : \e[1;37m%s", asctime(&msg_date));
		buffer[strlen(buffer) - 1] = '\0';
		strcat(buffer, "\r\n");
		s_putstring(socket, buffer);
		sprintf(buffer, "\e[1;32mAttribs : \e[1;37m%s\r\n", (msghs->msgs[mailno]->msg_h->Attribute & MSG_SENT ? "SENT" : ""));
		s_putstring(socket, buffer);
		s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");

		body = (char *)malloc(msghs->msgs[mailno]->msg_h->TxtLen);
		
		JAM_ReadMsgText(jb, msghs->msgs[mailno]->msg_h->TxtOffset, msghs->msgs[mailno]->msg_h->TxtLen, (char *)body);
		JAM_WriteLastRead(jb, user->id, &jlr);

		if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV) {
			body2 = (char *)malloc(msghs->msgs[mailno]->msg_h->TxtLen);
			z2 = 0;
			if (body[0] == 4 && body[1] == '0') {
				skip_line = 1;
			} else {
				skip_line = 0;
			}
			for (z=0;z<msghs->msgs[mailno]->msg_h->TxtLen;z++) {
				if (body[z] == '\r') {
					if (body[z+1] == '\n') {
						z++;
					}
					if (body[z+1] == 4 && body[z+2] == '0') {
						skip_line = 1;
					} else {
						body2[z2++] = '\r';
						skip_line = 0;
					}
				} else {
					if (!skip_line) {
						if (body[z] == 3 || body[z] == 4) {
							z++;
						} else {
							body2[z2++] = body[z];
						}
					}
				}
			}
			
			free(body);
			body = body2;
		} else {
			z2 = msghs->msgs[mailno]->msg_h->TxtLen;
		}
		
		lines = 0;
		chars = 0;
		
		for (z=0;z<z2;z++) {
			if (body[z] == '\r' || chars == 79) {
				chars = 0;
				s_putstring(socket, "\r\n");
				lines++;
				if (lines == 17) {
					s_putstring(socket, "Press a key to continue...");
					s_getc(socket);
					lines = 0;
					s_putstring(socket, "\e[7;1H\e[0J");
				}
			} else if (body[z] == '\e' && body[z + 1] == '[') {
				ansi = z;
				while (strchr("ABCDEFGHIGJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", body[z]) == NULL)
					z++;
				if (body[z] == 'm' || body[z] == 'C') {
					strncpy(buffer, &body[ansi], z - ansi);
					buffer[z - ansi] = '\0';
					s_putstring(socket, buffer);
				}
			} else {
				chars++;
				s_putchar(socket, body[z]);
			}
		}

		s_putstring(socket, "Press R to reply, Q to quit, SPACE for Next Mesage...");
		
		c = s_getc(socket);
		
		if (tolower(c) == 'r') {
			JAM_CloseMB(jb);
			if (user->sec_level < conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->write_sec_level) {
				s_putstring(socket, "\r\nSorry, you are not allowed to post in this area\r\n");
			} else {
				if (msghs->msgs[mailno]->subject != NULL) {
					if (strncasecmp(msghs->msgs[mailno]->subject, "RE:", 3) != 0) {
						snprintf(buffer, 256, "RE: %s", msghs->msgs[mailno]->subject);
					} else {
						snprintf(buffer, 256, "%s", msghs->msgs[mailno]->subject);
					}
				}
				subject = (char *)malloc(strlen(buffer) + 1);
				strcpy(subject, buffer);
				
				sprintf(buffer, "\r\n\r\nReplying to: %s\r\n", subject);
				s_putstring(socket, buffer);
				s_putstring(socket, "Change Subject? (Y/N) ");
				
				c = s_getc(socket);
				
				if (tolower(c) == 'y') {
					s_putstring(socket, "\r\nNew subject: ");
					s_readstring(socket, buffer, 25);
					
					if (strlen(buffer) == 0) {
						s_putstring(socket, "\r\nOk, not changing the subject line...");
					} else {
						free(subject);
						subject = (char *)malloc(strlen(buffer) + 1);
						strcpy(subject, buffer);
					}
				}
				s_putstring(socket, "\r\n");
				
				if (msghs->msgs[mailno]->from != NULL) {
					strcpy(buffer, msghs->msgs[mailno]->from);
				}
				if (conf.mail_conferences[user->cur_mail_conf]->realnames == 0) {
					if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV) {
						from = (char *)malloc(strlen(user->loginname) + 20);
						sprintf(from, "%s #%d @%d", user->loginname, user->id, conf.mail_conferences[user->cur_mail_conf]->wwivnode);
					} else {
						from = (char *)malloc(strlen(user->loginname) + 1);
						strcpy(from, user->loginname);
					}
				} else {
					if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV) {
						from = (char *)malloc(strlen(user->loginname) + 23 + strlen(user->firstname));
						sprintf(from, "%s #%d @%d (%s)", user->loginname, user->id, conf.mail_conferences[user->cur_mail_conf]->wwivnode, user->firstname);
					} else {				
						from = (char *)malloc(strlen(user->firstname) + strlen(user->lastname) + 2);
						sprintf(from, "%s %s", user->firstname, user->lastname);
					}
				}
				if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV && conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_ECHOMAIL_AREA) {
					to = (char *)malloc(4);
					strcpy(to, "ALL");
				} else {
					to = (char *)malloc(strlen(buffer) + 1);
					strcpy(to, buffer);
				}
				replybody = external_editor(socket, user, to, from, body, msghs->msgs[mailno]->from, subject, 0);
				if (replybody != NULL) {
					
					jb = open_jam_base(conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
					if (!jb) {
						printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
						free(replybody);
						free(body);
						free(subject);
						free(to);
						free(from);
						return;
					}
								
					JAM_ClearMsgHeader( &jmh );
					jmh.DateWritten = time(NULL);
					jmh.Attribute |= MSG_LOCAL;
								
					jsp = JAM_NewSubPacket();
					jsf.LoID   = JAMSFLD_SENDERNAME;
					jsf.HiID   = 0;
					jsf.DatLen = strlen(from);
					jsf.Buffer = (char *)from;
					JAM_PutSubfield(jsp, &jsf);

					jsf.LoID   = JAMSFLD_RECVRNAME;
					jsf.HiID   = 0;
					jsf.DatLen = strlen(to);
					jsf.Buffer = (char *)to;
					JAM_PutSubfield(jsp, &jsf);
								
					jsf.LoID   = JAMSFLD_SUBJECT;
					jsf.HiID   = 0;
					jsf.DatLen = strlen(subject);
					jsf.Buffer = (char *)subject;
					JAM_PutSubfield(jsp, &jsf);


					
					if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_ECHOMAIL_AREA) {
						jmh.Attribute |= MSG_TYPEECHO;
						
						if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
							if (conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point) {
								sprintf(buffer, "%d:%d/%d.%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
							} else {
								sprintf(buffer, "%d:%d/%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node);
							}
							jsf.LoID   = JAMSFLD_OADDRESS;
							jsf.HiID   = 0;
							jsf.DatLen = strlen(buffer);
							jsf.Buffer = (char *)buffer;
							JAM_PutSubfield(jsp, &jsf);
							
							snprintf(timestr, 16, "%016lx", time(NULL));
											
							sprintf(buffer, "%d:%d/%d.%d %s", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point,
															&timestr[strlen(timestr) - 8]);
							
							jsf.LoID   = JAMSFLD_MSGID;
							jsf.HiID   = 0;
							jsf.DatLen = strlen(buffer);
							jsf.Buffer = (char *)buffer;
							JAM_PutSubfield(jsp, &jsf);
							
							if (msghs->msgs[mailno]->msgid != NULL) {
								sprintf(buffer, "%d:%d/%d.%d %s", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
										conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
										conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
										conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point,
										&msghs->msgs[mailno]->msgid[strlen(timestr) - 8]);	
							}
							
							jsf.LoID   = JAMSFLD_REPLYID;
							jsf.HiID   = 0;
							jsf.DatLen = strlen(buffer);
							jsf.Buffer = (char *)buffer;
							JAM_PutSubfield(jsp, &jsf);	
							jmh.ReplyCRC = JAM_Crc32(buffer, strlen(buffer));						
						}
					} else if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_NETMAIL_AREA) {
						jmh.Attribute |= MSG_TYPENET;
						jmh.Attribute |= MSG_PRIVATE;
						
						if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
							if (conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point) {
								sprintf(buffer, "%d:%d/%d.%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
							} else {
								sprintf(buffer, "%d:%d/%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node);
							}
							jsf.LoID   = JAMSFLD_OADDRESS;
							jsf.HiID   = 0;
							jsf.DatLen = strlen(buffer);
							jsf.Buffer = (char *)buffer;
							JAM_PutSubfield(jsp, &jsf);
							jmh.MsgIdCRC = JAM_Crc32(buffer, strlen(buffer));	
							from_addr = parse_fido_addr(msghs->msgs[mailno]->oaddress);
							if (from_addr != NULL) {
								if (from_addr->point) {
									sprintf(buffer, "%d:%d/%d.%d", from_addr->zone,
																from_addr->net,
																from_addr->node,
																from_addr->point);
								} else {
									sprintf(buffer, "%d:%d/%d", from_addr->zone,
																from_addr->net,
																from_addr->node);							
								}
								jsf.LoID   = JAMSFLD_DADDRESS;
								jsf.HiID   = 0;
								jsf.DatLen = strlen(buffer);
								jsf.Buffer = (char *)buffer;
								JAM_PutSubfield(jsp, &jsf);
								free(from_addr);
							}
							
							snprintf(timestr, 16, "%016lx", time(NULL));
							
							sprintf(buffer, "%d:%d/%d.%d %s", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
															conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point,
															&timestr[strlen(timestr) - 8]);
							
							jsf.LoID   = JAMSFLD_MSGID;
							jsf.HiID   = 0;
							jsf.DatLen = strlen(buffer);
							jsf.Buffer = (char *)buffer;
							JAM_PutSubfield(jsp, &jsf);
							jmh.MsgIdCRC = JAM_Crc32(buffer, strlen(buffer));	
							if (msghs->msgs[mailno]->msgid != NULL) {
								sprintf(buffer, "%d:%d/%d.%d %s", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
										conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
										conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
										conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point,
										&msghs->msgs[mailno]->msgid[strlen(timestr) - 8]);	
							}
							
							jsf.LoID   = JAMSFLD_REPLYID;
							jsf.HiID   = 0;
							jsf.DatLen = strlen(buffer);
							jsf.Buffer = (char *)buffer;
							JAM_PutSubfield(jsp, &jsf);	
							jmh.ReplyCRC = JAM_Crc32(buffer, strlen(buffer));							
						} else if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV) {	
							sprintf(buffer, "%d", atoi(strchr(from, '@') + 1));
							jsf.LoID   = JAMSFLD_DADDRESS;
							jsf.HiID   = 0;
							jsf.DatLen = strlen(buffer);
							jsf.Buffer = (char *)buffer;
							JAM_PutSubfield(jsp, &jsf);			
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
							return;
						}
					}
					if (JAM_AddMessage(jb, &jmh, jsp, (char *)replybody, strlen(replybody))) {
						printf("Failed to add message\n");
					}
									
					JAM_UnlockMB(jb);
								
					JAM_DelSubPacket(jsp);
					free(replybody);
					JAM_CloseMB(jb);
					doquit = 1;
				} else {
					doquit = 1;
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
			
		} else if (tolower(c) == 'q') {
			doquit = 1;
		} else if (c == ' ') {
			mailno++;
			if (mailno >= msghs->msg_count) {
				s_putstring(socket, "\r\n\r\nNo more messages\r\n");
				doquit = 1;
			}
		}
	}
}

int mail_menu(int socket, struct user_record *user) {
	int doquit = 0;
	int domail = 0;
	char c;
	char prompt[128];
	char buffer[256];
	char buffer2[256];
	int i;
	int j;
	int z;

	struct msg_headers *msghs;
	
	s_JamBase *jb;
	s_JamBaseHeader jbh;
	s_JamMsgHeader jmh;
	s_JamSubPacket* jsp;
	s_JamSubfield jsf;
	s_JamLastRead jlr;
	
	struct tm msg_date;
	
	char *subject;
	char *from;
	char *to;
	char *body;
	char *replybody;
	char *wwiv_addressee;
	char timestr[17];
	char *msg;
	int closed;
	uint32_t jam_crc;
	unsigned int lastmsg,currmsg;
	int lines;
	struct fido_addr *from_addr = NULL;
	struct fido_addr *dest = NULL;
	char *dest_addr;
	int to_us;
	int wwiv_to;
	struct stat s;
	int do_internal_menu = 0;
	char *lRet;
	lua_State *L;
	int result;
	
	if (conf.script_path != NULL) {
		sprintf(buffer, "%s/mailmenu.lua", conf.script_path);
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
	
	while (!domail) {
		if (do_internal_menu == 1) {
			s_displayansi(socket, "mailmenu");
			
			
			sprintf(prompt, "\e[0m\r\nConf: (%d) %s\r\nArea: (%d) %s\r\nTL: %dm :> ", user->cur_mail_conf, conf.mail_conferences[user->cur_mail_conf]->name, user->cur_mail_area, conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->name, user->timeleft);
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
			case 'p':
				{
					if (user->sec_level < conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->write_sec_level) {
						s_putstring(socket, "\r\nSorry, you are not allowed to post in this area\r\n");
						break;
					}
					if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV && conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type  == TYPE_ECHOMAIL_AREA) {
						sprintf(buffer, "ALL");
					} else {
						s_putstring(socket, "\r\nTO: ");
						s_readstring(socket, buffer, 16);
					}
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
						s_readstring(socket, buffer2, 32);
						if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
							from_addr = parse_fido_addr(buffer2);
							if (!from_addr) {
								s_putstring(socket, "\r\n\r\nInvalid Address\r\n");
								break;							
							} else {
								if (from_addr->zone == 0 && from_addr->net == 0 && from_addr->node == 0 && from_addr->point == 0) {
									free(from_addr);
									s_putstring(socket, "\r\n\r\nInvalid Address\r\n");
									break;							
								}
								sprintf(buffer2, "\r\nMailing to %d:%d/%d.%d\r\n", from_addr->zone, from_addr->net, from_addr->node, from_addr->point);
								s_putstring(socket, buffer2);
							}
						} else if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV) {
							wwiv_to = atoi(buffer2);
							if (wwiv_to == 0) {
								s_putstring(socket, "\r\n\r\nInvalid Address\r\n");
								break;							
							} else {
								sprintf(buffer2, "\r\nMailing to @%d\r\n", wwiv_to);
								s_putstring(socket, buffer2);
							}
						}
					}
					to = strdup(buffer);
					s_putstring(socket, "\r\nSUBJECT: ");
					s_readstring(socket, buffer, 25);
					if (strlen(buffer) == 0) {
						s_putstring(socket, "\r\nAborted!\r\n");
						free(to);
						if (from_addr != NULL) {
							free(from_addr);
						}
						break;
					}
					subject = strdup(buffer);
					
					// post a message
					msg = external_editor(socket, user, to, from, NULL, NULL, subject, 0);
					
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
						jmh.DateWritten = (uint32_t)time(NULL);
						jmh.Attribute |= MSG_LOCAL;
						if (conf.mail_conferences[user->cur_mail_conf]->realnames == 0) {
							if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV) {
								sprintf(buffer, "%s #%d @%d", user->loginname, user->id, conf.mail_conferences[user->cur_mail_conf]->wwivnode);
							} else {
								strcpy(buffer, user->loginname);
							}
						} else {
							if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV) {
								sprintf(buffer, "%s #%d @%d (%s)", user->loginname, user->id, conf.mail_conferences[user->cur_mail_conf]->wwivnode, user->firstname);
							} else {				
								sprintf(from, "%s %s", user->firstname, user->lastname);
							}
						}
						
						jsp = JAM_NewSubPacket();
						
						jsf.LoID   = JAMSFLD_SENDERNAME;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(buffer);
						jsf.Buffer = (char *)buffer;
						JAM_PutSubfield(jsp, &jsf);

						jsf.LoID   = JAMSFLD_RECVRNAME;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(to);
						jsf.Buffer = (char *)to;
						JAM_PutSubfield(jsp, &jsf);
						
						jsf.LoID   = JAMSFLD_SUBJECT;
						jsf.HiID   = 0;
						jsf.DatLen = strlen(subject);
						jsf.Buffer = (char *)subject;
						JAM_PutSubfield(jsp, &jsf);
						
						if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_ECHOMAIL_AREA) {
							jmh.Attribute |= MSG_TYPEECHO;
							
							if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
								if (conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point) {
									sprintf(buffer, "%d:%d/%d.%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
								} else {
									sprintf(buffer, "%d:%d/%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node);
								}
								jsf.LoID   = JAMSFLD_OADDRESS;
								jsf.HiID   = 0;
								jsf.DatLen = strlen(buffer);
								jsf.Buffer = (char *)buffer;
								JAM_PutSubfield(jsp, &jsf);
								
								snprintf(timestr, 16, "%016lx", time(NULL));
												
								sprintf(buffer, "%d:%d/%d.%d %s", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point,
																&timestr[strlen(timestr) - 8]);
								
								jsf.LoID   = JAMSFLD_MSGID;
								jsf.HiID   = 0;
								jsf.DatLen = strlen(buffer);
								jsf.Buffer = (char *)buffer;
								JAM_PutSubfield(jsp, &jsf);
								jmh.MsgIdCRC = JAM_Crc32(buffer, strlen(buffer));
								
							}
						} else
						if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->type == TYPE_NETMAIL_AREA) {
							jmh.Attribute |= MSG_TYPENET;
							jmh.Attribute |= MSG_PRIVATE;
							if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_FIDO) {
								if (conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point) {
									sprintf(buffer, "%d:%d/%d.%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point);
								} else {
									sprintf(buffer, "%d:%d/%d", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node);
									
								}
								jsf.LoID   = JAMSFLD_OADDRESS;
								jsf.HiID   = 0;
								jsf.DatLen = strlen(buffer);
								jsf.Buffer = (char *)buffer;
								JAM_PutSubfield(jsp, &jsf);
								
								if (from_addr != NULL) {
									if (from_addr->point) {
										sprintf(buffer, "%d:%d/%d.%d", from_addr->zone,
																	from_addr->net,
																	from_addr->node,
																	from_addr->point);
									} else {
										sprintf(buffer, "%d:%d/%d", from_addr->zone,
																	from_addr->net,
																	from_addr->node);
									}
									jsf.LoID   = JAMSFLD_DADDRESS;
									jsf.HiID   = 0;
									jsf.DatLen = strlen(buffer);
									jsf.Buffer = (char *)buffer;
									JAM_PutSubfield(jsp, &jsf);
									free(from_addr);	
									from_addr = NULL;			
								}
								snprintf(timestr, 16, "%016lx", time(NULL));
												
								sprintf(buffer, "%d:%d/%d.%d %s", conf.mail_conferences[user->cur_mail_conf]->fidoaddr->zone,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->net,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->node,
																conf.mail_conferences[user->cur_mail_conf]->fidoaddr->point,
																&timestr[strlen(timestr) - 8]);
								
								jsf.LoID   = JAMSFLD_MSGID;
								jsf.HiID   = 0;
								jsf.DatLen = strlen(buffer);
								jsf.Buffer = (char *)buffer;
								JAM_PutSubfield(jsp, &jsf);
								jmh.MsgIdCRC = JAM_Crc32(buffer, strlen(buffer));
							} else if (conf.mail_conferences[user->cur_mail_conf]->nettype == NETWORK_WWIV) {	
								sprintf(buffer, "%d", wwiv_to);
								jsf.LoID   = JAMSFLD_DADDRESS;
								jsf.HiID   = 0;
								jsf.DatLen = strlen(buffer);
								jsf.Buffer = (char *)buffer;
								JAM_PutSubfield(jsp, &jsf);			
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
						
						if (JAM_AddMessage(jb, &jmh, jsp, (char *)msg, strlen(msg))) {
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
					msghs = read_message_headers(user->cur_mail_conf, user->cur_mail_area, user);
					if (msghs != NULL && msghs->msg_count > 0) {
						jb = open_jam_base(conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
						if (!jb) {
							printf("Error opening JAM base.. %s\n", conf.mail_conferences[user->cur_mail_conf]->mail_areas[user->cur_mail_area]->path);
							break;
						} else {
							if (JAM_ReadLastRead(jb, user->id, &jlr) == JAM_NO_USER) {
								jlr.LastReadMsg = 0;
								jlr.HighReadMsg = 0;
							}
							sprintf(buffer, "Start at message [0-%d] ? ", msghs->msg_count - 1);
							s_putstring(socket, buffer);
								
							s_readstring(socket, buffer, 6);
							i = atoi(buffer);
							if (i < 0) {
								i = 0;
							}
							closed = 0;
							s_putstring(socket, "\e[2J\e[1;37;44m[MSG#] Subject                   From            To              Date          \r\n\e[0m");
								
							for (j=i;j<msghs->msg_count;j++) {
								localtime_r((time_t *)&msghs->msgs[j]->msg_h->DateWritten, &msg_date);
								if (msghs->msgs[j]->msg_no > jlr.HighReadMsg) {
									sprintf(buffer, "\e[1;30m[\e[1;34m%4d\e[1;30m]\e[1;32m*\e[1;37m%-25.25s \e[1;32m%-15.15s \e[1;33m%-15.15s \e[1;35m%02d:%02d %02d-%02d-%02d\e[0m\r\n", j, msghs->msgs[j]->subject, msghs->msgs[j]->from, msghs->msgs[j]->to, msg_date.tm_hour, msg_date.tm_min, msg_date.tm_mday, msg_date.tm_mon + 1, msg_date.tm_year - 100);
								} else {
									sprintf(buffer, "\e[1;30m[\e[1;34m%4d\e[1;30m] \e[1;37m%-25.25s \e[1;32m%-15.15s \e[1;33m%-15.15s \e[1;35m%02d:%02d %02d-%02d-%02d\e[0m\r\n", j, msghs->msgs[j]->subject, msghs->msgs[j]->from, msghs->msgs[j]->to, msg_date.tm_hour, msg_date.tm_min, msg_date.tm_mday, msg_date.tm_mon + 1, msg_date.tm_year - 100);
								}
								s_putstring(socket, buffer);
								
								if ((j - i) != 0 && (j - i) % 20 == 0) {
									sprintf(buffer, "(#) Read Message # (Q) Quit (ENTER) Continue\r\n");
									s_putstring(socket, buffer);
									s_readstring(socket, buffer, 6);
									
									if (tolower(buffer[0]) == 'q') {
										JAM_CloseMB(jb);
										closed = 1;
										break;
									} else if (strlen(buffer) > 0) {
										z = atoi(buffer);
										if (z >= 0 && z <= msghs->msg_count) {
											JAM_CloseMB(jb);
											closed = 1;
											read_message(socket, user, msghs, z);
											break;
										}
									}
									s_putstring(socket, "\e[2J\e[1;37;44m[MSG#] Subject                   From            To              Date          \r\n\e[0m");
								}
								
							}
							if (closed == 0) {
								sprintf(buffer, "(#) Read Message # (ENTER) Quit\r\n");
								s_putstring(socket, buffer);
								s_readstring(socket, buffer, 6);
								if (strlen(buffer) > 0) {
									z = atoi(buffer);
									if (z >= 0 && z <= msghs->msg_count) {
										JAM_CloseMB(jb);
										closed = 1;
										read_message(socket, user, msghs, z);
									}
								}
							
								JAM_CloseMB(jb);
							}
						}
						
						if (msghs != NULL) {
							free_message_headers(msghs);
						}
					} else {
						s_putstring(socket, "\r\nThere is no mail in this area\r\n");
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
						if (i != 0 && i % 20 == 0) {
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
						if (i != 0 && i % 20 == 0) {
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
					send_email(socket, user);
				}
				break;
			case 'r':
				{
					// Read your email...
					s_putstring(socket, "\r\n");
					list_emails(socket, user);
				}
				break;
			case '}':
				{
					for (i=user->cur_mail_conf;i<conf.mail_conference_count;i++) {
						if (i + 1 == conf.mail_conference_count) {
							i = -1;
						}
						if (conf.mail_conferences[i+1]->sec_level <= user->sec_level) {
							user->cur_mail_conf = i + 1;
							user->cur_mail_area = 0;
							break;
						}
					}
				}
				break;
			case '{':
				{
					for (i=user->cur_mail_conf;i>=0;i--) {
						if (i - 1 == -1) {
							i = conf.mail_conference_count;
						}
						if (conf.mail_conferences[i-1]->sec_level <= user->sec_level) {
							user->cur_mail_conf = i - 1;
							user->cur_mail_area = 0;
							break;
						}
					}
				}
				break;
			case ']':
				{
					for (i=user->cur_mail_area;i<conf.mail_conferences[user->cur_mail_conf]->mail_area_count;i++) {
						if (i + 1 == conf.mail_conferences[user->cur_mail_conf]->mail_area_count) {
							i = -1;
						}
						if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[i+1]->read_sec_level <= user->sec_level) {
							user->cur_mail_area = i + 1;
							break;
						}
					}
				}
				break;
			case '[':
				{
					for (i=user->cur_mail_area;i>=0;i--) {
						if (i - 1 == -1) {
							i = conf.mail_conferences[user->cur_mail_conf]->mail_area_count;
						}
						if (conf.mail_conferences[user->cur_mail_conf]->mail_areas[i-1]->read_sec_level <= user->sec_level) {
							user->cur_mail_area = i - 1;
							break;
						}
					}
				}
				break;				
		}
	}
	if (do_internal_menu == 0) {
		lua_close(L);
	}
	return doquit;
}

void mail_scan(int socket, struct user_record *user) {
	s_JamBase *jb;
	s_JamBaseHeader jbh;
	s_JamLastRead jlr;
	
	char c;
	int i;
	int j;
	char buffer[256];
	int count;
	
	s_putstring(socket, "\r\nScan for new mail? (Y/N) :	");
	c = s_getc(socket);
	
	if (tolower(c) == 'y') {
		for (i=0;i<conf.mail_conference_count;i++) {
			if (conf.mail_conferences[i]->sec_level > user->sec_level) {
				continue;
			}
			sprintf(buffer, "\r\n%d. %s\r\n", i, conf.mail_conferences[i]->name);
			s_putstring(socket, buffer);
			for (j=0;j<conf.mail_conferences[i]->mail_area_count;j++) {
				if (conf.mail_conferences[i]->mail_areas[j]->read_sec_level > user->sec_level) {
					continue;
				}
				jb = open_jam_base(conf.mail_conferences[i]->mail_areas[j]->path);
				if (!jb) {
					printf("Unable to open message base\n");
					continue;
				}
				if (JAM_ReadMBHeader(jb, &jbh) != 0) {
					JAM_CloseMB(jb);
					continue;
				}
				if (JAM_ReadLastRead(jb, user->id, &jlr) == JAM_NO_USER) {
					if (jbh.ActiveMsgs == 0) {
						JAM_CloseMB(jb);
						continue;
					}
					sprintf(buffer, "   --> %d. %s (%d new)\r\n", j, conf.mail_conferences[i]->mail_areas[j]->name, jbh.ActiveMsgs);
				} else {
					if (jlr.HighReadMsg < (jbh.ActiveMsgs - 1)) {
						sprintf(buffer, "   --> %d. %s (%d new)\r\n", j, conf.mail_conferences[i]->mail_areas[j]->name, (jbh.ActiveMsgs - 1) - jlr.HighReadMsg);
					} else {
						JAM_CloseMB(jb);
						continue;
					}
				}
				s_putstring(socket, buffer);
				JAM_CloseMB(jb);
			}
		}
		sprintf(buffer, "\r\nPress any key to continue...\r\n");
		s_putstring(socket, buffer);
		s_getc(socket);
	}
}


