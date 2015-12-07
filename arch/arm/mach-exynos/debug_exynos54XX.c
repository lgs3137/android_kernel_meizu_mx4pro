/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * EXYNOS54XX - ARM Debug Architecture v7.1 support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/smp.h>
#include <linux/spinlock.h>

#include <plat/cpu.h>

#include <asm/cputype.h>

#include <mach/cpufreq.h>
#include <mach/regs-pmu.h>
#include <mach/debug.h>

/* With Trace32 attached debuging, T32 must set FLAG_T32_EN as true. */
bool FLAG_T32_EN = false;

static void __iomem *dbg_regs[CORE_CNT * CLUSTER_CNT];
static DEFINE_RAW_SPINLOCK(debug_lock);

static unsigned int comm_stat[CORE_CNT * CLUSTER_CNT][NR_COMM_REG];
static unsigned int break_stat[NR_BRK_REG];
static int flag_sleep[CORE_CNT * CLUSTER_CNT];

static void debug_lock_clear(int core)
{
	writel(UNLOCK_MAGIC, dbg_regs[core] + OFFSET_LAR);
}

inline void os_lock_set(void)
{
	/* DBGOSLAR */
	asm volatile("mcr p14, 0, %0, c1, c0, 4" : : "r" (UNLOCK_MAGIC));
}

inline void os_lock_clear(void)
{
	/* DBGOSLAR */
	asm volatile("mcr p14, 0, %0, c1, c0, 4" : : "r" (0x1));
}

inline void exec_ISB(void)
{
	asm volatile("mcr p15, 0, r0, c7, c5, 4" : : );
}

inline void break_stat_save(void)
{
	pr_debug("%s:\n", __func__);

	asm volatile(
		"mrc p14, 0, %0, c0, c0, 5\n"
		"mrc p14, 0, %1, c0, c1, 5\n"
		"mrc p14, 0, %2, c0, c2, 5\n"
		"mrc p14, 0, %3, c0, c3, 5\n"
		"mrc p14, 0, %4, c0, c4, 5\n"
		"mrc p14, 0, %5, c0, c5, 5\n"
		"mrc p14, 0, %6, c0, c0, 4\n"
		"mrc p14, 0, %7, c0, c1, 4\n"
		"mrc p14, 0, %8, c0, c2, 4\n"
		"mrc p14, 0, %9, c0, c3, 4\n"
		"mrc p14, 0, %10, c0, c4, 4\n"
		"mrc p14, 0, %11, c0, c5, 4\n"
		:
		"=r" (break_stat[DBGBCR0]),
		"=r" (break_stat[DBGBCR1]),
		"=r" (break_stat[DBGBCR2]),
		"=r" (break_stat[DBGBCR3]),
		"=r" (break_stat[DBGBCR4]),
		"=r" (break_stat[DBGBCR5]),
		"=r" (break_stat[DBGBVR0]),
		"=r" (break_stat[DBGBVR1]),
		"=r" (break_stat[DBGBVR2]),
		"=r" (break_stat[DBGBVR3]),
		"=r" (break_stat[DBGBVR4]),
		"=r" (break_stat[DBGBVR5])
	);

	asm volatile(
		"mrc p14, 0, %0, c0, c0, 6\n"
		"mrc p14, 0, %1, c0, c1, 6\n"
		"mrc p14, 0, %2, c0, c2, 6\n"
		"mrc p14, 0, %3, c0, c3, 6\n"
		"mrc p14, 0, %4, c0, c0, 7\n"
		"mrc p14, 0, %5, c0, c1, 7\n"
		"mrc p14, 0, %6, c0, c2, 7\n"
		"mrc p14, 0, %7, c0, c3, 7\n"
		"mrc p14, 0, %8, c0, c7, 0\n"
		:
		"=r" (break_stat[DBGWVR0]),
		"=r" (break_stat[DBGWVR1]),
		"=r" (break_stat[DBGWVR2]),
		"=r" (break_stat[DBGWVR3]),
		"=r" (break_stat[DBGWCR0]),
		"=r" (break_stat[DBGWCR1]),
		"=r" (break_stat[DBGWCR2]),
		"=r" (break_stat[DBGWCR3]),
		"=r" (break_stat[DBGVCR])
	);

	pr_debug("%s: done\n", __func__);
}

