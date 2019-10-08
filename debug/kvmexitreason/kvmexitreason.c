/*
 * Copyright (C) 2019 zhenwei pi pizhenwei@bytedance.com.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kvm_host.h>
#include <linux/version.h>
#include <asm/vmx.h>
#include "exit-reason.h"

static ktime_t __ktime;
static spinlock_t showing_lock;
static atomic_long_t total_exit;

void show_exitreason(void)
{
	int idx;
	ktime_t now = ktime_get();

	if (ktime_to_ns(now) - ktime_to_ns(__ktime) < 1000*1000*1000)
		return;

	if (!spin_trylock(&showing_lock))
		return;

	pr_info("VM EXIT REASON STATISTIC\n");
	pr_info("\tTOTAL EXITS : %ld\n", atomic_long_read(&total_exit));
	for (idx = 0; idx < REASON_NUM; idx++) {
		if (report_reason(idx))
			pr_info("\t%40s : %ld\n", reason2str(idx), report_reason(idx));
	}

	atomic_long_set(&total_exit, 0);
	reset_reason();
	__ktime = now;
	spin_unlock(&showing_lock);
}

/*
 * prefer to use jprobe, but 4.19 removes jprobe.
 * need to remove all jprobe code in the future.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
#define DECLEAR_PROBE_FUNC(REASON,SYMBOL) \
	static int kpre_##SYMBOL (struct kprobe *p, struct pt_regs *regs) \
	{	record_reason(REASON);\
		atomic_long_inc(&total_exit); \
		show_exitreason(); \
		return 0;}
#else
#define DECLEAR_PROBE_FUNC(REASON,SYMBOL) \
	static int jp_##SYMBOL (struct kvm_vcpu *vcpu) \
	{	record_reason(REASON);\
		atomic_long_inc(&total_exit); \
		show_exitreason(); \
		jprobe_return();\
		return 0;}
#endif

DECLEAR_PROBE_FUNC(EXIT_REASON_EXCEPTION_NMI, handle_exception)
DECLEAR_PROBE_FUNC(EXIT_REASON_EXTERNAL_INTERRUPT, handle_external_interrupt)
DECLEAR_PROBE_FUNC(EXIT_REASON_TRIPLE_FAULT, handle_triple_fault)
DECLEAR_PROBE_FUNC(EXIT_REASON_NMI_WINDOW, handle_nmi_window)
DECLEAR_PROBE_FUNC(EXIT_REASON_IO_INSTRUCTION, handle_io)
DECLEAR_PROBE_FUNC(EXIT_REASON_CR_ACCESS, handle_cr)
DECLEAR_PROBE_FUNC(EXIT_REASON_DR_ACCESS, handle_dr)
DECLEAR_PROBE_FUNC(EXIT_REASON_CPUID, handle_cpuid)
DECLEAR_PROBE_FUNC(EXIT_REASON_MSR_READ, handle_rdmsr)
DECLEAR_PROBE_FUNC(EXIT_REASON_MSR_WRITE, handle_wrmsr)
DECLEAR_PROBE_FUNC(EXIT_REASON_PENDING_INTERRUPT, handle_interrupt_window)
DECLEAR_PROBE_FUNC(EXIT_REASON_HLT, handle_halt)
DECLEAR_PROBE_FUNC(EXIT_REASON_INVD, handle_invd)
DECLEAR_PROBE_FUNC(EXIT_REASON_INVLPG, handle_invlpg)
DECLEAR_PROBE_FUNC(EXIT_REASON_RDPMC, handle_rdpmc)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMCALL, handle_vmcall)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMCLEAR, handle_vmclear)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMLAUNCH, handle_vmlaunch)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMPTRLD, handle_vmptrld)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMPTRST, handle_vmptrst)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMREAD, handle_vmread)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMRESUME, handle_vmresume)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMWRITE, handle_vmwrite)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMOFF, handle_vmoff)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMON, handle_vmon)
DECLEAR_PROBE_FUNC(EXIT_REASON_TPR_BELOW_THRESHOLD, handle_tpr_below_threshold)
DECLEAR_PROBE_FUNC(EXIT_REASON_APIC_ACCESS, handle_apic_access)
DECLEAR_PROBE_FUNC(EXIT_REASON_APIC_WRITE, handle_apic_write)
DECLEAR_PROBE_FUNC(EXIT_REASON_EOI_INDUCED, handle_apic_eoi_induced)
DECLEAR_PROBE_FUNC(EXIT_REASON_WBINVD, handle_wbinvd)
DECLEAR_PROBE_FUNC(EXIT_REASON_XSETBV, handle_xsetbv)
DECLEAR_PROBE_FUNC(EXIT_REASON_TASK_SWITCH, handle_task_switch)
DECLEAR_PROBE_FUNC(EXIT_REASON_MCE_DURING_VMENTRY, handle_machine_check)
DECLEAR_PROBE_FUNC(EXIT_REASON_EPT_VIOLATION, handle_ept_violation)
DECLEAR_PROBE_FUNC(EXIT_REASON_EPT_MISCONFIG, handle_ept_misconfig)
DECLEAR_PROBE_FUNC(EXIT_REASON_PAUSE_INSTRUCTION, handle_pause)
DECLEAR_PROBE_FUNC(EXIT_REASON_MWAIT_INSTRUCTION, handle_mwait)
DECLEAR_PROBE_FUNC(EXIT_REASON_MONITOR_TRAP_FLAG, handle_monitor_trap)
DECLEAR_PROBE_FUNC(EXIT_REASON_MONITOR_INSTRUCTION, handle_monitor)
DECLEAR_PROBE_FUNC(EXIT_REASON_INVEPT, handle_invept)
DECLEAR_PROBE_FUNC(EXIT_REASON_INVVPID, handle_invvpid)
/*
	EXIT_REASON_RDRAND
	EXIT_REASON_RDSEED
*/
DECLEAR_PROBE_FUNC(EXIT_REASON_XSAVES, handle_xsaves)
DECLEAR_PROBE_FUNC(EXIT_REASON_XRSTORS, handle_xrstors)
DECLEAR_PROBE_FUNC(EXIT_REASON_PML_FULL, handle_pml_full)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
DECLEAR_PROBE_FUNC(EXIT_REASON_VMFUNC, handle_vmfunc)
#endif
DECLEAR_PROBE_FUNC(EXIT_REASON_PREEMPTION_TIMER, handle_preemption_timer)


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
#define DECLEAR_PROBE(REASON,SYMBOL) \
	[REASON] = { \
		.pre_handler = kpre_##SYMBOL, \
		.symbol_name = #SYMBOL, \
	}

