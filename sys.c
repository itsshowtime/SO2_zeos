/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS; 
}

int sys_getpid()
{
	return current()->PID;
}

int global_PID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -EAGAIN; 
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));
  
  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;
  
  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  __asm__ __volatile__ (
    "movl %%ebp, %0\n\t"
      : "=g" (register_ebp)
      : );
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);
  
  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

int sys_clone(void (*function)(void), void *stack){
  if(!access_ok(VERIFY_WRITE, function, sizeof(function)) || !access_ok(VERIFY_WRITE, stack, sizeof(stack))) return -EFAULT;
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;
  
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;

  lhcurrent=list_first(&freequeue);
  list_del(lhcurrent);
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);

  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));

  struct task_struct *tchild = list_entry(lhcurrent, struct task_struct, list);  
  int pos = ((unsigned int)tchild->dir_pages_baseAddr - (unsigned int)dir_pages)/ (sizeof(dir_pages[NR_TASKS]) * TOTAL_PAGES);
  dir_pages_ref[pos]++;

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;
  
  /* Map Parent's ebp to child's stack */
  int register_ebp;             /* frame pointer */
  __asm__ __volatile__ (
    "movl %%ebp, %0\n\t"
      : "=g" (register_ebp)
      : );
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);
  uchild->task.register_esp=register_ebp + sizeof(DWord);
  DWord temp_ebp=*(DWord*)register_ebp;
 
 
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Esto es importante porque es un clone  */
  uchild->stack[KERNEL_STACK_SIZE - 2] = (unsigned int)stack;
  uchild->stack[KERNEL_STACK_SIZE - 5] = (unsigned int)function;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);

  return uchild->task.PID;
}

int sys_sem_init(int n_sem, unsigned int value){
  
  if(n_sem < 0 || n_sem >= NR_SEM) return -EINVAL;
  if(semaphores[n_sem].own_PID != -1) return -EBUSY;

  semaphores[n_sem].own_PID = current()->PID;
  semaphores[n_sem].counter = value;
  INIT_LIST_HEAD(&semaphores[n_sem].blockedqueue);

  return 0;
}

int sys_sem_wait(int n_sem){
  if(n_sem < 0 || n_sem >= NR_SEM || semaphores[n_sem].own_PID == -1) return -EINVAL;

  if(semaphores[n_sem].counter > 0) --semaphores[n_sem].counter;
  else {
    update_process_state_rr(current(), &semaphores[n_sem].blockedqueue);
    sched_next_rr();


    if(semaphores[n_sem].own_PID == -1) return -EPERM;
  } 

  return 0;
}

int sys_sem_signal(int n_sem){
  if(n_sem < 0 || n_sem >= NR_SEM || semaphores[n_sem].own_PID == -1) return -EINVAL;

  if(list_empty(&semaphores[n_sem].blockedqueue)) ++semaphores[n_sem].counter;
  else {
    struct list_head *lh_sem = list_first(&semaphores[n_sem].blockedqueue);
    struct task_struct *ts_sem = list_head_to_task_struct(lh_sem);
    list_del(lh_sem);
    list_add_tail(lh_sem, &readyqueue);    
    ts_sem->state = ST_READY;
  }
  
  return 0;
}

int sys_sem_destroy(int n_sem){
  if(n_sem < 0 || n_sem >= NR_SEM || semaphores[n_sem].own_PID == -1) return -EINVAL;
  if(semaphores[n_sem].own_PID != current()->PID) return -EPERM;

  while(!list_empty(&semaphores[n_sem].blockedqueue)){
    struct list_head *lh_sem = list_first(&semaphores[n_sem].blockedqueue);
    struct task_struct *ts_sem = list_head_to_task_struct(lh_sem);

    list_del(lh_sem);
    list_add_tail(lh_sem, &readyqueue);
    ts_sem->state = ST_READY;
  }
  semaphores[n_sem].own_PID = -1;
 
  return 0;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;
	
	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}

int sys_read_keyboard(char *buff, int count){
  int total = 0;
  int i;
  if(!list_empty(&keyboardqueue)){
      update_process_state_rr(current(),&keyboardqueue);
      sched_next_rr(); 
  }
  while(total < count){
    if(kbuff_getsize() >= count-total){
      for(i = 0; i < count-total; i++){
        buff[total++] = kbuff_popchar();
      }
    }
    else if(kbuff_isFull()){
      for (i = 0; i < TAM_BUFF; i++){
        buff[total++] = kbuff_popchar();
      }
    }
    else { 
      //current ha d'anar al principi de la keyboardqueue
      update_process_state_rr(current(),&keyboardqueue);
      sched_next_rr();
    }
  }
  return count;
}

int sys_read(int fd, char *buf, int count){
  int ret;
  
  if(!access_ok(VERIFY_WRITE, buf, count)) return -EFAULT;
  if(fd != 0) return -EBADF;
  if(count < 0) return -EINVAL;
  if(buf == NULL) return -EFAULT;

  ret = sys_read_keyboard(buf, count);
  
  return ret;
}

extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  // en el task_switch y aqui, cuando vaya a eliminar el PCB hay que comprobar que no haya threads activos
  int pos = ((unsigned int)current()->dir_pages_baseAddr - (unsigned int)dir_pages)/ (sizeof(dir_pages[NR_TASKS]) * TOTAL_PAGES);
  dir_pages_ref[pos]--;
  if(dir_pages_ref[pos] == 0){
    for (i=0; i<NUM_PAG_DATA; i++)
    {
      free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
      del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
    }
  }
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  int sem;
  for(sem = 0; sem < NR_SEM; ++sem){
    if(semaphores[sem].own_PID == current()->PID) sys_sem_destroy(sem);
  }

  current()->PID=-1;
 
  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}
