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

#define EXEC_FUNCTION "prepare_binprm"   // inject porbe into this function
#define OPEN_FUNCTION "do_filp_open"
// #define TEST_PATH "/usr/bin/cat"     //  this porcess can not be execvet(test)

static int container_name = 0;        // container init process pid
static int containerd_shim = 0;		  // containerd shime process pid 
static int list_type = 0;             // 0 is use exec blacklist; 1 is exec whitelist
static int openlist_type = 0;         // 0 is use open blacklist; 1 is open whitelist 
static char *dir_path = "default";    // the dir path of config

module_param(container_name,int, 0644);
module_param(containerd_shim,int, 0644);
module_param(list_type,int, 0644);
module_param(openlist_type,int,0644);
module_param(dir_path, charp, 0644);

char *pro_head = "process_head";
char tmp_dir[100];


struct exec_list{                     //to store exec_blacklist and exec_whitelist
	char process_name[100];        // attention : max len no more than 100 !!
	struct list_head list;
};

struct process_list                // to store open blacklist and open whitelist
{
 	char process[100];
 	struct list_head list_p;
};

struct open_list
{
	char file_name[100];
	struct process_list pro_list;
	struct list_head list_f;
};

struct list_head blacklist;              //list head of exec blacklist
struct list_head whitelist;              //list head of exec whitelist

struct list_head open_blacklist_head;    // list head of open blakclist     
struct list_head open_whitelist_head;    // list head of open whitelist

static struct                        //  store the newest containerd_shim tgid
{
    pid_t act_comm;
    pid_t nearly;
} tt ;


static struct jprobe get_exec_jp;   // get the exec function 
static struct kretprobe stop_exec_krp; 	// change the exec function return to stop exec
static struct kretprobe stop_open_krp;  // to stop open exec 

static int exec_change_flag = 0;   // 1 : stop exec process , 0 not 

int creat_list(char *buf1, int count)        // init exec_list
{
	struct exec_list *insert;
	struct list_head *pos_head;
	struct exec_list *p;

	struct list_head *tmp_list;

	char tmp[70];
	int i = 0,new_count = 0;

    while(i<count)
    {
		if(buf1[i] != '\n')
		{
			tmp[new_count] = buf1[i];
			new_count++;
			i++;
		}
		else
		{
	        insert = (struct exec_list *)kmalloc(sizeof(struct exec_list),GFP_KERNEL);    //  

	        tmp[new_count] = '\0';
			// memcpy(insert->process_name,tmp,sizeof(tmp));
			strcpy(insert->process_name,tmp);

			if(list_type == 0)
				list_add_tail(&insert->list,&blacklist);
			else
				list_add_tail(&insert->list,&whitelist);
       		i++;
       		new_count = 0;
       		tmp[0] = '\0';
		}
	}

	return 1;
}

int creat_openlist(char *buf1, int count)                //init open_list
{
	struct open_list *insert;

	char tmp[70];
	int i = 0,new_count = 0;

	if(openlist_type == 0)                 // read open blacklist
	{
	    while(i<count)
	    {
			if(buf1[i] != '\n')
			{
				tmp[new_count] = buf1[i];
				new_count++;
				i++;
			}
			else
			{
				if(tmp[0] == '/')
				{
					tmp[new_count] = '\0';
					insert = (struct open_list *)kmalloc(sizeof(struct open_list), GFP_KERNEL);
					strcpy(insert->file_name,tmp);
					INIT_LIST_HEAD(&insert->pro_list.list_p);
					strcpy(insert->pro_list.process,pro_head);
					list_add_tail(&insert->list_f,&open_blacklist_head);

					i++;
	       			new_count = 0;
	       			tmp[0] = '\0';
				}
				else
				{
					tmp[new_count] = '\0';
					struct process_list *p_insert;
					p_insert = (struct process_list *)kmalloc(sizeof(struct process_list),GFP_KERNEL);
					strcpy(p_insert->process,tmp);
					list_add_tail(&p_insert->list_p,&insert->pro_list.list_p);

					i++;
	       			new_count = 0;
	       			tmp[0] = '\0';

				}
			}
		}
	}
	else                                    // read white list
	{
	    while(i<count)
	    {
			if(buf1[i] != '\n')
			{
				tmp[new_count] = buf1[i];
				new_count++;
				i++;
			}
			else
			{
				if(tmp[0] == '/')
				{
					tmp[new_count] = '\0';
					insert = (struct open_list *)kmalloc(sizeof(struct open_list), GFP_KERNEL);
					strcpy(insert->file_name,tmp);
					INIT_LIST_HEAD(&insert->pro_list.list_p);
					strcpy(insert->pro_list.process,pro_head);
					list_add_tail(&insert->list_f,&open_whitelist_head);

					i++;
	       			new_count = 0;
	       			tmp[0] = '\0';
				}
				else
				{
					tmp[new_count] = '\0';
					struct process_list *p_insert;
					p_insert = (struct process_list *)kmalloc(sizeof(struct process_list),GFP_KERNEL);
					strcpy(p_insert->process,tmp);
					list_add_tail(&p_insert->list_p,&insert->pro_list.list_p);

					i++;
	       			new_count = 0;
	       			tmp[0] = '\0';

				}
			}
		}
	}
	return 1;
}

