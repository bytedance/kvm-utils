/*
 * Copyright (C) 2020 zhenwei pi pizhenwei@bytedance.com.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/ipi.h>
#include <asm/apic.h>
#include "../common/rdtsc.h"

#define LOOP 100000
#define MAXNUMA 4

static int apic_ipi_init(void)
{
	unsigned long start, end;
	int loop, node, nodes = num_online_nodes();
	unsigned int currentcpu, targetcpu;
	unsigned char numa[MAXNUMA] = {0};

	printk(KERN_INFO "apic_ipi: apic [%lx] [%s], benchmark apic->send_IPI\n", (unsigned long)apic, apic->name);
	printk(KERN_INFO "apic_ipi: %d NUMA node(s)\n", nodes);

	currentcpu = get_cpu();

	/* from current cpu to each NUMA node IPI */
	for (node = 0; node < nodes && node < MAXNUMA; node++) {
		for_each_online_cpu(targetcpu) {
			if (targetcpu == currentcpu)
				continue;

			if (numa[cpu_to_node(targetcpu)])
				continue;

			break;
		}


		printk(KERN_INFO "apic_ipi: ipi from CPU[%d] to CPU[%d]\n", currentcpu, targetcpu);

		start = ins_rdtsc();
		for (loop = 0; loop < LOOP; loop++) {
			apic->send_IPI(targetcpu, CALL_FUNCTION_SINGLE_VECTOR);
		}
		end = ins_rdtsc();

		printk(KERN_INFO "apic_ipi: total cycles %ld, avg %ld\n", end - start, (end - start) / LOOP);

		numa[cpu_to_node(targetcpu)] = 1;
	}

	put_cpu();

	return -1;
}

static void apic_ipi_exit(void)
{
	/* it should never run */
	printk(KERN_INFO "apic_ipi: %s\n", __func__);
}

module_init(apic_ipi_init);
module_exit(apic_ipi_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhenwei pi pizhewnei@bytedance.com");
