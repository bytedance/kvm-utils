/*
 * Copyright (C) 2019 zhenwei pi pizhenwei@bytedance.com.
 */
#ifndef __EXIT_REASON_H__
#define __EXIT_REASON_H__

#define REASON_NUM 66

static int inited = 0;
static char *reasons[REASON_NUM] = {0};

void init_reasons(void)
{
	if (inited == 1)
		return;
	else
		inited = 1;

	reasons[0]="EXIT_REASON_EXCEPTION_NMI";
	reasons[1]="EXIT_REASON_EXTERNAL_INTERRUPT";
	reasons[2]="EXIT_REASON_TRIPLE_FAULT";
	reasons[7]="EXIT_REASON_PENDING_INTERRUPT";
	reasons[8]="EXIT_REASON_NMI_WINDOW";
	reasons[9]="EXIT_REASON_TASK_SWITCH";
	reasons[10]="EXIT_REASON_CPUID";
	reasons[12]="EXIT_REASON_HLT";
	reasons[13]="EXIT_REASON_INVD";
	reasons[14]="EXIT_REASON_INVLPG";
	reasons[15]="EXIT_REASON_RDPMC";
	reasons[16]="EXIT_REASON_RDTSC";
	reasons[18]="EXIT_REASON_VMCALL";
	reasons[19]="EXIT_REASON_VMCLEAR";
	reasons[20]="EXIT_REASON_VMLAUNCH";
	reasons[21]="EXIT_REASON_VMPTRLD";
	reasons[22]="EXIT_REASON_VMPTRST";
	reasons[23]="EXIT_REASON_VMREAD";
	reasons[24]="EXIT_REASON_VMRESUME";
	reasons[25]="EXIT_REASON_VMWRITE";
	reasons[26]="EXIT_REASON_VMOFF";
	reasons[27]="EXIT_REASON_VMON";
	reasons[28]="EXIT_REASON_CR_ACCESS";
	reasons[29]="EXIT_REASON_DR_ACCESS";
	reasons[30]="EXIT_REASON_IO_INSTRUCTION";
	reasons[31]="EXIT_REASON_MSR_READ";
	reasons[32]="EXIT_REASON_MSR_WRITE";
	reasons[33]="EXIT_REASON_INVALID_STATE";
	reasons[34]="EXIT_REASON_MSR_LOAD_FAIL";
	reasons[36]="EXIT_REASON_MWAIT_INSTRUCTION";
	reasons[37]="EXIT_REASON_MONITOR_TRAP_FLAG";
	reasons[39]="EXIT_REASON_MONITOR_INSTRUCTION";
	reasons[40]="EXIT_REASON_PAUSE_INSTRUCTION";
	reasons[41]="EXIT_REASON_MCE_DURING_VMENTRY";
	reasons[43]="EXIT_REASON_TPR_BELOW_THRESHOLD";
	reasons[44]="EXIT_REASON_APIC_ACCESS";
	reasons[45]="EXIT_REASON_EOI_INDUCED";
	reasons[48]="EXIT_REASON_EPT_VIOLATION";
	reasons[49]="EXIT_REASON_EPT_MISCONFIG";
	reasons[50]="EXIT_REASON_INVEPT";
	reasons[51]="EXIT_REASON_RDTSCP";
	reasons[52]="EXIT_REASON_PREEMPTION_TIMER";
	reasons[53]="EXIT_REASON_INVVPID";
	reasons[54]="EXIT_REASON_WBINVD";
	reasons[55]="EXIT_REASON_XSETBV";
	reasons[56]="EXIT_REASON_APIC_WRITE";
	reasons[58]="EXIT_REASON_INVPCID";
	reasons[62]="EXIT_REASON_PML_FULL";
	reasons[63]="EXIT_REASON_XSAVES";
	reasons[64]="EXIT_REASON_XRSTORS";
	reasons[65]="EXIT_REASON_PCOMMIT";
}

static unsigned long reasons_num[REASON_NUM] = {0};

void record_reason(int r)
{
	if (r >= REASON_NUM)
		return;

	reasons_num[r]++;
}

unsigned long report_reason(int r)
{
	if (r >= REASON_NUM)
		return 0;

	return reasons_num[r];
}

void reset_reason(void)
{
	memset(reasons_num, 0x00, sizeof(reasons_num));
}

char *reason2str(int r)
{
	if (r >= REASON_NUM)
		return "EXIT_REASON_OVERFLOW";

	init_reasons();
	if (reasons[r])
		return reasons[r];
	else
		return "EXIT_REASON_UNKNOWN";
}

#endif