void insert_blacklist(char *process_path)
{
	struct exec_list *insert;
	insert = (struct exec_list *)kmalloc(sizeof(struct exec_list),GFP_KERNEL);
	// memcpy(insert->process_name,process_path,sizeof(process_path));
	strcpy(insert->process_name, process_path);
	list_add_tail(&insert->list,&blacklist);
}

void insert_whitelist(char *process_path)
{
	struct exec_list *insert;
	insert = (struct exec_list *)kmalloc(sizeof(struct exec_list),GFP_KERNEL);
	// memcpy(insert->process_name,process_path,sizeof(process_path));
	strcpy(insert->process_name, process_path);
	list_add_tail(&insert->list,&whitelist);
}

int delete_blacklist(char *process_path)
{
	struct list_head *pos, *n;
	struct exec_list *p;

    list_for_each_safe(pos, n, &blacklist) 
    {
        p = list_entry(pos, struct exec_list, list);
    	if( strcmp(process_path,p->process_name) == 0)
    	{
	        list_del(pos);
        	kfree(p);
        	printk("delete node !!\n");
        	return 1;
    	}
    }
    printk("no such process_name in blacklist \n");
    return 0;
}

int delete_whitelist(char *process_path)
{
	struct list_head *pos, *n;
	struct exec_list *p;

    list_for_each_safe(pos, n, &whitelist) 
    {
        p = list_entry(pos, struct exec_list, list);
    	if( strcmp(process_path,p->process_name) == 0)
    	{
	        list_del(pos);
        	kfree(p);
        	printk("delete node !!\n");
        	return 1;
    	}
    }
    printk("no such process_name in whitelist\n");
    return 0;
}

int insert_open_blacklist(char *process_path)
{
	char tmp[2][200];
	int i=0,newcount=0,j=0;
	while(i<strlen(process_path))
	{
		if(process_path[i] != ':')
		{
			tmp[j][newcount] = process_path[i];
			i++;
			newcount++;
		}
		else
		{
			tmp[j][newcount] = '\0';
			j++;
			newcount = 0;
			i++;
		}
	}
	printk("file:'%s'\n", tmp[0]);
	printk("process:'%s'\n", tmp[1]);

	struct open_list *f_insert,*p;
	struct list_head *pos;
	struct process_list *insert;

	list_for_each(pos,&open_blacklist_head){
		p = list_entry(pos,struct open_list,list_f);
		if(strcmp(tmp[0],p->file_name) == 0)
		{
			insert = (struct process_list *)kmalloc(sizeof(struct process_list),GFP_KERNEL);
			strcpy(insert->process, tmp[1]);
			list_add_tail(&insert->list_p,&p->pro_list.list_p);
			return 1;
		}
	}

	f_insert = (struct open_list *)kmalloc(sizeof(struct open_list),GFP_KERNEL);
	strcpy(f_insert->file_name, tmp[0]);
	INIT_LIST_HEAD(&f_insert->pro_list.list_p);
	strcpy(f_insert->pro_list.process,pro_head);
	list_add_tail(&f_insert->list_f,&open_blacklist_head);

	insert = (struct process_list *)kmalloc(sizeof(struct process_list),GFP_KERNEL);
	strcpy(insert->process, tmp[1]);
	list_add_tail(&insert->list_p,&f_insert->pro_list.list_p);
	return 0;
}

