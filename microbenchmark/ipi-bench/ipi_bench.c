/*
 * Copyright (C) 2019-2021 zhenwei pi pizhenwei@bytedance.com.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/random.h>
#include "../common/rdtsc.h"
#include "../common/getns.h"

static inline long unsigned gettime(void)
{
	return getns();
}

static int loops = 1000000;
module_param(loops, int, 0444);

static int srccpu = -1;
module_param(srccpu, int, 0444);

static int dstcpu = -1;
module_param(dstcpu, int, 0444);

static int pairs = -1;
module_param(pairs, int, 0444);

static int acrossnuma = 1;
module_param(acrossnuma, int, 0444);

static int broadcasts = -1;
module_param(broadcasts, int, 0444);

static int __read_mostly wait = 1;
module_param(wait, int, 0444);

static int __read_mostly lock = 1;
module_param(lock, int, 0444);

static int options;
module_param(options, int, 0444);

static unsigned long timeoutms = 10000;

static atomic64_t ready_to_run;
static atomic64_t should_run;
static atomic64_t complete_run;

static DECLARE_WAIT_QUEUE_HEAD(wait_complete);

#define SELF_IPI	(1<<0)
#define SINGLE_IPI	(1<<1)
#define MESH_IPI	(1<<2)
#define ALL_IPI		(1<<3)

static char *benchcases[] = {
	"self-ipi: send ipi to self, you can specify CPU by param srccpu=XX(default current CPU).",
	"single-ipi: send ipi from srccpu=XX to dstcpu=YY(default different random XX and YY), wait=0/1 to specify wait or not.",
	"mesh-ipi: send single ipi from one CPU to another CPU for all the CPUs, use pairs=XX to set number of benchmark pairs(default num_cpus / 2); use acrossnuma=0/1 to set IPI across NUMA(default = 1).",
	"all-ipi: send ipi from one CPU to all the CPUs, use lock=0/1 to specify spin lock option in callback function; wait=0/1 to specify wait or not; use broadcasts=XX to set workers(default 1).",
};

struct bench_args {
	int src;		/* src CPU */
	int dst;		/* dst CPU */
	atomic64_t forked;	/* forked timestamp */
	atomic64_t start;	/* start to run timestamp */
	atomic64_t finish;	/* finish bench timestamp */
	atomic64_t ipitime;	/* ipi time */
#ifdef CONFIG_SCHED_INFO
	atomic64_t run_delay;	/* sched run delay */
#endif
	char name[64];
};

static inline unsigned long __random(void)
{
	return get_random_long();
}

static void ipi_bench_gettime(void *info)
{
	unsigned long starttime = atomic64_read(info);
	unsigned long now = gettime();
	unsigned long diff = 0;

	if(now > starttime)
		diff = now - starttime;

	atomic64_set(info, diff);
}

static int ipi_bench_one(struct bench_args *ba)
{
	unsigned long ipitime = 0, now;
	int loop, ret, dst = ba->dst;

	for (loop = loops; loop > 0; loop--) {
		atomic64_set((atomic64_t *)&now, gettime());
		ret = smp_call_function_single(dst, ipi_bench_gettime, &now, wait);
		if (ret < 0)
			return ret;

		if (wait)
			ipitime += atomic64_read((const atomic64_t *)&now);
	}

	atomic64_set(&ba->ipitime, ipitime);

	return 0;
}

static void ipi_bench_record_run_delay(struct bench_args *ba)
{
#ifdef CONFIG_SCHED_INFO
	atomic64_set(&ba->run_delay, current->sched_info.run_delay);
#endif
}

static int ipi_bench_single_task(void *data)
{
	struct bench_args *ba = (struct bench_args*)data;

	atomic64_set(&ba->forked, gettime());

	/* let all threads run at the same time. to avoid wakeup delay */
	atomic64_add(1, (atomic64_t *)&ready_to_run);
	while (atomic64_read(&ready_to_run) < atomic64_read(&should_run));
	if (atomic64_read(&ready_to_run) != atomic64_read(&should_run)) {
		printk(KERN_INFO "ipi_bench: BUG, exit benchmark\n");
		return -1;
	}

	atomic64_set(&ba->start, gettime());
	ipi_bench_one(ba);
	atomic64_set(&ba->finish, gettime());
	ipi_bench_record_run_delay(ba);
	atomic64_add(1, (atomic64_t *)&complete_run);

	wake_up_interruptible(&wait_complete);

	return 0;
}

