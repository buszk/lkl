
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/pci.h>

int (*probe_func)(struct device*) = 0;
int (*remove_func)(struct device*) = 0;
struct device *fuzzed_dev = 0;

extern struct usb_device *udev;

struct device_attach_data {
	struct device *dev;
	bool check_async;
	bool want_async;
	bool have_async;
};
extern int __device_attach_driver(struct device_driver *drv, void *_data);
extern void driver_sysfs_remove(struct device *dev);
extern int port_status_queried;
extern void usb_disable_device(struct usb_device *dev, int skip_ep0);

int fixed_ids = 1;
void lkl_set_fuzz_ids(void) {
	fixed_ids = 0;
}
int lkl_pci_driver_run(void)
{
    int ret = 0, temp = 0;
	struct device_attach_data data;
	// ret = lkl_cpu_get();
	// if (ret < 0)
	// 	return ret;
	if (!fixed_ids) {
		// USB stuff
		if (fuzzed_dev->bus == &usb_bus_type && udev) {
			lkl_ops->usb_ops->get_device_desc(&udev->descriptor, sizeof(udev->descriptor), 1);
			udev->state = USB_STATE_ADDRESS;
			port_status_queried = 0;
			usb_reset_device(udev);
		}

		// PCI stuff
		if (fuzzed_dev->bus == &pci_bus_type) {
			struct pci_dev *pci_dev = to_pci_dev(fuzzed_dev);
			lkl_ops->pci_ops->read(NULL, 0, 4, &temp);
			pci_dev->vendor = temp & 0xffff;
			pci_dev->device = temp >> 16;
			lkl_ops->pci_ops->read(NULL, 8, 4, &temp);
			pci_dev->revision = temp & 0xff;
			data.dev = fuzzed_dev;
			data.check_async = false;
			data.want_async = false;
			driver_sysfs_remove(fuzzed_dev);
			bus_for_each_drv(fuzzed_dev->bus, NULL, &data, __device_attach_driver);
		}
	}
	ret = probe_func(fuzzed_dev);
	// lkl_cpu_put();
	flush_scheduled_work();
	return ret;
}


void lkl_pci_driver_remove(void)
{
	// int ret;
	// ret = lkl_cpu_get();
	// if (ret < 0)
	// 	return;
	
	if (!fixed_ids) {
		// USB stuff
		if (fuzzed_dev->bus == &usb_bus_type && udev) {
            usb_disable_device(udev, 1);
		}
	}
	printk(KERN_INFO "remove_func\n");
	remove_func(fuzzed_dev);
	printk(KERN_INFO "remove_func ends\n");
	// lkl_cpu_put();
}
