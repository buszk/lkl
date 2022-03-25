#define __USE_GNU
#include <stdlib.h>
#include <fcntl.h> 
#include <stdio.h> 
#include <stdarg.h>
#include <string.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <pthread.h>
#include <ucontext.h>
#include <lkl.h>

#include <capstone/capstone.h>

#include "list.h"
#include "memwatcher.h"

/* workaround for ucontext.h */
#define REG_RIP 16
#define REG_ERR 19

#define PAGE_SIZE 0x1000
#define INLINE inline __attribute__((always_inline))
#define MEMWATCHER_STR "MemWatcher"

#define PRINTF(fmt, ...) do { fprintf(stderr, "%s: " fmt, MEMWATCHER_STR, ##__VA_ARGS__); } while(0)
#define PERROR(fmt, ...) do { fprintf(stderr, "%s: Error " fmt "\n", MEMWATCHER_STR, ##__VA_ARGS__); exit(1); } while(0)
#ifdef MEMWATCHER_DEBUG
#define DPRINTF(fmt, ...) do { fprintf(stderr, "%s: " fmt, MEMWATCHER_STR, ##__VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif


/* 
 * mprotect() works with pages. 
 * We use this data structure to keep tracked pages 
 */
struct _watch_page {
    struct _watch_page *next, *prev;
    void  *addr;
    size_t size;
    uint16_t region_cnt;
    int prot;
};

/* 
 * tracked regions could be of arbitrary alignment and size
 * We use this data structure to keep tracked regions 
 */
struct _watch_region {
    struct _watch_region *next, *prev;
    void  *addr;
    size_t size;
    uint16_t ref_cnt;
    read_cb mem_read_cb;
};

/*
 * tracked page and tracked regions has n to n relations
 * We use this data structure to keep track of links
 */
struct _page_region_link {
    struct _page_region_link *next, *prev;
    struct _watch_page    *page;
    struct _watch_region  *region;
};

/*
 * After a tracked map access triggers a sigsegv, we craft
 * another payload to run and get callback from there to 
 * reenable memory access.
 */
struct _seg_callback {
    struct _seg_callback *next, *prev;
    void *page;
    void *next_inst;
    struct _watch_page *watched_page;
    struct _watch_region *watched_region;
};

struct _page_region_link  *_link_list         = NULL;
struct _watch_region      *_region_watchlist  = NULL;
struct _watch_page        *_page_watchlist    = NULL;
struct _seg_callback      *_callback_list     = NULL;

pthread_spinlock_t page_list_lock;
pthread_spinlock_t cb_list_lock;

uint32_t count;
struct sigaction sa;

void print_trace (void);
static void _sigsegv_protector(int s, siginfo_t *sig_info, void *context);

/* For testing purpose */
uint32_t _get_trap_count() {
    return count;
}

static void __attribute__((constructor)) _init_memwatcher() {
    int ret;
    if ((ret = pthread_spin_init(&page_list_lock, PTHREAD_PROCESS_PRIVATE)) != 0) {
        PERROR("init spin lock");
    }
    if ((ret = pthread_spin_init(&cb_list_lock, PTHREAD_PROCESS_PRIVATE)) != 0) {
        PERROR("init spin lock");
    }

    // Setup our signal handler 
    memset (&sa, 0, sizeof (sa)); 
    sa.sa_sigaction = &_sigsegv_protector;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGSEGV, &sa, NULL); 
}

INLINE void _pagelist_lock() {
    int ret;
    if ((ret = pthread_spin_lock(&page_list_lock)) != 0) {
        PERROR("lock");
    }
}

INLINE void _pagelist_unlock() {
    int ret;
    if ((ret = pthread_spin_unlock(&page_list_lock)) != 0) {
        PERROR("unlock");
    }
}

INLINE void _cblist_lock() {
    int ret;
    if ((ret = pthread_spin_lock(&cb_list_lock)) != 0) {
        PERROR("lock");
    }
}

INLINE void _callbackinfo_unlock() {
    int ret;
    if ((ret = pthread_spin_unlock(&cb_list_lock)) != 0) {
        PERROR("unlock");
    }
}

void _memwatcher_reset(void) {
    _region_watchlist  = NULL;
    _page_watchlist    = NULL;
    _callback_list     = NULL;
    pthread_spin_unlock(&page_list_lock);
    pthread_spin_unlock(&cb_list_lock);
}

void _watch_page(void *addr, int prot) {
    printf("%s %p\n", __func__, addr);
    struct _watch_page *pagep;
    list_for_each(pagep, _page_watchlist) {
        if (pagep->addr == addr) {
            if (pagep->prot == prot) {
                DPRINTF("inc\n");
                pagep->region_cnt ++;
            }
            else {
                PERROR("trying to set different protection on the same page");
            }
            return;
        }
    }
    /* new tracked page */
    struct _watch_page *watched_page = malloc(sizeof(*watched_page));
    watched_page->addr = addr;
    watched_page->size = PAGE_SIZE;
    watched_page->prot = prot;
    watched_page->region_cnt = 1;

    LIST_ADD(_page_watchlist, watched_page);

    // now protect the memory map
    if (mprotect(addr, PAGE_SIZE, prot) < 0) {
        DPRINTF("mprotect failed\n");
        perror(__func__);
    }
}

