// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/mm.h>
#include <linux/fuzz.h>
#include <asm/cpu.h>
#include <asm/host_ops.h>


static int giveback_counter = 0;

struct api_context {
	struct completion	done;
	int			status;
};

static const __u8 root_hub_hub_des[] =
{
	0x09,           /*  __u8  bLength; */
	USB_DT_HUB,     /*  __u8  bDescriptorType; Hub-descriptor */    
	0x08,           /*  __u8  bNbrPorts; */
	HUB_CHAR_NO_LPSM |  /* __u16  wHubCharacteristics; */
        HUB_CHAR_INDV_PORT_OCPM, /* (per-port OC, no power switching) */
	0x00,
	0x01,           /*  __u8  bPwrOn2pwrGood; 2ms */
	0x00,           /*  __u8  bHubContrCurrent; 0 mA */
	0x00,           /*  __u8  DeviceRemovable; *** 7 Ports max */   
	0xff            /*  __u8  PortPwrCtrlMask; *** 7 ports max */
};



static int lkl_hcd_start(struct usb_hcd *hcd) {
	return 0;
}

static void lkl_hcd_stop(struct usb_hcd *hcd) {
	;
}

static int lkl_hcd_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags) {
	struct api_context *ctx;
	if (usb_pipecontrol(urb->pipe) && usb_pipeout(urb->pipe)) {
		ctx = urb->context;
		ctx->status = 0;
		complete(&ctx->done);
		return 0;
	}
	printk(KERN_INFO "urb: %lx, pipe: %x\n", (uint64_t)urb, urb->pipe);
	printk(KERN_INFO "transfer_buffer_length: %x\n", urb->transfer_buffer_length);
	// TODO: check urb->setup_packet, (struct usb_ctrlrequest)
	// Check if bRequestType == USB_REQ_GET_DESCRIPTOR
	if (usb_pipecontrol(urb->pipe) && usb_pipein(urb->pipe)) {
		struct usb_ctrlrequest *req;
		req = (struct usb_ctrlrequest*)urb->setup_packet;
		ctx = urb->context;
		printk(KERN_INFO "bRequest: %x\n", req->bRequest);
		switch (req->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			printk(KERN_INFO "wValue: %x\n", req->wValue);
			switch (req->wValue >> 8)
			{
			case USB_DT_DEVICE:
				printk(KERN_INFO "Unimplemented USB_DT_DEVICE\n");
				break;
			case USB_DT_CONFIG:
				printk(KERN_INFO "USB_DT_CONFIG\n");
				urb->actual_length = 
					lkl_ops->usb_ops->get_config_desc(urb->transfer_buffer, req->wLength);
				break;
			case USB_DT_STRING:
				printk(KERN_INFO "USB_DT_STRING: len: %x\n", req->wLength);
				// if ((req->wValue & 0xff) == 0 && req->wLength == 2) {
				if (req->wLength == 2) {
					// set default lang to english
					*(uint16_t*)urb->transfer_buffer = 0x0409;
					urb->actual_length = 2;
					break;
				}
				break;
			default:
				printk(KERN_INFO "Control PIPE: Default\n");
				memset(urb->transfer_buffer, 'A', urb->transfer_buffer_length);
				urb->actual_length = urb->transfer_buffer_length;
				break;
			}
			break;
		default:
			printk(KERN_INFO "Unknown control request\n");
			memset(urb->transfer_buffer, 'A', urb->transfer_buffer_length);
			urb->actual_length = urb->transfer_buffer_length;
			break;
		}
		ctx->status = 0;
		complete(&ctx->done);
	}
	else if (usb_pipebulk(urb->pipe) && usb_pipein(urb->pipe)) {
		// lkl_bug("bulk pipe\n");
		urb->actual_length = 
			lkl_ops->usb_ops->get_data(urb->transfer_buffer, urb->transfer_buffer_length);
		// Giveback indefinitely can cause stack overflow
		// Set maximum to 16
		if (giveback_counter < 16 && (lkl_ops->usb_ops->get_control_byte() & 0x1)) {
			giveback_counter ++;
			usb_hcd_giveback_urb(hcd, urb, 0);
		}
		else {
			giveback_counter = 0;
		}
	}
	else {
		lkl_bug("Unimplementated pipe\n");
	}
	return 0;
}