inline void break_stat_restore(void)
{
	int cpu;

	pr_debug("%s:\n", __func__);

	for (cpu = 0; cpu < CORE_CNT * CLUSTER_CNT; cpu++)
		if (flag_sleep[cpu] == 0)
			break;

	if (cpu < CORE_CNT * CLUSTER_CNT) {
		void __iomem *dbg_reg;

		dbg_reg = dbg_regs[cpu];

		asm volatile(
			"mcr p14, 0, %0, c0, c0, 5\n"
			"mcr p14, 0, %1, c0, c1, 5\n"
			"mcr p14, 0, %2, c0, c2, 5\n"
			"mcr p14, 0, %3, c0, c3, 5\n"
			"mcr p14, 0, %4, c0, c4, 5\n"
			"mcr p14, 0, %5, c0, c5, 5\n"
			"mcr p14, 0, %6, c0, c0, 4\n"
			"mcr p14, 0, %7, c0, c1, 4\n"
			"mcr p14, 0, %8, c0, c2, 4\n"
			"mcr p14, 0, %9, c0, c3, 4\n"
			"mcr p14, 0, %10, c0, c4, 4\n"
			"mcr p14, 0, %11, c0, c5, 4\n"
			: :
			"r" (readl(dbg_reg + OFFSET_DBGBCR0)),
			"r" (readl(dbg_reg + OFFSET_DBGBCR1)),
			"r" (readl(dbg_reg + OFFSET_DBGBCR2)),
			"r" (readl(dbg_reg + OFFSET_DBGBCR3)),
			"r" (readl(dbg_reg + OFFSET_DBGBCR4)),
			"r" (readl(dbg_reg + OFFSET_DBGBCR5)),
			"r" (readl(dbg_reg + OFFSET_DBGBVR0)),
			"r" (readl(dbg_reg + OFFSET_DBGBVR1)),
			"r" (readl(dbg_reg + OFFSET_DBGBVR2)),
			"r" (readl(dbg_reg + OFFSET_DBGBVR3)),
			"r" (readl(dbg_reg + OFFSET_DBGBVR4)),
			"r" (readl(dbg_reg + OFFSET_DBGBVR5))
		);

		asm volatile(
			"mcr p14, 0, %0, c0, c0, 6\n"
			"mcr p14, 0, %1, c0, c1, 6\n"
			"mcr p14, 0, %2, c0, c2, 6\n"
			"mcr p14, 0, %3, c0, c3, 6\n"
			"mcr p14, 0, %4, c0, c0, 7\n"
			"mcr p14, 0, %5, c0, c1, 7\n"
			"mcr p14, 0, %6, c0, c2, 7\n"
			"mcr p14, 0, %7, c0, c3, 7\n"
			"mcr p14, 0, %8, c0, c7, 0\n"
			: :
			"r" (readl(dbg_reg + OFFSET_DBGWVR0)),
			"r" (readl(dbg_reg + OFFSET_DBGWVR1)),
			"r" (readl(dbg_reg + OFFSET_DBGWVR2)),
			"r" (readl(dbg_reg + OFFSET_DBGWVR3)),
			"r" (readl(dbg_reg + OFFSET_DBGWCR0)),
			"r" (readl(dbg_reg + OFFSET_DBGWCR1)),
			"r" (readl(dbg_reg + OFFSET_DBGWCR2)),
			"r" (readl(dbg_reg + OFFSET_DBGWCR3)),
			"r" (readl(dbg_reg + OFFSET_DBGVCR))
		);
	} else {
		asm volatile(
			"mcr p14, 0, %0, c0, c0, 5\n"
			"mcr p14, 0, %1, c0, c1, 5\n"
			"mcr p14, 0, %2, c0, c2, 5\n"
			"mcr p14, 0, %3, c0, c3, 5\n"
			"mcr p14, 0, %4, c0, c4, 5\n"
			"mcr p14, 0, %5, c0, c5, 5\n"
			"mcr p14, 0, %6, c0, c0, 4\n"
			"mcr p14, 0, %7, c0, c1, 4\n"
			"mcr p14, 0, %8, c0, c2, 4\n"
			"mcr p14, 0, %9, c0, c3, 4\n"
			"mcr p14, 0, %10, c0, c4, 4\n"
			"mcr p14, 0, %11, c0, c5, 4\n"
			: :
			"r" (break_stat[DBGBCR0]),
			"r" (break_stat[DBGBCR1]),
			"r" (break_stat[DBGBCR2]),
			"r" (break_stat[DBGBCR3]),
			"r" (break_stat[DBGBCR4]),
			"r" (break_stat[DBGBCR5]),
			"r" (break_stat[DBGBVR0]),
			"r" (break_stat[DBGBVR1]),
			"r" (break_stat[DBGBVR2]),
			"r" (break_stat[DBGBVR3]),
			"r" (break_stat[DBGBVR4]),
			"r" (break_stat[DBGBVR5])
		);

		asm volatile(
			"mcr p14, 0, %0, c0, c0, 6\n"
			"mcr p14, 0, %1, c0, c1, 6\n"
			"mcr p14, 0, %2, c0, c2, 6\n"
			"mcr p14, 0, %3, c0, c3, 6\n"
			"mcr p14, 0, %4, c0, c0, 7\n"
			"mcr p14, 0, %5, c0, c1, 7\n"
			"mcr p14, 0, %6, c0, c2, 7\n"
			"mcr p14, 0, %7, c0, c3, 7\n"
			"mcr p14, 0, %8, c0, c7, 0\n"
			: :
			"r" (break_stat[DBGWVR0]),
			"r" (break_stat[DBGWVR1]),
			"r" (break_stat[DBGWVR2]),
			"r" (break_stat[DBGWVR3]),
			"r" (break_stat[DBGWCR0]),
			"r" (break_stat[DBGWCR1]),
			"r" (break_stat[DBGWCR2]),
			"r" (break_stat[DBGWCR3]),
			"r" (break_stat[DBGVCR])
		);
	}

	pr_debug("%s: done\n", __func__);
}