void _watch_address(void *addr, size_t size, int prot, read_cb cb) {
    printf("%s %p[%lx]\n", __func__, addr, size);
    struct _watch_region *watched_region = malloc(sizeof(*watched_region));
    watched_region->addr = addr;
    watched_region->size = size;
    watched_region->ref_cnt = 1;
    watched_region->mem_read_cb = cb;
    
    _pagelist_lock();
    struct _watch_region *regionp;
    list_for_each(regionp, _region_watchlist) {
        if ((watched_region->addr >= regionp->addr && watched_region->addr < regionp->addr + regionp->size) ||
            (watched_region->addr+watched_region->size > regionp->addr && watched_region->addr+watched_region->size <= regionp->addr + regionp->size)) {
            PERROR("Request region in range\n");
        }
    }

    LIST_ADD(_region_watchlist, watched_region);
    for (uintptr_t page = (uintptr_t)addr & -PAGE_SIZE; 
            page < (uintptr_t)addr + size; page+=PAGE_SIZE) {
        _watch_page((void*)page, prot);
    }

    _pagelist_unlock();

}

void _unwatch_page(void*addr, int prot) {

    struct _watch_page *pagep;
    list_for_each(pagep, _page_watchlist) {
        if (pagep->addr == addr) {
            if (pagep->region_cnt > 1) {
                pagep->region_cnt--;
                DPRINTF("dec\n");
            }
            else {
                if (mprotect(pagep->addr, pagep->size, prot) < 0) {
                    DPRINTF("mprotect error\n");
                    perror(__func__);
                }
                LIST_DEL(_page_watchlist, pagep);
                free(pagep);
            }
            break;
        }
    }

}

void _unwatch_address(void *addr, int prot) {
    _pagelist_lock();
    struct _watch_region *regionp;
    list_for_each(regionp, _region_watchlist) {
        if (regionp->addr == addr) {
            break;
        }
    }
    if (!regionp) {
        PERROR("unwatch address does not exist");
    }
    for (uintptr_t page = (uintptr_t)regionp->addr & -PAGE_SIZE;
                    page < (uintptr_t)regionp->addr + regionp->size; 
                    page += PAGE_SIZE) {
                _unwatch_page((void*)page, prot);
    }
    if (--regionp->ref_cnt == 0)
        LIST_DEL(_region_watchlist, regionp);
    _pagelist_unlock();
}

csh handle;
static int init_csh() {
    static int init = 0;
    if (!init) {
        if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
            DPRINTF("%s: cs_open failed\n", __func__);
            return -1;
        }
        if (cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) != CS_ERR_OK) {
            DPRINTF("%s: cs_option failed\n", __func__);
            return -1;
        }
        init = 1;
    }
    return 0;
}

static int _instr_decode(void* pc, uint8_t *len, uint8_t *size) {
    cs_insn *insn;
    size_t count;
    if (init_csh()) {
        DPRINTF("%s: init_csh failed\n", __func__);
        return -1;
    }
    count = cs_disasm(handle, pc, 15, 1, 0, &insn);
    if (count < 1) {
        DPRINTF("%s: cs_disasm failed: count=%ld\n", __func__, count);
        return -1;
    }
    DPRINTF("%s: code: %s %s\n", __func__, insn->mnemonic, insn->op_str);
    if (insn->detail->x86.op_count < 2) {
        DPRINTF("%s: read/write instruction has less than 2 operands\n", __func__);
        printf("r/w instruction has less than 2 operands, code: %s %s\n", insn->mnemonic, insn->op_str);
        exit(1);
        return -1;
    }
    DPRINTF("%s: access: %d\n", __func__, insn->detail->x86.operands[1].size);
    *len = insn->size;
    *size = insn->detail->x86.operands[1].size;
    cs_free(insn, count);
    return 0;
}

