#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
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

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
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
  return  xticks;
}

uint64
sys_trace(void)
{
    int n;
    if(argint(0,&n)<0){
        return -1;
    }
    //将本进程要追踪的系统调用信息存储到PCB中
    myproc()->mask = n;
    return 0;
}

uint64
sys_sysinfo(void){
    uint64 addr;
    struct proc *p = myproc();
    struct sysinfo info;

    //获取a0即传入的第一个参数，即要保存内容的用户空间地址
    if(argaddr(0,&addr)<0){
        return -1;
    }

    //获取信息并填入info中
    info.freemem = calfreemem();
    info.nproc = calnproc();
    info.freefd = calfreefd();

    //从内核空间复制到用户空间
    //copyout各参数意义：
    //pagetable_t pagetable:要复制到的目标进程
    //char *dst:要复制到的目标进程中的地址
    //uint64 srcva:要复制的内容地址
    //uint64 len:要复制的长度
    if(copyout(p->pagetable,addr,(char *)&info,sizeof(info))<0){
        return -1;
    }

    return 0;
}
