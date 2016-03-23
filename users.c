#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <ctype.h>
#include "bbs.h"
#include "inih/ini.h"

extern struct bbs_config conf;

static int secLevel(void* user, const char* section, const char* name,
                   const char* value)
{
	struct sec_level_t *conf = (struct sec_level_t *)user;
	
	if (strcasecmp(section, "main") == 0) {
		if (strcasecmp(name, "time per day") == 0) {
			conf->timeperday = atoi(value);
		} 
	} 	
	return 1;
}

int save_user(struct user_record *user) {
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
	int rc;
 						
	char *update_sql = "UPDATE users SET password=?, firstname=?,"
					   "lastname=?, email=?, location=?, sec_level=?, last_on=?, time_left=?, cur_mail_conf=?, cur_mail_area=?, cur_file_dir=?, cur_file_sub=? where loginname LIKE ?";
    char *err_msg = 0;
     
 	sprintf(buffer, "%s/users.sq3", conf.bbs_path);
	
	rc = sqlite3_open(buffer, &db);
	
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        exit(1);
    }

    rc = sqlite3_prepare_v2(db, update_sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {    
        sqlite3_bind_text(res, 1, user->password, -1, 0);
        sqlite3_bind_text(res, 2, user->firstname, -1, 0);
        sqlite3_bind_text(res, 3, user->lastname, -1, 0);
        sqlite3_bind_text(res, 4, user->email, -1, 0);
        sqlite3_bind_text(res, 5, user->location, -1, 0);
        sqlite3_bind_int(res, 6, user->sec_level);
        sqlite3_bind_int(res, 7, user->laston);
        sqlite3_bind_int(res, 8, user->timeleft);
        sqlite3_bind_int(res, 9, user->cur_mail_conf);
        sqlite3_bind_int(res, 10, user->cur_mail_area);
        sqlite3_bind_int(res, 11, user->cur_file_dir);
        sqlite3_bind_int(res, 12, user->cur_file_sub);
        sqlite3_bind_text(res, 13, user->loginname, -1, 0);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }
    
    
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_DONE) {
        
        printf("execution failed: %s", sqlite3_errmsg(db));
		sqlite3_close(db);    
		exit(1);
    }
	sqlite3_close(db);
	return 1;

}

int inst_user(struct user_record *user) {
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
	int rc;
    char *create_sql = "CREATE TABLE IF NOT EXISTS users ("
						"Id INTEGER PRIMARY KEY,"
						"loginname TEXT,"
						"password TEXT,"
						"firstname TEXT,"
						"lastname TEXT,"
						"email TEXT,"
						"location TEXT,"
						"sec_level INTEGER,"
						"last_on INTEGER,"
						"time_left INTEGER,"
						"cur_mail_conf INTEGER,"
						"cur_mail_area INTEGER,"
						"cur_file_sub INTEGER,"
						"cur_file_dir INTEGER);";
						
	char *insert_sql = "INSERT INTO users (loginname, password, firstname,"
					   "lastname, email, location, sec_level, last_on, time_left, cur_mail_conf, cur_mail_area, cur_file_dir, cur_file_sub) VALUES(?,?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    char *err_msg = 0;
     
 	sprintf(buffer, "%s/users.sq3", conf.bbs_path);
	
	rc = sqlite3_open(buffer, &db);
	
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        exit(1);
    }
    
    rc = sqlite3_exec(db, create_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return 1;
    } 
    
    rc = sqlite3_prepare_v2(db, insert_sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {    
        sqlite3_bind_text(res, 1, user->loginname, -1, 0);
        sqlite3_bind_text(res, 2, user->password, -1, 0);
        sqlite3_bind_text(res, 3, user->firstname, -1, 0);
        sqlite3_bind_text(res, 4, user->lastname, -1, 0);
        sqlite3_bind_text(res, 5, user->email, -1, 0);
        sqlite3_bind_text(res, 6, user->location, -1, 0);
        sqlite3_bind_int(res, 7, user->sec_level);
        sqlite3_bind_int(res, 8, user->laston);
        sqlite3_bind_int(res, 9, user->timeleft);
        sqlite3_bind_int(res, 10, user->cur_mail_conf);
        sqlite3_bind_int(res, 11, user->cur_mail_area);
        sqlite3_bind_int(res, 12, user->cur_file_dir);
        sqlite3_bind_int(res, 13, user->cur_file_sub);
        
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }
    
    
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_DONE) {
        
        printf("execution failed: %s", sqlite3_errmsg(db));
		sqlite3_close(db);    
		exit(1);
    }
    
    user->id = sqlite3_last_insert_rowid(db);
    
	sqlite3_close(db);
	return 1;
}