int delete_open_blacklist(char *process_path)
{
	char tmp[2][200];                     // deal with the process_path       tmp[0] is file name, tmp[1] is process name 
	int i=0,newcount=0,j=0;
	while(i<strlen(process_path))
	{
		if(process_path[i] != ':')
		{
			tmp[j][newcount] = process_path[i];
			i++;
			newcount++;
		}
		else
		{
			tmp[j][newcount] = '\0';
			j++;
			newcount = 0;
			i++;
		}
	}

	struct open_list *p;
	struct list_head *pos;
	struct list_head *p_pos,*p_n;
	struct process_list *p_del;

	list_for_each(pos,&open_blacklist_head)
	{
		p = list_entry(pos,struct open_list,list_f);
		if(strcmp(tmp[0],p->file_name) == 0)
		{
		    list_for_each_safe(p_pos, p_n, &p->pro_list.list_p) 
    		{
    			p_del = list_entry(p_pos, struct process_list, list_p);
    			if( strcmp(tmp[1],p_del->process) == 0)
    			{
	        		list_del(p_pos);
        			kfree(p_del);
        			printk("delete node !!\n");
        			return 1;
    			}
    		}
		}
	}
    printk("no such process_name in open blacklist\n");
    return 0;
}

int insert_open_whitelist(char *process_path)
{
	char tmp[2][200];
	int i=0,newcount=0,j=0;
	while(i<strlen(process_path))
	{
		if(process_path[i] != ':')
		{
			tmp[j][newcount] = process_path[i];
			i++;
			newcount++;
		}
		else
		{
			tmp[j][newcount] = '\0';
			j++;
			newcount = 0;
			i++;
		}
	}
	printk("file:'%s'\n", tmp[0]);
	printk("process:'%s'\n", tmp[1]);

	struct open_list *f_insert,*p;
	struct list_head *pos;
	struct process_list *insert;

	list_for_each(pos,&open_whitelist_head){
		p = list_entry(pos,struct open_list,list_f);
		if(strcmp(tmp[0],p->file_name) == 0)
		{
			insert = (struct process_list *)kmalloc(sizeof(struct process_list),GFP_KERNEL);
			strcpy(insert->process, tmp[1]);
			list_add_tail(&insert->list_p,&p->pro_list.list_p);
			return 1;
		}
	}

	f_insert = (struct open_list *)kmalloc(sizeof(struct open_list),GFP_KERNEL);
	strcpy(f_insert->file_name, tmp[0]);
	INIT_LIST_HEAD(&f_insert->pro_list.list_p);
	strcpy(f_insert->pro_list.process,pro_head);
	list_add_tail(&f_insert->list_f,&open_whitelist_head);

	insert = (struct process_list *)kmalloc(sizeof(struct process_list),GFP_KERNEL);
	strcpy(insert->process, tmp[1]);
	list_add_tail(&insert->list_p,&f_insert->pro_list.list_p);
	return 0;
}

int delete_open_whitelist(char *process_path)
{
	char tmp[2][200];                     // deal with the process_path       tmp[0] is file name, tmp[1] is process name 
	int i=0,newcount=0,j=0;
	while(i<strlen(process_path))
	{
		if(process_path[i] != ':')
		{
			tmp[j][newcount] = process_path[i];
			i++;
			newcount++;
		}
		else
		{
			tmp[j][newcount] = '\0';
			j++;
			newcount = 0;
			i++;
		}
	}

	struct open_list *p;
	struct list_head *pos;
	struct list_head *p_pos,*p_n;
	struct process_list *p_del;

	list_for_each(pos,&open_whitelist_head)
	{
		p = list_entry(pos,struct open_list,list_f);
		if(strcmp(tmp[0],p->file_name) == 0)
		{
		    list_for_each_safe(p_pos, p_n, &p->pro_list.list_p) 
    		{
    			p_del = list_entry(p_pos, struct process_list, list_p);
    			if( strcmp(tmp[1],p_del->process) == 0)
    			{
	        		list_del(p_pos);
        			kfree(p_del);
        			printk("delete node !!\n");
        			return 1;
    			}
    		}
		}
	}
    printk("no such process_name in open whitelist\n");
    return 0;
}
void print_blacklist(void)
{
	struct list_head *pos_head;
	struct exec_list *p;

	list_for_each(pos_head,&blacklist)
	{
		p = list_entry(pos_head,struct exec_list,list);
		printk("black_list: '%s'\n", p->process_name);
	}
}

