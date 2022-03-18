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


extern short pci_vender;
extern short pci_device;
extern short pci_revision;

void *input_buffer =NULL;
ssize_t input_size =0;


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

    assert(argc > 1);

    // atlantic
    pci_vender = 0x1d6a;
    pci_device = 0x1;
    pci_revision = 0x1;

    // snic
    // pci_vender = 0x1137;
    // pci_device = 0x0046;
    // pci_revision = 0x1;

    // 8139cp
    // pci_vender = 0x10ec;
    // pci_device = 0x8139;
    // pci_revision = 0x20;

    // ath9k
    // pci_vender = 0x168c;
    // pci_device = 0x0023;
    // pci_revision = 0x0;

	fill_kasan_meta(&kasan_meta, "afl-harness");
    
    __AFL_INIT();
	lkl_kasan_init(&lkl_host_ops,
			128 * 1024 * 1024,
            kasan_meta.stack_base,
            kasan_meta.stack_size,
            kasan_meta.global_base,
            kasan_meta.global_size
            );

    lkl_delayed_pci_init();
    lkl_start_kernel(&lkl_host_ops, "mem=128M loglevel=8 lkl_pci=vfio");
    lkl_pci_init();
    // static int counter = 0;
    // while (counter ++ < 1000) {
    while (__AFL_LOOP(1000)) {
        get_afl_input(argv[1]);
        fuzz_driver();
        fprintf(stderr, "afl_loop iter ends\n");
    }
    // lkl_sys_halt();

	return 0;
}
