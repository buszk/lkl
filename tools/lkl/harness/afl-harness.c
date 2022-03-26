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



void *input_buffer =NULL;
ssize_t input_size =0;

int set_target(const char *);

void get_afl_input(char* fname) {
    int fd;
    struct stat buf;
    // fprintf(stderr, "%s\n", fname);
    fd = open(fname, O_RDONLY);
    if (fd < 0)
        perror(__func__), exit(1);
    fstat(fd, &buf);
    input_size = buf.st_size;
    if (input_buffer)
        free(input_buffer);
    input_buffer = malloc(input_size);
    if (!input_buffer)
        abort();
    assert(read(fd, input_buffer, input_size) == input_size);
    close(fd);

    // for (int i = 0; i < input_size; i++) {
    //     fprintf(stderr, "%x", ((uint8_t*)input_buffer)[i]);
    // }
    // fprintf(stderr, "\n");
    lkl_set_fuzz_input(input_buffer, input_size);
}

int main(int argc, char**argv) {
	struct lkl_kasan_meta kasan_meta = {0};

    assert(argc > 2);

    if (set_target(argv[1])) {
        fprintf(stderr, "Please provide a device driver target to fuzz");
        abort();
    }

	fill_kasan_meta(&kasan_meta, "afl-harness");
    
    __AFL_INIT();
	lkl_kasan_init(&lkl_host_ops,
			128 * 1024 * 1024,
            kasan_meta.stack_base,
            kasan_meta.stack_size,
            kasan_meta.global_base,
            kasan_meta.global_size
            );

    lkl_start_kernel(&lkl_host_ops, "mem=128M loglevel=8 lkl_pci=vfio");
    // static int counter = 0;
    // while (counter ++ < 1000) {
    while (__AFL_LOOP(1000)) {
        get_afl_input(argv[2]);
        fuzz_driver();
        fprintf(stderr, "afl_loop iter ends\n");
    }
    // lkl_sys_halt();

	return 0;
}