void print_openlist(void)                       // debug function!!
{
	struct list_head *pos;
	struct list_head *p_pos;
	struct open_list *p;
	struct process_list *pp;

	int k = 1,m=1;
    list_for_each(pos, &open_blacklist_head) {
        p = list_entry(pos, struct open_list, list_f);
        printk("file '%d' data:'%s'\n", k, p->file_name);
        k++;

        list_for_each(p_pos,&p->pro_list.list_p){
        	pp = list_entry(p_pos,struct process_list,list_p);
        	printk("process  '%d' data '%s'\n",m,pp->process);
        	m++;
        }
    }
}

char *get_path(char *file_name)    // get full path of target file
{
	char *tmp = dir_path;
	strcat(tmp, file_name);
	return tmp;
}
 

int read_file(char *file_path)     // read exec list    
{
	char *com_path = get_path(file_path);
	struct file *fp;
	char buf1[10000];
    mm_segment_t fs;
    loff_t pos;

    printk("read exec file start !! \n");
    fp = filp_open(com_path,O_RDWR,0644);

    if (IS_ERR(fp)){
        printk("open file error/n");
        return -1;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);
    pos =0;
    int count = 0;
    int i = 0;
    count = vfs_read(fp, buf1, sizeof(buf1), &pos);
  	printk("count: %d\n", count);

	int r;
	r = creat_list(buf1,count);
	if(r == 1)
	{
		printk("build list success !!\n");
		// print_list(&blacklist);
	}
	else 
		return -1;

  	// printk("buf :'%s'\n", buf1);     //print input stream
    
    filp_close(fp,NULL);
    set_fs(fs);
    return 1;
}

int read_open_file(char *file_path)
{
	struct file *fp;
	char buf1[10000];
    mm_segment_t fs;
    loff_t pos;

    printk("read open file start /n");
    fp = filp_open(file_path,O_RDWR,0644);

    if (IS_ERR(fp)){
        printk("open file error/n");
        return -1;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);
    pos =0;
    int count = 0;
    count = vfs_read(fp, buf1, sizeof(buf1), &pos);
  	printk("count: %d\n", count);

	int r;
	r = creat_openlist(buf1,count);
	if(r == 1)
	{
		printk("build open list success !!\n");
		print_openlist();
	}
	else 
		return -1;

    printk("buf :'%s'\n", buf1);
    filp_close(fp,NULL);
    set_fs(fs);
    return 1;
}


/* probes function !!*/ 

pid_t get_container_flag(void)    //  get the first process(init process) pid in container;
{
	pid_t flag;
	return (flag = current -> pids[PIDTYPE_PID].pid -> numbers[1].ns->child_reaper->pid);
}

int in_container(void)      // find out current process in the container  (in 1  not 0)
{
	if((current->nsproxy->pid_ns->level) >0) // in container 
		return 1;
	else
		return 0;
}

