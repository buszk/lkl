#include "generic_fuzz.h"

int main(int argc, char**argv) {
    pci_vender = 0x8888;
    pci_device = 0x3;
    pci_revision = 0x1;
    bus_type = BUS_PCI;
    return func(argc, argv);
}