struct user_record *check_user_pass(int socket, char *loginname, char *password) {
	struct user_record *user;
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    char *sql = "SELECT * FROM users WHERE loginname LIKE ?";
    
	sprintf(buffer, "%s/users.sq3", conf.bbs_path);
	
	rc = sqlite3_open(buffer, &db);
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        exit(1);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {    
        sqlite3_bind_text(res, 1, loginname, -1, 0);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(res);
		sqlite3_close(db);	
		return NULL;	        
    }
    
    int step = sqlite3_step(res);
    
    if (step == SQLITE_ROW) { 
		user = (struct user_record *)malloc(sizeof(struct user_record));
		user->id = sqlite3_column_int(res, 0);
		user->loginname = strdup((char *)sqlite3_column_text(res, 1));
		user->password = strdup((char *)sqlite3_column_text(res, 2));
		user->firstname = strdup((char *)sqlite3_column_text(res, 3));
		user->lastname = strdup((char *)sqlite3_column_text(res, 4));
		user->email = strdup((char *)sqlite3_column_text(res, 5));
		user->location = strdup((char *)sqlite3_column_text(res, 6));
		user->sec_level = sqlite3_column_int(res, 7);
		user->laston = (time_t)sqlite3_column_int(res, 8);
		user->timeleft = sqlite3_column_int(res, 9);
		user->cur_mail_conf = sqlite3_column_int(res, 10);
		user->cur_mail_area = sqlite3_column_int(res, 11);
		user->cur_file_dir = sqlite3_column_int(res, 12);
		user->cur_file_sub = sqlite3_column_int(res, 13);
		
		if (strcmp(password, user->password) != 0) {
			free(user);
		    sqlite3_finalize(res);
			sqlite3_close(db);	
			return NULL;
		}
    } else {
		sqlite3_finalize(res);
		sqlite3_close(db);	
		return NULL;		
	} 
  
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    user->sec_info = (struct sec_level_t *)malloc(sizeof(struct sec_level_t));
    
    sprintf(buffer, "%s/s%d.ini", conf.bbs_path, user->sec_level);
    if (ini_parse(buffer, secLevel, user->sec_info) <0) {
		printf("Unable to load sec Level ini (%s)!\n", buffer);
		exit(-1);
	}	
	
    return user;		   
}

int check_user(char *loginname) {
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    char *sql = "SELECT * FROM users WHERE loginname = ?";
    
	sprintf(buffer, "%s/users.sq3", conf.bbs_path);
	
	rc = sqlite3_open(buffer, &db);
	
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        exit(1);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {    
        sqlite3_bind_text(res, 1, loginname, -1, 0);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }
    
    int step = sqlite3_step(res);
    
    if (step == SQLITE_ROW) {
		sqlite3_finalize(res);
		sqlite3_close(db);
		return 0;
    }   
    
    sqlite3_finalize(res);
    sqlite3_close(db);
    return 1;
}

struct user_record *new_user(int socket) {
	char buffer[256];
	struct user_record *user;
	int done = 0;
	char c;
	int nameok = 0;
	int passok = 0;
	
	user = (struct user_record *)malloc(sizeof(struct user_record));
	s_putstring(socket, "\r\n\r\n");
	s_displayansi(socket, "newuser");
	
