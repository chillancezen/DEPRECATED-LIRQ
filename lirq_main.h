#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

enum LIRQ_STATE{
	LIRQ_STATE_ENABE=0,
	LIRQ_STATE_RUNNING,
};

struct lirq_context{
	int cpu;
	volatile long unsigned int state;
	struct task_struct *ts_worker;
	spinlock_t context_lock;/*should be held, when we are apllying some changes to this cpu node exclusivly*/

	__rcu struct list_head task_list_head ;
	int irq_count;
};
enum lirq_task_return_t{
	LIRQ_TASK_PENDING,
	LIRQ_TASK_DONE,
};
enum lirq_task_state{
	LIRQ_TASK_STATE_ENABLE=0,/*if diabled,will not be scheduled again*/	
};
struct lirq_task{
	#if 0
	int test_data;
	#endif
	
	int cpu;/*cpu on which this task is scheduled*/
	struct list_head list;
	struct rcu_head rcu;
	volatile long  unsigned int task_state;
	void (*rcu_callback_fun)(struct lirq_task *task);//mandatory argument
	enum lirq_task_return_t (*do_task)(struct lirq_task *task,void *arg);/*mandatory argument,,as with NAPI,if it finishes in this function,we should complete,*/
	void * arg;
};
void lirq_task_init(struct lirq_task* task);

void raise_lirq(int cpu);
int __lirq_schedule(struct lirq_task * task,int cpu);
int lirq_schedule(struct lirq_task *task);
int lirq_complete(struct lirq_task *task);

void default_lirq_task_rcu_callback(struct lirq_task* task);





