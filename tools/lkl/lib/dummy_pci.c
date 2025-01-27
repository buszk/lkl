// SPDX-License-Identifier: GPL-2.0
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <lkl_host.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/vfio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/vfio.h>
#include <sys/eventfd.h>
#include <execinfo.h>
#include "iomem.h"
#include "memwatcher.h"
#include "fuzz_input.h"

static int dummy_pci_irq = -1;

short pci_vender = 0;
short pci_device = 0;
short pci_revision = 0;

#define DEFAULT_VENDOR_ID	0x1d6a
#define DEFAULT_DEVICE_ID	0x1
#define DEFAULT_REVISION_ID	0x1
// #define DEFAULT_VENDOR_ID	0x8888
// #define DEFAULT_DEVICE_ID	0x2
// #define DEFAULT_REVISION_ID	0x2

#define VENDOR_ID (pci_vender) ? pci_vender : DEFAULT_VENDOR_ID
#define DEVICE_ID (pci_vender) ? pci_device : DEFAULT_DEVICE_ID
#define REVISION_ID (pci_vender) ? pci_revision : DEFAULT_REVISION_ID

/* Obtain a backtrace and print it to stdout. */
void print_trace (void)
{
  void *array[10];
  char **strings;
  int size, i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  if (strings != NULL)
  {

    printf ("Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++)
      printf ("%s\n", strings[i]);
  }

  free (strings);
}

// TODO: A better interrupt scheduling method
#define INT_FREQ 75
void may_interrupt(void) {
	int ret;
	static size_t io_count = 0;
	if ((++io_count % INT_FREQ) == 0) {
		assert(dummy_pci_irq != -1 && "Interrupt line for dummy_pci device not set");
		fprintf(stderr, "Issue interrupt %d. io count: %ld\n", dummy_pci_irq, io_count);
		ret = lkl_trigger_irq(dummy_pci_irq);
		fprintf(stderr, "lkl_trigger_irq returns %d\n", ret);
	}
}

struct lkl_pci_dev {
	int irq;
	int quit;
	int fd;
	struct vfio_iommu_type1_dma_map dma_map;
	// lkl_thread_t int_thread;
	// struct lkl_sem *thread_init_sem;
	// int irq_fd;
};

/**
 * dummy_pci_add - Create a new pci device
 *
 * The device should be assigned to VFIO by the host in advance.
 *
 * @name - PCI device name (as %x:%x:%x.%x format)
 * @kernel_ram - the start address of kernel memory needed to be mapped for DMA.
 * The address must be aligned to the page size.
 * @ram_size - the size of kernel memory, should be page-aligned as well.
 */

static struct lkl_pci_dev *dummy_pci_add(const char *name, void *kernel_ram,
					unsigned long ram_size)
{
	struct lkl_pci_dev *dev;

	dev = malloc(sizeof(*dev));
	if (!dev)
		return NULL;

	memset(dev, 0, sizeof(*dev));

	/* if kernel_ram is null, assume the memory is already initialized
	 * by another device, and skip this step.
	 */
	if (kernel_ram) {
		dev->dma_map.vaddr = (uint64_t)kernel_ram;
		dev->dma_map.size = ram_size;
		dev->dma_map.iova = 0;
		dev->dma_map.flags =
			VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;
	}

	time_t t;
	srand((unsigned) time(&t));

	return dev;
}

static void dummy_pci_remove(struct lkl_pci_dev *dev)
{
	dev->quit = 1;
	// lkl_host_ops.thread_join(dev->int_thread);
	close(dev->fd);
	free(dev);
}

/* TODO */
__attribute__((unused))
static void dummy_int_thread(void *_dev)
{
	// if should trigger
	// lkl_trigger_irq(dev->irq);
}

static int dummy_pci_irq_init(struct lkl_pci_dev *dev, int irq)
{
	dummy_pci_irq = irq;
	// dev->int_thread =
	// 	lkl_host_ops.thread_create(dummy_int_thread, (void *)dev);

	// TODO: wait until thread running
	return 0;
}

static const char* dma_type(int consistent) {
	if (consistent)
		return "consistent";
	else
		return "stream";
}

static void consistent_dma_read(void* addr, uint64_t offset, uint8_t size) {
	may_interrupt();
	switch (size) {
	case 8:
		*(uint64_t *)addr = (uint64_t)get_qword();
		fprintf(stderr, "%s 0x%08lx[%02x] = 0x%016lx\n",
					 __func__, offset, size, *(uint64_t *)addr);
		break;
	case 4:
		// print_trace();
		*(uint32_t *)addr = (uint32_t)get_dword();
		fprintf(stderr, "%s 0x%08lx[%02x] = 0x%08x\n",
					 __func__, offset, size, *(uint32_t *)addr);
		break;
	case 2:
		*(uint16_t *)addr = (uint16_t)get_word();
		fprintf(stderr, "%s 0x%08lx[%02x] = 0x%04x\n",
					 __func__, offset, size, *(uint16_t *)addr);
		break;
	case 1:
		*(uint8_t *)addr = (uint8_t)get_byte();
		fprintf(stderr, "%s 0x%08lx[%02x] = 0x%02x\n",
					 __func__, offset, size, *(uint8_t *)addr);
		break;
	}
}

static unsigned long long dummy_map_page(struct lkl_pci_dev *dev, void *vaddr,
					unsigned long size, int consistent)
{
	unsigned long long ret;
	ret = (unsigned long long)vaddr - dev->dma_map.vaddr;
	fprintf(stderr, "[%s][%s] vaddr: %p, size: %lx, ret: %llx\n",
			__func__, dma_type(consistent), vaddr, size, ret);
	memset(vaddr, 'A', size);
	if (consistent)
		_watch_address(vaddr, size, PROT_NONE, consistent_dma_read);
	return ret;
}

static void dummy_unmap_page(struct lkl_pci_dev *dev,
			    unsigned long long dma_handle,
				unsigned long size, int consistent)
{
	fprintf(stderr, "[%s][%s] dma_handle: %llx, size: %lx\n",
			__func__, dma_type(consistent), dma_handle, size);
	void *vaddr = (void*)(dma_handle + dev->dma_map.vaddr);
	if (consistent)
		_unwatch_address(vaddr, PROT_WRITE);
}


static int dummy_pci_read(struct lkl_pci_dev *dev, int where, int size,
			 void *val)
{
	// lkl_printf("dummy_pci_read: %02x[%02x]\n", where, size);
	if (where == 0 && size == 4) {
		*(uint16_t *)val = VENDOR_ID; 	// vendor_id
		*((uint16_t *)val+1) = DEVICE_ID; 	// device_id
		return size;
	}
	else if (where == 8 && size == 4) {
		*((uint32_t *)val) = REVISION_ID;		// revision_id
		return size;
	}
	else if (where == 0xc && size == 1) {
		*((uint8_t *)val) = 128;			//cache line size
	}
	else if (where == 0x10 && size == 4) { // BAR0
		static int bar_0_count = 0;
		if (bar_0_count == 0) {
			//TODO: proper addr handling
			*(uint32_t*) val = 0xfef0000;
			bar_0_count ++;
		}
		else {
			*(uint32_t*) val = 0xfff00000;
		}
	}
	else if (where == 0x14 && size == 4) { // BAR1
		static int bar_1_count = 0;
		if (bar_1_count == 0) {
			//TODO: proper addr handling
			*(uint32_t*) val = 0xfee00000;
			bar_1_count ++;
		}
		else {
			*(uint32_t*) val = 0xfff00000;
		}
	}
	else if (where == 0x18 && size == 4) { // BAR2
		static int bar_2_count = 0;
		if (bar_2_count == 0) {
			//TODO: proper addr handling
			*(uint32_t*) val = 0xfed00000;
			bar_2_count ++;
		}
		else {
			*(uint32_t*) val = 0xfff00000;
		}
	}
	return 0;
}

static int dummy_pci_write(struct lkl_pci_dev *dev, int where, int size,
			  void *val)
{
	return size;
}

static int pci_resource_read(void *data, int offset, void *res, int size)
{
	may_interrupt();
	switch (size) {
	case 8:
		*(uint64_t *)res = (uint64_t)get_qword();
		lkl_printf("%s 0x%08x[%02x] = 0x%016lx\n",
					 __func__, offset, size, *(uint64_t *)res);
		break;
	case 4:
		// print_trace();
		*(uint32_t *)res = (uint32_t)get_dword();
		lkl_printf("%s 0x%08x[%02x] = 0x%08x\n",
					 __func__, offset, size, *(uint32_t *)res);
		break;
	case 2:
		*(uint16_t *)res = (uint16_t)get_word();
		lkl_printf("%s 0x%08x[%02x] = 0x%04x\n",
					 __func__, offset, size, *(uint16_t *)res);
		break;
	case 1:
		*(uint8_t *)res = (uint8_t)get_byte();
		lkl_printf("%s 0x%08x[%02x] = 0x%02x\n",
					 __func__, offset, size, *(uint8_t *)res);
		break;
	default:
		return -LKL_EOPNOTSUPP;
	}
	return 0;
}

static int pci_resource_write(void *data, int offset, void *res, int size)
{
	may_interrupt();
	void *addr = data + offset;

	switch (size) {
	case 8:
		*(uint64_t *)addr = *(uint64_t *)res;
		break;
	case 4:
		*(uint32_t *)addr = *(uint32_t *)res;
		break;
	case 2:
		*(uint16_t *)addr = *(uint16_t *)res;
		break;
	case 1:
		*(uint8_t *)addr = *(uint8_t *)res;
		break;
	default:
		return -LKL_EOPNOTSUPP;
	}
	return 0;
}

static const struct lkl_iomem_ops pci_resource_ops = {
	.read = pci_resource_read,
	.write = pci_resource_write,
};

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static void *dummy_resource_alloc(struct lkl_pci_dev *dev,
				 unsigned long resource_size,
				 int resource_index)
{
	lkl_printf("%s: start\n", __func__);
	unsigned int region_index_list[] = {
		VFIO_PCI_BAR0_REGION_INDEX, VFIO_PCI_BAR1_REGION_INDEX,
		VFIO_PCI_BAR2_REGION_INDEX, VFIO_PCI_BAR3_REGION_INDEX,
		VFIO_PCI_BAR4_REGION_INDEX, VFIO_PCI_BAR5_REGION_INDEX,
	};
	// struct vfio_region_info reg = { .argsz = sizeof(reg) };
	void *mmio_addr;


	if ((unsigned int)resource_index >= ARRAY_SIZE(region_index_list))
		return NULL;

	// reg.index = region_index_list[resource_index];

	// if (dev->device_info.num_regions <= reg.index)
	// 	return NULL;

	/* We assume the resource is a memory space. */

	// if (reg.size < resource_size)
	// 	return NULL;

	// mmio_addr = mmap(NULL, resource_size, PROT_READ | PROT_WRITE,
	// 		 MAP_SHARED, dev->fd, reg.offset);
	mmio_addr = mmap(NULL, resource_size, PROT_READ | PROT_WRITE, 
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (mmio_addr == MAP_FAILED)
		return NULL;

	return register_iomem(mmio_addr, resource_size, &pci_resource_ops);
}

struct lkl_dev_pci_ops vfio_pci_ops = {
	.add = dummy_pci_add,
	.remove = dummy_pci_remove,
	.irq_init = dummy_pci_irq_init,
	.read = dummy_pci_read,
	.write = dummy_pci_write,
	.resource_alloc = dummy_resource_alloc,
	.map_page = dummy_map_page,
	.unmap_page = dummy_unmap_page,
};
