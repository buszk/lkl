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

#include "../afl.h"
#include "../pmparser.h"
#include "../targets.h"


extern short pci_vender;
extern short pci_device;
extern short pci_revision;
extern int bus_type; 

void *input_buffer =NULL;
ssize_t input_size =0;

void get_afl_input(char* fname) {
    int fd;
    struct stat buf;
    fd = open(fname, O_RDONLY);
    fstat(fd, &buf);
    input_size = buf.st_size;
    input_buffer = malloc(input_size);
    if (!input_buffer)
        abort();
    assert(read(fd, input_buffer, input_size) == input_size);
    lkl_set_fuzz_input(input_buffer, input_size);
}

int func(int argc, char**argv) {
	struct lkl_kasan_meta kasan_meta = {0};

    assert(argc > 1);


	fill_kasan_meta(&kasan_meta, "fuzz-test");
	lkl_kasan_init(&lkl_host_ops,
			128 * 1024 * 1024,
            kasan_meta.stack_base,
            kasan_meta.stack_size,
            kasan_meta.global_base,
            kasan_meta.global_size
            );

    lkl_start_kernel(&lkl_host_ops, "mem=128M loglevel=8 lkl_pci=vfio");
    while (__AFL_LOOP(1000)) {
        get_afl_input(argv[1]);
        fuzz_driver();
    }
    // lkl_sys_halt();

	return 0;
}
