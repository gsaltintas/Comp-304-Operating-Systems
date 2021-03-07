/*
Gül Sena Altıntaş, 64284
Hw 1, Problem 1.c
Zombie process
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    // long parent_id = (long)getpid();
    pid_t pid;
    pid = fork();
    if (pid == 0)

    {
        printf("Process ID: %ld,\tParent ID: %ld\n", (long)getpid(), (long)getppid());
        // exit(0);
    }
    else
    {
        printf("I am the parent process with id %ld\n", (long)getpid());
        sleep(5);
        wait(NULL);
    }

    return 0;
}

/*
Result:
ps -l
F S   UID   PID  PPID  C PRI  NI ADDR SZ WCHAN  TTY          TIME CMD
0 S  1000  3662  3622  0  80   0 -  6235 wait   pts/1    00:00:00 bash
0 S  1000  7406  3662  0  80   0 -  1127 hrtime pts/1    00:00:00 problem1_c
1 Z  1000  7407  7406  0  80   0 -     0 -      pts/1    00:00:00 problem1_c <defunct>
4 R  1000  7409  3662  0  80   0 -  7582 -      pts/1    00:00:00 ps
*/