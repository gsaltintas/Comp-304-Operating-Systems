
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
#include <string.h>

#define BUFFER_SIZE 25
#define READ_END 0
#define WRITE_END 1

int subtask()
{
    char write_msg[BUFFER_SIZE] = "Greetings";
    char read_msg[BUFFER_SIZE];
    pid_t pid;
    int fd[2];
    struct timeval current_time;

    /* create the pipe */
    if (pipe(fd) == -1)
    {
        fprintf(stderr, "Pipe failed");
        return 1;
    }

    gettimeofday(&current_time, NULL);
    /* now fork a child process */
    pid = fork();

    if (pid < 0)
    {
        fprintf(stderr, "Fork failed");
        return 1;
    }

    if (pid > 0)
    { /* parent process */
        /* close the unused end of the pipe */
        close(fd[READ_END]);

        wait(NULL);
        // this will block the child and the program won't terminate because
        //child reading from the pipe blocks it until something is written to the pipe.
        /* write to the pipe */
        gettimeofday(&current_time, NULL);
        sprintf(write_msg, "%ld", current_time.tv_sec);

        write(fd[WRITE_END], write_msg, strlen(write_msg) + 1);

        // wait(NULL);

        /* close the write end of the pipe */
        close(fd[WRITE_END]);
        sleep(3);
    }
    else
    { /* child process */
        /* close the unused end of the pipe */
        close(fd[WRITE_END]);

        /* read from the pipe */
        read(fd[READ_END], read_msg, BUFFER_SIZE);
        printf("For process with PId: %ld time read as %s\n", (long)getpid(), read_msg);

        /* close the write end of the pipe */
        close(fd[READ_END]);
        subtask();
    }
    return 0;
}

int main()
{
    subtask();

    // char write_msg[BUFFER_SIZE] = "Greetings";
    // char read_msg[BUFFER_SIZE];
    // pid_t pid;
    // int fd[2];

    // /* create the pipe */
    // if (pipe(fd) == -1)
    // {
    //     fprintf(stderr, "Pipe failed");
    //     return 1;
    // }

    // /* now fork a child process */
    // pid = fork();

    // if (pid < 0)
    // {
    //     fprintf(stderr, "Fork failed");
    //     return 1;
    // }

    // if (pid > 0)
    // { /* parent process */
    //     /* close the unused end of the pipe */
    //     close(fd[READ_END]);

    //     //wait(NULL); this will block the child and the program won't terminate because
    //     //child reading from the pipe blocks it until something is written to the pipe.
    //     /* write to the pipe */
    //     write(fd[WRITE_END], write_msg, strlen(write_msg) + 1);

    //     /* close the write end of the pipe */
    //     close(fd[WRITE_END]);
    // }
    // else
    // { /* child process */
    //     /* close the unused end of the pipe */
    //     close(fd[WRITE_END]);

    //     /* read from the pipe */
    //     read(fd[READ_END], read_msg, BUFFER_SIZE);
    //     printf("child read %s\n", read_msg);

    //     /* close the write end of the pipe */
    //     close(fd[READ_END]);
    // }

    return 0;
    // struct timeval current_time;
    // pid_t pid;
    // pid = fork();
    // if (pid == 0)
    // {
    //     // process B
    //     gettimeofday(&current_time, NULL);
    //     printf("For Process B, PId: %ld, Current time in seconds: %ld\n", (long)getpid(), current_time.tv_sec);
    //     sleep(3);
    //     pid = fork();

    //     if (pid == 0)
    //     {
    //         gettimeofday(&current_time, NULL);
    //         printf("For Process C, PId: %ld, Current time in seconds: %ld\n", (long)getpid(), current_time.tv_sec);
    //         sleep(3);
    //     }
    //     else
    //     {
    //         wait(NULL);
    //         gettimeofday(&current_time, NULL);
    //         printf("For Process B, PId: %ld, Current time in seconds: %ld\n", (long)getpid(), current_time.tv_sec);
    //         sleep(3);
    //         printf("%ld\n", (long)pid);
    //         kill(pid, SIGKILL);
    //     }
    // }
    // else
    // {
    //     // parent process A
    //     wait(NULL);
    //     gettimeofday(&current_time, NULL);
    //     printf("For Process A, PId: %ld, Current time in seconds: %ld\n", (long)getpid(), current_time.tv_sec);
    //     printf("%ld\n", (long)pid);
    //     kill(pid, SIGKILL);
    // }
    // return 0;
}