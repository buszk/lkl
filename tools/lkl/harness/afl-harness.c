// SPDX-License-Identifier: GPL-2.0
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <lkl.h>
#include <lkl_host.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "pmparser.h"


struct lkl_kasan_meta {
    unsigned long stack_base;
    unsigned long stack_size;
    unsigned long global_base;
    unsigned long global_size;
};

void fill_kasan_meta(struct lkl_kasan_meta* to) {
    static int add =0;
    struct procmaps_struct *head, *pt;
    pid_t pid = getpid();
    printf("my pid: %d\n",pid);

    head = pmparser_parse(pid);
    //pmparser_print(head, -1);
    
    pt = head;
    to->global_base = 0;
    to->global_size = 0;
    while(pt != NULL) {
        if(strncmp(pt->pathname,"[stack]",10) == 0) {
            to->stack_base = (unsigned long)pt->addr_start;
            to->stack_size = (unsigned long)pt->length; 
        }
        if(strcasestr(pt->pathname,"afl-harness")) {
            if (to->global_base == 0) {
                to->global_base = (unsigned long)pt->addr_start;
                to->global_size = (unsigned long)pt->length; 
            }
            else if ((unsigned long)pt->addr_start > to->global_base) {
                to->global_size = (unsigned long)pt->addr_start - to->global_base + (unsigned long)pt->length;
            }
            else {
                abort();
            }
            add =1;
        }
        else if (add)
        {
            add =0;
            to->global_size = (unsigned long)pt->addr_start - to->global_base + (unsigned long)pt->length;
        }
        
        pt = pt->next;
    }
    return;

}

int main() {
	unsigned long stack_base;
	struct lkl_kasan_meta kasan_meta;

	fill_kasan_meta(&kasan_meta);
	lkl_kasan_init(&lkl_host_ops,
			16 * 1024 * 1024,
			kasan_meta.stack_base,
			kasan_meta.stack_size,
            kasan_meta.global_base,
            kasan_meta.global_size
            );

	lkl_start_kernel(&lkl_host_ops, "mem=16M loglevel=8 lkl_pci=vfio");
	// lkl_sys_halt();

	return 0;
}
