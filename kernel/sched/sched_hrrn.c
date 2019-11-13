#include "sched.h"


/*
 * casio-task scheduling class.
 *
 *
 */

/* =========================================================================
 *                       Log Functions Implementation
 */
/*
struct casio_event_log casio_event_log;

struct casio_event_log * get_casio_event_log(void)
{
	return &casio_event_log;
}
void init_casio_event_log(void)
{
	char msg[CASIO_MSG_SIZE];
	casio_event_log.lines=casio_event_log.cursor=0;
	snprintf(msg,CASIO_MSG_SIZE,"init_casio_event_log:(%lu:%lu)", casio_event_log.lines, casio_event_log.cursor);
	register_casio_event(sched_clock(), msg, CASIO_MSG);

}*/
/* Logs an event if there's room */
/*
void register_hrrn_event(unsigned long long t, char *m, int a)
{

	if(casio_event_log.lines < CASIO_MAX_EVENT_LINES){
		casio_event_log.casio_event[casio_event_log.lines].action=a;
		casio_event_log.casio_event[casio_event_log.lines].timestamp=t;
		strncpy(casio_event_log.casio_event[casio_event_log.lines].msg,m,CASIO_MSG_SIZE-1);
		casio_event_log.lines++;
		
		printk(KERN_ALERT "cas_event: %s\n", m);
	}
	else{
		printk(KERN_ALERT "ERROR register_casio_event full\n");
	}

}
*/

/* =========================================================================
 *             Funcs for casio tasks, and lists
 */ 

void init_hrrn_rq(struct hrrn_rq *hrrn_rq)
{
	hrrn_rq->hrrn_rb_root=RB_ROOT;
	INIT_LIST_HEAD(&hrrn_rq->hrrn_list_head);
	atomic_set(&hrrn_rq->nr_running,0);
}

/* =========================================================================
 *                       rb_trees of casio_tasks
 */ 

void remove_hrrn_task_rb_tree(struct hrrn_rq *rq, struct sched_hrrn_entity *p)
{
	rb_erase(&(p->hrrn_rb_node),&(rq->hrrn_rb_root));
	p->hrrn_rb_node.rb_left=p->hrrn_rb_node.rb_right=NULL;
}

void insert_hrrn_task_rb_tree(struct hrrn_rq *rq, struct sched_hrrn_entity *p)
{
	struct rb_node **node=NULL;
	struct rb_node *parent=NULL;
	struct sched_hrrn_entity *entry=NULL;
	node=&rq->hrrn_rb_root.rb_node;
	while(*node!=NULL){
		parent=*node;
		entry=rb_entry(parent, struct sched_hrrn_entity,hrrn_rb_node);
		if(entry){
			if(((p->waiting_time+p->burst_time)/p->burst_time) < (entry->waiting_time+entry->burst_time)/entry->burst_time){
				node=&parent->rb_left;
			}else{
				node=&parent->rb_right;
			}
		}
	}
	rb_link_node(&p->hrrn_rb_node,parent,node);
	rb_insert_color(&p->hrrn_rb_node,&rq->hrrn_rb_root);
}
struct sched_hrrn_entity * highest_hrrn_task_rb_tree(struct hrrn_rq *rq)
{
	struct rb_node *node=NULL;
	struct sched_hrrn_entity *p=NULL;
	node=rq->hrrn_rb_root.rb_node;
	if(node==NULL)
		return NULL;

	while(node->rb_left!=NULL){
		node=node->rb_left;
	}
	p=rb_entry(node, struct sched_hrrn_entity,hrrn_rb_node);
	return p;
}

/* If curr task is priority < CASIO, or some other task has an earlier deadline, preempt */
static void check_preempt_curr_hrrn(struct rq *rq, struct task_struct *p, int flags)
{

	printk(KERN_ALERT "hrrn: check_preempt_currl\n");
}

/* =========================================================================
 *                  Implementation of Scheduler class functions
 */

/* Returns next task struct to be scheduled (Earliest deadline CASIO task) */
static struct task_struct *pick_next_task_hrrn(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	//printk(KERN_ALERT "hrrn: pick_next_task\n");
	
	struct sched_hrrn_entity *ce=NULL;
	struct task_struct *p=NULL;
	ce = highest_hrrn_task_rb_tree(&rq->hrrn);
	if(ce){
		p = container_of(ce, struct task_struct, hrrn);
		printk(KERN_ALERT "hrrn: pick_next_task: picked cid%d, pid%d\n", ce->hrrn_id, p->pid );
		return p;
	}
	
	return NULL;
}


/* Called when casio task becomes runnable */
/* Finds corresponding casio_task in the given rq */
/* inserts it into the rb-tree, updates deadline */
/* If task already in the structure, */
static void enqueue_task_hrrn(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_hrrn_entity *ce;
	char msg[CASIO_MSG_SIZE];

	if(p){
		ce=&(p->hrrn);

		ce->waiting_time = sched_clock() - ce->arrival_time;
		insert_hrrn_task_rb_tree(&rq->hrrn, ce);
		atomic_inc(&rq->hrrn.nr_running);
		snprintf(msg,CASIO_MSG_SIZE,"ENQ(cid%d:pid%d:dl%llu.%09llu)",ce->hrrn_id,p->pid,
			ce->waiting_time / 1000000000, ce->burst_time % 1000000000);
		//register_hrrn_event(sched_clock(), msg, CASIO_ENQUEUE);
	}
}

