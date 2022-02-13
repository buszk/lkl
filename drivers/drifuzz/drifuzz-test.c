#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/ioport.h>       
#include <linux/pci.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zekun Shen");
MODULE_DESCRIPTION("A test driver for LKL PCI emulation");
MODULE_VERSION("0.01");

#define DRIVER_NAME "LKL_DRIVER_TEST"


struct qemu_adapter {
	void* hw_addr;
	struct pci_dev *pdev;
};

static struct qemu_adapter *adapter = NULL;

/* Tests */
static uint32_t read_offset(uint32_t offset) {
	if (!adapter) return 0;
	return readl((char*)adapter->hw_addr + offset);
}

static void write_offset(uint32_t offset, uint32_t val) {
	if (!adapter) return;
	writel(val, (char*)adapter->hw_addr + offset);
}

/* PCI */
static int drifuzz_test_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {
	int err;
    dma_addr_t dma_handle;
    void *const_dma, *stream_dma;
	printk(KERN_INFO "%s\n", DRIVER_NAME);
	if ((err = pci_enable_device(pdev)))
		return err;

	adapter = kmalloc(sizeof(struct qemu_adapter), GFP_KERNEL);
	adapter->hw_addr = pci_ioremap_bar(pdev, 0);
	pci_set_drvdata(pdev, adapter);

    const_dma = dma_alloc_coherent(&pdev->dev, 0x1000, &dma_handle, GFP_KERNEL);
	printk(KERN_INFO "%x\n", *(uint8_t*)const_dma);
	printk(KERN_INFO "%x\n", *((uint16_t*)const_dma +0x55));
    printk(KERN_INFO "%x\n", *((uint32_t*)const_dma + 0x111));

	stream_dma = kmalloc(0x101, GFP_KERNEL);
	*((char*)stream_dma + 0x100) = '\x00';
	dma_handle = dma_map_single(&pdev->dev, stream_dma, 0x10, DMA_FROM_DEVICE);
    dma_unmap_single(&pdev->dev, dma_handle, 0x10, DMA_FROM_DEVICE);
    printk(KERN_INFO "%s stream dma data: %s\n", DRIVER_NAME, (char*)stream_dma);
    kfree(stream_dma);

	return 0;
}

static void drifuzz_test_remove(struct pci_dev *pdev) {
	//pci_release_selected_regions(pdev, bars);
	kfree(pci_get_drvdata(pdev));
	pci_disable_device(pdev);
}

static const struct pci_device_id qemu_pci_tbl[] = {
	{PCI_DEVICE(0x8888, 0)},
	{}
};
static struct pci_driver qemu_driver = {
	.name 		= DRIVER_NAME,
	.id_table 	= qemu_pci_tbl,
	.probe 		= drifuzz_test_probe,
	.remove 	= drifuzz_test_remove,
};

/* PCI ends */


int __init qemu_init_module(void) {
	int ret;
	printk(KERN_INFO "Test init\n");
	if ((ret = pci_register_driver(&qemu_driver)) != 0) {
		printk(KERN_ALERT "Could not register pci: %d\b", ret);
		return ret;
	}
	return 0;
}
module_init(qemu_init_module);