/*
 * Copyright (C) 2019 zhenwei pi pizhenwei@bytedance.com.
 */
#ifndef __MSR_H__
#define __MSR_H__

#include "msr-index.h"
#include "local-msr-index.h"
#include "apicdef.h"
#include "kvm_para.h"

/* wrmsr reasons */
static unsigned long wrmsr_exits_set_apic_base = 0;
static unsigned long wrmsr_exits_set_tscadjust = 0;
static unsigned long wrmsr_exits_set_tscdeadline = 0;
static unsigned long wrmsr_exits_set_misc_enable = 0;
static unsigned long wrmsr_exits_set_mcg_status = 0;
static unsigned long wrmsr_exits_set_mcg_ctl = 0;
static unsigned long wrmsr_exits_set_mcg_ext_ctl = 0;
static unsigned long wrmsr_exits_set_smbase = 0;
static unsigned long wrmsr_exits_set_platform_info = 0;
static unsigned long wrmsr_exits_set_misc_features_enables = 0;
static unsigned long wrmsr_exits_set_wall_clock = 0;
static unsigned long wrmsr_exits_set_system_time = 0;

/* lapic reasons */
static unsigned long wrmsr_exits_set_lapic_tpr = 0;
static unsigned long wrmsr_exits_set_lapic_eoi = 0;
static unsigned long wrmsr_exits_set_lapic_ldr = 0;
static unsigned long wrmsr_exits_set_lapic_dfr = 0;
static unsigned long wrmsr_exits_set_lapic_spiv = 0;
static unsigned long wrmsr_exits_set_lapic_icr = 0;
static unsigned long wrmsr_exits_set_lapic_icr2 = 0;
static unsigned long wrmsr_exits_set_lapic_lvt = 0;
static unsigned long wrmsr_exits_set_lapic_lvtt = 0;
static unsigned long wrmsr_exits_set_lapic_self_ipi = 0;
static unsigned long wrmsr_exits_set_lapic_others = 0;
static unsigned long wrmsr_exits_others = 0;

/* pmu reasons : Intel Core-based CPU performance counters */
static unsigned long wrmsr_exits_set_perf_fixed_ctr0 = 0;
static unsigned long wrmsr_exits_set_perf_fixed_ctr1 = 0;
static unsigned long wrmsr_exits_set_perf_fixed_ctr2 = 0;
static unsigned long wrmsr_exits_set_perf_fixed_ctr_ctrl = 0;
static unsigned long wrmsr_exits_set_perf_global_status = 0;
static unsigned long wrmsr_exits_set_perf_global_ctrl = 0;
static unsigned long wrmsr_exits_set_perf_global_ovf_ctrl = 0;

struct msr_index_count {
	unsigned int index;
	unsigned int count;
};

#define OTHER_MSRS 128
static spinlock_t other_msrs_lock;
static struct msr_index_count other_msrs[OTHER_MSRS];

void record_other_wrmsr(unsigned int msr)
{
	int idx;

	spin_lock(&other_msrs_lock);
	for (idx = 0; idx < OTHER_MSRS; idx++) {
		if ((msr == other_msrs[idx].index) || (other_msrs[idx].index == 0)) {
			other_msrs[idx].index = msr;
			other_msrs[idx].count++;
			break;
		}
	}
	spin_unlock(&other_msrs_lock);
}

int report_other_wrmsr(int idx, unsigned int *msr, unsigned int *count)
{
	spin_lock(&other_msrs_lock);
	if (other_msrs[idx].index && other_msrs[idx].count) {
		*msr = other_msrs[idx].index;
		*count = other_msrs[idx].count;
		spin_unlock(&other_msrs_lock);
		return 0;
	}

	spin_unlock(&other_msrs_lock);
	return -1;
}

void reset_other_wrmsr(void)
{
	spin_lock(&other_msrs_lock);
	memset(other_msrs, 0x00, sizeof(other_msrs));
	spin_unlock(&other_msrs_lock);
}

