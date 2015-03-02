#ifndef SHAREDMEM_H_
#define SHAREDMEM_H_


static unsigned char *shared_memory_ptr;
static int shared_memory_id = 0;
// Darwin by default has a very small limit on the maximum amount of shared
// memory, which will quickly be exhausted by KLEE running its tests in
// parallel. For now, we work around this by just requesting a smaller size --
// in practice users hitting this limit on counterexample sizes probably already
// are hitting more serious scalability issues.
#ifdef __APPLE__
static const unsigned shared_memory_size = 1<<16;
#else
static const unsigned shared_memory_size = 1<<20;
#endif

#endif /* SHAREDMEM_H_ */