static int lkl_hcd_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status) {
	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

static int lkl_hcd_hub_status_data(struct usb_hcd *hcd, char *buf) {
	// printk(KERN_INFO "%s\n", __func__);
	static int n = 1;
	if (n) {
		n = 0;
		return 1;
	}
	return 0;
}

static int lkl_hcd_hub_control(struct usb_hcd *hcd, u16 typeReq, u16 wValue,
								u16 wIndex, char *buf, u16 wLength) {
	int retval = 0;
	uint8_t type = wValue >> 8;
	switch (typeReq) {
	case GetHubStatus:
		lkl_printf("GetHubStatus\n");
		*(uint32_t *)buf = 0;
		retval = 4;
		break;
	case GetHubDescriptor:
		lkl_printf("GetHubDescriptor\n");
		retval = min_t(unsigned int, sizeof(root_hub_hub_des), wLength);
		memcpy(buf, root_hub_hub_des, retval);
		break;
	case GetPortStatus:
		lkl_printf("GetPortStatus\n");
		static int cnt = 0;
		if (cnt) {
			*(uint32_t *)buf = 2;
			cnt = 0;
		}
		else {
			*(uint32_t *)buf = 0;
		}
		retval = 4;
		break;
	case SetPortFeature:
		lkl_printf("SetPortFeature\n");
		break;
	case ClearPortFeature:
		lkl_printf("ClearPortFeature\n");
		break;
	default:
		lkl_printf("lkl_hcd_hub_control: default: %x\n", typeReq);
		return -EPIPE;
	}
	return retval;
}

static const struct hc_driver lkl_hc_driver = {
	.description =		"LKL USB HUB",
	.product_desc =		"LKL Host Controller",
	.hcd_priv_size =	0,

	/* Generic hardware linkage */
	// .irq =			uhci_irq,
	.flags =		HCD_USB11,

	/* Basic lifecycle operations */
	// .reset =		uhci_pci_init,
	.start =		lkl_hcd_start,
	.stop =			lkl_hcd_stop,

	.urb_enqueue =		lkl_hcd_urb_enqueue,
	.urb_dequeue =		lkl_hcd_urb_dequeue,

	// .endpoint_disable =	uhci_hcd_endpoint_disable,
	// .get_frame_number =	uhci_hcd_get_frame_number,

	.hub_status_data =	lkl_hcd_hub_status_data,
	.hub_control =		lkl_hcd_hub_control,
};


static int lkl_usb_probe(struct platform_device *pdev)
{
	int ret;
	struct usb_hcd		*hcd;
	struct usb_device *udev;
	hcd = usb_create_hcd(&lkl_hc_driver, &pdev->dev, dev_name(&pdev->dev));
	ret = usb_add_hcd(hcd, 11, 0);
	if (ret) {
		printk(KERN_INFO "usb_add_hcd: return %d\n", ret);
		goto end;
	}
	udev = usb_alloc_dev(hcd->self.root_hub, &hcd->self, 7);
	// usb_set_configuration(udev, 0);
	// udev->descriptor.bNumConfigurations = 1;
	// memcpy(&udev->descriptor, device_desc, sizeof(udev->descriptor));
	lkl_ops->usb_ops->get_device_desc(&udev->descriptor, sizeof(udev->descriptor));
	udev->ep0.desc.wMaxPacketSize = 0x40;
	udev->state = USB_STATE_ADDRESS;
	usb_new_device(udev);
	udev->state = USB_STATE_CONFIGURED;
	// lkl_bug("usb probe end\n");
end:
	return ret;
}

static void lkl_usb_shutdown(struct platform_device *pdev)
{
	
}

static struct platform_driver lkl_usb_driver = {
	.driver = {
		.name = "lkl_usb",
	},
	.probe = lkl_usb_probe,
	.shutdown = lkl_usb_shutdown,
};

// int (*probe_func)(struct device*) = 0;
// int (*remove_func)(struct device*) = 0;
// struct device *fuzzed_dev = 0;

// static struct platform_device *dev;

// int lkl_usb_driver_run(void)
// {
//     int ret = 0;
// 	ret = probe_func(fuzzed_dev);
// 	return ret;
// }


// void lkl_usb_driver_remove(void)
// {
// 	int ret;
// 	remove_func(fuzzed_dev);
// }

static int lkl_usb_enabled = 0;

static int __init setup_usb_device(char *str)
{
	printk(KERN_INFO "setup_usb_device\n");
	lkl_usb_enabled = 1;
	return 0;
}

early_param("lkl_usb", setup_usb_device);


int __init lkl_usb_init(void)
{
	int ret;
	struct platform_device *dev;

	if (!lkl_usb_enabled)
		return 0;

	/*register a platform driver*/
	ret = platform_driver_register(&lkl_usb_driver);
	if (ret != 0)
		return ret;

	dev = platform_device_alloc("lkl_usb", -1);
	if (!dev)
		return -ENOMEM;

	ret = platform_device_add(dev);
	if (ret != 0)
		goto error;

	return 0;
error:
	platform_device_put(dev);
	return ret;
}

subsys_initcall(lkl_usb_init);