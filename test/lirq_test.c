#include "../lirq_main.h"
#include <linux/slab.h>

struct lirq_task * task;
int test_data=3;
enum lirq_task_return_t do_task(struct lirq_task* task,void * arg)
{
	if(!*(int*)arg){
		lirq_complete(task);
		return LIRQ_TASK_DONE;
	}
	printk("tasking:%d\n",*(int*)arg);
	*(int*)arg=*(int*)arg-1;
	return LIRQ_TASK_PENDING;
}
void recollect_callback(struct lirq_task *task)
{
	printk("kfree---\n");
	kfree(task);
}
int mod_init(void)
{
	task=kmalloc(sizeof(struct lirq_task),GFP_KERNEL);
	lirq_task_init(task);
	task->arg=&test_data;
	task->do_task=do_task;
	task->rcu_callback_fun=recollect_callback;
	lirq_schedule(task);
	
	return 0;
}
void mod_exit(void)
{
	
}
module_init(mod_init);
module_exit(mod_exit);
MODULE_AUTHOR("jzheng from bjtu");
MODULE_LICENSE("GPL v2");