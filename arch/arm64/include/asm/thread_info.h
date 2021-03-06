/*
 * Based on arch/arm/include/asm/thread_info.h
 *
 * Copyright (C) 2002 Russell King.
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_THREAD_INFO_H
#define __ASM_THREAD_INFO_H

#ifdef __KERNEL__

#include <linux/compiler.h>

#ifdef CONFIG_ARM64_4K_PAGES
#define THREAD_SIZE_ORDER	2
#elif defined(CONFIG_ARM64_16K_PAGES)
#define THREAD_SIZE_ORDER	0
#endif

#define THREAD_SIZE		16384
#define THREAD_START_SP		(THREAD_SIZE - 16)

#ifndef __ASSEMBLY__

struct task_struct;

#include <asm/stack_pointer.h>
#include <asm/types.h>
#include <asm/unistd.h>

typedef unsigned long mm_segment_t;

/*
 * low level task data that entry.S needs immediate access to.
 */
struct thread_info {
	unsigned long		flags;		/* low level flags */
	mm_segment_t		addr_limit;	/* address limit */
	int			preempt_count;	/* 0 => preemptable, <0 => bug */
#ifdef CONFIG_ALT_SYSCALL
	unsigned int		nr_syscalls;
	const void		*sys_call_table;
#ifdef CONFIG_COMPAT
	unsigned int		compat_nr_syscalls;
	const void		*compat_sys_call_table;
#endif
#endif
};

#ifdef CONFIG_ALT_SYSCALL
#ifdef CONFIG_COMPAT
extern void * const compat_sys_call_table[];
#define INIT_THREAD_INFO_SYSCALL_COMPAT					\
	.compat_nr_syscalls	= __NR_compat_syscalls,			\
	.compat_sys_call_table	= &compat_sys_call_table,
#else
#define INIT_THREAD_INFO_SYSCALL_COMPAT
#endif
#define INIT_THREAD_INFO_SYSCALL					\
	.nr_syscalls		= __NR_syscalls,			\
	.sys_call_table		= &sys_call_table,			\
	INIT_THREAD_INFO_SYSCALL_COMPAT
#else
#define INIT_THREAD_INFO_SYSCALL
#endif

#define INIT_THREAD_INFO(tsk)						\
{									\
	.preempt_count	= INIT_PREEMPT_COUNT,				\
	.addr_limit	= KERNEL_DS,					\
	INIT_THREAD_INFO_SYSCALL					\
}

#define init_stack		(init_thread_union.stack)

#define thread_saved_pc(tsk)	\
	((unsigned long)(tsk->thread.cpu_context.pc))
#define thread_saved_sp(tsk)	\
	((unsigned long)(tsk->thread.cpu_context.sp))
#define thread_saved_fp(tsk)	\
	((unsigned long)(tsk->thread.cpu_context.fp))

#endif

/*
 * thread information flags:
 *  TIF_SYSCALL_TRACE	- syscall trace active
 *  TIF_SYSCALL_TRACEPOINT - syscall tracepoint for ftrace
 *  TIF_SYSCALL_AUDIT	- syscall auditing
 *  TIF_SECOMP		- syscall secure computing
 *  TIF_SIGPENDING	- signal pending
 *  TIF_NEED_RESCHED	- rescheduling necessary
 *  TIF_NOTIFY_RESUME	- callback before returning to user
 *  TIF_USEDFPU		- FPU was used by this task this quantum (SMP)
 */
#define TIF_SIGPENDING		0
#define TIF_NEED_RESCHED	1
#define TIF_NOTIFY_RESUME	2	/* callback before returning to user */
#define TIF_FOREIGN_FPSTATE	3	/* CPU's FP state is not current's */
#define TIF_NOHZ		7
#define TIF_SYSCALL_TRACE	8
#define TIF_SYSCALL_AUDIT	9
#define TIF_SYSCALL_TRACEPOINT	10
#define TIF_SECCOMP		11
#define TIF_MEMDIE		18	/* is terminating due to OOM killer */
#define TIF_FREEZE		19
#define TIF_RESTORE_SIGMASK	20
#define TIF_SINGLESTEP		21
#define TIF_32BIT		22	/* 32bit process */

#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_FOREIGN_FPSTATE	(1 << TIF_FOREIGN_FPSTATE)
#define _TIF_NOHZ		(1 << TIF_NOHZ)
#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_SYSCALL_AUDIT	(1 << TIF_SYSCALL_AUDIT)
#define _TIF_SYSCALL_TRACEPOINT	(1 << TIF_SYSCALL_TRACEPOINT)
#define _TIF_SECCOMP		(1 << TIF_SECCOMP)
#define _TIF_32BIT		(1 << TIF_32BIT)

#define _TIF_WORK_MASK		(_TIF_NEED_RESCHED | _TIF_SIGPENDING | \
				 _TIF_NOTIFY_RESUME | _TIF_FOREIGN_FPSTATE)

#define _TIF_SYSCALL_WORK	(_TIF_SYSCALL_TRACE | _TIF_SYSCALL_AUDIT | \
				 _TIF_SYSCALL_TRACEPOINT | _TIF_SECCOMP | \
				 _TIF_NOHZ)

#endif /* __KERNEL__ */
#endif /* __ASM_THREAD_INFO_H */
