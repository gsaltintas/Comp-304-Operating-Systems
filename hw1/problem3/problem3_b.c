#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/pid.h>

static int pid;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Process id of a process");

int kernel_init(void)
{
    printk(KERN_INFO "Started executing kernel module");
    // check if pid is valid
    struct pid p = *find_get_pid(pid);
    // int
    // if (pid_nr(&p) >= 0)
    if (pid_nr(get_task_pid(p, PIDTYPE_PID)) >= 0)
    {
        struct task_struct *task = *get_pid_task(&p, PIDTYPE_PID);
        // printk(KERN_INFO "Process with PID: %d, Parent ID %d, Executable name: %s, Siblings:", pid, &task->real_parent->pid, &task->nameidata);
    }
    else
    {
        printk(KERN_INFO "Provided pid %d doesn't exit\n", pid);
    }
    return 0;
}

void kernel_exit(void)
{
    printk(KERN_INFO "Removing Module\n");
}

module_init(kernel_init);
module_exit(kernel_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("COMP304 - Hw 1 Problem 3.b");
MODULE_AUTHOR("Gül Sena Altıntaş");
