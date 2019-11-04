/*
 * Copyright (C) 2019 zhenwei pi pizhenwei@bytedance.com.
 */
#ifndef __GETNS_H__
#define __GETNS_H__

static inline unsigned long getns(void)
{
	return ktime_to_ns(ktime_get());
}

#endif