/* Called when casio task unrunnable */
/* Finds which rq's casio list it's in */
/* Removes it from rb tree */
/* If task exited, destroy the casio_task */
static void dequeue_task_hrrn(struct rq *rq, struct task_struct *p, int sleep)
{
	struct sched_hrrn_entity *ce;
	char msg[CASIO_MSG_SIZE];
	if(p){
		ce = &(p->hrrn);
		
		if(1){
			snprintf(msg,CASIO_MSG_SIZE,"DEQ(cid%d:pid%d:dl%llu.%09llu)",ce->hrrn_id,p->pid,
				ce->waiting_time / 1000000000, ce->burst_time % 1000000000);
			//register_hrrn_event(sched_clock(), msg, CASIO_DEQUEUE);

			remove_hrrn_task_rb_tree(&rq->hrrn, ce);

			atomic_dec(&rq->hrrn.nr_running);

		}
		else{
			printk(KERN_ALERT "error in dequeue_task_hrrn\n");
		}
	}

}

static void put_prev_task_hrrn(struct rq *rq, struct task_struct *prev) { }

static void task_tick_hrrn(struct rq *rq, struct task_struct *p, int queued)
{
	printk(KERN_ALERT "hrrn: task_tick cid%d, pid%d\n", p->hrrn.hrrn_id, p->pid);
	//check_preempt_curr_casio(rq, p);
}

static void set_curr_task_hrrn(struct rq *rq) { }


/*
 * When switching a task to RT, we may overload the runqueue
 * with RT tasks. In this case we try to push them off to
 * other runqueues.
 */
static void switched_to_hrrn(struct rq *rq, struct task_struct *p)
{
	printk(KERN_ALERT "hrrn: switched_to\n");
        /*
         * If we are already running, then there's nothing
         * that needs to be done. But if we are not running
         * we may need to preempt the current running task.
         * If that current running task is also an RT task
         * then see if we can move to another run queue.
         */
}


unsigned int get_rr_interval_hrrn(struct rq *rq, struct task_struct *task)
{
	printk(KERN_ALERT "hrrn: get_rr_interval\n");
	return 0;
}

static void yield_task_hrrn(struct rq *rq) { 
	printk(KERN_ALERT "hrrn: yield_task\n");
}


/*
 * Priority of the task has changed. This may cause
 * us to initiate a push or pull.
 */
static void prio_changed_hrrn(struct rq *rq, struct task_struct *p, int oldprio) { 
	printk(KERN_ALERT "hrrn: prio_changed\n");
}

static int select_task_rq_hrrn(struct task_struct *task, int task_cpu, int sd_flag, int flags)
{
	printk(KERN_ALERT "hrrn: select_task_rq\n");
//	struct rq *rq = task_rq(p);

	if (sd_flag != SD_BALANCE_WAKE)
		return smp_processor_id();

	return task_cpu;
}


static void set_cpus_allowed_hrrn(struct task_struct *p, const struct cpumask *new_mask) { }

/* Assumes rq->lock is held */
static void rq_online_hrrn(struct rq *rq) { }
/* Assumes rq->lock is held */
static void rq_offline_hrrn(struct rq *rq) { }

// OLD static void pre_schedule_casio(struct rq *rq, struct task_struct *prev) { } 
// OLD static void post_schedule_casio(struct rq *rq) { }
//
/*
 * If we are not running and we are not going to reschedule soon, we should
 * try to push tasks away now
 */
static void task_woken_hrrn(struct rq *rq, struct task_struct *p)
{
	printk(KERN_ALERT "hrrn: task_woken\n");
}

/*
 * When switch from the rt queue, we bring ourselves to a position
 * that we might want to pull RT tasks from other runqueues.
 */
static void switched_from_hrrn(struct rq *rq, struct task_struct *p) { }

/*
 * Simple, special scheduling class for the per-CPU casio tasks:
 */
const struct sched_class hrrn_sched_class = {
/* old sched_class_highest was set to these in kernel/sched/sched.h */
#ifdef CONFIG_SMP
	.next 			= &stop_sched_class,
#else
	.next 			= &casio_sched_class,
#endif
	.enqueue_task		= enqueue_task_hrrn,
	.dequeue_task		= dequeue_task_hrrn,

	.yield_task		= yield_task_hrrn,
	.check_preempt_curr	= check_preempt_curr_hrrn,

	.pick_next_task		= pick_next_task_hrrn,
	.put_prev_task		= put_prev_task_hrrn,

#ifdef CONFIG_SMP
	// OLD .load_balance		= load_balance_casio,
	// OLD .move_one_task		= move_one_task_casio,

	.select_task_rq		= select_task_rq_hrrn,
	
	.task_woken		= task_woken_hrrn,
	.set_cpus_allowed       = set_cpus_allowed_hrrn,

	.rq_online              = rq_online_hrrn,
	.rq_offline             = rq_offline_hrrn,
	
	// OLD .pre_schedule		= pre_schedule_casio,
	// OLD .post_schedule		= post_schedule_casio,
#endif

	.set_curr_task          = set_curr_task_hrrn,
	.task_tick		= task_tick_hrrn,

	.switched_from		= switched_from_hrrn,
	.switched_to		= switched_to_hrrn,
	.prio_changed		= prio_changed_hrrn,

	.get_rr_interval	= get_rr_interval_hrrn,

};
