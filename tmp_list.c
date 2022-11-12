#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ntci_cz");

#define N 10   //Á´±í½Úµã


struct numlist {
  int num;//Êý¾Ý
  struct list_head list;//Ö¸ÏòË«Áª±íÇ°ºó½ÚµãµÄÖ¸Õë
};

struct list_head listhead;//Í·½Úµã

static int __init doublelist_init(void)
{
    struct numlist *listnode;//Ã¿´ÎÉêÇëÁ´±í½ÚµãÊ±ËùÓÃµÄÖ¸Õë
    struct list_head *pos;
    struct numlist *p;
    int i;

    printk("doublelist is starting...\n");
    //³õÊ¼»¯Í·½Úµã
    INIT_LIST_HEAD(&listhead);

    //½¨Á¢N¸ö½Úµã£¬ÒÀ´Î¼ÓÈëµ½Á´±íµ±ÖÐ
    for (i = 0; i < N; i++) {
        listnode = (struct numlist *)kmalloc(sizeof(struct numlist), GFP_KERNEL); // kmalloc£¨£©ÔÚÄÚºË¿Õ¼äÉêÇëÄÚ´æ£¬ÀàËÆÓÚmalloc
        listnode->num = i+1;
        list_add_tail(&listnode->list, &listhead);
        printk("Node %d has added to the doublelist...\n", i+1);
    }

    //±éÀúÁ´±í
    i = 1;
    list_for_each(pos, &listhead) {
        p = list_entry(pos, struct numlist, list);
        printk("Node %d's data:%d\n", i, p->num);
        i++;
    }

    return 0;
}

static void __exit doublelist_exit(void)
{
    struct list_head *pos, *n;
    struct numlist *p;
    int i;

    //ÒÀ´ÎÉ¾³ýN¸ö½Úµã
    i = 1;
    list_for_each_safe(pos, n, &listhead) {  //ÎªÁË°²È«É¾³ý½Úµã¶ø½øÐÐµÄ±éÀú
        list_del(pos);//´ÓË«Á´±íÖÐÉ¾³ýµ±Ç°½Úµã
        p = list_entry(pos, struct numlist, list);//µÃµ½µ±Ç°Êý¾Ý½ÚµãµÄÊ×µØÖ·£¬¼´Ö¸Õë
        kfree(p);//ÊÍ·Å¸ÃÊý¾Ý½ÚµãËùÕ¼¿Õ¼ä
        printk("Node %d has removed from the doublelist...\n", i++);
    }
    printk("doublelist is exiting..\n");
}

module_init(doublelist_init);
module_exit(doublelist_exit);