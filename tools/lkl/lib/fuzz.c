#include <lkl.h>
#include <stdio.h>
#include <setjmp.h>

extern jmp_buf jmp_env;
extern int jmp_env_set;

void fuzz_driver(void) {
    int i, ret, idx;
    char* ifnames[2] = { "eth0", "wlan0" };
    
    if (!setjmp(jmp_env)) {
        jmp_env_set = 1;
        // run driver probe
        ret = lkl_pci_driver_run();
        if (ret)
            goto end;

        jmp_env_set = 0;

        // run ifup
        fprintf(stderr, "Run interface up\n");
        for (i = 0; i < 2; i++) {
            idx = lkl_ifname_to_ifindex(ifnames[i]);
            fprintf(stderr, "%s interface index: %d\n", ifnames[i], idx);
            if (idx > 0) {
                ret = lkl_if_up(idx);
                fprintf(stderr, "%s: lkl_if_up: %d\n", ifnames[i], ret);
                ret = lkl_if_down(idx);
                fprintf(stderr, "%s: lkl_if_down: %d\n", ifnames[i], ret);
                break;
            }
        }

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