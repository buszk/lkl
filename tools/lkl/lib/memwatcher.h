

#include <sys/mman.h> 
#include <signal.h> 
#include <inttypes.h>

typedef void (*read_cb)(void* addr, uint64_t offset, uint8_t size);

void _watch_address(void *addr, size_t size, int prot, read_cb cb);
void _unwatch_address(void *addr, int prot);
void _memwatcher_reset(void);
int _in_cb(void);

/* for test purpose */
uint32_t _get_trap_count();