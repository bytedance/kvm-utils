/*
 * Copyright (C) 2019 zhenwei pi pizhenwei@bytedance.com.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include "../common/rdtsc.h"
#include "../common/getns.h"

#define LOOP 100000
#define MAXNUMA 4

//#define USE_CYCLES
#ifdef USE_CYCLES
#define REPORT_STR "cycles"

static inline unsigned long gettime(void)
{
	return ins_rdtsc();
}
#else
#define REPORT_STR "ns"

static inline unsigned long gettime(void)
{
	return getns();
}
#endif

static inline void ipi_bench_report(char *tag, int wait, int srccpu, int dstcpu, unsigned long elapsed, unsigned long ipitime)
{
	printk(KERN_INFO "ipi_bench: %30s wait[%d], CPU%d[NODE%d] -> CPU%d[NODE%d], loop = %d\n",
			tag, wait, srccpu, cpu_to_node(srccpu), dstcpu, cpu_to_node(dstcpu), LOOP);
	printk(KERN_INFO "ipi_bench: %40s  elapsed = %16ld %s, average = %8ld %s\n",
			"", elapsed, REPORT_STR, elapsed / LOOP, REPORT_STR);
	if (ipitime != 0) {
		printk(KERN_INFO "ipi_bench: %40s  ipitime = %16ld %s, average = %8ld %s\n",
			"", ipitime, REPORT_STR, ipitime / LOOP, REPORT_STR);
	}
}

static void ipi_bench_empty(void *info)
{
}

static void ipi_bench_spinlock(void *info)
{
	spinlock_t *lock = (spinlock_t *)info;

	spin_lock(lock);
	spin_unlock(lock);
}

static void ipi_bench_gettime(void *info)
{
	unsigned long *starttime = (unsigned long*)info;
	unsigned long now = gettime();

	if(now > *starttime)
		*starttime = now - *starttime;
	else
		*starttime = 0;
	/*	printk(KERN_INFO "ipi_bench: %s\n", __func__);	*/
}

static int ipi_bench_single(int currentcpu, int targetcpu, int wait)
{
	int loop, ret;
	unsigned long starttime, elapsed, ipitime;

	ipitime = 0;
	starttime = gettime();

	for (loop = LOOP; loop > 0; loop--) {
		unsigned long tsc = gettime();
		ret = smp_call_function_single(targetcpu, ipi_bench_gettime, &tsc, wait);
		if (ret < 0)
			return ret;

		if (wait)
			ipitime += tsc;
	}

	elapsed = gettime() - starttime;
	ipi_bench_report("ipi_bench_single", !!wait, currentcpu,
			targetcpu, elapsed, ipitime);

	return 0;
}

static int ipi_bench_many(int currentcpu, int wait, int uselock)
{
	int loop;
	unsigned long starttime, elapsed;
	DEFINE_SPINLOCK(lock);

	starttime = gettime();

	for (loop = LOOP; loop > 0; loop--) {
		if (uselock)
			smp_call_function_many(cpu_online_mask, ipi_bench_spinlock, &lock, wait);
		else
			smp_call_function_many(cpu_online_mask, ipi_bench_empty, NULL, wait);
	}

	elapsed = gettime() - starttime;
	if (uselock) {
		ipi_bench_report("ipi_bench_many lock", !!wait, currentcpu,
				255, elapsed, 0);
	} else {
		ipi_bench_report("ipi_bench_many nolcok", !!wait, currentcpu,
				255, elapsed, 0);
	}

	return 0;
}

static int ipi_bench_init(void)
{
	unsigned int currentcpu, targetcpu;
	int nodes = num_online_nodes();
	unsigned int node;
	unsigned char numa[MAXNUMA] = {0};

	printk(KERN_INFO "ipi_bench: %s start\n", __func__);
	printk(KERN_INFO "ipi_bench: %d NUMA node(s)\n", nodes);

	/* case self ipi, in fact, kernel just call func without IPI */
	currentcpu = get_cpu();
	ipi_bench_single(currentcpu, currentcpu, 1);

	/* from current cpu to each NUMA node IPI */
	for (node = 0; node < nodes && node < MAXNUMA; node++) {
		for_each_online_cpu(targetcpu) {
			if (targetcpu == currentcpu)
				continue;

			if (numa[cpu_to_node(targetcpu)])
				continue;

			break;
		}

		/* case other cpu ipi accross NUMA node, wait ipi */
		ipi_bench_single(currentcpu, targetcpu, 1);

		/* case other cpu ipi accross NUMA node, don't wait ipi */
		ipi_bench_single(currentcpu, targetcpu, 0);

		numa[cpu_to_node(targetcpu)] = 1;
	}

	/* case ipi broadcast, wait ipi with lock */
	ipi_bench_many(currentcpu, 1, 1);

	/* case ipi broadcast, don't wait ipi with lock */
	ipi_bench_many(currentcpu, 0, 1);

	/* case ipi broadcast, wait ipi */
	ipi_bench_many(currentcpu, 1, 0);

	/* case ipi broadcast, don't wait ipi */
	ipi_bench_many(currentcpu, 0, 0);

	put_cpu();
	printk(KERN_INFO "ipi_bench: %s finish\n", __func__);

	return -1;
}

static void ipi_bench_exit(void)
{
	/* should never run */
	printk(KERN_INFO "ipi_bench: %s\n", __func__);
}

module_init(ipi_bench_init);
module_exit(ipi_bench_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhenwei pi pizhewnei@bytedance.com");