int stop_exec(int flag, struct linux_binprm *bprm)
{
	if(list_type == 0)                          //use blacklist	
	{
		if(container_name == flag)
		{
			struct list_head *pos_head;
			struct exec_list *p;

			list_for_each(pos_head,&blacklist)
			{
				p = list_entry(pos_head,struct exec_list,list);
				if(strcmp(bprm->filename,p->process_name) == 0)
					return 1;
			}
		}
		else 
			return 0;
	}
	else                                    //use white
	{
		if(container_name == flag)
		{
			struct list_head *pos_head;
			struct exec_list *p;

			list_for_each(pos_head,&whitelist)
			{
				p = list_entry(pos_head,struct exec_list,list);
				if(strcmp(bprm->filename,p->process_name) == 0)
				{
					printk("'%s'\n", p->process_name);
					return 0;
				}
			}
			// printk("current comm :'%s', tgid:'%d',pid:'%d', parent name :'%s', parent tgid:'%d'\n",current->comm,current->tgid,current->pid,current->parent->comm,current->parent->tgid);
			// printk("exec file path:'%s'\n", bprm->filename);
			return 1;
		}
		else 
			return 0;
	}
}

int jprobe_exec_value(struct linux_binprm *bprm)   // 
{
	if(in_container() > 0)         //  process in container 
	{
		int flag;
		flag = get_container_flag();
		exec_change_flag = stop_exec(flag, bprm);
	}
	else                                        // process not in container but it is a containerd-shim process 
	{
		if(strcmp(current->comm, "containerd-shim") == 0)
		{
	        if((current->parent->tgid) == tt.act_comm)
        	{
	            tt.nearly = current->tgid;
	            // printk("update shim nearly value '%d'\n", tt.nearly);
        	}
		}

	    if(strcmp(current->comm, "runc:[2:INIT]") == 0)
	    {
	        if((current->parent->tgid) == tt.nearly)
	        {
	            printk("target container\n");

	            exec_change_flag = stop_exec(container_name,bprm);

	        }
	    }

	}
	jprobe_return();
	return 0;
}

static int krp_exec_check(struct kretprobe_instance *ri, struct pt_regs *regs) // 
{
	if(exec_change_flag == 1)
	{
        printk("change regsax, set exec_change_flag back to 0 \n ");
        exec_change_flag = 0;
        regs->ax = -13;
	}
	return 0;
}

static int open_krp_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
 	if (!current->mm)
  		return 1; /* Skip kernel threads */

	if(in_container() > 0)         //  process in container 
	{
		int flag;
		flag = get_container_flag();
		if(flag != container_name)
			return 0;
		else                            // current process in target container
		{
 			struct filename *tmp;
 			tmp = regs->si;

			if(openlist_type == 0)       // use open blacklist 
			{
				struct list_head *pos;
				struct list_head *p_pos;
				struct open_list *p;
				struct process_list *pp;

			    list_for_each(pos, &open_blacklist_head) 
			    {
		        	p = list_entry(pos, struct open_list, list_f);
		        	if(strcmp(p->file_name,tmp->name) == 0)
		        	{
			        	list_for_each(p_pos,&p->pro_list.list_p)
		        		{
		        			pp = list_entry(p_pos,struct process_list,list_p);
		        			if(strcmp(pp->process,current->comm) == 0)
		        			{
		        				char *test = tmp->name;
						 		strcpy(test,"no");          
        				 		printk("cannot not open!!!\n");
        				 		return 0;
		        			}
       					}
		        	}
    			}
			}
			else
			{
				struct list_head *pos;
				struct list_head *p_pos;
				struct open_list *p;
				struct process_list *pp;

			    list_for_each(pos, &open_whitelist_head) 
			    {
		        	p = list_entry(pos, struct open_list, list_f);
		        	if(strcmp(p->file_name,tmp->name) == 0)
		        	{
			        	list_for_each(p_pos,&p->pro_list.list_p)
		        		{
		        			pp = list_entry(p_pos,struct process_list,list_p);
		        			if(strcmp(pp->process,current->comm) == 0)
        				 		return 0;
       					}
        				char *test = tmp->name;
				 		strcpy(test,"no");          
				 		printk("cannot not open!!!\n");
				 		return 0;
		        	}
    			}
				return 0;
			}
		}
	}
}

static int open_krp_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	 return 0;
}

