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
        // wait(NULL);
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
Base Process ID: 12198, level: 0
Process ID:: 12199, Parent ID: 12198, level1
Process ID:: 12200, Parent ID: 12198, level1
Process ID:: 12201, Parent ID: 12198, level1
Process ID:: 12208, Parent ID: 12199, level2
Process ID:: 12204, Parent ID: 12199, level2
Process ID:: 12209, Parent ID: 12204, level3
Process ID:: 12203, Parent ID: 12200, level2
Process ID:: 12212, Parent ID: 12203, level3
Process ID:: 12210, Parent ID: 12204, level3
Process ID:: 12205, Parent ID: 12201, level2
Process ID:: 12211, Parent ID: 12209, level4
Process ID:: 12207, Parent ID: 12200, level2
Process ID:: 12202, Parent ID: 12198, level1
Process ID:: 12206, Parent ID: 12199, level2
Process ID:: 12213, Parent ID: 12206, level3
 */
