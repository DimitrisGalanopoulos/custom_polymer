#include "bfs.h"


long * parents;


inline
int
update(long src, long trg)                         // Update() (destination/child pulls from source/parent).
{
	if (parents[trg] == -1)
	{
		parents[trg] = src;
		return 1;
	}
	else
		return 0;
}


inline
int
update_atomic(long src, long trg)                  // Atomic version of Update() (source/parent pushes to destination/child).
{
	long expected = -1;
	return (__atomic_compare_exchange_n(&parents[trg], &expected, src, 0,  __ATOMIC_SEQ_CST,  __ATOMIC_RELAXED));
}


inline
int
cond(long trg)                                      // 'cond()' function checks if vertex (destination/child) has been visited yet
{                                                  // (for avoiding useless CAS invocations, and stopping dense earlier).
	return (parents[trg] == -1);
}


inline
void
init(long N)
{
	alloc(N, parents);
	numa_init_array_long(parents, -1);
}


void
print_parents()
{
	printf("\nParents:\n");
	for (long i=0;i<N;i++)
	{
		printf("%ld: %ld\n", i, parents[i]);
	}
}


