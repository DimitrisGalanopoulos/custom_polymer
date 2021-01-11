#ifndef GOMP_POLYMER_H
#define GOMP_POLYMER_H

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#include "macros/cpp_defines.h"
#include "macros/common_macros.h"
#include "measure_time.h"
#include "debug.h"

#include "structs.h"
#include "env.h"
#include "loop_partitioners.h"


#ifndef SCHEDULE
	#define SCHEDULE  static
	// #define SCHEDULE  dynamic
	// #define SCHEDULE  hierarchical
#endif

#ifndef GRAIN_SIZE
	#define GRAIN_SIZE  1
#endif

#ifndef WAIT_POLICY
	// #define WAIT_POLICY  nowait
	#define WAIT_POLICY 
#endif


//==========================================================================================================================================
//= Declarations
//==========================================================================================================================================


// io.c
void create_graph_structures(char * file, int balance_in_edges);

// frontier.c
void clear_next_frontiers();

// user functions (e.g. bfs.c)
int update(long src, long trg);
int update_atomic(long src, long trg);
int cond(long d);
void init(long N);


//==========================================================================================================================================
//= Parallel Array Initialization
//==========================================================================================================================================


/*
 * Init array of N longs so that the each elem 'i' is placed in the NUMA node that
 * the 'i' graph node belongs.
 */

static inline
void
numa_init_array_long(long * array, long value)
{
	_Pragma("omp parallel")
	{
		long i;
		omp_set_hierarchical_static(1);
		omp_set_loop_partitioner(loop_partitioner_owner_group);
		PRAGMA(omp for schedule(hierarchical, GRAIN_SIZE))
		for (i=0;i<N;i++)
			array[i] = value;
	}
}


#endif /* GOMP_POLYMER_H */

