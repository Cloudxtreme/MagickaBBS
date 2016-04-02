#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <ctype.h>
#include <errno.h>
#include "Xmodem/zmodem.h"
#include "bbs.h"

extern struct bbs_config conf;

static int doCancel = 0;

struct file_entry {
	char *filename;
	char *description;
	int size;
	int dlcount;
};

char **tagged_files;
int tagged_count = 0;

int ZXmitStr(u_char *str, int len, ZModem *info) {
	int i;
	
	for (i=0;i<len;i++) {
		if (str[i] == 255) {
			if (write(info->ofd, &str[i], 1) == 0) {
				return ZmErrSys;
			}
		}
		if (write(info->ofd, &str[i], 1) == 0) {
			return ZmErrSys;
		}
	}
	
	return 0;
}

void ZIFlush(ZModem *info) {
}

void ZOFlush(ZModem *info) {
}

int ZAttn(ZModem *info) {
	char *ptr ;

	if( info->attn == NULL )
		return 0 ;

	for(ptr = info->attn; *ptr != '\0'; ++ptr) {
		if( *ptr == ATTNBRK ) {
			
		} else if( *ptr == ATTNPSE ) {
			sleep(1);
		} else {
			write(info->ifd, ptr, 1) ;
		}
	}
	return 0;
}

void ZFlowControl(int onoff, ZModem *info) {
}

void ZStatus(int type, int value, char *status) {
}


char *upload_path;
char upload_filename[1024];

FILE *ZOpenFile(char *name, u_long crc, ZModem *info) {
	
	FILE *fptr;
	struct stat s;
	
	snprintf(upload_filename, 1023, "%s/%s", upload_path, basename(name));
	fprintf(stderr, "%s\n", upload_filename);
	if (stat(upload_filename, &s) == 0) {
		return NULL;
	}
	
	fptr = fopen(upload_filename, "wb");
	
	return fptr;
}

int	ZWriteFile(u_char *buffer, int len, FILE *file, ZModem *info) {
	return fwrite(buffer, 1, len, file) == len ? 0 : ZmErrSys;
}

int	ZCloseFile(ZModem *info) {
	fclose(info->file);
	return 0;
}

void ZIdleStr(u_char *buffer, int len, ZModem *info) {
}

int doIO(ZModem *zm) {
	fd_set readfds;
	struct timeval timeout;
	u_char buffer[2048];
	u_char buffer2[1024];
	int	len;
	int pos;
	int done = 0;
	int	i;
	int j;
	
	while(!done) {
		FD_ZERO(&readfds);
		FD_SET(zm->ifd, &readfds) ;
		timeout.tv_sec = zm->timeout ;
		timeout.tv_usec = 0 ;
		i = select(zm->ifd+1, &readfds,NULL,NULL, &timeout) ;
		
		if( i==0 ) {
			done = ZmodemTimeout(zm) ;
		} else if (i > 0) {
			len = read(zm->ifd, buffer, 2048);
			if (len == 0) {
				disconnect(zm->ifd);
			}
			
			pos = 0;
			for (j=0;j<len;j++) {
				if (buffer[j] == 255) {
					if (buffer[j+1] == 255) {
						buffer2[pos] = 255;
						pos++;
						j++;
					} else {
						j++;
						j++;
					}
				} else {
					buffer2[pos] = buffer[j];
					pos++;
				}
			}
			if (pos > 0) {
				done = ZmodemRcv(buffer2, pos, zm) ;
			}
		} else {
			// SIG INT catch
			if (errno != EINTR) {
				printf("SELECT ERROR %s\n", strerror(errno));
			}
		}
	}
	return done;
}

void upload_zmodem(int socket, struct user_record *user) {
	ZModem zm;
	int done;


	upload_path = conf.file_directories[user->cur_file_dir]->file_subs[user->cur_file_sub]->upload_path;
	
	zm.attn = NULL;
	zm.windowsize = 0;
	zm.bufsize = 0;
	
	zm.ifd = socket;
	zm.ofd = socket;
	
	zm.zrinitflags = 0;
	zm.zsinitflags = 0;
	
	zm.packetsize = 1024;
	
	done = ZmodemRInit(&zm);

	doIO(&zm);
}

