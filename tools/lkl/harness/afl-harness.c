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
size_t input_size =0;

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

int main(int argc, char**argv) {
    int i, ret, idx = 0;
	struct lkl_kasan_meta kasan_meta = {0};
    char* ifnames[2] = { "eth0", "wlan0" };

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
	lkl_kasan_init(&lkl_host_ops,
			128 * 1024 * 1024,
            kasan_meta.stack_base,
            kasan_meta.stack_size,
            kasan_meta.global_base,
            kasan_meta.global_size
            );

    lkl_delayed_pci_init();
    lkl_start_kernel(&lkl_host_ops, "mem=128M loglevel=8 net.ifnames=0 lkl_pci=vfio");
    __AFL_INIT();
    get_afl_input(argv[1]);
    lkl_pci_init();
    for (i = 0; i < 2; i++) {
        idx = lkl_ifname_to_ifindex(ifnames[i]);
        printf("%s interface index: %d\n", ifnames[i], idx);
        if (idx > 0) {
            ret = lkl_if_up(idx);
            printf("%s: lkl_if_up: %d\n", ifnames[i], ret);
            break;
        }
    }
    // lkl_sys_halt();

	return 0;
}
