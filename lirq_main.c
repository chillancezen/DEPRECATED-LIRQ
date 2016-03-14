
#include "lirq_main.h"

#include <linux/slab.h>


void lirq_task_init(struct lirq_task* task)
{
	memset(task,0x0,sizeof(struct lirq_task));
	INIT_LIST_HEAD(&task->list);
	clear_bit(LIRQ_TASK_STATE_ENABLE,&task->task_state);
}

EXPORT_SYMBOL(lirq_task_init);

DEFINE_PER_CPU(struct lirq_context,lirq_context);
int __lirq_schedule(struct lirq_task * task,int cpu)
{
	/*get this task prepared,*/
	struct lirq_context * context=&per_cpu(lirq_context,cpu);
	if(test_bit(LIRQ_TASK_STATE_ENABLE,&task->task_state))
		return -1;/*already in the task list*/
	
	spin_lock(&context->context_lock);
	/*next we add the task into context task list*/
	list_add_tail_rcu(&task->list,&context->task_list_head);
	task->cpu=cpu;
	set_bit(LIRQ_TASK_STATE_ENABLE,&task->task_state);
	context->irq_count++;
	spin_unlock(&context->context_lock);
	/*here I will try to notify the CPU context :it's time to work*/
	raise_lirq(cpu);
	return 0;
}

EXPORT_SYMBOL(__lirq_schedule);
int lirq_schedule(struct lirq_task *task)
{
	return __lirq_schedule(task,smp_processor_id());
}
EXPORT_SYMBOL(lirq_schedule);
void default_lirq_task_rcu_callback(struct lirq_task* task)
{
	;/*do nothing here*/
}
EXPORT_SYMBOL(default_lirq_task_rcu_callback);

void lirq_rcu_callback(struct rcu_head *rcu)
{
	struct lirq_task * task=container_of(rcu,struct lirq_task,rcu);
	if(task->rcu_callback_fun)
		task->rcu_callback_fun(task);
}

int lirq_complete(struct lirq_task *task)
{
	/*should be called by task->do_task(),this is what NAPI does in network cases*/
	/*first we must make sure it's already in the list*/
	struct lirq_context * context;
	if(!test_bit(LIRQ_TASK_STATE_ENABLE,&task->task_state))
		return -1;
	context=&per_cpu(lirq_context,task->cpu);
	spin_lock(&context->context_lock);
	
	list_del_rcu(&task->list);
	clear_bit(LIRQ_TASK_STATE_ENABLE,&task->task_state);
	call_rcu(&task->rcu,lirq_rcu_callback);
	spin_unlock(&context->context_lock);
	return 0;
}

EXPORT_SYMBOL(lirq_complete);

