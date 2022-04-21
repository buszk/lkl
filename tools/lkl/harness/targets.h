#ifndef __TARGETS_H
#define __TARGETS_H
extern short pci_vender;
extern short pci_device;
extern short pci_revision;

extern int bus_type;
#define BUS_DEFAULT 0
#define BUS_PCI 1
#define BUS_USB 2

int set_target(const char*);
#endif
