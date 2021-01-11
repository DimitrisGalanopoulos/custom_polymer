#include "loop_partitioners.h"


void
loop_partitioner_entire_field(long start, long end, long * part_start, long * part_end)
{
	*part_start = start;
	*part_end = end;
}


void
loop_partitioner_owner_group(long start __attribute__((unused)), long end __attribute__((unused)), long * part_start, long * part_end)
{
	int tgnum = omp_get_thread_group_num();
	struct thread_group_data    * tgd   = &TGD[tgnum];
	struct graph_partition_data * gpart = tgd->graph_partition;

	*part_start = gpart->node_id_s;
	*part_end = gpart->node_id_e;
}


