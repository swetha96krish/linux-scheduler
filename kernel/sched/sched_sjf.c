#include "sched.h"
#define HIGHEST_PRIO 5
struct sjf_event_log sjf_event_log;

struct sjf_event_log * get_sjf_event_log(void)
{
	return &sjf_event_log;
}
void init_sjf_event_log(void)
{
	char msg_sjf[SJF_MSG_SIZE];
	sjf_event_log.lines=sjf_event_log.cursor=0;
	snprintf(msg_sjf,SJF_MSG_SIZE,"init_sjf_event_log:(%lu:%lu)", sjf_event_log.lines, sjf_event_log.cursor);
	register_sjf_event(sched_clock(), msg_sjf, SJF_MSG);

}
/* Logs an event if there's room */
void register_sjf_event(unsigned long long t, char *m, int a)
{

	if(sjf_event_log.lines < SJF_MAX_EVENT_LINES){
		sjf_event_log.sjf_event[sjf_event_log.lines].action=a;
		sjf_event_log.sjf_event[sjf_event_log.lines].timestamp=t;
		strncpy(sjf_event_log.sjf_event[sjf_event_log.lines].msg_sjf,m,SJF_MSG_SIZE-1);
		sjf_event_log.lines++;
		
		printk(KERN_ALERT "sjf_event: %s\n", m);
	}
	else{
		printk(KERN_ALERT "ERROR register_sjf_event full\n");
	}

}

void init_sjf_rq(struct sjf_rq *sjf_rq)
{
    int i;
    for(i=0;i<HIGHEST_PRIO;i++){
	sjf_rq[i].sjf_rb_root=RB_ROOT;
	INIT_LIST_HEAD(&sjf_rq[i].sjf_list_head);
	atomic_set(&sjf_rq[i].nr_running,0);
	}
}

void remove_sjf_task_rb_tree(struct sjf_rq *rq, struct sched_sjf_entity *p)
{
	rb_erase(&(p->sjf_rb_node),&(rq[p->sjf_prio].sjf_rb_root));
	p->sjf_rb_node.rb_left=p->sjf_rb_node.rb_right=NULL;
}

void insert_sjf_task_rb_tree(struct sjf_rq *rq, struct sched_sjf_entity *p)
{
	struct rb_node **node=NULL;
	struct rb_node *parent=NULL;
	struct sched_sjf_entity *entry=NULL;
	node=&rq[p->sjf_prio].sjf_rb_root.rb_node;
	while(*node!=NULL){
		parent=*node;
		entry=rb_entry(parent, struct sched_sjf_entity,sjf_rb_node);
		if(entry){
			if( p->sjf_bt < entry->sjf_bt){
				node=&parent->rb_left;
			}else{
				node=&parent->rb_right;
			}
		}
	}
	rb_link_node(&p->sjf_rb_node,parent,node);
	rb_insert_color(&p->sjf_rb_node,&rq[p->sjf_prio].sjf_rb_root);
}

struct sched_sjf_entity * shortest_job_sjf_task_rb_tree(struct sjf_rq *rq)
{
	struct rb_node *node=NULL;
	struct sched_sjf_entity *p=NULL;
	node=rq->sjf_rb_root.rb_node;
	if(node==NULL)
		return NULL;

	while(node->rb_left!=NULL){
		node=node->rb_left;
	}
	p=rb_entry(node, struct sched_sjf_entity,sjf_rb_node);
	return p;
}

static void check_preempt_curr_sjf(struct rq *rq, struct task_struct *p, int flags)
{

	printk(KERN_ALERT "sjf: check_preempt_currl\n");
}

