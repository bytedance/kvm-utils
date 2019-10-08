/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_X86_KVM_PARA_H
#define _UAPI_ASM_X86_KVM_PARA_H

/* This CPUID returns the signature 'KVMKVMKVM' in ebx, ecx, and edx.  It
 * should be used to determine that a VM is running under KVM.
 */
#define KVM_CPUID_SIGNATURE	0x40000000

/* This CPUID returns a feature bitmap in eax.  Before enabling a particular
 * paravirtualization, the appropriate feature bit should be checked.
 */
#define KVM_CPUID_FEATURES	0x40000001
#define KVM_FEATURE_CLOCKSOURCE		0
#define KVM_FEATURE_NOP_IO_DELAY	1
#define KVM_FEATURE_MMU_OP		2
/* This indicates that the new set of kvmclock msrs
 * are available. The use of 0x11 and 0x12 is deprecated
 */
#define KVM_FEATURE_CLOCKSOURCE2        3
#define KVM_FEATURE_ASYNC_PF		4
#define KVM_FEATURE_STEAL_TIME		5
#define KVM_FEATURE_PV_EOI		6
#define KVM_FEATURE_PV_UNHALT		7
#define KVM_FEATURE_ASYNC_PF_VMEXIT	10
#define KVM_FEATURE_PV_SEND_IPI		11

/* The last 8 bits are used to indicate how to interpret the flags field
 * in pvclock structure. If no bits are set, all flags are ignored.
 */
#define KVM_FEATURE_CLOCKSOURCE_STABLE_BIT	24

#define MSR_KVM_WALL_CLOCK  0x11
#define MSR_KVM_SYSTEM_TIME 0x12

#define KVM_MSR_ENABLED 1
/* Custom MSRs falls in the range 0x4b564d00-0x4b564dff */
#define MSR_KVM_WALL_CLOCK_NEW  0x4b564d00
#define MSR_KVM_SYSTEM_TIME_NEW 0x4b564d01
#define MSR_KVM_ASYNC_PF_EN 0x4b564d02
#define MSR_KVM_STEAL_TIME  0x4b564d03
#define MSR_KVM_PV_EOI_EN      0x4b564d04

#define KVM_STEAL_ALIGNMENT_BITS 5
#define KVM_STEAL_VALID_BITS ((-1ULL << (KVM_STEAL_ALIGNMENT_BITS + 1)))
#define KVM_STEAL_RESERVED_MASK (((1 << KVM_STEAL_ALIGNMENT_BITS) - 1 ) << 1)

#define KVM_MAX_MMU_OP_BATCH           32

#define KVM_ASYNC_PF_ENABLED			(1 << 0)
#define KVM_ASYNC_PF_SEND_ALWAYS		(1 << 1)
#define KVM_ASYNC_PF_DELIVERY_AS_PF_VMEXIT	(1 << 2)

/* Operations for KVM_HC_MMU_OP */
#define KVM_MMU_OP_WRITE_PTE            1
#define KVM_MMU_OP_FLUSH_TLB	        2
#define KVM_MMU_OP_RELEASE_PT	        3
#define KVM_PV_REASON_PAGE_NOT_PRESENT 1
#define KVM_PV_REASON_PAGE_READY 2

#define KVM_PV_EOI_BIT 0
#define KVM_PV_EOI_MASK (0x1 << KVM_PV_EOI_BIT)
#define KVM_PV_EOI_ENABLED KVM_PV_EOI_MASK
#define KVM_PV_EOI_DISABLED 0x0


#endif /* _UAPI_ASM_X86_KVM_PARA_H */
