/*
 * Copyright (C) 2018 zhenwei pi pizhenwei@bytedance.com.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include "msr-index.h"
#include "msr.h"
#include "../common/rdtsc.h"

#define LOOP 100000

static inline void msr_bench_report(char *tag, unsigned long elapsed)
{
	printk(KERN_INFO "msr_bench: %s loop = %d, elapsed = %ld cycles, "
			"average = %ld cycles\n", tag, LOOP, elapsed, elapsed / LOOP);
}

static inline int wrmsr_bench(char *tag, unsigned int msr, unsigned long val)
{
	int loop;
	u32 low, high;

	low = val & 0xffffffffULL;
	high = val >> 32;

	/* check operation firstly, prefer to use asm instruction */
	if (native_write_msr_safe(msr, low, high) == 0) {
		printk(KERN_INFO "msr_bench: wrmsr %s OK, "
				"benchmark asm instrucion\n", tag);

		/* bench loop */
		for (loop = LOOP; loop > 0; loop--)
			ins_wrmsrl(MSR_IA32_TSCDEADLINE, val);
	} else {
		printk(KERN_INFO "msr_bench: wrmsr %s fail, "
				"benchmark native safe API\n", tag);

		/* bench loop */
		for (loop = LOOP; loop > 0; loop--)
			native_write_msr_safe(msr, low, high);
	}

	return 0;
}

static int wrmsr_bench_tscdeadline(void)
{
	/* a little long time, make sure no timer irq within test case
	 * but val should less than 22s to avoid soft watchdog schedule.
	 */
	unsigned long val = ins_rdtsc() + 10*1000*1000*1000UL;

	return wrmsr_bench("MSR_IA32_TSCDEADLINE", MSR_IA32_TSCDEADLINE, val);
}

static int wrmsr_bench_power(void)
{
	/* kvm does not support this msr, just test vm-exit. */
	unsigned long val = 0;

	return wrmsr_bench("MSR_IA32_POWER_CTL", MSR_IA32_POWER_CTL, val);
}


static int msr_bench_init(void)
{
	unsigned long starttime, elapsed;
	printk(KERN_INFO "msr_bench: %s start\n", __func__);

	/* case MSR_IA32_POWER_CTL */
	starttime = ins_rdtsc();
	if (wrmsr_bench_power() == 0) {

		elapsed = ins_rdtsc() - starttime;
		msr_bench_report("MSR_IA32_POWER_CTL", elapsed);
	}

	/* case MSR_IA32_TSCDEADLINE */
	starttime = ins_rdtsc();
	if (wrmsr_bench_tscdeadline() == 0) {

		elapsed = ins_rdtsc() - starttime;
		msr_bench_report("MSR_IA32_TSCDEADLINE", elapsed);
	}

	printk(KERN_INFO "msr_bench: %s finish\n", __func__);

	return -1;
}

static void msr_bench_exit(void)
{
	/* should never run */
	printk(KERN_INFO "msr_bench: %s\n", __func__);
}

module_init(msr_bench_init);
module_exit(msr_bench_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhenwei pi pizhewnei@bytedance.com");