	do {
		passok = 0;
		nameok = 0;
		do {
			s_putstring(socket, "\r\nWhat is your login name: ");
			s_readstring(socket, buffer, 16);
			s_putstring(socket, "\r\n");
			if (strlen(buffer) < 3) {
				s_putstring(socket, "Sorry, that name is too short.\r\n");
				continue;
			}
			
			if (strchr(buffer, '%') != NULL) {
				s_putstring(socket, "Sorry, invalid character.\r\n");
				continue;				
			}
		
			if (strcasecmp(buffer, "unknown") == 0) {
				s_putstring(socket, "Sorry, that name is reserved.\r\n");
				continue;				
			}
		
			user->loginname = strdup(buffer);
			nameok = check_user(user->loginname);
			if (!nameok) {
				s_putstring(socket, "Sorry, that name is in use.\r\n");
				free(user->loginname);
				memset(buffer, 0, 256);
			}
		} while (!nameok);
		s_putstring(socket, "What is your first name: ");
		memset(buffer, 0, 256);
		s_readstring(socket, buffer, 32);
		s_putstring(socket, "\r\n");
		user->firstname = strdup(buffer);
		
		s_putstring(socket, "What is your last name: ");
		memset(buffer, 0, 256);
		s_readstring(socket, buffer, 32);
		s_putstring(socket, "\r\n");
		user->lastname = strdup(buffer);

		s_putstring(socket, "What is your e-mail address: ");
		memset(buffer, 0, 256);
		s_readstring(socket, buffer, 64);
		s_putstring(socket, "\r\n");
		user->email = strdup(buffer);

		s_putstring(socket, "Where are you located: ");
		memset(buffer, 0, 256);
		s_readstring(socket, buffer, 32);
		s_putstring(socket, "\r\n");
		user->location = strdup(buffer);	

		do {
			s_putstring(socket, "What password would you like (at least 8 characters): ");
			memset(buffer, 0, 256);
			s_readstring(socket, buffer, 16);
			s_putstring(socket, "\r\n");
			if (strlen(buffer) >= 8) {
				passok = 1;
			} else {
				s_putstring(socket, "Password too short!\r\n");
			}
		} while (!passok);
		user->password = strdup(buffer);
		
		s_putstring(socket, "You Entered:\r\n");
		s_putstring(socket, "-------------------------------------\r\n");
		s_putstring(socket, "Login Name: ");
		s_putstring(socket, user->loginname);
		s_putstring(socket, "\r\nFirst Name: ");
		s_putstring(socket, user->firstname);
		s_putstring(socket, "\r\nLast Name: ");
		s_putstring(socket, user->lastname);
		s_putstring(socket, "\r\nE-mail: ");
		s_putstring(socket, user->email);
		s_putstring(socket, "\r\nLocation: ");
		s_putstring(socket, user->location);
		s_putstring(socket, "\r\nPassword: ");
		s_putstring(socket, user->password);
		s_putstring(socket, "\r\n-------------------------------------\r\n");
		s_putstring(socket, "Is this Correct? (Y/N)");
		c = s_getchar(socket);
		while (tolower(c) != 'y' && tolower(c) != 'n') {
			c = s_getchar(socket);
		}
		
		if (tolower(c) == 'y') {
			done = 1;
		}
	} while (!done);
	user->sec_level = conf.newuserlvl;

	user->sec_info = (struct sec_level_t *)malloc(sizeof(struct sec_level_t));
	
	sprintf(buffer, "%s/s%d.ini", conf.bbs_path, user->sec_level);
	
	if (ini_parse(buffer, secLevel, user->sec_info) <0) {
		printf("Unable to load sec Level ini (%s)!\n", buffer);
		exit(-1);
	}	

	user->laston = time(NULL);
	user->timeleft = user->sec_info->timeperday;
	user->cur_file_dir = 0;
	user->cur_file_sub = 0;
	user->cur_mail_area = 0;
	user->cur_mail_conf = 0;
	inst_user(user);
	
	return user;
}