int lirq_worker(void * arg)
{/*in our lirq context,preemption is disabled,which means normal context will never preempt this while it holds this CPU,so we could simply say ,it's atomic, */
	struct lirq_context *context=arg;
	struct lirq_task *task;
	int should_continue;
	enum lirq_task_return_t ret_t;
	while(!kthread_should_stop()){
		//printk("[x]msg from:%d %d\n",context->cpu,smp_processor_id());
		get_cpu();
		/*get through  the task lis, and find all the  tasks ,finish all them ,then end this round */
		should_continue=1;
		while(should_continue){/*in the while loop,if there is no work to do,we should stop it right now*/
			should_continue=0;
			rcu_read_lock();
			list_for_each_entry_rcu(task,&context->task_list_head,list){
				if(!test_bit(LIRQ_TASK_STATE_ENABLE,&task->task_state))
					continue;
				if(!task->do_task)
					continue;
				ret_t=task->do_task(task,task->arg);
				switch(ret_t)
				{
					case LIRQ_TASK_PENDING:
						should_continue=1;/*say it will be scheduled again at the next loop*/
						break;
					case LIRQ_TASK_DONE:
						break;
				}
				
			}
			rcu_read_unlock();
			
		}
		put_cpu();
		clear_bit(LIRQ_STATE_RUNNING,&context->state);
		/*here we will not use timeout - schedule API no more,and if any thing must */
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		//schedule_timeout_interruptible(1000);/*here we'd better take use of  timeout schedule ,so we do not need to raise another IRQ when we release lirq worker*/
		/*and again ,this timeout- schedule will automatically set current scheduling state,it's rather convenient*/
	}
	printk("[x] kworker stop:%d\n",context->cpu);
	return 0;
}
void raise_lirq(int cpu)/*it only notify the CPU :ok,wakeup,it's time to work*/
{
	struct lirq_context *context=&per_cpu(lirq_context,cpu);
	if(!test_and_set_bit(LIRQ_STATE_RUNNING,&context->state))
		wake_up_process(context->ts_worker);
	
}
EXPORT_SYMBOL(raise_lirq);
int proc_open (struct inode * inode, struct file *flip)
{

	return 0;
}
int proc_relase (struct inode * inode, struct file *flip)
{
	return 0;
}
ssize_t proc_read (struct file *flip, char __user * buff, size_t size, loff_t *fptr)
{

	char buffer[512];
	int len=0;
	int cpu;
	struct lirq_context * context;
	if(*fptr)
		return 0;/*0 indicates an end*/
	for_each_online_cpu(cpu){
		context=&per_cpu(lirq_context,cpu);
		len+=snprintf(buffer+len,sizeof(buffer)-len,"cpu %2d   :%8d\n",cpu,context->irq_count);
	}
	*fptr=len;
	/*copy it into userspace*/
	copy_to_user(buff,buffer,len);
	return size;
}
#if 0
enum lirq_task_return_t test_do_task(struct lirq_task * task,void *arg)
{
	if(!*(int*)arg)
	{
		lirq_complete(task);
		return LIRQ_TASK_DONE;
	}

	printk("current:%d\n",*(int*)arg);
	*(int*)arg=*(int*)arg-1;
	return LIRQ_TASK_PENDING;
}
void test_rcu_callback_fun(struct lirq_task *task)
{
	printk("kfree\n");
	kfree(task);
}

#endif
ssize_t proc_write (struct file *flip, const char __user * buff, size_t size, loff_t *fptr)
{
	#if 0
	struct lirq_task * task=kmalloc(sizeof(struct lirq_task),GFP_KERNEL);
	lirq_task_init(task);
	task->test_data=3;
	task->arg=&task->test_data;
	task->do_task=test_do_task;
	task->rcu_callback_fun=test_rcu_callback_fun;
	lirq_schedule(task);
	#endif
	
	return size;
}

struct file_operations proc_fops=
{
	.open=proc_open,
	.release=proc_relase,
	.read=proc_read,
	.write=proc_write
};
struct proc_dir_entry *proc_entry;
int LIRQ_init(void)
{
	struct lirq_context*context;
	int cpu;
	
	char buffer[256];
	for_each_online_cpu(cpu){
		context=&per_cpu(lirq_context,cpu);
		context->cpu=cpu;
		context->irq_count=0;
		spin_lock_init(&context->context_lock);
		INIT_LIST_HEAD(&context->task_list_head);
		
		memset(buffer,0x0,sizeof(buffer));
		set_bit(LIRQ_STATE_ENABE,&context->state);
		sprintf(buffer,"lirq_worker%d",cpu);
		context->ts_worker=kthread_create(lirq_worker,context,buffer);
		//context->ts_worker=kthread_create_on_cpu(lirq_worker,context,cpu,buffer);
		if(context->ts_worker!= ERR_PTR(-ENOMEM)){
			kthread_bind(context->ts_worker,context->cpu);
			wake_up_process(context->ts_worker);
			printk("[x]cpu init worker:%d\n",context->cpu);
		}else{
			printk("[x]something get wrong with spawning per-cpu:%d worker\n",context->cpu);
		}
	}

	proc_entry=proc_create("lirqs",0,NULL,&proc_fops);
	
	return 0;
}
void LIRQ_exit(void)
{

	struct lirq_context*context;
	
	int cpu;
	for_each_online_cpu(cpu){
		context=&per_cpu(lirq_context,cpu);
		raise_lirq(cpu);
		kthread_stop(context->ts_worker);
	}
	remove_proc_entry("lirqs",NULL);
}

module_init(LIRQ_init);
module_exit(LIRQ_exit);
MODULE_AUTHOR("jzheng from BJTU&Juniper");
MODULE_LICENSE("GPL v2");

