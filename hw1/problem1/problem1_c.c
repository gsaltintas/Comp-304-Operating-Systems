/*
Gül Sena Altıntaş, 64284
Hw 1, Problem 1.c

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
        // execlp("ps", "f", NULL);
    }
    else
    {
        printf("I am the parent process with id %ld\n", (long)getpid());
        sleep(5);
        printf("Child finished execution\n");
    }

    return 0;
}

/*
Result:

*/