static int ipi_bench_one_task(struct bench_args *ba)
{
	struct task_struct *tsk;
	int src = ba->src;
	int dst = ba->dst;

	printk(KERN_INFO "ipi_bench: prepare single IPI from CPU[%3d] to CPU[%3d]\n", src, dst);
	snprintf(ba->name, sizeof(ba->name), "ipi_bench_%d_%d", src, dst);

	tsk = kthread_create_on_node(ipi_bench_single_task, ba, cpu_to_node(src), ba->name);
	if (IS_ERR(tsk)) {
		printk(KERN_INFO "ipi_bench: create kthread failed\n");
		return -1;
	}

	kthread_bind(tsk, src);
	wake_up_process(tsk);

	return 0;
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

static int ipi_bench_many(void)
{
	int loop;
	DEFINE_SPINLOCK(spinlock);

	for (loop = loops; loop > 0; loop--) {
		if (lock) {
			smp_call_function_many(cpu_online_mask, ipi_bench_spinlock, &spinlock, wait);
		} else {
			smp_call_function_many(cpu_online_mask, ipi_bench_empty, NULL, wait);
		}
	}

	return 0;
}

static int ipi_bench_many_task(void *data)
{
	struct bench_args *ba = (struct bench_args*)data;

	atomic64_set(&ba->forked, gettime());

	/* let all threads run at the same time. to avoid wakeup delay */
	atomic64_add(1, (atomic64_t *)&ready_to_run);
	while (atomic64_read(&ready_to_run) < atomic64_read(&should_run));
	if (atomic64_read(&ready_to_run) != atomic64_read(&should_run)) {
		printk(KERN_INFO "ipi_bench: BUG, exit benchmark\n");
		return -1;
	}

	atomic64_set(&ba->start, gettime());
	ipi_bench_many();
	atomic64_set(&ba->finish, gettime());
	ipi_bench_record_run_delay(ba);
	atomic64_add(1, (atomic64_t *)&complete_run);

	wake_up_interruptible(&wait_complete);

	return 0;
}

static int ipi_bench_all_task(struct bench_args *ba)
{
	struct task_struct *tsk;
	int src = ba->src;

	printk(KERN_INFO "ipi_bench: prepare broadcast IPI from CPU[%3d] to all CPUs\n", src);
	snprintf(ba->name, sizeof(ba->name), "ipi_bench_all_%d", src);

	tsk = kthread_create_on_node(ipi_bench_many_task, ba, cpu_to_node(src), ba->name);
	if (IS_ERR(tsk)) {
		printk(KERN_INFO "ipi_bench: create kthread failed\n");
		return -1;
	}

	kthread_bind(tsk, src);
	wake_up_process(tsk);

	return 0;
}

static inline void ipi_bench_wait_all(void)
{
	wait_event_interruptible_timeout(wait_complete,
			atomic64_read(&complete_run) == atomic64_read(&should_run),
			msecs_to_jiffies(timeoutms));
}

static inline void ipi_bench_report_single(struct bench_args *ba)
{
	int src = ba->src;
	int dst = ba->dst;
	unsigned long forked = atomic64_read(&ba->forked);
	unsigned long start = atomic64_read(&ba->start);
	unsigned long finish = atomic64_read(&ba->finish);
	unsigned long ipitime = atomic64_read(&ba->ipitime);
	unsigned long run_delay = atomic64_read(&ba->run_delay);
	unsigned long elapsed = finish - start;

	if (!finish) {
		printk(KERN_INFO "ipi_bench: too many loops\n");
		return;
	}

	if (elapsed > run_delay) {
		elapsed -= run_delay;
	}

	printk(KERN_INFO "ipi_bench: CPU [%3d] [NODE%d] -> CPU [%3d] [NODE%d], wait [%d], loops [%d] "
			"forked [%ld], start [%ld], finish [%ld], elapsed [%ld], ipitime [%ld], run delay [%ld] in ms, "
			"AVG call [%ld], ipi [%ld] in ns\n",
			src, cpu_to_node(src), dst, cpu_to_node(dst), wait, loops,
			forked / 1000, start / 1000, finish / 1000, elapsed / 1000, ipitime / 1000, run_delay / 1000,
			elapsed / loops, ipitime / loops);
}

static inline void ipi_bench_report_all(struct bench_args *ba)
{
	int src = ba->src;
	unsigned long forked = atomic64_read(&ba->forked);
	unsigned long start = atomic64_read(&ba->start);
	unsigned long finish = atomic64_read(&ba->finish);
	unsigned long run_delay = atomic64_read(&ba->run_delay);
	unsigned long elapsed = finish - start;

	if (!finish) {
		printk(KERN_INFO "ipi_bench: too many loops\n");
		return;
	}

	if (elapsed > run_delay) {
		elapsed -= run_delay;
	}

	printk(KERN_INFO "ipi_bench: CPU [%3d] [NODE%d] -> all CPUs, wait [%d], loops [%d] "
			"forked [%ld], start [%ld], finish [%ld], elapsed [%ld], run delay [%ld] in ms "
			"AVG call [%ld] in ns\n",
			src, cpu_to_node(src), wait, loops,
			forked / 1000, start / 1000, finish / 1000, elapsed / 1000, run_delay / 1000,
			elapsed / loops);
}

static int ipi_bench_self(int src)
{
	struct bench_args *ba;

	ba = kzalloc(sizeof(*ba), GFP_KERNEL);
	if (!ba) {
		printk(KERN_INFO "ipi_bench: no enough memory\n");
		return -1;
	}

	ba->src = src;
	ba->dst = src;

	atomic64_set(&should_run, 1);
	ipi_bench_one_task(ba);
	ipi_bench_wait_all();
	ipi_bench_report_single(ba);

	kfree(ba);

	return 0;
}

static int ipi_bench_single(int src, int dst)
{
	struct bench_args *ba;

	ba = kzalloc(sizeof(*ba), GFP_KERNEL);
	if (!ba) {
		printk(KERN_INFO "ipi_bench: no enough memory\n");
		return -1;
	}

	ba->src = src;
	ba->dst = dst;

	atomic64_set(&should_run, 1);
	ipi_bench_one_task(ba);
	ipi_bench_wait_all();
	ipi_bench_report_single(ba);

	kfree(ba);

	return 0;
}

/* node: select an valid CPU which belongs to. ignore -1 */
static int __random_unused_cpu_in_cpumask(cpumask_var_t cpumask, int node)
{
	int num_cpus = num_online_cpus();
	int retry = 100000;
	int cpu = -1;

	while (retry--) {
		cpu = __random() % num_cpus;
		if (cpumask_test_cpu(cpu, cpumask)) {
			continue;
		}

		if ((node >= 0) && (cpu_to_node(cpu) != node)) {
			continue;
		}

		cpumask_set_cpu(cpu, cpumask);
		return cpu;
	};

	return -1;
}

static int ipi_bench_mesh(int pairs, int acrossnuma)
{
	cpumask_var_t cpumask;
	struct bench_args *bas = NULL;
	struct bench_args *ba;
	unsigned long startns, elapsed;
	int ret = -1, i, node = -1;

	zalloc_cpumask_var(&cpumask, GFP_KERNEL);
	bas = kzalloc(sizeof(*ba) * pairs, GFP_KERNEL);
	if (!bas) {
		printk(KERN_INFO "ipi_bench: no enough memory\n");
		goto out;
	}

	atomic64_set(&should_run, pairs);

	/* build pairs one by one */
	for (i = 0; i < pairs; i++) {
		ba = bas + i;
		ba->src = __random_unused_cpu_in_cpumask(cpumask, -1);
		if (!acrossnuma) {
			node = cpu_to_node(ba->src);
		}
		ba->dst = __random_unused_cpu_in_cpumask(cpumask, node);

		if ((ba->src < 0) || (ba->dst < 0)) {
			printk(KERN_INFO "ipi_bench: init mesh failed\n");
			goto out;
		}
	}

	for (i = 0; i < pairs; i++) {
		ba = bas + i;
		ipi_bench_one_task(ba);
	}

	startns = getns();
	ipi_bench_wait_all();
	elapsed = getns() - startns;

	for (i = 0; i < pairs; i++) {
		ba = bas + i;
		ipi_bench_report_single(ba);
	}

	printk(KERN_INFO "ipi_bench: throughput %ld ipi/s\n", pairs * loops * 1000000000UL / elapsed);

	ret = 0;

out:
	kfree(bas);
	free_cpumask_var(cpumask);

	return ret;
}

static int ipi_bench_all(int broadcasts)
{
	cpumask_var_t cpumask;
	struct bench_args *bas = NULL;
	struct bench_args *ba;
	unsigned long startns, elapsed;
	int ret = -1, i;

	zalloc_cpumask_var(&cpumask, GFP_KERNEL);
	bas = kzalloc(sizeof(*ba) * broadcasts, GFP_KERNEL);
	if (!bas) {
		printk(KERN_INFO "ipi_bench: no enough memory\n");
		goto out;
	}

	atomic64_set(&should_run, broadcasts);

	/* build broadcasts one by one */
	for (i = 0; i < broadcasts; i++) {
		ba = bas + i;
		ba->src = __random_unused_cpu_in_cpumask(cpumask, -1);
		if (ba->src < 0) {
			printk(KERN_INFO "ipi_bench: init broadcast workers failed\n");
			goto out;
		}
	}

	for (i = 0; i < broadcasts; i++) {
		ba = bas + i;
		ipi_bench_all_task(ba);
	}

	startns = getns();
	ipi_bench_wait_all();
	elapsed = getns() - startns;

	for (i = 0; i < broadcasts; i++) {
		ba = bas + i;
		ipi_bench_report_all(ba);
	}


	printk(KERN_INFO "ipi_bench: throughput %ld ipi/s\n", broadcasts * (num_online_cpus() - 1) * loops * 1000000000UL / elapsed);

	ret = 0;

out:
	kfree(bas);
	free_cpumask_var(cpumask);

	return ret;
}

static void ipi_bench_options(void)
{
	int i;

	printk(KERN_INFO "ipi_bench: you should run insmod ipi_bench.ko options=XX, bit flags:\n");
	for (i = 0; i < sizeof(benchcases) / sizeof(benchcases[0]); i++) {
		printk(KERN_INFO "ipi_bench:\tbit[%d] %s\n", i, benchcases[i]);
	}
}

/* assign different src & dst cpu if unspecified */
static int ipi_bench_init_params(void)
{
	int num_cpus = num_online_cpus();

	if (num_cpus < 2) {
		printk(KERN_INFO "ipi_bench: total cpu num %d, no need to test\n", num_cpus);
		return -1;
	}

	if ((srccpu >= num_cpus) || (dstcpu >= num_cpus)) {
		printk(KERN_INFO "ipi_bench: cpu out of range, total cpu num %d\n", num_cpus);
		return -1;
	}

	if (pairs > (num_cpus / 2)) {
		printk(KERN_INFO "ipi_bench: pairs out of range, total cpu num %d\n", num_cpus);
		return -1;
	}

	while ((srccpu == -1) || (srccpu == dstcpu)) {
		srccpu = __random() % num_cpus;
	}

	while ((dstcpu == -1) || (srccpu == dstcpu)) {
		dstcpu = __random() % num_cpus;
	}

	if (pairs == -1) {
		pairs = num_cpus / 2;
	}

	if (broadcasts == -1) {
		broadcasts = 1;
	}

	return 0;
}

static int ipi_bench_init(void)
{
	if (!options) {
		ipi_bench_options();
		return -1;
	}

	if (ipi_bench_init_params() < 0) {
		return -1;
	}

	if (options & SELF_IPI) {
		ipi_bench_self(srccpu);
	}

	if (options & SINGLE_IPI) {
		ipi_bench_single(srccpu, dstcpu);
	}

	if (options & MESH_IPI) {
		ipi_bench_mesh(pairs, acrossnuma);
	}

	if (options & ALL_IPI) {
		ipi_bench_all(broadcasts);
	}

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
