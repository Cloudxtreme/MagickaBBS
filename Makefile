CC=cc
CFLAGS=-I/usr/local/include
DEPS = bbs.h
JAMLIB = jamlib/jamlib.a

OBJ = inih/ini.o bbs.o main.o users.o main_menu.o mail_menu.o doors.o bbs_list.o
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

magicka: $(OBJ) 
	$(CC) -o magicka -o $@ $^ $(CFLAGS) -L/usr/local/lib -lsqlite3 $(JAMLIB)
	
.PHONY: clean

clean:
	rm -f $(OBJ) magicka 
