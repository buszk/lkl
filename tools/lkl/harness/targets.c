#include <stdlib.h>
#include <string.h>


extern short pci_vender;
extern short pci_device;
extern short pci_revision;


int set_target(const char*);
int set_target(const char* target) {
    // atlantic
    if (!strcmp(target, "atlantic")) {
        pci_vender = 0x1d6a;
        pci_device = 0x1;
        pci_revision = 0x1;
        return 0;
    }

    // snic
    if (!strcmp(target, "snic")) {
        pci_vender = 0x1137;
        pci_device = 0x0046;
        pci_revision = 0x1;
        return 0;
    }

    // 8139cp
    if (!strcmp(target, "8139cp")) {
        pci_vender = 0x10ec;
        pci_device = 0x8139;
        pci_revision = 0x20;
        return 0;
    }

    // ath9k
    if (!strcmp(target, "ath9k")) {
        pci_vender = 0x168c;
        pci_device = 0x0023;
        pci_revision = 0x0;
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