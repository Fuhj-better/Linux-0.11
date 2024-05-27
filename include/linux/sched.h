#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64			/*task的数量*/		
#define HZ 100				/*可能是10ms产生一次时钟中断*/

#define FIRST_TASK task[0]		/*第一个task*/
#define LAST_TASK task[NR_TASKS-1]	/*最后一个task*/

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <signal.h>

#if (NR_OPEN > 32)
#error "Currently the close-on-exec-flags are in one word, max 32 files/proc" 	/*32位体系结构一个进程最多打开32个文件吗*/
#endif

#define TASK_RUNNING		0	/*运行*/
#define TASK_INTERRUPTIBLE	1	/*中断睡眠状态，收到信号唤醒*/	
#define TASK_UNINTERRUPTIBLE	2	/*不可中断睡眠，不能被大多数信号唤醒*/
#define TASK_ZOMBIE		3	/*僵尸进程，已经结束，等待父进程回收*/
#define TASK_STOPPED		4	/*停止状态，收到了SIGSTOP信号*/

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern int copy_page_tables(unsigned long from, unsigned long to, long size);	/*复制页表*/
extern int free_page_tables(unsigned long from, unsigned long size);		/*释放页表*/

extern void sched_init(void);	/*初始化调度器*/
extern void schedule(void);	/*核心调度*/
extern void trap_init(void);	/*初始化中断和异常处理*/
#ifndef PANIC
void panic(const char * str);	/*处理fatal错误，内核恐慌*/
#endif
extern int tty_write(unsigned minor,char * buf,int count);	/*写设备？*/

typedef int (*fn_ptr)();	/*函数指针 int f()*/

/*保存和恢复浮点计算单元*/
struct i387_struct {	
	long	cwd;
	long	swd;
	long	twd;
	long	fip;
	long	fcs;
	long	foo;
	long	fos;
	long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
};

/*保存和恢复任务段*/
struct tss_struct {
	long	back_link;	/* 16 high bits zero */
	long	esp0;
	long	ss0;		/* 16 high bits zero */
	long	esp1;
	long	ss1;		/* 16 high bits zero */
	long	esp2;
	long	ss2;		/* 16 high bits zero */
	long	cr3;
	long	eip;
	long	eflags;
	long	eax,ecx,edx,ebx;
	long	esp;
	long	ebp;
	long	esi;
	long	edi;
	long	es;		/* 16 high bits zero */
	long	cs;		/* 16 high bits zero */
	long	ss;		/* 16 high bits zero */
	long	ds;		/* 16 high bits zero */
	long	fs;		/* 16 high bits zero */
	long	gs;		/* 16 high bits zero */
	long	ldt;		/* 16 high bits zero */
	long	trace_bitmap;	/* bits: trace 0, bitmap 16-31 */
	struct i387_struct i387;
};

/*most important!!! The core struct of linux!!!*/
struct task_struct {
/* these are hardcoded - don't touch */
	long state;			/* -1 unrunnable, 0 runnable, >0 stopped */
	long counter;			/*时间片记数器*/
	long priority;			/*优先级*/
	long signal;			/*收到的信号*/
	struct sigaction sigaction[32]; /*32种信号处理行为*/
	long blocked;			/* bitmap of masked signals，信号掩码，用于进程阻塞信号 */
/* various fields */
	int exit_code;	/*task退出状态码*/
	unsigned long start_code,end_code,end_data,brk,start_stack;	/*代码段开始，代码段结束，数据段结束，brk应该是堆顶，start_stack栈开始*/
	long pid,father,pgrp,session,leader;				/*进程ID，父进程ID，进程组ID（属于哪个进程组），会话ID，进程组领导者ID*/
	unsigned short uid,euid,suid;					/*实际用户id,有效用户id(权限检查),保存的用户id（保存执行前的euid）*/
	unsigned short gid,egid,sgid;					/*同用户id*/
	long alarm;							/*进程的闹钟时间，进程多少秒后会受到SIGALRM信号*/
	long utime,stime,cutime,cstime,start_time;			/*用户态执行时间，核心态执行时间，子进程在用户态运行时间，子进程核心态运行时间，开始运行时刻*/
	unsigned short used_math;					/*是否使用数学协处理器*/
/* file system info */
	int tty;	/* -1 if no tty, so it must be signed */	/*进程使用tty终端的子设备号，-1未使用*/
	unsigned short umask;						/*文件创建属性的掩码*/
	struct m_inode * pwd;						/*当前工作目录inode指针*/
	struct m_inode * root;						/*根目前inode指针*/
	struct m_inode * executable;					/*可执行文件inode指针*/
	unsigned long close_on_exec;					/*执行时关闭文件句柄位图标志，不太懂，用于标记哪些文件描述符在exec系统调用后应该关闭*/
	struct file * filp[NR_OPEN];					/*文件结构指针表，最多 32 项。表项号即是文件描述符的值*/
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	struct desc_struct ldt[3];					/*局部描述符表，0空，1代码段，2数据和堆栈段*/
/* tss for this task */
	struct tss_struct tss;						/*任务状态段，每个进程一个*/
};

