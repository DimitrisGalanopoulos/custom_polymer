#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#include "macros/cpp_defines.h"
#include "macros/common_macros.h"
#include "debug.h"


//==========================================================================================================================================
//= Edge
//==========================================================================================================================================


struct edge_data {
	long node_id;
};


//==========================================================================================================================================
//= Node
//==========================================================================================================================================


struct node_agent_data {
	long num_edges;
	struct edge_data * edges;
};


struct node_data {
	long rank_out;
	long rank_in;
};


//==========================================================================================================================================
//= Frontier
//==========================================================================================================================================


struct frontier_data {
	char * dense;
	long * sparse;
	long sparse_size;
};


static inline
char *
frontier_dense_new(long n)
{
	char * a;
	alloc(n, a);
	return a;
}


static inline
long *
frontier_sparse_new(long n)
{
	long * a;
	alloc(n, a);
	return a;
}


static inline
struct frontier_data *
frontier_new(long n)
{
	struct frontier_data * f;
	f->sparse_size = 16;
	f->sparse = frontier_sparse_new(f->sparse_size);
	return f;
}


//==========================================================================================================================================
//= Graph Partition
//==========================================================================================================================================


/* 
 * 
 * The node distribution logic is opposite to ligra.
 * - The nodes are distributed sequentially to the groups.
 *   Then, according to the node distribution:
 *       - For push, the out-edges are distributed based on their target.
 *       - For pull, the in-edges are distributed based on their source.
 *
 * If we have a two-way partition of the graph nodes, one for balancing the out-edges and one for the in-edges, 
 * it will be difficult to partition the application data, or we might have to keep multiple copies of them.
 * When we change from push to pull (or the other way around), we will have to pass that data,
 * from the out-partition of the vertices in push, to the in-partition of the vertices in pull.
 *
 * Since the bulk of the computation will usually be done in pull mode either way, we usually choose to
 * partition the vertices according to the in-edges.
 */
struct graph_partition_data {
	long * partitions_start_nodes;              // The start nodes of each partition.

	// Total nodes records.
	// Keep out and in data separated for locality, because at each stage (push or pull) we only need one of the two.
	struct node_agent_data * nodes_out;         // Out-edges the group owns (residing in 'edges_out').
	struct node_agent_data * nodes_in;          // In-edges the group owns (residing in 'edges_in').

	// The node partition the group owns.
	long node_id_s;                             // inclusive
	long node_id_e;                             // exclusive
	long num_local_nodes;
	struct node_data * local_node_data;

	// Partition edges.

	long num_edges_out;                         // Out-partition (the group owns the TARGETS of these edges).
	struct edge_data * edges_out;

	long num_edges_in;                          // In-partition (the group owns the SOURCES of these edges).
	struct edge_data * edges_in;

	// Frontiers.
	// More accurately, the part of each frontier that belongs to this partition.

	struct frontier_data * frontier_curr;
	struct frontier_data * frontier_next;
};


static inline
void
graph_partition_data_init(struct graph_partition_data * gpd)
{
	int num_groups = omp_get_num_thread_groups();
	alloc(num_groups + 1, gpd->partitions_start_nodes);
	alloc(1, gpd->frontier_curr);
	alloc(1, gpd->frontier_next);
}


//==========================================================================================================================================
//= Group Private Data
//==========================================================================================================================================


struct thread_group_data {
	struct graph_partition_data * graph_partition;

	char padding[0] __attribute__ ((aligned (CACHE_LINE_SIZE)));
} __attribute__ ((aligned (CACHE_LINE_SIZE)));


static inline
void
thread_group_data_init(struct thread_group_data * tgd)
{
	alloc(1, tgd->graph_partition);
	graph_partition_data_init(tgd->graph_partition);
}


#endif /* STRUCTS_H */