static void* _alloc_payload(void* inst, uint8_t inst_len) {
    char *res;
    res = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (res == 0) {
        exit(2);
    }
    DPRINTF("%s: Page allocated at %p\n", __func__, res);
    memcpy(res, inst, inst_len);
    // mov    QWORD PTR [rip+0x0],0x1
    // should trigger another sigsegv
    char asms[] = { 0x48, 0xC7, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
    memcpy(res+inst_len, asms, sizeof(asms));
    if (mprotect(res, PAGE_SIZE, PROT_EXEC|PROT_READ) < 0) {
        DPRINTF("mprotect failed\n");
        perror(__func__);
        exit(1);
    }
    return res;
}

static void _real_segfault() {
    PRINTF("segfault\n");
    print_trace();
    exit(139);
}

static void _sigsegv_protector(int s, siginfo_t *sig_info, void *vcontext)
{
    ucontext_t *context = (ucontext_t*)vcontext;
    uint8_t len = 0, size = 0;
    void *rip;
    uint64_t offset = 0, err_code = 0;
    int is_read = 0;
    DPRINTF("---\n");
    DPRINTF("%s: Process received segmentation fault, examening ...\n", __func__);
    DPRINTF("%s: cause was address %p\n", __func__, sig_info->si_addr);

    rip = (void*)context->uc_mcontext.gregs[REG_RIP];

    if (_instr_decode(rip, &len, &size) < 0) {
        DPRINTF("%s: _instr_decode failed\n", __func__);
    }
    
    err_code = context->uc_mcontext.gregs[REG_ERR];
    #define PF_WRITE 1 << 1
    is_read = !(err_code & PF_WRITE);

    DPRINTF("%s: Access Instr loc:  %p\n", __func__, rip);
    DPRINTF("%s:              mem:  %p\n", __func__, sig_info->si_addr);
    DPRINTF("%s:              len:  %d\n", __func__, len);
    DPRINTF("%s:              size: %d\n", __func__, size);
    DPRINTF("%s:              read: %d\n", __func__, is_read);
    

    struct _seg_callback *callback;
    _cblist_lock();

    list_for_each(callback, _callback_list) {
        if ( sig_info->si_addr >= callback->page &&
             sig_info->si_addr < callback->page + PAGE_SIZE)
             break;
    }

    _pagelist_lock();

    if (callback) {
        /* change page permission back and set RIP back */
        DPRINTF("%s: raised because of trap from callback ...\n", __func__);
        mprotect(callback->watched_page->addr, callback->watched_page->size, callback->watched_page->prot);
        context->uc_mcontext.gregs[REG_RIP] = (uintptr_t)callback->next_inst;

        /* dealloc callback */
        munmap(callback->page, PAGE_SIZE);
        if (callback->watched_region && --callback->watched_region->ref_cnt == 0) {
            LIST_DEL(_region_watchlist, callback->watched_region);
        }
        
        LIST_DEL(_callback_list, callback);
        if (callback->watched_region) {
            DPRINTF("%s: region watched\n", __func__);
            count ++;
        }
        free(callback);
        DPRINTF("---\n");
        goto release;
    } 

    struct _watch_page *watched_page;

    list_for_each(watched_page, _page_watchlist) {
        if ( sig_info->si_addr >= watched_page->addr && 
             sig_info->si_addr < watched_page->addr + watched_page->size)
            break;
    }

    struct _watch_region *watched_region;
    list_for_each(watched_region, _region_watchlist) {
        if (sig_info->si_addr >= watched_region->addr &&
            sig_info->si_addr < watched_region->addr + watched_region->size)
            break;
    }


    if (watched_page) {
        DPRINTF("%s: raised because of invalid r/w acces to address (was in watchlist) ...\n", __func__);
        // printf("Fault instruction loc: 0x%016llx, len: %d\n", context->uc_mcontext.gregs[REG_RIP], len);
        // context->uc_mcontext.gregs[REG_RIP] += len;

            
        mprotect (watched_page->addr, watched_page->size, PROT_READ | PROT_WRITE);
        
        // if callback exists, call here to modify the memory
        if (is_read && watched_region && watched_region->mem_read_cb) {
            offset = sig_info->si_addr - watched_region->addr;
            watched_region->mem_read_cb((void*)sig_info->si_addr, offset, size);
        }

        void *payload_page = (void*)_alloc_payload(rip, len);
        context->uc_mcontext.gregs[REG_RIP] = (uintptr_t)payload_page;
        struct _seg_callback *cb = malloc(sizeof(struct _seg_callback));
        cb->page = payload_page;
        cb->next_inst = rip+len;
        cb->watched_page = watched_page;
        cb->watched_region = watched_region;
        /* watched_region could be NULL */
        if (watched_region)
            watched_region->ref_cnt ++;

        LIST_ADD(_callback_list, cb);
        

        DPRINTF("---\n");
        goto release;
    }
    

    /* !watched_page && !callback */
    
    /* Real sigsegv */
    DPRINTF("---\n");
    PRINTF("%s: Access Instr loc:  %p\n", __func__, rip);
    PRINTF("%s:              mem:  %p\n", __func__, sig_info->si_addr);
    PRINTF("%s:              len:  %d\n", __func__, len);
    PRINTF("%s:              size: %d\n", __func__, size);
    PRINTF("%s:              read: %d\n", __func__, is_read);
    lkl_dump_stack();
    abort();
    _real_segfault();

release:
    // ignore exit above
    _pagelist_unlock();
    _callbackinfo_unlock();
}