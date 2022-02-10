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

#include "test.h"
#include "cla.h"

static char bootparams[128];

// struct cl_arg args[] = {
// 	{ "type", 't', "filesystem type", 1, CL_ARG_STR, &cla.fstype },
// 	{ "pciname", 'n', "PCI device name (as %x:%x:%x.%x format)", 1,
// 	  CL_ARG_STR, &cla.pciname },
// 	{ 0 },
// };

LKL_TEST_CALL(start_kernel, lkl_start_kernel, 0, &lkl_host_ops, bootparams);
LKL_TEST_CALL(stop_kernel, lkl_sys_halt, 0);

struct lkl_test tests[] = {
	LKL_TEST(start_kernel),	   LKL_TEST(stop_kernel),
};

int main(int argc, const char **argv)
{
	// if (parse_args(argc, argv, args) < 0)
	// 	return -1;
        
	snprintf(bootparams, sizeof(bootparams),
		 "mem=16M loglevel=8 lkl_pci=vfio");

	lkl_host_ops.print = lkl_test_log;

	return lkl_test_run(tests, sizeof(tests) / sizeof(struct lkl_test),
			    "net-pci");
}
