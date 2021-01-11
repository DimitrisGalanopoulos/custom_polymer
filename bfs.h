#ifndef BFS_H
#define BFS_H

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#include "macros/cpp_defines.h"
#include "macros/common_macros.h"
#include "measure_time.h"
#include "debug.h"

#include "gomp_polymer.h"

extern long * parents;


extern int update(long src, long trg);
extern int update_atomic(long src, long trg);
extern int cond(long d);
extern void init(long N);

void print_parents();


#endif /* BFS_H */

