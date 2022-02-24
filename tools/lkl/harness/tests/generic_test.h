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
#include "../pmparser.h"

#define KASAN_SOOB_TEST_ID 1
#define KASAN_UAF_TEST_ID 2

extern short pci_vender;
extern short pci_device;
extern short pci_revision;

static int kasan_test(short id) {
    struct lkl_kasan_meta kasan_meta = {0};
    
    pci_vender = 0x8888;
    pci_device = 2;
    pci_revision = id;
    
    fill_kasan_meta(&kasan_meta, "kasan-test");
    lkl_kasan_init(&lkl_host_ops,
            16 * 1024 * 1024,
            kasan_meta.stack_base,
            kasan_meta.stack_size,
            kasan_meta.global_base,
            kasan_meta.global_size
            );

    lkl_start_kernel(&lkl_host_ops, "mem=16M loglevel=8 lkl_pci=vfio");
    lkl_pci_init();
    // lkl_sys_halt();

    return 0;
}
