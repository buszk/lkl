#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zekun Shen");
MODULE_DESCRIPTION("A kasan test target");
MODULE_VERSION("0.01");

#define DRIVER_NAME "LKL_FUZZ_TARGET"

struct qemu_adapter {
	void *hw_addr;
	struct pci_dev *pdev;
};

static struct qemu_adapter *adapter = NULL;

void get_random_bytes(void *buf, int nbytes);
/* PCI */
static int drifuzz_test_probe(struct pci_dev *pdev,
			      const struct pci_device_id *ent)
{
	int err;
	void *a;
	if ((err = pci_enable_device(pdev)))
		return err;
	adapter = kmalloc(sizeof(struct qemu_adapter), GFP_KERNEL);
	adapter->hw_addr = pci_ioremap_bar(pdev, 0);
    uint32_t val = ioread32(adapter->hw_addr);
    switch (pdev->revision) {
    case 1:
        if (val == 0xdeadbeef) {
            // Bug
            a = kmalloc(1000, GFP_KERNEL);
            *((uint8_t*)a + 1101) = '\x00';
        }
        break;
    case 2:
        if ((val & 0xff) == 0xef) {
            if ((val & 0xff00) == 0xbe00) {
                if ((val & 0xff0000) == 0xad0000) {
                    if ((val & 0xff000000) == 0xde000000) {
                        // Bug
                        a = kmalloc(1000, GFP_KERNEL);
                        *((uint8_t*)a + 1101) = '\x00';
                    }
                    else {
                        return 1;
                    }
                }
                else {
                    return 2;
                }
            }
            else {
                return 3;
            }
        }
        else {
            return 4;
        }
    }
	pci_disable_device(pdev);
}

static const struct pci_device_id qemu_pci_tbl[] = { { PCI_DEVICE(0x8888, 3) },
						     {} };
static struct pci_driver qemu_driver = {
	.name = DRIVER_NAME,
	.id_table = qemu_pci_tbl,
	.probe = drifuzz_test_probe,
};

/* PCI ends */

static int __init qemu_init_module(void)
{
	int ret;
	if ((ret = pci_register_driver(&qemu_driver)) != 0) {
		printk(KERN_ALERT "Could not register pci: %d\b", ret);
		return ret;
	}
	return 0;
}
module_init(qemu_init_module);