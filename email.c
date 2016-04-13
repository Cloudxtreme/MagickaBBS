#include <sqlite3.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "bbs.h"

extern struct bbs_config conf;

void send_email(int socket, struct user_record *user) {
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    char *recipient;
    char *subject;
    char *msg;
    char *csql = "CREATE TABLE IF NOT EXISTS email ("
    					"id INTEGER PRIMARY KEY,"
						"sender TEXT COLLATE NOCASE,"
						"recipient TEXT COLLATE NOCASE,"
						"subject TEXT,"
						"body TEXT,"
						"date INTEGER,"
						"seen INTEGER);";
    char *isql = "INSERT INTO email (sender, recipient, subject, body, date, seen) VALUES(?, ?, ?, ?, ?, 0)";
    char *err_msg = 0;
     
	s_putstring(socket, "\r\nTO: ");
	s_readstring(socket, buffer, 16);
					
	if (strlen(buffer) == 0) {
		s_putstring(socket, "\r\nAborted\r\n");
		return;
	}
	if (check_user(buffer)) {
		s_putstring(socket, "\r\n\r\nInvalid Username\r\n");
		return;
	}
					
	recipient = strdup(buffer);
	s_putstring(socket, "\r\nSUBJECT: ");
	s_readstring(socket, buffer, 25);
	if (strlen(buffer) == 0) {
		free(recipient);
		s_putstring(socket, "\r\nAborted\r\n");
		return;
	}
	subject = strdup(buffer);
					
	// post a message
	msg = external_editor(socket, user, user->loginname, recipient, NULL, NULL, subject, 1);
					
	if (msg != NULL) {
		sprintf(buffer, "%s/email.sq3", conf.bbs_path);

		rc = sqlite3_open(buffer, &db);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
				
			exit(1);
		}		
		
		
		rc = sqlite3_exec(db, csql, 0, 0, &err_msg);
		if (rc != SQLITE_OK ) {
			
			fprintf(stderr, "SQL error: %s\n", err_msg);
			
			sqlite3_free(err_msg);        
			sqlite3_close(db);
			
			return;
		} 		
		
		rc = sqlite3_prepare_v2(db, isql, -1, &res, 0);
		
		if (rc == SQLITE_OK) {    
			sqlite3_bind_text(res, 1, user->loginname, -1, 0);
			sqlite3_bind_text(res, 2, recipient, -1, 0);
			sqlite3_bind_text(res, 3, subject, -1, 0);
			sqlite3_bind_text(res, 4, msg, -1, 0);
			sqlite3_bind_int(res, 5, time(NULL));
		} else {
			fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(res);
			sqlite3_close(db);	
			s_putstring(socket, "\r\nNo such email\r\n");	        
			return;
		}
		sqlite3_step(res);
				
		sqlite3_finalize(res);
		sqlite3_close(db);
		free(msg);
	}				
    free(subject);
    free(recipient);
}