inline void debug_register_save(int cpu)
{
	pr_debug("%s: cpu : %d\n", __func__, cpu);

	asm volatile(
		"mrc p14, 0, %0, c0, c2, 2\n"
		"mrc p14, 0, %1, c0, c6, 0\n"
		:
		"=r" (comm_stat[cpu][DBGDSCR]),
		"=r" (comm_stat[cpu][DBGWFAR])
	);

	raw_spin_lock(&debug_lock);
	flag_sleep[cpu] = 1;
	break_stat_save();
	raw_spin_unlock(&debug_lock);

	/* In save sequence, read DBGCLAIMCLR. */
	asm volatile(
		"mrc p14, 0, %0, c7, c9, 6"
		:
		"=r" (comm_stat[cpu][DBGCLAIM])
	);

	asm volatile(
		"mrc p14, 0, %0, c0, c3, 2\n"
		"mrc p14, 0, %1, c0, c0, 2\n"
		:
		"=r" (comm_stat[cpu][DBGTRTX]),
		"=r" (comm_stat[cpu][DBGTRRX])
	);

	comm_stat[cpu][FLAG_SAVED] = 1;

	pr_debug("%s: %d done\n", __func__, cpu);
}

inline void debug_register_restore(int cpu)
{
	pr_debug("%s:\n", __func__);

	if (!comm_stat[cpu][FLAG_SAVED])
		goto pass;

	asm volatile(
		"mcr p14, 0, %0, c0, c2, 2\n"
		"mcr p14, 0, %1, c0, c6, 0\n"
		: :
		"r" (comm_stat[cpu][DBGDSCR] | (1 << 14)),
		"r" (comm_stat[cpu][DBGWFAR])
	);

	raw_spin_lock(&debug_lock);
	break_stat_restore();
	flag_sleep[cpu] = 0;
	raw_spin_unlock(&debug_lock);

	/* In save sequence, write DBGCLAIMSET. */
	asm volatile(
		"mcr p14, 0, %0, c7, c8, 6"
		: :
		"r" (comm_stat[cpu][DBGCLAIM])
	);

	asm volatile(
		"mcr p14, 0, %0, c0, c3, 2\n"
		"mcr p14, 0, %1, c0, c0, 2\n"
		: :
		"r" (comm_stat[cpu][DBGTRTX]),
		"r" (comm_stat[cpu][DBGTRRX])
	);

pass:
	comm_stat[cpu][FLAG_SAVED] = 0;

	pr_debug("%s: %d done\n", __func__, cpu);
}

static void armdebug_suspend_cpu(int cpu)
{
	pr_debug("%s: cpu %d\n", __func__, cpu);

	/*(C7.3.3) Start v7.1 Debug OS Save sequence */
	/* 1. Set the OS Lock by writing the key value. */
	os_lock_set();

	/* 2. Execute an ISB instruction. */
	exec_ISB();

	/* 3. Save the values. */
	debug_register_save(cpu);

	pr_debug("%s: %d done\n", __func__, cpu);
}

