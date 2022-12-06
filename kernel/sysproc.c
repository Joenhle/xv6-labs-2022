#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 base_va = 0, mask_va = 0;
  uint32 mask = 0, i = -1;
  int page_num = 0;
  pte_t* pte;
  struct proc* p = myproc();

  argaddr(0, &base_va);
  argint(1, &page_num);
  argaddr(2, &mask_va);

  if (page_num <= 0 || page_num > 32) {
    panic("the page_num should be grater than 0");
  }

  uint64 a = PGROUNDDOWN(base_va);
  uint64 last = PGROUNDDOWN(base_va + page_num * PGSIZE);
  for (;;) {
    i++;
    if ((pte = walk(p->pagetable, a, 0)) == 0) {
      continue;
    }
    if (*pte & PTE_A) {
      mask |= (1 << i);
      *pte &= (~PTE_A);
    }
    if (a == last) {
      break;
    }
    a += PGSIZE;
  }
  if(copyout(p->pagetable, mask_va, (char*)&mask, sizeof(mask)) != 0) {
    panic("copyout error");
  }
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