void record_wrmsr(unsigned int msr)
{
	switch (msr) {
		case MSR_IA32_APICBASE:
			wrmsr_exits_set_apic_base++;
			break;

		case MSR_IA32_TSC_ADJUST:
			wrmsr_exits_set_tscadjust++;
			break;

		case MSR_IA32_TSCDEADLINE:
			wrmsr_exits_set_tscdeadline++;
			break;

		case MSR_IA32_MISC_ENABLE:
			wrmsr_exits_set_misc_enable++;
			break;

		case MSR_IA32_MCG_STATUS:
			wrmsr_exits_set_mcg_status++;
			break;

		case MSR_IA32_MCG_CTL:
			wrmsr_exits_set_mcg_ctl++;
			break;

		case MSR_IA32_MCG_EXT_CTL:
			wrmsr_exits_set_mcg_ext_ctl++;
			break;

		case MSR_IA32_SMBASE:
			wrmsr_exits_set_smbase++;
			break;

		case MSR_PLATFORM_INFO:
			wrmsr_exits_set_platform_info++;
			break;

		case MSR_MISC_FEATURES_ENABLES:
			wrmsr_exits_set_misc_features_enables++;
			break;

		case MSR_KVM_WALL_CLOCK_NEW:
		case MSR_KVM_WALL_CLOCK:
			wrmsr_exits_set_wall_clock++;
			break;

		case MSR_KVM_SYSTEM_TIME_NEW:
		case MSR_KVM_SYSTEM_TIME:
			wrmsr_exits_set_system_time++;
			break;

		case APIC_BASE_MSR ... (APIC_BASE_MSR + 0x3ff): 
			{
				unsigned int reg = (msr - APIC_BASE_MSR) << 4;
				switch (reg) {
					case APIC_TASKPRI:		
						wrmsr_exits_set_lapic_tpr++;
						break;

					case APIC_EOI:
						wrmsr_exits_set_lapic_eoi++;
						break;

					case APIC_LDR:
						wrmsr_exits_set_lapic_ldr++;
						break;

					case APIC_DFR:
						wrmsr_exits_set_lapic_dfr++;
						break;

					case APIC_SPIV:
						wrmsr_exits_set_lapic_spiv++;
						break;

					case APIC_ICR:
						wrmsr_exits_set_lapic_icr++;
						break;

					case APIC_ICR2:
						wrmsr_exits_set_lapic_icr2++;
						break;

					case APIC_LVT0:
					case APIC_LVTTHMR:
					case APIC_LVTPC:
					case APIC_LVT1:
					case APIC_LVTERR:
						wrmsr_exits_set_lapic_lvt++;
						break;

					case APIC_LVTT:
						wrmsr_exits_set_lapic_lvtt++;
						break;

					case APIC_SELF_IPI:
						wrmsr_exits_set_lapic_self_ipi++;
						break;

					default :
						wrmsr_exits_set_lapic_others++;
				}
				break;
			}

		case MSR_CORE_PERF_FIXED_CTR0:
			wrmsr_exits_set_perf_fixed_ctr0++;
			break;

		case MSR_CORE_PERF_FIXED_CTR1:
			wrmsr_exits_set_perf_fixed_ctr1++;
			break;

		case MSR_CORE_PERF_FIXED_CTR2:
			wrmsr_exits_set_perf_fixed_ctr2++;
			break;

		case MSR_CORE_PERF_FIXED_CTR_CTRL:
			wrmsr_exits_set_perf_fixed_ctr_ctrl++;
			break;

		case MSR_CORE_PERF_GLOBAL_STATUS:
			wrmsr_exits_set_perf_global_status++;
			break;

		case MSR_CORE_PERF_GLOBAL_CTRL:
			wrmsr_exits_set_perf_global_ctrl++;
			break;

		case MSR_CORE_PERF_GLOBAL_OVF_CTRL:
			wrmsr_exits_set_perf_global_ovf_ctrl++;
			break;

		default :
			wrmsr_exits_others++;
			record_other_wrmsr(msr);
			break;
	}
}