void show_email(int socket, struct user_record *user, int msgno) {
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    char *sql = "SELECT id,sender,subject,body,date FROM email WHERE recipient LIKE ? LIMIT ?, 1";
    char *dsql = "DELETE FROM email WHERE id=?";
    char *isql = "INSERT INTO email (sender, recipient, subject, body, date, seen) VALUES(?, ?, ?, ?, ?, 0)";
    char *ssql = "UPDATE email SET seen=1 WHERE id=?";
	int id;
	char *sender;
	char *subject;
	char *body;
	time_t date;
	struct tm msg_date;
	int z;
	int lines;
	char c;
	char *replybody;
	int chars;
	
	sprintf(buffer, "%s/email.sq3", conf.bbs_path);
	
	rc = sqlite3_open(buffer, &db);
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        exit(1);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {    
        sqlite3_bind_text(res, 1, user->loginname, -1, 0);
        sqlite3_bind_int(res, 2, msgno);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(res);
		sqlite3_close(db);	
		s_putstring(socket, "\r\nNo such email\r\n");	        
		return;
    }
	if (sqlite3_step(res) == SQLITE_ROW) {
		id = sqlite3_column_int(res, 0);
		sender = strdup((char *)sqlite3_column_text(res, 1));
		subject = strdup((char *)sqlite3_column_text(res, 2));
		body = strdup((char *)sqlite3_column_text(res, 3));
		date = (time_t)sqlite3_column_int(res, 4);
		
		
		sprintf(buffer, "\e[2J\e[1;32mFrom    : \e[1;37m%s\r\n", sender);
		s_putstring(socket, buffer);
		sprintf(buffer, "\e[1;32mSubject : \e[1;37m%s\r\n", subject);
		s_putstring(socket, buffer);
		localtime_r(&date, &msg_date);
		sprintf(buffer, "\e[1;32mDate    : \e[1;37m%s", asctime(&msg_date));
		buffer[strlen(buffer) - 1] = '\0';
		strcat(buffer, "\r\n");
		s_putstring(socket, buffer);
		s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");
		
		lines = 0;
		chars = 0;
		
		for (z=0;z<strlen(body);z++) {
			if (body[z] == '\r' || chars == 79) {
				chars = 0;
				s_putstring(socket, "\r\n");
				lines++;
				if (lines == 19) {
					s_putstring(socket, "Press a key to continue...");
					s_getc(socket);
					lines = 0;
					s_putstring(socket, "\e[5;1H\e[0J");
				}
			} else {
				s_putchar(socket, body[z]);
				chars++;
			}
		}
		
		sqlite3_finalize(res);

		rc = sqlite3_prepare_v2(db, ssql, -1, &res, 0);
    
		if (rc == SQLITE_OK) {    
			sqlite3_bind_int(res, 1, id);
		} else {
			fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(res);
			sqlite3_close(db);	
			return;
		}
		sqlite3_step(res);
		
		s_putstring(socket, "Press R to reply, D to delete Enter to quit...\r\n");
		c = s_getc(socket);
		if (tolower(c) == 'r') {		
			if (subject != NULL) {
				if (strncasecmp(buffer, "RE:", 3) != 0) {
					snprintf(buffer, 256, "RE: %s", subject);
				} else {
					snprintf(buffer, 256, "%s", subject);
				}
				free(subject);
			}
			subject = (char *)malloc(strlen(buffer) + 1);
			strcpy(subject, buffer);
						
			//replybody = editor(socket, user, body, sender);
			replybody = external_editor(socket, user, user->loginname, sender, body, sender, subject, 1);	
			if (replybody != NULL) {
				rc = sqlite3_prepare_v2(db, isql, -1, &res, 0);
		
				if (rc == SQLITE_OK) {    
					sqlite3_bind_text(res, 1, user->loginname, -1, 0);
					sqlite3_bind_text(res, 2, sender, -1, 0);
					sqlite3_bind_text(res, 3, subject, -1, 0);
					sqlite3_bind_text(res, 4, replybody, -1, 0);
					sqlite3_bind_int(res, 5, time(NULL));
				} else {
					fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
					sqlite3_finalize(res);
					sqlite3_close(db);	
					s_putstring(socket, "\r\nNo such email\r\n");	        
					return;
				}
				sqlite3_step(res);
				
				sqlite3_finalize(res);
				free(replybody);
			}
			free(sender);
			free(subject);
			free(body);
			sqlite3_close(db);						
		} else if (tolower(c) == 'd') {
			free(sender);
			free(subject);
			free(body);
			
			rc = sqlite3_prepare_v2(db, dsql, -1, &res, 0);
    
			if (rc == SQLITE_OK) {    
				sqlite3_bind_int(res, 1, id);
			} else {
				fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
				sqlite3_finalize(res);
				sqlite3_close(db);	
				s_putstring(socket, "\r\nNo such email\r\n");	        
				return;
			}
			sqlite3_step(res);
			
			sqlite3_finalize(res);
			sqlite3_close(db);			
		}
		
	} else {
		printf("Failed\n");
		sqlite3_finalize(res);
		sqlite3_close(db);
	}
}

