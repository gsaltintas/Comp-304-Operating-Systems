CC		= gcc
CFLAGS	= 

all: clean part1 part2
EXEC1	= virtmem.c
EXEC2	= virtmem2.c
OBJ1	= part1
OBJ2	= part2

part1:
	$(CC) $(EXEC1) $(CFLAGS) -o $(OBJ1)
part2:
	$(CC) $(EXEC2) $(CFLAGS) -o $(OBJ2)

run:
	if [ ! -f $(OBJ1) || ! -f $(OBJ2) ]; then  echo "Object file doesn't exist, compiling...\n\n"; make all; fi
	./$(OBJ1) BACKING_STORE.bin address.txt

clean:
	rm -f $(OBJ1)
	rm -f $(OBJ2)