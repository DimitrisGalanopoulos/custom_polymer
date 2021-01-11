#ifndef MEASURE_TIME_H
#define MEASURE_TIME_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>


#define CLOCK_GETTIME_OVERHEAD_N 100000
#define NANO 1000000000LL

static __attribute__ ((unused))
uint64_t
measure_clock_gettime_overhead()
{
	struct timespec t1, t2;
	uint64_t tmp, overhead = ~0;
	int i;
	
	for (i=0;i<CLOCK_GETTIME_OVERHEAD_N;i++)
	{
		
		clock_gettime(CLOCK_MONOTONIC, &t1);
		asm volatile("");
		clock_gettime(CLOCK_MONOTONIC, &t2);
		tmp = (t2.tv_sec-t1.tv_sec) * NANO + t2.tv_nsec - t1.tv_nsec;
		
		if (tmp < overhead)
			overhead = tmp;
	}
	return overhead;
}

#define start_timer(id)                                                          \
	struct timespec __##id##t1, __##id##t2;                                  \
	do clock_gettime(CLOCK_MONOTONIC, &__##id##t1); while (0)

#define stop_timer(id)                                                                                                       \
({                                                                                                                           \
	clock_gettime(CLOCK_MONOTONIC, &__##id##t2);                                                                         \
	((double) ((__##id##t2.tv_sec-__##id##t1.tv_sec) * NANO + __##id##t2.tv_nsec - __##id##t1.tv_nsec)) / NANO;          \
})


#define time_it(times, ...)                            \
({                                                     \
	__auto_type __times = times;                   \
	typeof(__times) __i;                           \
	                                               \
	start_timer(time_it);                          \
	for (__i=0;__i<__times;__i++)                  \
	{                                              \
		__VA_ARGS__                            \
	}                                              \
	stop_timer(time_it);                           \
})



#endif /* MEASURE_TIME_H */

