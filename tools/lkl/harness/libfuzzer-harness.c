// SPDX-License-Identifier: GPL-2.0
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


int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	lkl_printf("pid: %d\n", (int)getpid());

	lkl_start_kernel(&lkl_host_ops, "mem=16M loglevel=8 lkl_pci=vfio");
	lkl_sys_halt();

	return 0;
}
