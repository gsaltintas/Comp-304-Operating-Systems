/*
Gül Sena Altıntaş, 64284
Hw 1, Problem 1.a
calls fork operation 4 times and prints the id and parent id for each process call
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int num_to_fork = 4;
struct Process
{
    // struct to hold details of a process
    long pid;
    int level;
};

struct Process fork_(struct Process p)
{
    struct Process res;
    int pid = fork();
    if (pid == 0)
    {
        printf("Process ID:: %ld,\tParent ID: %ld,\tlevel: %d\n", (long)getpid(), p.pid, p.level);
        res.pid = (long)getpid();
        res.level = (long)p.level + 1;
    }
    else
    {
        res = p;
    }
    return res;
}

int main(void)
{

    printf("Starting fork program\n");
    int process = 0;
    long parent_id = (long)getpid();
    struct Process parent = {parent_id, process};

    printf("Base Process ID: %ld, level: %d\n", parent.pid, parent.level);
    parent.level++;
    for (int level = 1; level <= num_to_fork; level++)
    {
        parent = fork_(parent);
    }
    sleep(1);
    return 0;
}

/* 
Result:
Starting fork program
Base Process ID: 27482, level: 0
Process ID:: 27483,	Parent ID: 27482,	level: 1
Process ID:: 27484,	Parent ID: 27482,	level: 1
Process ID:: 27485,	Parent ID: 27482,	level: 1
Process ID:: 27488,	Parent ID: 27483,	level: 2
Process ID:: 27492,	Parent ID: 27484,	level: 2
Process ID:: 27486,	Parent ID: 27483,	level: 2
Process ID:: 27489,	Parent ID: 27484,	level: 2
Process ID:: 27490,	Parent ID: 27485,	level: 2
Process ID:: 27491,	Parent ID: 27483,	level: 2
Process ID:: 27493,	Parent ID: 27488,	level: 3
Process ID:: 27487,	Parent ID: 27482,	level: 1
Process ID:: 27494,	Parent ID: 27486,	level: 3
Process ID:: 27496,	Parent ID: 27489,	level: 3
Process ID:: 27495,	Parent ID: 27486,	level: 3
Process ID:: 27497,	Parent ID: 27494,	level: 4

 */