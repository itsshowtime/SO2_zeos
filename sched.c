/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>
#include <stats.h>
/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
};
#endif

extern struct list_head blocked;

int gPID=1; // Global PID

#define DEF_QUANTUM 10 // Default quantum
int quantum_left = 0;

struct list_head freequeue;  // queue of the available PCBs
struct list_head readyqueue; // queue of candidates to use CPU

struct task_struct *idle_task=NULL; //4.4.1 5)

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_idle (void) // task 0
{
  // Get an available task_union from the freequeue to contain the characteristics of this process
  struct list_head *fproc = list_first(&freequeue);
  //4.4.1 6)
  idle_task = list_head_to_task_struct(fproc);
  list_del(fproc);

  //Assign PID 0 to the process
  idle_task -> PID = 0;
  
  allocate_DIR(idle_task);
 
  //4) 6.1) 6.2) 6.3) 
  union task_union *utask = (union task_union*) idle_task;
  utask -> stack[KERNEL_STACK_SIZE-1] = (unsigned long)&cpu_idle;
  utask -> stack[KERNEL_STACK_SIZE-2] = 0;
  idle_task -> ebp_esp_reg = (int)&(utask->stack[KERNEL_STACK_SIZE-2]);  
}

void init_task1(void)
{
  // Get the init process
  struct list_head *fproc = list_first(&freequeue);
  struct task_struct *iproc = list_head_to_task_struct(fproc);
  list_del(fproc);
  
  // Assign PID 1
  iproc -> PID = 1;
  // 2) 3)
  allocate_DIR(iproc);
  set_user_pages(iproc);
 
  // 4) 5) 
  tss.esp0 = KERNEL_ESP((union task_union*)iproc);
  set_cr3(get_DIR(iproc));  

}

void inner_task_switch(union task_union *new){

  page_table_entry *new_DIR = get_DIR(&new -> task);
  // Update the TSS to make it point to the new_task system stack
  tss.esp0 = KERNEL_ESP(new);
  // Change the user address space by updating the current page directory
  set_cr3(new_DIR);
  // Stores the current ebp, restores esp in the new PCB, restores ebp from the stack, return 
  __asm__ __volatile__ (
        "mov %%ebp,%0\n"
        "movl %1, %%esp\n"
        "popl %%ebp\n"
        "ret\n"
        : "=g" (current()->ebp_esp_reg)
        : "r"  (new->task.ebp_esp_reg)
	);
}

void task_switch(union task_union *new){
//new =  pointer to the task_union of the process that will be executed
   __asm__ __volatile__ (
  	"pushl %esi\n\t"
	"pushl %edi\n\t"
	"pushl %ebx\n\t"
	);
  inner_task_switch(new);
  __asm__ __volatile__ (
  	"popl %ebx\n\t"
	"popl %edi\n\t"
	"popl %esi\n\t"
	);
}

void init_sched(){

  INIT_LIST_HEAD(&freequeue);
  int i = 0;
  for(i = 0; i < NR_TASKS; ++i) // NR_TASKS+1 ????
  {
    task[i].task.PID=-1;
    list_add_tail(&(task[i].task.list), &freequeue);
  }

  INIT_LIST_HEAD(&readyqueue);

}

void update_stats(unsigned long *v, unsigned long *elapsed)
{
  unsigned long current_ticks;
  current_ticks=get_ticks();
  
  *v += current_ticks - *elapsed;
  
  *elapsed=current_ticks;
}

int get_quantum(struct task_struct *t)
{
  return t->total_quantum;
}

void set_quantum(struct task_struct *t, int new_quantum)
{
  t->total_quantum = new_quantum;
}

void update_sched_data_rr(void)
{
  quantum_left--;
}

int needs_sched_rr(void)
{
  if((quantum_left==0)&&(!list_empty(&readyqueue))) return 1;
  if( quantum_left==0) quantum_left=get_quantum(current());
  return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
  if(t->state!=ST_RUN) list_del(&(t->list));
  if(dst_queue!=NULL)
  {
    list_add_tail(&(t->list), dst_queue);
    if(dst_queue!=&readyqueue) t->state = ST_BLOCKED;
    else
    {
      update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
    }
  }
  else t->state=ST_RUN;
}

void sched_next_rr(void)
{

  struct list_head *nproc;
  struct task_struct *ntask; 

  nproc = list_first(&readyqueue);
 
  if(nproc)
  {
  list_del(nproc);
  ntask = list_head_to_task_struct(nproc);
  }
  else ntask = idle_task;

  ntask->state=ST_RUN;
  quantum_left = get_quantum(ntask);

  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
  update_stats(&(ntask->p_stats.ready_ticks), &(ntask->p_stats.elapsed_total_ticks));
  ntask->p_stats.total_trans++;

  task_switch((union task_union*)ntask);
}

void schedule()
{
  update_sched_data_rr();
  if(needs_sched_rr())
  {
    update_process_state_rr(current(),&readyqueue);
    sched_next_rr();
  }
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}
