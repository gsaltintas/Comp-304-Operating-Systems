/*
Gül Sena Altıntaş, 64284
Hw 1, Problem 2
calls fork operation 4 times and prints the id and parent id for each process call
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

void subtask(){
    
}

int main()
{
    struct timeval current_time;
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        // process B
        gettimeofday(&current_time, NULL);
        printf("For Process B, PId: %ld, Current time in seconds: %ld\n", (long)getpid(), current_time.tv_sec);
        sleep(3);
        pid = fork();

        if (pid == 0)
        {
            gettimeofday(&current_time, NULL);
            printf("For Process C, PId: %ld, Current time in seconds: %ld\n", (long)getpid(), current_time.tv_sec);
            sleep(3);
        }
        else
        {
            wait(NULL);
            gettimeofday(&current_time, NULL);
            printf("For Process B, PId: %ld, Current time in seconds: %ld\n", (long)getpid(), current_time.tv_sec);
            sleep(3);
            printf("%ld\n", (long)pid);
            kill(pid, SIGKILL);
        }
    }
    else
    {
        // parent process A
        wait(NULL);
        gettimeofday(&current_time, NULL);
        printf("For Process A, PId: %ld, Current time in seconds: %ld\n", (long)getpid(), current_time.tv_sec);
        printf("%ld\n", (long)pid);
        kill(pid, SIGKILL);
    }
    return 0;
}