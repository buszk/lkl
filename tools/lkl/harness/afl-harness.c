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
#include "afl.h"
#include "pmparser.h"


extern short pci_vender;
extern short pci_device;
extern short pci_revision;

int main() {
	struct lkl_kasan_meta kasan_meta = {0};

    pci_vender = 0x1d6a;
    pci_device = 0x1;
    pci_revision = 0x1;

	fill_kasan_meta(&kasan_meta, "afl-harness");
	lkl_kasan_init(&lkl_host_ops,
			16 * 1024 * 1024,
            kasan_meta.stack_base,
            kasan_meta.stack_size,
            kasan_meta.global_base,
            kasan_meta.global_size
            );

    lkl_delayed_pci_init();
    lkl_start_kernel(&lkl_host_ops, "mem=16M loglevel=8 lkl_pci=vfio");
    __AFL_INIT();
    lkl_pci_init();
    // lkl_sys_halt();

	return 0;
}