unsigned long report_wrmsr(unsigned int msr)
{
	switch (msr) {
		case MSR_IA32_APICBASE:
			return wrmsr_exits_set_apic_base;

		case MSR_IA32_TSC_ADJUST:
			return wrmsr_exits_set_tscadjust;

		case MSR_IA32_TSCDEADLINE:
			return wrmsr_exits_set_tscdeadline;

		case MSR_IA32_MISC_ENABLE:
			return wrmsr_exits_set_misc_enable;

		case MSR_IA32_MCG_STATUS:
			return wrmsr_exits_set_mcg_status;

		case MSR_IA32_MCG_CTL:
			return wrmsr_exits_set_mcg_ctl;

		case MSR_IA32_MCG_EXT_CTL:
			return wrmsr_exits_set_mcg_ext_ctl;

		case MSR_IA32_SMBASE:
			return wrmsr_exits_set_smbase;

		case MSR_PLATFORM_INFO:
			return wrmsr_exits_set_platform_info;

		case MSR_MISC_FEATURES_ENABLES:
			return wrmsr_exits_set_misc_enable;

		case MSR_KVM_WALL_CLOCK_NEW:
		case MSR_KVM_WALL_CLOCK:
			return wrmsr_exits_set_wall_clock;

		case MSR_KVM_SYSTEM_TIME_NEW:
		case MSR_KVM_SYSTEM_TIME:
			return wrmsr_exits_set_system_time;

		case APIC_BASE_MSR ... APIC_BASE_MSR + 0x3ff: 
			{
				unsigned int reg = (msr - APIC_BASE_MSR) << 4;
				switch (reg) {
					case APIC_TASKPRI:		
						return wrmsr_exits_set_lapic_tpr;

					case APIC_EOI:
						return wrmsr_exits_set_lapic_eoi;

					case APIC_LDR:
						return wrmsr_exits_set_lapic_ldr;

					case APIC_DFR:
						return wrmsr_exits_set_lapic_dfr;

					case APIC_SPIV:
						return wrmsr_exits_set_lapic_spiv;

					case APIC_ICR:
						return wrmsr_exits_set_lapic_icr;

					case APIC_ICR2:
						return wrmsr_exits_set_lapic_icr2;

					case APIC_LVT0:
					case APIC_LVTTHMR:
					case APIC_LVTPC:
					case APIC_LVT1:
					case APIC_LVTERR:
						return wrmsr_exits_set_lapic_lvt;

					case APIC_LVTT:
						return wrmsr_exits_set_lapic_lvtt;

					case APIC_SELF_IPI:
						return wrmsr_exits_set_lapic_self_ipi;

					default :
						return wrmsr_exits_set_lapic_others;
				}
			}

		case MSR_CORE_PERF_FIXED_CTR0:
			return wrmsr_exits_set_perf_fixed_ctr0;

		case MSR_CORE_PERF_FIXED_CTR1:
			return wrmsr_exits_set_perf_fixed_ctr1;

		case MSR_CORE_PERF_FIXED_CTR2:
			return wrmsr_exits_set_perf_fixed_ctr2;

		case MSR_CORE_PERF_FIXED_CTR_CTRL:
			return wrmsr_exits_set_perf_fixed_ctr_ctrl;

		case MSR_CORE_PERF_GLOBAL_STATUS:
			return wrmsr_exits_set_perf_global_status;

		case MSR_CORE_PERF_GLOBAL_CTRL:
			return wrmsr_exits_set_perf_global_ctrl;

		case MSR_CORE_PERF_GLOBAL_OVF_CTRL:
			return wrmsr_exits_set_perf_global_ovf_ctrl;

		default :
			return wrmsr_exits_others;
	}
}

int reset_wrmsr(void)
{
	wrmsr_exits_set_apic_base = 0;
	wrmsr_exits_set_tscadjust = 0;
	wrmsr_exits_set_tscdeadline = 0;
	wrmsr_exits_set_misc_enable = 0;
	wrmsr_exits_set_mcg_status = 0;
	wrmsr_exits_set_mcg_ctl = 0;
	wrmsr_exits_set_mcg_ext_ctl = 0;
	wrmsr_exits_set_smbase = 0;
	wrmsr_exits_set_platform_info = 0;
	wrmsr_exits_set_misc_features_enables = 0;
	wrmsr_exits_set_wall_clock = 0;
	wrmsr_exits_set_system_time = 0;

	wrmsr_exits_set_lapic_tpr = 0;
	wrmsr_exits_set_lapic_eoi = 0;
	wrmsr_exits_set_lapic_ldr = 0;
	wrmsr_exits_set_lapic_dfr = 0;
	wrmsr_exits_set_lapic_spiv = 0;
	wrmsr_exits_set_lapic_icr = 0;
	wrmsr_exits_set_lapic_icr2 = 0;
	wrmsr_exits_set_lapic_lvt = 0;
	wrmsr_exits_set_lapic_lvtt = 0;
	wrmsr_exits_set_lapic_self_ipi = 0;
	wrmsr_exits_set_lapic_others = 0;
	wrmsr_exits_others = 0;

	wrmsr_exits_set_perf_fixed_ctr0 = 0;
	wrmsr_exits_set_perf_fixed_ctr1 = 0;
	wrmsr_exits_set_perf_fixed_ctr2 = 0;
	wrmsr_exits_set_perf_fixed_ctr_ctrl = 0;
	wrmsr_exits_set_perf_global_status = 0;
	wrmsr_exits_set_perf_global_ctrl = 0;
	wrmsr_exits_set_perf_global_ovf_ctrl = 0;

	reset_other_wrmsr();

	return 0;
}

