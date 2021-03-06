#ifndef BFS_MULTINODE_CONSTANTS_H
#define BFS_MULTINODE_CONSTANTS_H

#define NULL_VERTEX (1L << 32) -  1 // -1UL = (2^32)L
#define OMP_CHUNK 2

#define _unused(x) ((void)(x))

#if HAVE_AVX
#define ALIGNMENT 32UL
#else
#define ALIGNMENT 16UL
#endif


#endif // BFS_MULTINODE_CONSTANTS_H
