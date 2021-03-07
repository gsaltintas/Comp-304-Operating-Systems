/**
Gül Sena Altıntaş, 64284
Hw 1, Problem 2
Pipe example sending time time
Adapted from Example program demonstrating UNIX pipes given in BlackBoard
    @author Silberschatz, Galvin, and Gagne
    Operating System Concepts  - Ninth Edition
    Copyright John Wiley & Sons - 2013
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>

#define BUFFER_SIZE 25
#define READ_END 0
#define WRITE_END 1

int send_to_pipe(int fd[2], struct timeval current_time, char write_msg[BUFFER_SIZE])
{ /* parent process */
    /* close the unused end of the pipe */
    close(fd[READ_END]);

    gettimeofday(&current_time, NULL);
    // sprintf(write_msg, "%ld", current_time.tv_sec);
    strftime(write_msg, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", localtime(&current_time.tv_sec));

    //wait(NULL); this will block the child and the program won't terminate because
    //child reading from the pipe blocks it until something is written to the pipe.
    /* write to the pipe */
    write(fd[WRITE_END], write_msg, strlen(write_msg) + 1);

    /* close the write end of the pipe */
    close(fd[WRITE_END]);

    return 0;
}

void read_to_pipe(int fd[2], char read_msg[BUFFER_SIZE], char process)
{ /* child process */
    /* close the unused end of the pipe */
    close(fd[WRITE_END]);

    /* read from the pipe */
    read(fd[READ_END], read_msg, BUFFER_SIZE);
    printf("%c with PID %ld (parent id: %ld) read %s\n", process, (long)getpid(), (long)getppid(), read_msg);

    /* close the write end of the pipe */
    close(fd[READ_END]);
}

int fork_(int fd[2], int fd2[2], struct timeval current_time, char write_msg[BUFFER_SIZE], char read_msg[BUFFER_SIZE], char child)
{
    /* create the pipe */
    if (pipe(fd) == -1)
    {
        fprintf(stderr, "Pipe failed");
        return 1;
    }

    if (pipe(fd2) == -1)
    {
        fprintf(stderr, "Pipe failed");
        return 1;
    }
    /* now fork a child process */
    pid_t pid = fork();

    if (pid < 0)
    {
        fprintf(stderr, "Fork failed");
        return 1;
    }

    if (pid > 0)
    {
        send_to_pipe(fd, current_time, write_msg);
        if (child == 'B')
        {
            read_to_pipe(fd2, read_msg, 'A');
            kill(pid, SIGKILL);
        }
        else
        {
            read_to_pipe(fd2, read_msg, 'B');
            kill(pid, SIGKILL);
        }
    }
    else
    {
        // send_to_pipe(fd, current_time, write_msg);
        // B reads and prints the time
        read_to_pipe(fd, read_msg, child);
        if (child == 'B')
        {
            sleep(3);
            // B forks child process C
            int fd_new[2];
            int fd_new2[2];
            fork_(fd_new, fd_new2, current_time, write_msg, read_msg, 'C');
        }
        sleep(3);
        send_to_pipe(fd2, current_time, write_msg);
    }
    return 0;
}

int main(void)
{
    char write_msg[BUFFER_SIZE];
    char read_msg[BUFFER_SIZE];
    pid_t pid;
    int fd[2];
    int fd2[2];
    struct timeval current_time;

    fork_(fd, fd2, current_time, write_msg, read_msg, 'B');
    return 0;
}
