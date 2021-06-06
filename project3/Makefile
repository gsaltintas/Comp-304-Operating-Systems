CC		= gcc
CFLAGS	= 

all: project3
EXEC	= virtmem.c
OBJ		= project3

project3:
	$(CC) $(EXEC) $(CFLAGS) -o $(OBJ)

run:
	if [ ! -f $(OBJ) ]; then  echo "Object file doesn't exist, compiling...\n\n"; make project3; fi
	# ./$(OBJ) BACKING_STORE.bin address.txt

clean:
	rm -f $(OBJ)