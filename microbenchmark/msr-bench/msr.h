/*
 * Copyright (C) 2018 zhenwei pi pizhenwei@bytedance.com.
 */
#ifndef __MSR_H__
#define __MSR_H__

static inline void ins_wrmsr(unsigned int msr, unsigned int low, unsigned int high)
{
    asm volatile("wrmsr\n"
            : : "c" (msr), "a"(low), "d" (high) : "memory");
}

static inline void ins_wrmsrl(unsigned int msr, unsigned long val)
{
    ins_wrmsr(msr, (unsigned int)(val & 0xffffffffULL), (unsigned int)(val >> 32));
}

#endif