void upload(int socket, struct user_record *user) {
	char buffer[331];
	char buffer2[66];
	char buffer3[256];
	int i;
	char *create_sql = "CREATE TABLE IF NOT EXISTS files ("
						"Id INTEGER PRIMARY KEY,"
						"filename TEXT,"
						"description TEXT,"
						"size INTEGER,"
						"dlcount INTEGER,"
						"approved INTEGER);";
	char *sql = "INSERT INTO files (filename, description, size, dlcount, approved) VALUES(?, ?, ?, 0, 0)";
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    struct stat s;
    char *err_msg = NULL;
    
	upload_zmodem(socket, user);
	
	s_putstring(socket, "\r\nPlease enter a description:\r\n");
	buffer[0] = '\0';
	for (i=0;i<5;i++) {
		sprintf(buffer2, "\r\n%d: ", i);
		s_putstring(socket, buffer2);
		s_readstring(socket, buffer2, 65);
		if (strlen(buffer2) == 0) {
			break;
		}
		strcat(buffer, buffer2);
		strcat(buffer, "\n");
	}
	
	sprintf(buffer3, "%s/%s.sq3", conf.bbs_path, conf.file_directories[user->cur_file_dir]->file_subs[user->cur_file_sub]->database);
	
	rc = sqlite3_open(buffer3, &db);
	
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
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {  
		stat(upload_filename, &s);
		  
        sqlite3_bind_text(res, 1, upload_filename, -1, 0);
        sqlite3_bind_text(res, 2, buffer, -1, 0);
        sqlite3_bind_int(res, 3, s.st_size);      
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(res);
		sqlite3_close(db);    
		return;    
    }
    
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_DONE) {
        printf("execution failed: %s", sqlite3_errmsg(db));
        sqlite3_finalize(res);
		sqlite3_close(db);    
		return;
    }
    sqlite3_finalize(res);
    sqlite3_close(db);
}

void download_zmodem(int socket, struct user_record *user, char *filename) {
	ZModem zm;
	int	done ;
	fd_set readfds;
	struct timeval timeout;
	int	i;
	int j;
	int	len;
	int pos;
	
	u_char buffer[2048];
	u_char buffer2[1024];
	
	printf("Attempting to upload %s\n", filename);
	
	zm.attn = NULL;
	zm.windowsize = 0;
	zm.bufsize = 0;
	
	zm.ifd = socket;
	zm.ofd = socket;
	
	zm.zrinitflags = 0;
	zm.zsinitflags = 0;
	
	zm.packetsize = 1024;
	
	
	ZmodemTInit(&zm) ;
	done = doIO(&zm);
	if ( done != ZmDone ) {
		return;
	}
	
	done = ZmodemTFile(filename, basename(filename), ZCBIN,0,0,0,0,0, &zm) ;
	
	switch( done ) {
	  case 0:	  	  
	    break ;

	  case ZmErrCantOpen:
	    fprintf(stderr, "cannot open file \"%s\": %s\n", filename, strerror(errno)) ;
	    return;

	  case ZmFileTooLong:
	    fprintf(stderr, "filename \"%s\" too long, skipping...\n", filename) ;
	    return;

	  case ZmDone:
	    return;

	  default:
	    return;
	}
	
	if (!done) {
		done = doIO(&zm);
	}
	
	if ( done != ZmDone ) {
		return;
	}	
	
	done = ZmodemTFinish(&zm);
	
	if (!done) {
		done = doIO(&zm);
	}
}

