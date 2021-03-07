#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/slab.h>

int PERSONS_NO = 5;

struct birthday
{
	int day;
	int month;
	int year;
	struct list_head list;
};

/* This macro defines and initializes the variable birthday list, which is of type
struct list head */
static LIST_HEAD(birthday_list);

int birthday_init(void)
{
	printk(KERN_INFO "Loading Module\n");

	struct birthday *person;
	int dates[][3] = {{1, 1, 1990}, {15, 5, 1998}, {26, 3, 1998}, {17, 8, 1998}, {23, 12, 1997}};
	int i;
	for (i = 0; i < PERSONS_NO; i++)
	{
		// kernel equivalent of malloc()
		// GFP_KERNEL: indicates routine kernel memory allocation
		person = kmalloc(sizeof(*person), GFP_KERNEL);
		person->day = dates[i][0];
		person->month = dates[i][1];
		person->year = dates[i][2];
		if (i == 0)
			INIT_LIST_HEAD(&person->list);
		list_add_tail(&person->list, &birthday_list);
	}

	struct birthday *ptr;

	list_for_each_entry(ptr, &birthday_list, list)
	{
		printk("%d-%d-%d\n", ptr->day, ptr->month, ptr->year);
	}
	return 0;
}

void birthday_exit(void)
{

	printk(KERN_INFO "Removing Module\n");
	struct birthday *ptr, *next;
	list_for_each_entry_safe(ptr, next, &birthday_list, list)
	{
		list_del(&ptr->list);
		kfree(ptr);
	}
	printk(KERN_INFO "Completed kfree, module removed\n");
}

module_init(birthday_init);
module_exit(birthday_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Exercise for COMP304");
MODULE_AUTHOR("Gül Sena Altıntaş");