int __init control_init(void)
{
	int rd = 0;
	int rd2 = 0;

	INIT_LIST_HEAD(&blacklist);
	INIT_LIST_HEAD(&whitelist);
	INIT_LIST_HEAD(&open_blacklist_head);
	INIT_LIST_HEAD(&open_whitelist_head);

	char tmp_dir[100];
	strcpy(tmp_dir,dir_path);


	if(strcmp(dir_path, "default") == 0)
	{
		printk("no para!!!\n");
		return -1;
	}

	if(container_name == 0 && containerd_shim == 0)
	{
		printk("no contianer para!!!\n");
		return -1;
	}

	if(list_type != 0 && list_type != 1)
	{
		printk("error list type !!!\n");
		return -1;
	} 

	if(openlist_type != 0 && openlist_type != 1)
	{
		printk("error openlist type !!!\n");
		return -1;
	} 

	if(list_type == 0)
	{
		if((rd=read_file("exec_blacklist")) == -1)    // use exec blacklist 
		{
			printk("read blacklist faild!!\n");
			return -1;
		}
	}
	else
	{
		if((rd=read_file("exec_whitelist")) == -1)    // use exec whitelist
		{
			printk("read whitelist failed!!\n");
			return -1;
		}
	}

	if(openlist_type == 0)                             // use open blacklist     
	{
		strcat(tmp_dir,"open_blacklist");
		printk("path :'%s'\n",tmp_dir);

		if((rd2=read_open_file(tmp_dir)) == -1 )
		{
			printk("read open list failed!!\n");
			return -1;
		}
		print_openlist();
	}
	else                                               // use open whitelist
	{
		strcat(tmp_dir,"open_whitelist");
		printk("path :'%s'\n",tmp_dir);
		if((rd2=read_open_file(tmp_dir)) == -1 )
		{
			printk("read open list failed!!\n");
			return -1;
		}
	}

	// char *insert_test = "/test/jiangchao:ntci666";
	// insert_open_blacklist(insert_test);
	// print_openlist();

	// delete_open_blacklist(insert_test);
	// print_openlist();

	// struct list_head *pos_head;
	// struct exec_list *p;

	// if(list_type == 0)                      // print information
	// {
	// list_for_each(pos_head,&blacklist)
	// {
	// 	p = list_entry(pos_head,struct exec_list,list);
	// 	printk("black_list: '%s'\n", p->process_name);
	// }
	// }
	// else
	// {
	// list_for_each(pos_head,&whitelist)
	// {
	// 	p = list_entry(pos_head,struct exec_list,list);
	// 	printk("white_list: '%s'\n", p->process_name);
	// }}

	/*jprobe registe */
	printk("install control_module !!\n");
	int ret_get_exec_jp, ret_stop_exec_krp;
	int ret_stop_open_krp;

	tt.act_comm = containerd_shim;     // container_name's parent process( containerd_shim)

    get_exec_jp.entry = (kprobe_opcode_t *)jprobe_exec_value;     //init get_exec_jp
    get_exec_jp.kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name(EXEC_FUNCTION);
    if(!get_exec_jp.kp.addr)
    {
    	printk("Cannot find the address of EXEC_FUNCTION");
    	return -1;
    }
    if((ret_get_exec_jp = register_jprobe(&get_exec_jp)) < 0)
    {
    	printk("register_jprobe failed ,return %d\n", ret_get_exec_jp);
    	return -1;
    } 
    printk("Registered a prepare_binprm_jprobe.\n");


    /* kretprobe registe*/
    stop_exec_krp.kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name(EXEC_FUNCTION); // init stop_exec_krp
    if (!stop_exec_krp.kp.addr) {
        printk("Couldn't find prepare_binprm.\n");
        return -1;
    }

    stop_exec_krp.handler = krp_exec_check,
    stop_exec_krp.maxactive = 20;

    if ((ret_stop_exec_krp = register_kretprobe(&stop_exec_krp)) < 0) {
        printk("register_kretprobe failed, returned %d\n", ret_stop_exec_krp);
        return -1;
    }
    printk("Registered a exec return probe.\n");

    /*  stop_open_krp kretprobe registe */
    stop_open_krp.kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name(OPEN_FUNCTION);    // init stop_open_krp
    if (!stop_open_krp.kp.addr) {
    	printk("Couldn't find do_filp_open.\n");
    	return -1;
    }

    stop_open_krp.entry_handler  = open_krp_entry_handler,
    stop_open_krp.handler = open_krp_handler,
    stop_open_krp.maxactive = 20;

    if ((ret_stop_open_krp = register_kretprobe(&stop_open_krp)) < 0) {
	    printk("register_kretprobe failed, returned %d\n", ret_stop_open_krp);
	    return -1;
    }
    printk("Registered a exec return probe.\n");


	return 0;
}

