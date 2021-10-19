/*
 * Copyright (C) 2020 zhenwei pi pizhenwei@bytedance.com.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/irq_vectors.h>
#include <linux/kallsyms.h>
#include <asm/apic.h>
#include "../common/rdtsc.h"

static int options;
module_param(options, int, 0444);

#define LOOP 100000
#define MAXNUMA 4

static char *ipi_funcs[] = {
	"kvm_send_ipi_mask",
	"default_send_IPI_mask_sequence_phys",
	"flat_send_IPI_mask",
	"x2apic_send_IPI_mask",
};

static void bench(char *func, int nodes)
{
	unsigned long start, end;
	int loop, node;
	unsigned int currentcpu, targetcpu;
	unsigned char numa[MAXNUMA] = {0};
	void (*send_IPI_mask)(const struct cpumask *mask, int vector);

	send_IPI_mask = (void (*)(const struct cpumask *mask, int vector))kallsyms_lookup_name(func);

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


		printk(KERN_INFO "apic_ipi: 	IPI[%s] from CPU[%d] to CPU[%d]\n", func, currentcpu, targetcpu);

		start = ins_rdtsc();
		for (loop = 0; loop < LOOP; loop++) {
			send_IPI_mask(cpumask_of(targetcpu), CALL_FUNCTION_SINGLE_VECTOR);
		}
		end = ins_rdtsc();

		printk(KERN_INFO "apic_ipi:		total cycles %ld, avg %ld\n", end - start, (end - start) / LOOP);

		numa[cpu_to_node(targetcpu)] = 1;
	}

	put_cpu();
}

static int apic_ipi_init(void)
{
	int nodes = num_online_nodes();
	char name[KSYM_NAME_LEN];
	int i;

	if (!options) {
		printk(KERN_INFO "apic_ipi: you should run insmod apic_ipi.ko options=XX, bit flags:\n");
		for (i = 0; i < sizeof(ipi_funcs) / sizeof(ipi_funcs[0]); i++) {
			printk(KERN_INFO "apic_ipi:	bit[%d] %s\n", i, ipi_funcs[i]);
		}

		return -1;
	}

	printk(KERN_INFO "apic_ipi: %d NUMA node(s)\n", nodes);
	printk(KERN_INFO "apic_ipi: apic [%s]\n", apic->name);

	if (sprint_symbol(name, (unsigned long)apic->send_IPI) > 0) {
		printk(KERN_INFO "apic_ipi: apic->send_IPI[%s]\n", name);
	} else {
		printk(KERN_INFO "apic_ipi: apic->send_IPI[%lx]\n", (unsigned long)apic->send_IPI);
	}

	if (sprint_symbol(name, (unsigned long)apic->send_IPI_mask) > 0) {
		printk(KERN_INFO "apic_ipi: apic->send_IPI_mask[%s]\n", name);
	} else {
		printk(KERN_INFO "apic_ipi: apic->send_IPI_mask[%lx]\n", (unsigned long)apic->send_IPI_mask);
	}

	for (i = 0; i < sizeof(ipi_funcs) / sizeof(ipi_funcs[0]); i++) {
		if (options & (1 << i)) {
			bench(ipi_funcs[i], nodes);
		}
	}

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