/*
 *  INIT_TASK is used to set up the first task table, touch at
 * your own risk!. Base=0, limit=0x9ffff (=640kB)
 */
#define INIT_TASK \
/* state etc */	{ 0,15,15, \
/* signals */	0,{{},},0, \
/* ec,brk... */	0,0,0,0,0,0, \
/* pid etc.. */	0,-1,0,0,0, \
/* uid etc */	0,0,0,0,0,0, \
/* alarm */	0,0,0,0,0,0, \
/* math */	0, \
/* fs info */	-1,0022,NULL,NULL,NULL,0, \
/* filp */	{NULL,}, \
	{ \
		{0,0}, \
/* ldt */	{0x9f,0xc0fa00}, \
		{0x9f,0xc0f200}, \
	}, \
/*tss*/	{0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir,\
	 0,0,0,0,0,0,0,0, \
	 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \
	 _LDT(0),0x80000000, \
		{} \
	}, \
}

extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern long volatile jiffies;
extern long startup_time;

#define CURRENT_TIME (startup_time+jiffies/HZ)

extern void add_timer(long jiffies, void (*fn)(void));
extern void sleep_on(struct task_struct ** p);
extern void interruptible_sleep_on(struct task_struct ** p);
extern void wake_up(struct task_struct ** p);

/*
 * Entry into gdt where to find first TSS. 0-nul, 1-cs, 2-ds, 3-syscall
 * 4-TSS0, 5-LDT0, 6-TSS1 etc ...
 */
#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))
#define ltr(n) __asm__("ltr %%ax"::"a" (_TSS(n)))
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))
#define str(n) \
__asm__("str %%ax\n\t" \
	"subl %2,%%eax\n\t" \
	"shrl $4,%%eax" \
	:"=a" (n) \
	:"a" (0),"i" (FIRST_TSS_ENTRY<<3))
/*
 * switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 * This also clears the TS-flag if the task we switched to has used
 * tha math co-processor latest.
 */
#define switch_to(n) {\
struct {long a,b;} __tmp; \
__asm__("cmpl %%ecx,current\n\t" \
	"je 1f\n\t" \
	"movw %%dx,%1\n\t" \
	"xchgl %%ecx,current\n\t" \
	"ljmp *%0\n\t" \
	"cmpl %%ecx,last_task_used_math\n\t" \
	"jne 1f\n\t" \
	"clts\n" \
	"1:" \
	::"m" (*&__tmp.a),"m" (*&__tmp.b), \
	"d" (_TSS(n)),"c" ((long) task[n])); \
}

#define PAGE_ALIGN(n) (((n)+0xfff)&0xfffff000)

#define _set_base(addr,base)  \
__asm__ ("push %%edx\n\t" \
	"movw %%dx,%0\n\t" \
	"rorl $16,%%edx\n\t" \
	"movb %%dl,%1\n\t" \
	"movb %%dh,%2\n\t" \
	"pop %%edx" \
	::"m" (*((addr)+2)), \
	 "m" (*((addr)+4)), \
	 "m" (*((addr)+7)), \
	 "d" (base) \
	)

#define _set_limit(addr,limit) \
__asm__ ("push %%edx\n\t" \
	"movw %%dx,%0\n\t" \
	"rorl $16,%%edx\n\t" \
	"movb %1,%%dh\n\t" \
	"andb $0xf0,%%dh\n\t" \
	"orb %%dh,%%dl\n\t" \
	"movb %%dl,%1\n\t" \
	"pop %%edx" \
	::"m" (*(addr)), \
	 "m" (*((addr)+6)), \
	 "d" (limit) \
	)

#define set_base(ldt,base) _set_base( ((char *)&(ldt)) , (base) )
#define set_limit(ldt,limit) _set_limit( ((char *)&(ldt)) , (limit-1)>>12 )

/**
#define _get_base(addr) ({\
unsigned long __base; \
__asm__("movb %3,%%dh\n\t" \
	"movb %2,%%dl\n\t" \
	"shll $16,%%edx\n\t" \
	"movw %1,%%dx" \
	:"=d" (__base) \
	:"m" (*((addr)+2)), \
	 "m" (*((addr)+4)), \
	 "m" (*((addr)+7)) \
        :"memory"); \
__base;})
**/

static inline unsigned long _get_base(char * addr)
{
         unsigned long __base;
         __asm__("movb %3,%%dh\n\t"
                 "movb %2,%%dl\n\t"
                 "shll $16,%%edx\n\t"
                 "movw %1,%%dx"
                 :"=&d" (__base)
                 :"m" (*((addr)+2)),
                  "m" (*((addr)+4)),
                  "m" (*((addr)+7)));
         return __base;
}

#define get_base(ldt) _get_base( ((char *)&(ldt)) )

#define get_limit(segment) ({ \
unsigned long __limit; \
__asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment)); \
__limit;})

#endif
