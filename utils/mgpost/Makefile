CC=cc
CFLAGS=-I/usr/local/include
DEPS = mgpost.c
JAMLIB = ../../jamlib/jamlib.a
OBJ = mgpost.o 
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

mgpost: $(OBJ) 
	$(CC) -o mgpost -o $@ $^ $(CFLAGS) -L/usr/local/lib $(JAMLIB)
	
.PHONY: clean

clean:
	rm -f $(OBJ) mgpost