void list_emails(int socket, struct user_record *user) {
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    char *sql = "SELECT sender,subject,seen,date FROM email WHERE recipient LIKE ?";
    char *subject;
    char *from;
    char *to;
    char *body;
    time_t date;
    int seen;
    int msgid;
	int msgtoread;
	struct tm msg_date;
	
	sprintf(buffer, "%s/email.sq3", conf.bbs_path);
	
	rc = sqlite3_open(buffer, &db);
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        exit(1);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {    
        sqlite3_bind_text(res, 1, user->loginname, -1, 0);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(res);
		sqlite3_close(db);	
		s_putstring(socket, "\r\nYou have no email\r\n");	        
		return;
    }
    
    msgid = 0;
    
    while (sqlite3_step(res) == SQLITE_ROW) { 
		from = strdup((char *)sqlite3_column_text(res, 0));
		subject = strdup((char *)sqlite3_column_text(res, 1));
		seen = sqlite3_column_int(res, 2);
		date = (time_t)sqlite3_column_int(res, 3);
		localtime_r(&date, &msg_date);
		if (seen == 0) {
			sprintf(buffer, "\e[1;30m[\e[1;34m%4d\e[1;30m]\e[1;32m*\e[1;37m%-39.39s \e[1;32m%-16.16s \e[1;35m%02d:%02d %02d-%02d-%02d\e[0m\r\n", msgid, subject, from, msg_date.tm_hour, msg_date.tm_min, msg_date.tm_mday, msg_date.tm_mon + 1, msg_date.tm_year - 100);
		} else {
			sprintf(buffer, "\e[1;30m[\e[1;34m%4d\e[1;30m] \e[1;37m%-39.39s \e[1;32m%-16.16s \e[1;35m%02d:%02d %02d-%02d-%02d\e[0m\r\n", msgid, subject, from, msg_date.tm_hour, msg_date.tm_min, msg_date.tm_mday, msg_date.tm_mon + 1, msg_date.tm_year - 100);
		}
		s_putstring(socket, buffer);
		
		free(from);
		free(subject);
		
		if (msgid % 22 == 0 && msgid != 0) {
			s_putstring(socket, "Enter # to read, Q to quit or Enter to continue\r\n");

			s_readstring(socket, buffer, 5);
			if (strlen(buffer) > 0) {
				if (tolower(buffer[0]) == 'q') {
					sqlite3_finalize(res);
					sqlite3_close(db);
					return;
				} else {
					msgtoread = atoi(buffer);
					sqlite3_finalize(res);
					sqlite3_close(db);
					show_email(socket, user, msgtoread);
					return;
				}
			}
		
		}
		
		msgid++;
	}
	if (msgid == 0) {
		s_putstring(socket, "\r\nYou have no email\r\n");
	} else {
		s_putstring(socket, "Enter # to read, or Enter to quit\r\n");
		s_readstring(socket, buffer, 5);
		if (strlen(buffer) > 0) {
			msgtoread = atoi(buffer);
			sqlite3_finalize(res);
			sqlite3_close(db);
			show_email(socket, user, msgtoread);
			return;
		}
		
	}
	sqlite3_finalize(res);
	sqlite3_close(db);
}


int mail_getemailcount(struct user_record *user) {
	char *sql = "SELECT COUNT(*) FROM email WHERE recipient LIKE ?";
	int count;
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;	
		
	sprintf(buffer, "%s/email.sq3", conf.bbs_path);
	
	rc = sqlite3_open(buffer, &db);
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        exit(1);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {    
        sqlite3_bind_text(res, 1, user->loginname, -1, 0);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(res);
		sqlite3_close(db);		        
		return 0;
    }
    
	count = 0;

    if (sqlite3_step(res) == SQLITE_ROW) { 
		count = sqlite3_column_int(res, 0);
	}
	
	sqlite3_finalize(res);
	sqlite3_close(db);		        
	
	return count;
}
