CC=cc
CFLAGS=-I/usr/local/include
DEPS = filepost.c

OBJ = filepost.o 
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

filepost: $(OBJ) 
	$(CC) -o filepost -o $@ $^ $(CFLAGS) -L/usr/local/lib -lsqlite3 
	
.PHONY: clean

clean:
	rm -f $(OBJ) filepost
