// #include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
// #include <linux/slab.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int PERSONS_NO = 5;

struct birthday
{
    int day;
    int month;
    int year;
    struct list_head list;
};

static LIST_HEAD(birthday_list);

int main(void)
{
    struct birthday *person;
    const char *names[PERSONS_NO];
    names = {"John",
             "Juliette",
             "Pierre",
             "Max",
             "Lauren"};
    int[] dates = {{1, 1, 1990}, {15, 5, 1998}, {26, 3, 1998}, {17, 8, 1998}, {23, 12, 1997}};
    for (int i = 0; i < PERSONS_NO; i++)
    {
        // kernel equivalent of malloc()
        // GFP_KERNEL: indicates routine kernel memory allocation
        person = malloc(sizeof(*person), GFP_KERNEL);
        person->day = dates[i][0];
        person->month = dates[i][1];
        person->month = dates[i][2];
        if (i = 0)
            INIT_LIST_HEAD(&person->list);
        list_add_tail(&person->list, &birthday_list);
        printf("%d\n",person->day);
    }

    return 0;
}