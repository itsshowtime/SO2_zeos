/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int quantum_left; // needed for sys_get_stats

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

int sys_ni_syscall()
{
	return -ENOSYS;
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork()
{
  return 0;
}

int sys_fork()
{
  //int PID=-1;
  
  //Get a free task_struct for the process. If there is no space for a new process, an error will be returned.
  if (list_empty(&freequeue)) return -ENOMEM;
  
  struct list_head *new_pcb = list_first(&freequeue);
  list_del(new_pcb);

  //Inherit system data
  union task_union *child = (union task_union*)list_head_to_task_struct(new_pcb);
  union task_union *parent = (union task_union*)current();
  struct task_struct *child_pcb = &(child->task);
  struct task_struct *parent_pcb = &(parent->task);
  copy_data(parent, child, sizeof(union task_union));

  //Initialize dir
  allocate_DIR(child_pcb);

  // Search physical pages for the child, return error if not enough
  int user_data_frames[NUM_PAG_DATA];
  int pag;

  for (pag = 0; pag < NUM_PAG_DATA; pag++) {
    if ((user_data_frames[pag] = alloc_frame()) == -1) {
      while (pag >= 0) free_frame(user_data_frames[pag--]);
      list_add_tail(&(child_pcb->list), &freequeue);
      //update_stats(current(), RSYS_TO_RUSER);
      return -ENOMEM;
    }
  }

  // Inherit user code
  page_table_entry* child_pt = get_PT(child_pcb);
  page_table_entry* parent_pt = get_PT(parent_pcb);

  for(pag = 0; pag < PAG_LOG_INIT_DATA; pag++)
  {
     set_ss_pag(child_pt, pag, get_frame(parent_pt, pag));
  }

  //Inherit user data
  for (pag = 0; pag < NUM_PAG_DATA; pag++) {
    set_ss_pag(child_pt, PAG_LOG_INIT_DATA+pag, user_data_frames[pag]);

    unsigned int log_addr = (pag+PAG_LOG_INIT_DATA) * PAGE_SIZE;
    set_ss_pag(parent_pt, pag+PAG_LOG_INIT_DATA+NUM_PAG_DATA, user_data_frames[pag]);
    copy_data((void *)(log_addr), (void *)(log_addr+PAGE_SIZE*NUM_PAG_DATA), PAGE_SIZE);
    del_ss_pag(parent_pt, pag+PAG_LOG_INIT_DATA+NUM_PAG_DATA);
  }

  //flush the entire TLB
  set_cr3(get_DIR(current()));

  child_pcb->PID=++gPID;
  //child_pcb->task_state=ST_READY;

  int reg_ebp;
  __asm__ __volatile__(
        "mov %%ebp,%0\n"
        :"=g"(reg_ebp)
  ); 
  
  reg_ebp=(reg_ebp - (int)current()) + (int)(child);

  child_pcb->ebp_esp_reg=reg_ebp + sizeof(DWord);
  
  DWord temp_ebp=*(DWord*)reg_ebp;
  child_pcb->ebp_esp_reg-=sizeof(DWord);
  *(DWord*)(child_pcb->ebp_esp_reg)=(DWord)&ret_from_fork;
  child_pcb->ebp_esp_reg-=sizeof(DWord);
  *(DWord*)(child_pcb->ebp_esp_reg=reg_ebp)=temp_ebp; 

  list_add_tail(&(child_pcb->list), &readyqueue);
  return child_pcb->PID;
}

int sys_write(int fd, char *buffer, int size)
{
  //check fd
  int ret = check_fd(fd,ESCRIPTURA);  
  if(ret) return ret;
  //check buffer
  if(buffer == NULL) return -EFAULT; 
  
  //check size
  if(size < 0)
    {
      return -EINVAL; 
    }
  
  //write
  ret = sys_write_console(buffer, size);
  return ret;
}

int sys_gettime()
{
  return zeos_ticks;
}

int sys_get_stats(int pid, struct stats *st)
{

  int i;
  if(!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT;
  if(pid<0) return -EINVAL;

  for(i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=quantum_left;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH;
}

void sys_exit()
{
  page_table_entry *proc_PT = get_PT(current()); 
  int i;
  for(i=0; i<NUM_PAG_DATA; ++i)
  {
  
    free_frame(get_frame(proc_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(proc_PT, PAG_LOG_INIT_DATA+i);
  }

  list_add_tail(&(current()->list),&freequeue);

  current()->PID = -1;

  sched_next_rr();
}

