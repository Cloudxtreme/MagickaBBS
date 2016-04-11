#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
	int i;
	char *create_sql = "CREATE TABLE IF NOT EXISTS files ("
						"Id INTEGER PRIMARY KEY,"
						"filename TEXT,"
						"description TEXT,"
						"size INTEGER,"
						"dlcount INTEGER,"
						"approved INTEGER);";
	char *sql = "INSERT INTO files (filename, description, size, dlcount, approved) VALUES(?, ?, ?, 0, 1)";
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    struct stat s;
    char *err_msg = NULL;
    FILE *fptr;
	char *body;
	int totlen;
	int len;
	char buffer[256];
	
    if (argc < 4) {
		printf("Usage:\n");
		printf("%s filename desc_file database\n", argv[0]);
		exit(1);
	}
    
    fptr = fopen(argv[2], "r");
    if (!fptr) {
		body = strdup("No description.");
	} else {
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
	}
	
    
    rc = sqlite3_open(argv[3], &db);
    
    if (rc != SQLITE_OK) {
		printf("Cannot open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    rc = sqlite3_exec(db, create_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) { 
        printf("SQL error: %s\n", err_msg);      
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        exit(1);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {  
		stat(argv[1], &s);
		  
        sqlite3_bind_text(res, 1, argv[1], -1, 0);
        sqlite3_bind_text(res, 2, body, -1, 0);
        sqlite3_bind_int(res, 3, s.st_size);      
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(res);
		sqlite3_close(db);    
		exit(1);    
    }
    
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_DONE) {
        printf("execution failed: %s", sqlite3_errmsg(db));
        sqlite3_finalize(res);
		sqlite3_close(db);    
		exit(1);
    }
    sqlite3_finalize(res);
    sqlite3_close(db);
     return 0;
}
