# Adapted from MakeFile shared in the Discussion Forum by classmate Ahmet Akkoç
obj-m += problem3_a.o
obj-m += problem3_b.o

all: parta partb

parta: problem3_a.c
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

partb: problem3_b.c
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean