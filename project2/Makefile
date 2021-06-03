CC		= gcc
CFLAGS	= -g -lpthread

all: project2
EXEC	= src/project2_code.c
OBJ		= project2

project2:
	$(CC) $(EXEC) $(CFLAGS) -o $(OBJ)

run:
	if [ ! -f $(OBJ) ]; then  echo "Object file doesn't exist, compiling...\n\n"; make project2; fi
	./$(OBJ) -n 4 -p 0.75 -q 5 -t 3 -b 0.05

clean:
	rm -f $(OBJ)