#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <ctype.h>
#include "bbs.h"

extern struct bbs_config conf;
extern int mynode;

void add_bbs(int socket, struct user_record *user) {
	char *create_sql = "CREATE TABLE IF NOT EXISTS bbslist ("
						"id INTEGER PRIMARY KEY,"
						"bbsname TEXT,"
						"sysop TEXT,"
						"telnet TEXT,"
						"owner INTEGER);";
						
	char *insert_sql = "INSERT INTO bbslist (bbsname, sysop, telnet, owner) VALUES(?,?, ?, ?)";

	char bbsname[19];
	char sysop[17];
	char telnet[39];
	char buffer[256];
	char c;
	char *err_msg = 0;	
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc; 
    
	s_putstring(socket, "\r\n\e[1;37mEnter the BBS Name: \e[0m");
	s_readstring(socket, bbsname, 18);
	
	s_putstring(socket, "\r\n\e[1;37mEnter the Sysop's Name: \e[0m");
	s_readstring(socket, sysop, 16);
	
	s_putstring(socket, "\r\n\e[1;37mEnter the Telnet URL: \e[0m");
	s_readstring(socket, telnet, 38);
	
	s_putstring(socket, "\r\nYou entered:\r\n");
	s_putstring(socket, "\e[1;30m----------------------------------------------\e[0m\r\n");
	sprintf(buffer, "\e[1;37mBBS Name: \e[1;32m%s\r\n", bbsname);
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;37mSysop: \e[1;32m%s\r\n", sysop);
	s_putstring(socket, buffer);
	sprintf(buffer, "\e[1;37mTelnet URL: \e[1;32m%s\r\n", telnet);
	s_putstring(socket, buffer);
	s_putstring(socket, "\e[1;30m----------------------------------------------\e[0m\r\n");
	s_putstring(socket, "Is this correct? (Y/N) :");
	
	c = s_getc(socket);
	if (tolower(c) == 'y') {
		sprintf(buffer, "%s/bbslist.sq3", conf.bbs_path);
		
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
			
			return;
		} 
		
		rc = sqlite3_prepare_v2(db, insert_sql, -1, &res, 0);
		
		if (rc == SQLITE_OK) {    
			sqlite3_bind_text(res, 1, bbsname, -1, 0);
			sqlite3_bind_text(res, 2, sysop, -1, 0);
			sqlite3_bind_text(res, 3, telnet, -1, 0);
			sqlite3_bind_int(res, 4, user->id);

			
		} else {
			fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
		
		
		rc = sqlite3_step(res);
		
		if (rc != SQLITE_DONE) {
			
			printf("execution failed: %s", sqlite3_errmsg(db));
			sqlite3_close(db);    
			exit(1);
		}
	    sqlite3_finalize(res);   
		sqlite3_close(db); 
		s_putstring(socket, "\r\n\e[1;32mAdded!\e[0m\r\n");
	} else {
		s_putstring(socket, "\r\n\e[1;31mAborted!\e[0m\r\n");
	}
}

void delete_bbs(int socket, struct user_record *user) {
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    char *sql = "SELECT bbsname FROM bbslist WHERE id=? and owner=?";
    char *dsql = "DELETE FROM bbslist WHERE id=?";
    int i;
    char c;
    
    s_putstring(socket, "\r\nPlease enter the id of the BBS you want to delete: ");
    s_readstring(socket, buffer, 5);
    i = atoi(buffer);
    
    sprintf(buffer, "%s/bbslist.sq3", conf.bbs_path);
    
    rc = sqlite3_open(buffer, &db);
	if (rc != SQLITE_OK) {
		return;
	}
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(res, 1, i);
		sqlite3_bind_int(res, 2, user->id);
	} else {
        sqlite3_close(db);
        s_putstring(socket, "\r\nThere are no BBSes in the list yet!\r\n");
        return;
    }
    if (sqlite3_step(res) == SQLITE_ROW) {
		sprintf(buffer, "\r\nAre you sure you want to delete %s?\r\n", sqlite3_column_text(res, 0));
		s_putstring(socket, buffer);
		sqlite3_finalize(res);
		c = s_getc(socket);
		if (tolower(c) == 'y') {
			rc = sqlite3_prepare_v2(db, dsql, -1, &res, 0);
			if (rc == SQLITE_OK) {
				sqlite3_bind_int(res, 1, i);
			} else {
				sqlite3_close(db);
				s_putstring(socket, "\r\nThere are no BBSes in the list yet!\r\n");
				return;
			}
			sqlite3_step(res);
			s_putstring(socket, "\r\n\e[1;32mDeleted!\e[0m\r\n");
			sqlite3_finalize(res);
		} else {
			s_putstring(socket, "\r\n\e[1;32mAborted!\e[0m\r\n");
		}
	} else {
		sqlite3_finalize(res);
		s_putstring(socket, "\r\nThat BBS entry either doesn't exist or wasn't created by you.\r\n");
	}
	sqlite3_close(db);
}

void list_bbses(int socket) {
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    char *sql = "SELECT id,bbsname,sysop,telnet FROM bbslist";
    int i;
    
    sprintf(buffer, "%s/bbslist.sq3", conf.bbs_path);
    
    rc = sqlite3_open(buffer, &db);
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
	if (rc != SQLITE_OK) {
        sqlite3_close(db);
        s_putstring(socket, "\r\nThere are no BBSes in the list yet!\r\n");
        return;
    }
    i = 0;
    s_putstring(socket, "\r\n\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");

    while (sqlite3_step(res) == SQLITE_ROW) {
		sprintf(buffer, "\e[1;30m[\e[1;34m%3d\e[1;30m] \e[1;37m%-18s \e[1;33m%-16s \e[1;32m%-37s\e[0m\r\n", sqlite3_column_int(res, 0), sqlite3_column_text(res, 1), sqlite3_column_text(res, 2), sqlite3_column_text(res, 3));
		s_putstring(socket, buffer);
		i++;
		if (i == 20) {
			sprintf(buffer, "Press any key to continue...\r\n");
			s_putstring(socket, buffer);
			s_getc(socket);	
			i = 0;
		}		
	}
	
	s_putstring(socket, "\e[1;30m-------------------------------------------------------------------------------\e[0m\r\n");
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    sprintf(buffer, "Press any key to continue...\r\n");
	s_putstring(socket, buffer);
	s_getc(socket);	
}

void bbs_list(int socket, struct user_record *user) {
	int doquit = 0;
	char c;
	
	while(!doquit) {
		s_putstring(socket, "\r\n\e[1;32mBBS Listings: \e[1;37m(\e[1;33mL\e[1;37m) \e[1;32mList, \e[1;37m(\e[1;33mA\e[1;37m) \e[1;32mAdd \e[1;37m(\e[1;33mD\e[1;37m) \e[1;32mDelete \e[1;37m(\e[1;33mQ\e[1;37m) \e[1;32mQuit\e[0m\r\n");
		
		c = s_getc(socket);
		
		switch(tolower(c)) {
			case 'l':
				list_bbses(socket);
				break;
			case 'a':
				add_bbs(socket, user);
				break;
			case 'd':
				delete_bbs(socket, user);
				break;
			case 'q':
				doquit = 1;
				break;
		}
	}
}
