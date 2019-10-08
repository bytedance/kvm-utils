/*
 * Copyright (C) 2019 zhenwei pi pizhenwei@bytedance.com.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/version.h>
#include <asm/kvm_host.h>
#include "msr.h"

static ktime_t __ktime;
static spinlock_t showing_lock;
static atomic_long_t total_wrmsr;

void show_wrmsr(void)
{
	unsigned int idx;
	unsigned int msr, count;

	pr_info("WRMSR STATISTIC\n");
	idx = MSR_IA32_APICBASE;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_IA32_TSC_ADJUST;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_IA32_TSCDEADLINE;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_IA32_MISC_ENABLE;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_IA32_MCG_STATUS;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_IA32_MCG_CTL;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_IA32_MCG_EXT_CTL;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_IA32_SMBASE;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_PLATFORM_INFO;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_MISC_FEATURES_ENABLES;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_KVM_WALL_CLOCK;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_KVM_SYSTEM_TIME;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_CORE_PERF_FIXED_CTR0;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_CORE_PERF_FIXED_CTR1;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_CORE_PERF_FIXED_CTR2;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_CORE_PERF_FIXED_CTR_CTRL;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_CORE_PERF_GLOBAL_STATUS;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_CORE_PERF_GLOBAL_CTRL;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = MSR_CORE_PERF_GLOBAL_OVF_CTRL;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = 0;
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));

	/* show lapic */
	pr_info("APIC STATISTIC\n");
	idx = APIC_BASE_MSR + (APIC_TASKPRI>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_EOI>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_LDR>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_DFR>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_SPIV>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_ICR>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_ICR2>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_LVT0>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_LVTT>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (APIC_SELF_IPI>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));
	idx = APIC_BASE_MSR + (0>>4);
	pr_info("\t[%s] %ld\n", msr2str(idx), report_wrmsr(idx));

	/* other msrs*/
	if (report_other_wrmsr(0, &msr, &count) == 0) {
		pr_info("OTHER MSRS STATISTIC\n");
		for (idx = 0; idx < OTHER_MSRS; idx++) {
			if (report_other_wrmsr(idx, &msr, &count))
				break;

			pr_info("\t[OTHER MSR 0x%x] %d\n", msr, count);
		}
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
static int kp_vmx_set_msr(struct kprobe *p, struct pt_regs *regs)
{
	struct msr_data *msr_info = (struct msr_data *)regs->si;
	unsigned int idx = msr_info->index;
#else
static int kp_vmx_set_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info)
{
	unsigned int idx = msr_info->index;
#endif
	ktime_t now = ktime_get();
	//pr_info("kprobe : msr_index = %d", msr_info->index);

	atomic_long_inc(&total_wrmsr);
	record_wrmsr(idx);
	//pr_info("WRMSR[%s] %ld\n", msr2str(idx), report_wrmsr(idx));

	if (ktime_to_ns(now) - ktime_to_ns(__ktime) > 1000*1000*1000) {
		if (!spin_trylock(&showing_lock))
			goto out;

		pr_info("total_wrmsr = %ld\n", atomic_long_read(&total_wrmsr));
		atomic_long_set(&total_wrmsr, 0);
		show_wrmsr();
		reset_wrmsr();

		__ktime = now;
		spin_unlock(&showing_lock);
	}

out :
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
	/* Always end with a call to jprobe_return(). */
	jprobe_return();
#endif
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
static struct kprobe vmx_set_msr_probe = {
	.pre_handler = kp_vmx_set_msr,
	.symbol_name = "vmx_set_msr",
};
#else
static struct jprobe vmx_set_msr_probe = {
	.entry			= kp_vmx_set_msr,
	.kp = {
		.symbol_name	= "vmx_set_msr",
	},
};
#endif

static int __init probe_init(void)
{
	int ret;
	void *addr;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
	ret = register_kprobe(&vmx_set_msr_probe);
	addr = vmx_set_msr_probe.addr;
#else
	ret = register_jprobe(&vmx_set_msr_probe);
	addr = vmx_set_msr_probe.kp.addr;
#endif
	if (ret < 0) {
		pr_err("kvmwrmsr : register_probe failed, returned %d\n", ret);
		return -1;
	}

	spin_lock_init(&showing_lock);
	atomic_long_set(&total_wrmsr, 0);
	init_wrmsr();
	__ktime = ktime_get();
	pr_info("kvmwrmsr : planted probe at %p\n", addr);
	return 0;
}

static void __exit probe_exit(void)
{
	void *addr;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
	addr = vmx_set_msr_probe.addr;
	unregister_kprobe(&vmx_set_msr_probe);
#else
	addr = vmx_set_msr_probe.kp.addr;
	unregister_jprobe(&vmx_set_msr_probe);
#endif
	pr_info("kvmwrmsr : probe at %p unregistered\n", addr);
}

module_init(probe_init)
module_exit(probe_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhenwei pi pizhewnei@bytedance.com");
