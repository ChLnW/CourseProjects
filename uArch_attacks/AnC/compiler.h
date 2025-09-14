#pragma once
#define barrier() asm volatile("" ::: "memory")

#define ACCESS_ONCE(x) (*(volatile typeof(x) *) &(x))
#define ARR_SZ(a)      (sizeof(a) / sizeof(*a))

#define _ptr(val) ((void *) (unsigned long) (val))
#define _ul(val)  ((unsigned long) (val))
#define _u(val)   ((unsigned int) (val))
#define _int(val) ((int) (val))

#define __used     __attribute__((__used__))
#define __noinline __attribute__((__noinline__))

#define KB(x) (_ul(x) << 10)
#define MB(x) (_ul(x) << 20)
#define GB(x) (_ul(x) << 30)
