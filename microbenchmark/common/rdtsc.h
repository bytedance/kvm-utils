/*
 * Copyright (C) 2019 zhenwei pi pizhenwei@bytedance.com.
 */
#ifndef __RDTSC_H__
#define __RDTSC_H__

static inline unsigned long ins_rdtsc(void)
{
    unsigned long low, high;

    asm volatile("rdtsc" : "=a" (low), "=d" (high) );

    return ((low) | (high) << 32);
}

#endif