static void armdebug_resume_cpu(int cpu)
{
	pr_debug("%s: cpu %d\n", __func__, cpu);

	/*( C7.3.3) Start v7.1 Debug OS Restore sequence */

	if (!comm_stat[cpu][FLAG_SAVED]) {
		unsigned int reg_DSCR;

		/* For restore breakpoint register from other live cpu. */
		debug_lock_clear(cpu);
		os_lock_set();
		raw_spin_lock(&debug_lock);
		break_stat_restore();
		flag_sleep[cpu] = 0;
		raw_spin_unlock(&debug_lock);

		/* Keep DSCR[14] as 1. */
		asm volatile("mrc p14, 0, %0, c0, c2, 2"
				: "=r"(reg_DSCR));
		asm volatile("mcr p14, 0, %0, c0, c2, 2"
				: : "r"(reg_DSCR | (1 << 14)));

		os_lock_clear();
	} else {
		/* 5-1. Clear LAR if cpu is A7. */
		debug_lock_clear(cpu);

		/* 1. Set the OS Lock by writing the key value. */
		os_lock_set();

		/* 2. Execute an ISB instruction. */
		exec_ISB();

		/* 3. Restore the values. */
		debug_register_restore(cpu);

		/* 4. Execute an ISB instruction. */
		exec_ISB();

		/* 5. Clear the OS Lock by writing any non-key value. */
		os_lock_clear();

		/* 6. Execute a Context synchronization operation.
		 *  - the execution of an ISB instruction.
		 */
		exec_ISB();
	}

	pr_debug("%s: %d done\n", __func__, cpu);
}

static int armdebug_cpu_pm_notifier(struct notifier_block *self,
		unsigned long cmd, void *v)
{
	int cpu = smp_processor_id();

	switch (cmd) {
	case CPU_PM_ENTER:
		armdebug_suspend_cpu(cpu);
		break;
	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		armdebug_resume_cpu(cpu);
		break;
	case CPU_CLUSTER_PM_ENTER:
		break;
	case CPU_CLUSTER_PM_ENTER_FAILED:
	case CPU_CLUSTER_PM_EXIT:
		break;
	}

	return NOTIFY_OK;
}

static int __cpuinit armdebug_cpu_notifier(struct notifier_block *nfb,
		unsigned long action, void *hcpu)
{
	int cpu = (unsigned long)hcpu;

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_STARTING:
	case CPU_DOWN_FAILED:
		armdebug_resume_cpu(cpu);
		break;
	case CPU_DYING:
		armdebug_suspend_cpu(cpu);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata armdebug_cpu_pm_notifier_block = {
	.notifier_call = armdebug_cpu_pm_notifier,
};

static struct notifier_block __cpuinitdata armdebug_cpu_notifier_block = {
	.notifier_call = armdebug_cpu_notifier,
};

static int __init armdebug_exynos5_init(void)
{
	int i, ret;

	if(!FLAG_T32_EN) {
		pr_err("%s : FLAG_T32_EN is not true.\n", __func__);
		return -ENODEV;
	}

	if(!soc_is_exynos5430()) {
		pr_err("%s : SOC is not support.\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < CORE_CNT * CLUSTER_CNT; i++)
		if (i / CORE_CNT == CA7)
			dbg_regs[i] = ioremap(
				ROM_CA7_CPU0 + (OFFSET_ROM * (i % CORE_CNT)),
				SZ_4K);
		else
			dbg_regs[i] = ioremap(
				ROM_CA15_CPU0 + (OFFSET_ROM * (i % CORE_CNT)),
				SZ_4K);

	for (i = 0; i < CORE_CNT * CLUSTER_CNT; i++)
		if (!dbg_regs[i]) {
			pr_err("%s : failed ioremap of %d core.\n",
					__func__, i);
			return -ENOMEM;
		}

	ret = cpu_pm_register_notifier(&armdebug_cpu_pm_notifier_block);
	if (ret < 0)
		return ret;

	ret = register_cpu_notifier(&armdebug_cpu_notifier_block);
	if (ret < 0)
		return ret;

	pr_info("%s: debug architecture v7.1 support.\n", __func__);
	return 0;
}

subsys_initcall(armdebug_exynos5_init);
