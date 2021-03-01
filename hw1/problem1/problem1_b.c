/*
Gül Sena Altıntaş, 64284
Hw 1, Problem 1.b
Overwrites the fork process in the child with exec command
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    long parent_id = (long)getpid();
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        printf("Process ID: %ld,\tParent ID: %ld\n", (long)getpid(), (long)getppid());
        // execlp allows us to execute linux commands in c code 
        execlp("ps", "f", NULL);
    }
    else
    {
        printf("I am the parent process with id %ld\n", (long)getpid());
        wait(NULL);
        printf("Child finished execution\n");
    }

    return 0;
}

/*
Result:
I am the parent process with id 14720
Process ID:: 14721,	Parent ID: 14720
  PID TTY          TIME CMD
 4062 pts/0    00:00:01 bash
14720 pts/0    00:00:00 problem1_b
14721 pts/0    00:00:00 ps
Child finished execution
*/