char *msr2str(unsigned int msr)
{
	switch (msr) {
		case MSR_IA32_APICBASE:
			return "MSR_IA32_APICBASE";

		case MSR_IA32_TSC_ADJUST:
			return "MSR_IA32_TSC_ADJUST";

		case MSR_IA32_TSCDEADLINE:
			return "MSR_IA32_TSCDEADLINE";

		case MSR_IA32_MISC_ENABLE:
			return "MSR_IA32_MISC_ENABLE";

		case MSR_IA32_MCG_STATUS:
			return "MSR_IA32_MCG_STATUS";

		case MSR_IA32_MCG_CTL:
			return "MSR_IA32_MCG_CTL";

		case MSR_IA32_MCG_EXT_CTL:
			return "MSR_IA32_MCG_EXT_CTL";

		case MSR_IA32_SMBASE:
			return "MSR_IA32_SMBASE";

		case MSR_PLATFORM_INFO:
			return "MSR_PLATFORM_INFO";

		case MSR_MISC_FEATURES_ENABLES:
			return "MSR_MISC_FEATURES_ENABLES";

		case MSR_KVM_WALL_CLOCK_NEW:
		case MSR_KVM_WALL_CLOCK:
			return "MSR_KVM_WALL_CLOCK";

		case MSR_KVM_SYSTEM_TIME_NEW:
		case MSR_KVM_SYSTEM_TIME:
			return "MSR_KVM_SYSTEM_TIME";

		case APIC_BASE_MSR ... (APIC_BASE_MSR + 0x3ff): 
			{
				unsigned int reg = (msr - APIC_BASE_MSR) << 4;
				switch (reg) {
					case APIC_TASKPRI:		
						return "APIC_TASKPRI";

					case APIC_EOI:
						return "APIC_EOI";

					case APIC_LDR:
						return "APIC_LDR";

					case APIC_DFR:
						return "APIC_DFR";

					case APIC_SPIV:
						return "APIC_SPIV";

					case APIC_ICR:
						return "APIC_ICR";

					case APIC_ICR2:
						return "APIC_ICR2";

					case APIC_LVT0:
					case APIC_LVTTHMR:
					case APIC_LVTPC:
					case APIC_LVT1:
					case APIC_LVTERR:
						return "APIC_LVT0";

					case APIC_LVTT:
						return "APIC_LVTT";

					case APIC_SELF_IPI:
						return "APIC_SELF_IPI";

					default :
						return "APIC_OTHERS";
				}
			}

		case MSR_CORE_PERF_FIXED_CTR0:
			return "MSR_CORE_PERF_FIXED_CTR0";

		case MSR_CORE_PERF_FIXED_CTR1:
			return "MSR_CORE_PERF_FIXED_CTR1";

		case MSR_CORE_PERF_FIXED_CTR2:
			return "MSR_CORE_PERF_FIXED_CTR2";

		case MSR_CORE_PERF_FIXED_CTR_CTRL:
			return "MSR_CORE_PERF_FIXED_CTR_CTRL";

		case MSR_CORE_PERF_GLOBAL_STATUS:
			return "MSR_CORE_PERF_GLOBAL_STATUS";

		case MSR_CORE_PERF_GLOBAL_CTRL:
			return "MSR_CORE_PERF_GLOBAL_CTRL";

		case MSR_CORE_PERF_GLOBAL_OVF_CTRL:
			return "MSR_CORE_PERF_GLOBAL_OVF_CTRL";

		default :
			return "MSR_OTHERS";
	}
}

void init_wrmsr(void)
{
	spin_lock_init(&other_msrs_lock);
	reset_other_wrmsr();
}

#endif
