/**
Gül Sena Altıntaş, 64284
Hw 1, Problem 3.b
Kernel module
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int mypid;
module_param(mypid, int, 0);
MODULE_PARM_DESC(mypid, "Process id of a process");

int kernel_init(void)
{
    printk(KERN_INFO "Started executing kernel module");
    // check if pid is valid
    struct pid *p;
    struct task_struct *task;
    p = find_get_pid(mypid);
    task = pid_task(p, PIDTYPE_PID);

    if (task != NULL)
    {
        printk(KERN_INFO "Process with PID: %d, Parent ID %d, Executable name: %s, Siblings:\n", mypid, task->real_parent->pid, task->comm);
        // iterate over siblings
        struct task_struct *ptr;
        list_for_each_entry(ptr, &task->sibling, sibling)
        {
            printk(KERN_INFO "\t\tSibling with PID: %d, Executable name: %s\n", ptr->pid, ptr->comm);
        }
    }
    else
    {
        printk(KERN_INFO "Provided pid %d doesn't exist.\n", mypid);
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