void __exit control_exit(void)
{
	struct list_head *pos, *n;
    struct exec_list *p;
    int i,j;

    if(list_type == 0)
    {
	    i = 1;
	    list_for_each_safe(pos, n, &blacklist) {  
	        list_del(pos);
	        p = list_entry(pos, struct exec_list, list);

	        printk("delete:'%s'\n", p->process_name);
	        
	        kfree(p);
	        printk("Node %d has removed from the blacklist\n", i++);
	    }
    }
    else
    {
	    i = 1;
	    list_for_each_safe(pos, n, &whitelist) {  
	        list_del(pos);
	        p = list_entry(pos, struct exec_list, list);
	        kfree(p);
	        printk("Node %d has removed from the whitelist\n", i++);
	    }
    }

    struct list_head *op_pos, *op_n;
    struct list_head *p_pos,*p_n;


    struct open_list *op;
    struct process_list *pp;

    if(openlist_type == 0)
    {
    	i = 1,j = 1;
    	list_for_each_safe(op_pos, op_n, &open_blacklist_head) {  
	        op = list_entry(op_pos, struct open_list, list_f);
	        list_for_each_safe(p_pos,p_n,&op->pro_list.list_p)
	        {
	        	pp = list_entry(p_pos,struct process_list, list_p);
	        	list_del(p_pos);
	        	kfree(pp);
	        	printk("process remove '%d'\n", j++);
	        }
	        list_del(op_pos);
	        kfree(op);

	        printk("Node %d has removed from the doublelist...\n", i++);
    	}
    }
    else
    {
    	i = 1,j = 1;
    	list_for_each_safe(op_pos, op_n, &open_whitelist_head) {  
	        op = list_entry(op_pos, struct open_list, list_f);
	        list_for_each_safe(p_pos,p_n,&op->pro_list.list_p)
	        {
	        	pp = list_entry(p_pos,struct process_list, list_p);
	        	list_del(p_pos);
	        	kfree(pp);
	        	printk("process remove '%d'\n", j++);
	        }
	        list_del(op_pos);
	        kfree(op);

	        printk("Node %d has removed from the doublelist...\n", i++);
    	}
    }

    
    // printk("module exit/n");

    unregister_jprobe(&get_exec_jp);
    printk("jprobe at %pF unregistered\n",get_exec_jp.kp.addr);

    unregister_kretprobe(&stop_exec_krp);
    printk("kretprobe exec unregistered\n");
    printk("Missed %d exec probe instances.\n", stop_exec_krp.nmissed);

    unregister_kretprobe(&stop_open_krp);
    printk("kretprobe open unregistered\n");
    printk("Missed %d open probe instances.\n", stop_open_krp.nmissed);

	printk("control_module exit !!\n");
}

EXPORT_SYMBOL(blacklist);                            // export these function and var to offer update_list module
EXPORT_SYMBOL(whitelist);
EXPORT_SYMBOL(open_blacklist_head);
EXPORT_SYMBOL(open_whitelist_head);
EXPORT_SYMBOL(insert_blacklist);
EXPORT_SYMBOL(delete_blacklist);
EXPORT_SYMBOL(insert_whitelist);
EXPORT_SYMBOL(delete_whitelist);
EXPORT_SYMBOL(insert_open_blacklist);           // insert to open blacklist !!
EXPORT_SYMBOL(insert_open_whitelist);
EXPORT_SYMBOL(delete_open_blacklist);
EXPORT_SYMBOL(delete_open_whitelist);
EXPORT_SYMBOL(print_blacklist);
EXPORT_SYMBOL(print_openlist);


 
module_init(control_init);
module_exit(control_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("NTCI");