void download(int socket, struct user_record *user) {
	int i;
	char *ssql = "select dlcount from files where filename like ?";
	char *usql = "update files set dlcount=? where filename like ?";
	char buffer[256];
	int dloads;
	char *err_msg = NULL;
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    
	for (i=0;i<tagged_count;i++) {
		download_zmodem(socket, user, tagged_files[i]);
	
		sprintf(buffer, "%s/%s.sq3", conf.bbs_path, conf.file_directories[user->cur_file_dir]->file_subs[user->cur_file_sub]->database);
		
		rc = sqlite3_open(buffer, &db);
		
		if (rc != SQLITE_OK) {
			fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			exit(1);
		}
		rc = sqlite3_prepare_v2(db, ssql, -1, &res, 0);
		
		if (rc == SQLITE_OK) {    
			sqlite3_bind_text(res, 1, tagged_files[i], -1, 0);
		} else {
			fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
			
		rc = sqlite3_step(res);  
		
		if (rc != SQLITE_ROW) {
			fprintf(stderr, "Downloaded a file not in database!!!!!");
			sqlite3_finalize(res);
			sqlite3_close(db);
			exit(1);
		}
		
		dloads = sqlite3_column_int(res, 0);
		dloads++;
		sqlite3_finalize(res);
		
		rc = sqlite3_prepare_v2(db, usql, -1, &res, 0);
		
		if (rc == SQLITE_OK) {    
			sqlite3_bind_int(res, 1, dloads);
			sqlite3_bind_text(res, 2, tagged_files[i], -1, 0);
		} else {
			fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
			
		rc = sqlite3_step(res);  
		
		sqlite3_finalize(res);
		sqlite3_close(db);
	}
	
	for (i=0;i<tagged_count;i++) {
		free(tagged_files[i]);
	}
	free(tagged_files);
	tagged_count = 0;
}

void list_files(int socket, struct user_record *user) {
	char *sql = "select filename, description, size, dlcount from files where approved=1";
	char buffer[256];
	sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
	int files_c;
	int file_size;
	char file_unit;
	int lines = 0;
	char desc;
	int i;
	int j;
	int z;
	int k;
	int match;
	
	struct file_entry **files_e;
	
	sprintf(buffer, "%s/%s.sq3", conf.bbs_path, conf.file_directories[user->cur_file_dir]->file_subs[user->cur_file_sub]->database);

	rc = sqlite3_open(buffer, &db);
	if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        exit(1);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc != SQLITE_OK) {    
        sqlite3_finalize(res);
		sqlite3_close(db);
		s_putstring(socket, "\r\nNo files in this area!\r\n");	
		return;	        
    }
    
   
    files_c = 0;

	while (sqlite3_step(res) == SQLITE_ROW) {
		if (files_c == 0) {
			files_e = (struct file_entry **)malloc(sizeof(struct file_entry *));
		} else {
			files_e = (struct file_entry **)realloc(files_e, sizeof(struct file_entry *) * (files_c + 1));
		}
		files_e[files_c] = (struct file_entry *)malloc(sizeof(struct file_entry));
		files_e[files_c]->filename = strdup((char *)sqlite3_column_text(res, 0));
		files_e[files_c]->description = strdup((char *)sqlite3_column_text(res, 1));
		files_e[files_c]->size = sqlite3_column_int(res, 2);
		files_e[files_c]->dlcount = sqlite3_column_int(res, 3);
		
		files_c++;
	}
	sqlite3_finalize(res);
	sqlite3_close(db);	
	
	if (files_c == 0) {
		s_putstring(socket, "\r\nNo files in this area!\r\n");	
		return;	  
	}
	s_putstring(socket, "\r\n");
	for (i=0;i<files_c;i++) {
		file_size = files_e[i]->size;
		if (file_size > 1024 * 1024 * 1024) {
			file_size = file_size / 1024 / 1024 / 1024;
			file_unit = 'G';
		} else if (file_size > 1024 * 1024) {
			file_size = file_size / 1024 / 1024;
			file_unit = 'M';
		} else if (file_size > 1024) {
			file_size = file_size / 1024;
			file_unit = 'K';
		} else {
			file_unit = 'b';
		}
		sprintf(buffer, "\e[1;30m[\e[1;34m%3d\e[1;30m] \e[1;33m%3ddloads \e[1;36m%4d%c \e[1;37m%-58s\r\n     \e[0;32m", i, files_e[i]->dlcount, file_size, file_unit, basename(files_e[i]->filename));
		s_putstring(socket, buffer);
		lines++;
		for (j=0;j<strlen(files_e[i]->description);j++) {
			if (files_e[i]->description[j] == '\n') {
				s_putstring(socket, "\r\n");
				lines++;
				if (lines % 22 == 0 && lines != 0) {
					while (1) {
						s_putstring(socket, "\r\n\e[0mEnter # to tag, Q to quit, Enter to continue: ");
						s_readstring(socket, buffer, 5);
						if (strlen(buffer) == 0) {
							break;
						} else if (tolower(buffer[0]) == 'q') {
							for (z=0;z<files_c;z++) {
								free(files_e[z]->filename);
								free(files_e[z]->description);
								free(files_e[z]);
							}
							free(files_e);
							s_putstring(socket, "\r\n");
							return;
						}  else {
							z = atoi(buffer);
							if (z >= 0 && z < files_c) {
								if (conf.file_directories[user->cur_file_dir]->file_subs[user->cur_file_sub]->download_sec_level <= user->sec_level) {
									match = 0;
									for (k=0;k<tagged_count;k++) {
										if (strcmp(tagged_files[k], files_e[z]->filename) == 0) {
											match = 1;
											break;
										}
									}
									if (match == 0) {
										if (tagged_count == 0) {
											tagged_files = (char **)malloc(sizeof(char *));
										} else {
											tagged_files = (char **)realloc(tagged_files, sizeof(char *) * (tagged_count + 1));
										}
										tagged_files[tagged_count] = strdup(files_e[z]->filename);
										tagged_count++;
										sprintf(buffer, "\r\nTagged %s\r\n", basename(files_e[z]->filename));
										s_putstring(socket, buffer);
									} else {
										s_putstring(socket, "\r\nAlready Tagged\r\n");
									}
								} else {
									s_putstring(socket, "\r\nSorry, you don't have permission to download from this area\r\n");
								}
							}
						}
					}
				} else {
					s_putstring(socket, "     \e[0;32m");
				}
			} else {
				s_putchar(socket, files_e[i]->description[j]);
			}
		}	
	}
	while (1) {
		s_putstring(socket, "\r\n\e[0mEnter # to tag, Enter to quit: ");
		s_readstring(socket, buffer, 5);
		if (strlen(buffer) == 0) {
			for (z=0;z<files_c;z++) {
				free(files_e[z]->filename);
				free(files_e[z]->description);
				free(files_e[z]);
			}
			free(files_e);
			s_putstring(socket, "\r\n");
			return;
		} else {
			z = atoi(buffer);
			if (z >= 0 && z < files_c) {
				if (conf.file_directories[user->cur_file_dir]->file_subs[user->cur_file_sub]->download_sec_level <= user->sec_level) {
					match = 0;
					for (k=0;k<tagged_count;k++) {
						if (strcmp(tagged_files[k], files_e[z]->filename) == 0) {
							match = 1;
							break;
						}
					}
					if (match == 0) {
						if (tagged_count == 0) {
							tagged_files = (char **)malloc(sizeof(char *));
						} else {
							tagged_files = (char **)realloc(tagged_files, sizeof(char *) * (tagged_count + 1));
						}
						tagged_files[tagged_count] = strdup(files_e[z]->filename);
						tagged_count++;
						sprintf(buffer, "\r\nTagged %s\r\n", basename(files_e[z]->filename));
						s_putstring(socket, buffer);
					} else {
						s_putstring(socket, "\r\nAlready Tagged\r\n");
					}
				} else {
					s_putstring(socket, "\r\nSorry, you don't have permission to download from this area\r\n");
				}
			}
		} 
	}
}

int file_menu(int socket, struct user_record *user) {
	int doquit = 0;
	int dofiles = 0;
	char c;
	int i;
	int j;
	char prompt[256];
	
	while (!dofiles) {
		s_displayansi(socket, "filemenu");
		
		sprintf(prompt, "\e[0m\r\nDir: (%d) %s\r\nSub: (%d) %s\r\nTL: %dm :> ", user->cur_file_dir, conf.file_directories[user->cur_file_dir]->name, user->cur_file_sub, conf.file_directories[user->cur_file_dir]->file_subs[user->cur_file_sub]->name, user->timeleft);
		s_putstring(socket, prompt);
		
		c = s_getc(socket);
		switch(tolower(c)) {
			case 'i':
				{
					s_putstring(socket, "\r\n\r\nFile Directories:\r\n\r\n");
					for (i=0;i<conf.file_directory_count;i++) {
						if (conf.file_directories[i]->sec_level <= user->sec_level) {
							sprintf(prompt, "  %d. %s\r\n", i, conf.file_directories[i]->name);
							s_putstring(socket, prompt);
						}
						if (i != 0 && i % 22 == 0) {
							s_putstring(socket, "Press any key to continue...\r\n");
							c = s_getc(socket);
						}
					}
					s_putstring(socket, "Enter the directory number: ");
					s_readstring(socket, prompt, 5);
					if (tolower(prompt[0]) != 'q') {
						j = atoi(prompt);
						if (j < 0 || j >= conf.file_directory_count || conf.file_directories[j]->sec_level > user->sec_level) {
							s_putstring(socket, "\r\nInvalid directory number!\r\n");
						} else {
							s_putstring(socket, "\r\n");
							user->cur_file_dir = j;
							user->cur_file_sub = 0;
						}
					}
				}
				break;
			case 's':
				{
					s_putstring(socket, "\r\n\r\nFile Subdirectories:\r\n\r\n");
					for (i=0;i<conf.file_directories[user->cur_file_dir]->file_sub_count;i++) {
						sprintf(prompt, "  %d. %s\r\n", i, conf.file_directories[user->cur_file_dir]->file_subs[i]->name);
						s_putstring(socket, prompt);

						if (i != 0 && i % 22 == 0) {
							s_putstring(socket, "Press any key to continue...\r\n");
							c = s_getc(socket);
						}
					}
					s_putstring(socket, "Enter the sub directory number: ");
					s_readstring(socket, prompt, 5);
					if (tolower(prompt[0]) != 'q') {
						j = atoi(prompt);
						if (j < 0 || j >= conf.file_directories[user->cur_file_dir]->file_sub_count) {
							s_putstring(socket, "\r\nInvalid sub directiry number!\r\n");
						} else {
							s_putstring(socket, "\r\n");
							user->cur_file_sub = j;
						}
					}
				}
				break;			
			case 'l':
				list_files(socket, user);
				break;
			case 'u':
				{
					if (user->sec_level >= conf.file_directories[user->cur_file_dir]->file_subs[user->cur_file_sub]->upload_sec_level) {
						upload(socket, user);
					} else {
						s_putstring(socket, "Sorry, you don't have permission to upload in this Sub\r\n");
					}
				}
				break;
			case 'd':
				download(socket, user);
				break;
			case 'c':
				{
					// Clear tagged files	
					if (tagged_count > 0) {
						for (i=0;i<tagged_count;i++) {
							free(tagged_files[i]);
						}
						free(tagged_files);
						tagged_count = 0;
					}
				}
				break;
			case '}':
				{
					for (i=user->cur_file_dir;i<conf.file_directory_count;i++) {
						if (i + 1 == conf.file_directory_count) {
							i = -1;
						}
						if (conf.file_directories[i+1]->sec_level <= user->sec_level) {
							user->cur_file_dir = i + 1;
							user->cur_file_sub = 0;
							break;
						}
					}
				}
				break;
			case '{':
				{
					for (i=user->cur_file_dir;i>=0;i--) {
						if (i - 1 == -1) {
							i = conf.file_directory_count;
						}
						if (conf.file_directories[i-1]->sec_level <= user->sec_level) {
							user->cur_file_dir = i - 1;
							user->cur_file_sub = 0;
							break;
						}
					}
				}
				break;
			case ']':
				{
					i=user->cur_file_sub;
					if (i + 1 == conf.file_directories[user->cur_file_dir]->file_sub_count) {
						i = -1;
					}
					user->cur_mail_area = i + 1;
				}
				break;
			case '[':
				{
					i=user->cur_file_sub;
					if (i - 1 == -1) {
						i = conf.file_directories[user->cur_file_dir]->file_sub_count;
					}
					user->cur_mail_area = i - 1;
				}
				break;
			case 'q':
				dofiles = 1;
				break;
			case 'g':
				{
					s_putstring(socket, "\r\nAre you sure you want to log off? (Y/N)");
					c = s_getc(socket);
					if (tolower(c) == 'y') {
						dofiles = 1;
						doquit = 1;
					}
				}
				break;
		}
	}
	return doquit;
}
