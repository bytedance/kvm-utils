/*
 * Copyright (C) 2019 zhenwei pi pizhenwei@bytedance.com.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include "../common/rdtsc.h"

#define LOOP 10000
#define DEBUG(...)
//#define DEBUG(fmt, ...) printk(fmt, ##__VA_ARGS__)

static inline void pio_mmio_bench_report(char *tag, const char *name, unsigned long addr, unsigned long elapsed)
{
	printk(KERN_INFO "pio_mmio_bench: %4s %18s, address = 0x%lx,  elapsed = %ld cycles, average = %ld cycles\n", tag, name, addr, elapsed, elapsed / LOOP);
}

int pio_bench(char *tag, const char *name, unsigned long addr)
{
	int loop;
	unsigned long starttime, elapsed;

	starttime = ins_rdtsc();
	for (loop = LOOP; loop > 0; loop--) {
		inb(addr);
	}

	elapsed = ins_rdtsc() - starttime;

	pio_mmio_bench_report(tag, name, addr, elapsed);

	return 0;
}

int mmio_bench(char *tag, const char *name, unsigned long addr)
{
	int loop;
	unsigned long starttime, elapsed;
	void *va;

	va = ioremap(addr, 0x1000);

	starttime = ins_rdtsc();
	for (loop = LOOP; loop > 0; loop--) {
		readb((void*)va);
	}

	elapsed = ins_rdtsc() - starttime;

	iounmap(va);
	pio_mmio_bench_report(tag, name, addr, elapsed);

	return 0;
}

/*
 * in fact, we need hold the resource lock, but it's decleared a static
 * variable. we should NOT attach/detach any device during test.
 */
void pio_walk_resource(void)
{
	struct resource *res, *tmp;

	for (res = ioport_resource.child; res; res = res->sibling) {
		DEBUG("pio_walk_resource name=%s, start=%llx, end=%llx\n", res->name, res->start, res->end);
		/* walk PCI Bus 0000:00 */
		if (!strcmp(res->name, "PCI Bus 0000:00")) {
			for (tmp = res->child; tmp; tmp = tmp->sibling) {
				DEBUG("pio_walk_resource name=%s, start=%llx, end=%llx\n", tmp->name, tmp->start, tmp->end);
				if (!strcmp(tmp->name, "pic1") 
						|| !(strcmp(tmp->name, "timer0"))) {
					pio_bench("PIO", tmp->name, tmp->start);
				} else if (!strcmp(tmp->name, "keyboard")) {
					pio_bench("PIO", "empty", tmp->start - 1);
					pio_bench("PIO", tmp->name, tmp->start);
					break;
				}
			}
		}
	}
}

void mmio_walk_resource(void)
{
	struct resource *res, *tmp;

	for (res = iomem_resource.child; res; res = res->sibling) {
		DEBUG("mmio_walk_resource name=%s, start=%llx, end=%llx\n", res->name, res->start, res->end);
		if (!strcmp(res->name, "IOAPIC 0")
				|| !strcmp(res->name, "Local APIC")) {
			mmio_bench("MMIO", res->name, res->start);
		}
		if (!strcmp(res->name, "PCI Bus 0000:00")) {
			mmio_bench("MMIO", res->name, res->start);
			for (tmp = res->child; tmp; tmp = tmp->sibling) {
				DEBUG("mmio_walk_resource PCI Bus name=%s, start=%llx, end=%llx\n", tmp->name, tmp->start, tmp->end);
				if (tmp->child) {
					DEBUG("mmio_walk_resource PCI Device name=%s, start=%llx, end=%llx\n", tmp->child->name, tmp->child->start, tmp->child->end);
					if (strstr(tmp->child->name, "vram")) {
						mmio_bench("MMIO", tmp->child->name, tmp->child->start);
					} else if (strstr(tmp->child->name, "virtio-pci-modern")) {
						/* only bench the first virtio device */
						mmio_bench("MMIO", tmp->child->name, tmp->child->start);
					}
				}
			}
		}
	}
}

static int pio_mmio_bench_init(void)
{
	printk(KERN_INFO "pio_mmio_bench: %s start\n", __func__);

	pio_walk_resource();
	mmio_walk_resource();

	printk(KERN_INFO "pio_mmio_bench: %s finish\n", __func__);

	return -1;
}

static void pio_mmio_bench_exit(void)
{
	/* should never run */
	printk(KERN_INFO "pio_mmio_bench: %s\n", __func__);
}

module_init(pio_mmio_bench_init);
module_exit(pio_mmio_bench_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhenwei pi pizhewnei@bytedance.com");