static struct task_struct *pick_next_task_sjf(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{

	struct sched_sjf_entity *ce=NULL;
	struct task_struct *p=NULL;
	ce = shortest_job_sjf_task_rb_tree(rq->sjf);
	if(ce){
		p = container_of(ce, struct task_struct, sjf);
		printk(KERN_ALERT "sjf: pick_next_task: picked cid%d, pid%d\n", ce->sjf_id, p->pid );
		return p;
	}
	
	return NULL;
}

static void enqueue_task_sjf(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_sjf_entity *ce;
	char msg_sjf[SJF_MSG_SIZE];

	if(p){
		ce=&(p->sjf);

		insert_sjf_task_rb_tree(rq->sjf, ce);
		atomic_inc(&rq->sjf[p->sjf.sjf_prio].nr_running);
		snprintf(msg_sjf,SJF_MSG_SIZE,"ENQ(cid%d:pid%d:bt%d)",ce->sjf_id,p->pid,ce->sjf_bt);
		register_sjf_event(sched_clock(), msg_sjf, SJF_ENQUEUE);
	}
}

static void dequeue_task_sjf(struct rq *rq, struct task_struct *p, int sleep)
{
	struct sched_sjf_entity *ce;
	char msg_sjf[SJF_MSG_SIZE];
	if(p){
		ce = &(p->sjf);
		
		if(1){
			snprintf(msg_sjf,SJF_MSG_SIZE,"DEQ(cid%d:pid%d:bt%d)",ce->sjf_id,p->pid, ce->sjf_bt);
			register_sjf_event(sched_clock(), msg_sjf, SJF_DEQUEUE);
			remove_sjf_task_rb_tree(rq->sjf, ce);

			atomic_dec(&rq->sjf[p->sjf.sjf_prio].nr_running);

		}
		else{
			printk(KERN_ALERT "error in dequeue_task_sjf\n");
		}
	}

}

static void put_prev_task_sjf(struct rq *rq, struct task_struct *prev) { }

static void task_tick_sjf(struct rq *rq, struct task_struct *p, int queued)
{
	printk(KERN_ALERT "sjf: task_tick cid%d, pid%d\n", p->sjf.sjf_id, p->pid);
	//check_preempt_curr_casio(rq, p);
}

static void set_curr_task_sjf(struct rq *rq) { }

static void switched_to_sjf(struct rq *rq, struct task_struct *p)
{
	printk(KERN_ALERT "sjf: switched_to\n");
        /*
+         * If we are already running, then there's nothing
+         * that needs to be done. But if we are not running
+         * we may need to preempt the current running task.
+         * If that current running task is also an RT task
+         * then see if we can move to another run queue.
+         */
}


unsigned int get_rr_interval_sjf(struct rq *rq, struct task_struct *task)
{
	printk(KERN_ALERT "sjf: get_rr_interval\n");
	return 0;
}

static void yield_task_sjf(struct rq *rq) { 
	printk(KERN_ALERT "sjf: yield_task\n");
}

static void prio_changed_sjf(struct rq *rq, struct task_struct *p, int oldprio) { 
	printk(KERN_ALERT "sjf: prio_changed\n");
}

static int select_task_rq_sjf(struct task_struct *task, int task_cpu, int sd_flag, int flags)
{
	printk(KERN_ALERT "sjf: select_task_rq\n");
//	struct rq *rq = task_rq(p);

	if (sd_flag != SD_BALANCE_WAKE)
		return smp_processor_id();

	return task_cpu;
}

static void set_cpus_allowed_sjf(struct task_struct *p, const struct cpumask *new_mask) { }

/* Assumes rq->lock is held */
static void rq_online_sjf(struct rq *rq) { }
/* Assumes rq->lock is held */
static void rq_offline_sjf(struct rq *rq) { }

// OLD static void pre_schedule_casio(struct rq *rq, struct task_struct *prev) { } 
// OLD static void post_schedule_casio(struct rq *rq) { }
//
/*
+ * If we are not running and we are not going to reschedule soon, we should
+ * try to push tasks away now
+ */
static void task_woken_sjf(struct rq *rq, struct task_struct *p)
{
	printk(KERN_ALERT "sjf: task_woken\n");
}

static void switched_from_sjf(struct rq *rq, struct task_struct *p) { }

const struct sched_class sjf_sched_class = {
/* old sched_class_highest was set to these in kernel/sched/sched.h */
#ifdef CONFIG_SCHED_CASIO_POLICY
	.next 			= &casio_sched_class,
#else
#ifdef CONFIG_SMP
	.next 			= &dl_sched_class,
#else
	.next 			= &stop_sched_class,
#endif
#endif
	.enqueue_task		= enqueue_task_sjf,
	.dequeue_task		= dequeue_task_sjf,

	.yield_task		= yield_task_sjf,
	.check_preempt_curr	= check_preempt_curr_sjf,

	.pick_next_task		= pick_next_task_sjf,
	.put_prev_task		= put_prev_task_sjf,

#ifdef CONFIG_SMP
	// OLD .load_balance		= load_balance_casio,
	// OLD .move_one_task		= move_one_task_casio,

	.select_task_rq		= select_task_rq_sjf,
	
	.task_woken		= task_woken_sjf,
	.set_cpus_allowed       = set_cpus_allowed_sjf,

	.rq_online              = rq_online_sjf,
	.rq_offline             = rq_offline_sjf,
	
	// OLD .pre_schedule		= pre_schedule_casio,
	// OLD .post_schedule		= post_schedule_casio,
#endif

	.set_curr_task          = set_curr_task_sjf,
	.task_tick		= task_tick_sjf,

	.switched_from		= switched_from_sjf,
	.switched_to		= switched_to_sjf,
	.prio_changed		= prio_changed_sjf,

	.get_rr_interval	= get_rr_interval_sjf,

};