static struct kprobe vmx_handle_exit_probe[REASON_NUM] = {
#else
#define DECLEAR_PROBE(REASON,SYMBOL) \
	[REASON] = { \
		.entry = jp_##SYMBOL, \
		.kp = { \
			.symbol_name	= #SYMBOL, \
		}, \
	}

static struct jprobe vmx_handle_exit_probe[REASON_NUM] = {
#endif

	DECLEAR_PROBE(EXIT_REASON_EXCEPTION_NMI, handle_exception),
	DECLEAR_PROBE(EXIT_REASON_EXCEPTION_NMI, handle_exception),
	DECLEAR_PROBE(EXIT_REASON_EXTERNAL_INTERRUPT, handle_external_interrupt),
	DECLEAR_PROBE(EXIT_REASON_TRIPLE_FAULT, handle_triple_fault),
	DECLEAR_PROBE(EXIT_REASON_NMI_WINDOW, handle_nmi_window),
	DECLEAR_PROBE(EXIT_REASON_IO_INSTRUCTION, handle_io),
	DECLEAR_PROBE(EXIT_REASON_CR_ACCESS, handle_cr),
	DECLEAR_PROBE(EXIT_REASON_DR_ACCESS, handle_dr),
	DECLEAR_PROBE(EXIT_REASON_CPUID, handle_cpuid),
	DECLEAR_PROBE(EXIT_REASON_MSR_READ, handle_rdmsr),
	DECLEAR_PROBE(EXIT_REASON_MSR_WRITE, handle_wrmsr),
	DECLEAR_PROBE(EXIT_REASON_PENDING_INTERRUPT, handle_interrupt_window),
	DECLEAR_PROBE(EXIT_REASON_HLT, handle_halt),
	DECLEAR_PROBE(EXIT_REASON_INVD, handle_invd),
	DECLEAR_PROBE(EXIT_REASON_INVLPG, handle_invlpg),
	DECLEAR_PROBE(EXIT_REASON_RDPMC, handle_rdpmc),
	DECLEAR_PROBE(EXIT_REASON_VMCALL, handle_vmcall),
	DECLEAR_PROBE(EXIT_REASON_VMCLEAR, handle_vmclear),
	DECLEAR_PROBE(EXIT_REASON_VMLAUNCH, handle_vmlaunch),
	DECLEAR_PROBE(EXIT_REASON_VMPTRLD, handle_vmptrld),
	DECLEAR_PROBE(EXIT_REASON_VMPTRST, handle_vmptrst),
	DECLEAR_PROBE(EXIT_REASON_VMREAD, handle_vmread),
	DECLEAR_PROBE(EXIT_REASON_VMRESUME, handle_vmresume),
	DECLEAR_PROBE(EXIT_REASON_VMWRITE, handle_vmwrite),
	DECLEAR_PROBE(EXIT_REASON_VMOFF, handle_vmoff),
	DECLEAR_PROBE(EXIT_REASON_VMON, handle_vmon),
	DECLEAR_PROBE(EXIT_REASON_TPR_BELOW_THRESHOLD, handle_tpr_below_threshold),
	DECLEAR_PROBE(EXIT_REASON_APIC_ACCESS, handle_apic_access),
	DECLEAR_PROBE(EXIT_REASON_APIC_WRITE, handle_apic_write),
	DECLEAR_PROBE(EXIT_REASON_EOI_INDUCED, handle_apic_eoi_induced),
	DECLEAR_PROBE(EXIT_REASON_WBINVD, handle_wbinvd),
	DECLEAR_PROBE(EXIT_REASON_XSETBV, handle_xsetbv),
	DECLEAR_PROBE(EXIT_REASON_TASK_SWITCH, handle_task_switch),
	DECLEAR_PROBE(EXIT_REASON_MCE_DURING_VMENTRY, handle_machine_check),
	DECLEAR_PROBE(EXIT_REASON_EPT_VIOLATION, handle_ept_violation),
	DECLEAR_PROBE(EXIT_REASON_EPT_MISCONFIG, handle_ept_misconfig),
	DECLEAR_PROBE(EXIT_REASON_PAUSE_INSTRUCTION, handle_pause),
	DECLEAR_PROBE(EXIT_REASON_MWAIT_INSTRUCTION, handle_mwait),
	DECLEAR_PROBE(EXIT_REASON_MONITOR_TRAP_FLAG, handle_monitor_trap),
	DECLEAR_PROBE(EXIT_REASON_MONITOR_INSTRUCTION, handle_monitor),
	DECLEAR_PROBE(EXIT_REASON_INVEPT, handle_invept),
	DECLEAR_PROBE(EXIT_REASON_INVVPID, handle_invvpid),
	/*
		EXIT_REASON_RDRAND
		EXIT_REASON_RDSEED
	*/
	DECLEAR_PROBE(EXIT_REASON_XSAVES, handle_xsaves),
	DECLEAR_PROBE(EXIT_REASON_XRSTORS, handle_xrstors),
	DECLEAR_PROBE(EXIT_REASON_PML_FULL, handle_pml_full),
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	DECLEAR_PROBE(EXIT_REASON_VMFUNC, handle_vmfunc),
#endif
	DECLEAR_PROBE(EXIT_REASON_PREEMPTION_TIMER, handle_preemption_timer),
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

void unregister_all_probes(void)
{
	int idx;

	for (idx = 0; idx < ARRAY_SIZE(vmx_handle_exit_probe); idx++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		if (!vmx_handle_exit_probe[idx].symbol_name) {
			pr_info("kvmexitreason : unregister_probe skip reason %d\n", idx);
			continue;
		}

		unregister_kprobe(&vmx_handle_exit_probe[idx]);
#else
		if (!vmx_handle_exit_probe[idx].entry) {
			pr_info("kvmexitreason : unregister_jprobe skip reason %d\n", idx);
			continue;
		}

		unregister_jprobe(&vmx_handle_exit_probe[idx]);
#endif
	}
}

static int __init probe_init(void)
{
	int idx;
	int ret;

	for (idx = 0; idx < ARRAY_SIZE(vmx_handle_exit_probe); idx++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		if (!vmx_handle_exit_probe[idx].symbol_name) {
			pr_info("kvmexitreason : register_probe skip reason %d\n", idx);
			continue;
		}

		ret = register_kprobe(&vmx_handle_exit_probe[idx]);
		if (ret < 0) {
			pr_err("kvmexitreason : register_kprobe %d jprobe failed : %d\n",
					idx, ret);
			unregister_all_probes();
			return -1;
		}
#else
		if (!vmx_handle_exit_probe[idx].entry) {
			pr_info("kvmexitreason : register_jprobe skip reason %d\n", idx);
			continue;
		}

		ret = register_jprobe(&vmx_handle_exit_probe[idx]);
		if (ret < 0) {
			pr_err("kvmexitreason : register_jprobe %d jprobe failed : %d\n",
					idx, ret);
			unregister_all_probes();
			return -1;
		}
#endif
	}

	init_reasons();
	spin_lock_init(&showing_lock);
	atomic_long_set(&total_exit, 0);
	__ktime = ktime_get();

	return 0;
}

static void __exit probe_exit(void)
{
	unregister_all_probes();
}

module_init(probe_init)
module_exit(probe_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhenwei pi pizhewnei@bytedance.com");
