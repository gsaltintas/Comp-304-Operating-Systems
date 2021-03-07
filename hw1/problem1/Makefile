CC      = gcc
CFLAGS  = -g
all: parta partb partc

parta: problem1_a.c
	$(CC) problem1_a.c $(CFLAGS) -o problem1_a 

partb: problem1_b.c
	$(CC) problem1_b.c $(CFLAGS) -o problem1_b

partc: problem1_c.c
	$(CC) problem1_c.c $(CFLAGS) -o problem1_c

clean:
	rm -f problem1_a problem1_b problem1_c
