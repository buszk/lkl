#include <lkl.h>
#include <stdio.h>
#include <setjmp.h>
#include "fuzz_input.h"

extern jmp_buf jmp_env;
extern int jmp_env_set;
int fuzz_mode = MODE_DEFAULT;

void fuzz_driver(void) {
    int i, ret, idx;
    char* ifnames[2] = { "eth0", "wlan0" };
    
    if (!setjmp(jmp_env)) {
        jmp_env_set = 1;
        // run driver probe
        ret = lkl_pci_driver_run();
        fprintf(stderr, "driver probe ret: %d\n", ret);
        if (ret)
            goto end;

        jmp_env_set = 0;

        // run ifup
        if (overflow)
            goto end_remove;

        fprintf(stderr, "Run interface up\n");
        for (i = 0; i < 2; i++) {
            idx = lkl_ifname_to_ifindex(ifnames[i]);
            fprintf(stderr, "%s interface index: %d\n", ifnames[i], idx);
            if (idx > 0) {
                ret = lkl_if_up(idx);
                fprintf(stderr, "%s: lkl_if_up: %d\n", ifnames[i], ret);
                if (fuzz_mode != MODE_FORKSERVER) {
                    ret = lkl_if_down(idx);
                    fprintf(stderr, "%s: lkl_if_down: %d\n", ifnames[i], ret);
                }
                break;
            }
        }
end_remove:
        if (fuzz_mode != MODE_FORKSERVER)
            lkl_pci_driver_remove();
end:
        fprintf(stderr, "normal ends\n");
    }
    else {
        fprintf(stderr, "jmp \n");
        // Getting here it means the driver functions exit early without cpu_put
        lkl_cpu_put();
        lkl_pci_driver_remove();
        fprintf(stderr, "jmp ends\n");
    }

}


void set_fuzz_mode(int mode) {
    fuzz_mode = mode;
}