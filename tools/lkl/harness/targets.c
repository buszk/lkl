#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "targets.h"


int bus_type = BUS_DEFAULT;
int fuzz_ids = 0;
int set_target(const char* target) {
    // atlantic
    if (!strcmp(target, "atlantic")) {
        bus_type = BUS_PCI;
        pci_vender = 0x1d6a;
        pci_device = 0x1;
        pci_revision = 0x1;
        return 0;
    }

    // snic
    if (!strcmp(target, "snic")) {
        bus_type = BUS_PCI;
        pci_vender = 0x1137;
        pci_device = 0x0046;
        pci_revision = 0x1;
        return 0;
    }

    // 8139cp
    if (!strcmp(target, "8139cp")) {
        bus_type = BUS_PCI;
        pci_vender = 0x10ec;
        pci_device = 0x8139;
        pci_revision = 0x20;
        return 0;
    }

    // ath9k
    if (!strcmp(target, "ath9k")) {
        bus_type = BUS_PCI;
        pci_vender = 0x168c;
        pci_device = 0x0023;
        pci_revision = 0x0;
        return 0;
    }

    // ath10k_pci
    if (!strcmp(target, "ath10k_pci")) {
        bus_type = BUS_PCI;
        pci_vender = 0x168c;
        pci_device = 0x003e;
        pci_revision = 0x0;
        return 0;
    }

    // rtwpci
    if (!strcmp(target, "rtwpci")) {
        bus_type = BUS_PCI;
        pci_vender = 0x10ec;
        pci_device = 0xB822;
        pci_revision = 0x0;
        return 0;
    }

    // stmmac_pci
    if (!strcmp(target, "stmmac_pci")) {
        bus_type = BUS_PCI;
        pci_vender = 0x8086;
        pci_device = 0x0937;
        pci_revision = 0x1;
        return 0;
    }

    // usb
    if (!strcmp(target, "usb")) {
        bus_type = BUS_USB;
        return 0;
    }

    if (!strcmp(target, "rsi_usb")) {
        bus_type = BUS_USB;
        usb_vendor = 0x1618;
        usb_product = 0x9113;
        return 0;
    }

    if (!strcmp(target, "ar5523")) {
        bus_type = BUS_USB;
        usb_vendor = 0x168c;
        usb_product = 0x0001;
        return 0;
    }

    if (!strcmp(target, "mwifiex_usb")) {
        bus_type = BUS_USB;
        usb_vendor = 0x1286;
        usb_product = 0x2042;
        return 0;
    }

    // For fuzzing campaign
    if (!strcmp(target, "pci:random") ||
        !strcmp(target, "pci:fuzz")) {
        bus_type = BUS_PCI;
        fuzz_ids = 1;
        return 0;
    }

    if (!strcmp(target, "usb:random") ||
        !strcmp(target, "usb:fuzz")) {
        bus_type = BUS_USB;
        fuzz_ids = 1;
        return 0;
    }

    // Specify ids in argument
    if (!strncmp(target, "pci:", 4)) {
        bus_type = BUS_PCI;
        char *token = strtok(target, ":");
        assert(token);
        token = strtok(NULL, ":");
        assert(token);
        pci_vender = strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        assert(token);
        pci_device = strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        assert(token);
        pci_revision = strtol(token, NULL, 16);
        return 0;
    }


    if (!strncmp(target, "usb:", 4)) {
        bus_type = BUS_USB;
        char *token = strtok(target, ":");
        assert(token);
        token = strtok(NULL, ":");
        assert(token);
        usb_vendor = strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        assert(token);
        usb_product = strtol(token, NULL, 16);
        return 0;
    }


    if (getenv("PCI_VENDOR") &&
        atoi(getenv("PCI_VENDOR"))) {
        pci_vender = atoi(getenv("PCI_VENDOR"));
        pci_device = getenv("PCI_DEVICE") ? 
                atoi(getenv("PCI_DEVICE")) : 0;
        pci_device = getenv("PCI_REVISION") ? 
                atoi(getenv("PCI_REVISION")) : 0;
        return 0;
    }

    return -1;
}