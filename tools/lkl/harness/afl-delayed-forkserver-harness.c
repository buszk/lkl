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
#include <sys/types.h>

#include "afl.h"
#include "pmparser.h"
#include "targets.h"



void *input_buffer =NULL;
ssize_t input_size =0;

static struct lkl_disk disk;
static int disk_id = -1;

#include "afl-harness-common.h"

int main(int argc, char**argv) {
    int ret;
    char mount_dir[64] = "/lib/firmware";
	struct lkl_kasan_meta kasan_meta = {0};

    assert(argc > 2);

    if (set_target(argv[1])) {
        fprintf(stderr, "Please provide a device driver target to fuzz");
        abort();
    }

	fill_kasan_meta(&kasan_meta, "afl-delayed-forkserver-harness");
    
	lkl_kasan_init(&lkl_host_ops,
			512 * 1024 * 1024,
            kasan_meta.stack_base,
            kasan_meta.stack_size,
            kasan_meta.global_base,
            kasan_meta.global_size
            );
    switch (bus_type)
    {
    case BUS_PCI:
        lkl_start_kernel(&lkl_host_ops, "mem=512M loglevel=8 lkl_pci=fuzz");
        break;
    case BUS_USB:
        lkl_start_kernel(&lkl_host_ops, "mem=512M loglevel=8 lkl_usb=fuzz");
        break;
    default:
        fprintf(stderr, "Unknown BUS_TYPE %d\n", bus_type);
        break;
    }
    load_firmware_disk();
    ret = lkl_mount_dev(disk_id, 0, "ext4", 0, "", mount_dir, sizeof(mount_dir));
    assert(ret == 0);
    __AFL_INIT();
    get_afl_input(argv[2]);
    fuzz_driver();
    // NOTE: umount and disk remove cause the harness to hang for some reason
    // NOTE: currently broken due to flush_schedued_work in lkl_pci_driver_run
	// lkl_umount_dev(disk_id, 0, 0, 1000);
    // lkl_disk_remove(disk);
    // lkl_sys_halt();

	return 0;
}
