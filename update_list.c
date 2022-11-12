#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/binfmts.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/kernel.h>

static char *blacklist_insert = "default";
static char *whitelist_insert = "default";
static char *blacklist_delete = "default";
static char *whitelist_delete = "default";

static char *open_blacklist_insert = "default";
static char *open_whitelist_insert = "default";
static char *open_blacklist_delete = "default";
static char *open_whitelist_delete = "default";


module_param(blacklist_insert, charp, 0644);
module_param(whitelist_insert, charp, 0644);
module_param(blacklist_delete, charp, 0644);
module_param(whitelist_delete, charp, 0644);

module_param(open_blacklist_insert, charp, 0644);
module_param(open_whitelist_insert, charp, 0644);
module_param(open_blacklist_delete, charp, 0644);
module_param(open_whitelist_delete, charp, 0644);


int __init update_list_init(void)
{	
	extern struct list_head blacklist;
	extern struct list_head whitelist;
	extern struct list_head open_blacklist_head;
	extern struct list_head open_whitelist_head;

	extern void insert_blacklist(char *process_path);
	extern void insert_whitelist(char *process_path);
	extern int delete_blacklist(char *process_path);
	extern int delete_whitelist(char *process_path);

	extern int insert_open_blacklist(char *process_path);
	extern int insert_open_whitelist(char *process_path);
	extern int delete_open_blacklist(char *process_path);
	extern int delete_open_whitelist(char *process_path);

	extern void print_blacklist(void);
	extern void print_openlist(void);


	if(strcmp(blacklist_insert,"default") == 0)
		printk("no node need to insert blacklist\n");
	else 
		insert_blacklist(blacklist_insert);

	if(strcmp(whitelist_insert,"default") == 0)
		printk("no node need to insert whitelist\n");
	else 
		insert_whitelist(whitelist_insert);

	if(strcmp(blacklist_delete,"default") == 0)
		printk("no node need to delete blacklist\n");
	else 
	{
		int ret = 0;
		ret = delete_blacklist(blacklist_delete);
	}

	if(strcmp(whitelist_delete,"default") == 0)
		printk("no node need to delete whitelist\n");
	else 
	{
		int ret = 0;
		ret = delete_whitelist(whitelist_delete);
	}

	if(strcmp(open_blacklist_insert,"default") == 0)
		printk("no node need to insert open blacklist\n");
	else 
		insert_open_blacklist(open_blacklist_insert);

	if(strcmp(open_whitelist_insert,"default") == 0)
		printk("no node need to insert open whitelist\n");
	else 
		insert_open_whitelist(open_whitelist_insert);

	if(strcmp(open_blacklist_delete,"default") == 0)
		printk("no node need to delete open blacklist\n");
	else 
	{
		int ret = 0;
		ret = delete_open_blacklist(open_blacklist_delete);
	}

	if(strcmp(open_whitelist_delete,"default") == 0)
		printk("no node need to delete open whitelist\n");
	else 
	{
		int ret = 0;
		ret = delete_open_whitelist(open_whitelist_delete);
	}


	print_blacklist();
	print_openlist();

	return 0;
}


void __exit update_list_exit(void)
{
	printk("change module exit!!!\n");
}





module_init(update_list_init);
module_exit(update_list_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("NTCI");