#ifndef LOOP_PARTITIONERS_H
#define LOOP_PARTITIONERS_H

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#include "structs.h"
#include "env.h"


void loop_partitioner_entire_field(long start, long end, long * part_start, long * part_end);
void loop_partitioner_owner_group(long start, long end, long * part_start, long * part_end);


#endif /* LOOP_PARTITIONERS_H */

