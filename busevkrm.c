// busevkrm.c - Vulkan GPU-agnostic VRAMdisk
// used because I have an AMD-based 8GB laptop with 2GB VRAM carveout, and I want that memory back, dammit
// possible optimization would be to try and find some way to forcefully shutdown Vulkan without losing access to the memory...
// but that's probably undefined behaviour.
//  - 20kdc

#include <arpa/inet.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "buse.h"

static char * memoryPtr;
static size_t memorySize;

static int rwc(void * buf, uint32_t len, uint64_t offset, int isRead) {
	if ((offset >= memorySize) || ((offset + len) > memorySize)) {
		printf("Erroneous read access outside of size\n");
		return htonl(EFAULT);
	}
	if (isRead) {
		memcpy(buf, memoryPtr + (size_t) offset, len);
	} else {
		memcpy(memoryPtr + (size_t) offset, buf, len);
	}
	return 0;
}

static int memoryRead(void * buf, uint32_t len, uint64_t offset, void * _blah) {
	(void) _blah;
	return rwc(buf, len, offset, 1);
}
static int memoryWrite(const void * buf, uint32_t len, uint64_t offset, void * _blah) {
	(void) _blah;
	return rwc((void *) buf, len, offset, 0);
}

int main(int argc, char ** argv) {
	// step 1. args
	if (argc != 5) {
		printf("busevkrm gpuId memoryTypeIdx megabytes nbddev\n");
		printf("Choose parameters by looking at vulkaninfo output\n");
		printf("Memory MUST BE MAPPABLE (must have MEMORY_PROPERTY_HOST_VISIBLE_BIT)\n");
		return 1;
	}
	uint32_t gpuId = (uint32_t) atoi(argv[1]);
	int memoryTypeIdx = atoi(argv[2]);
	memorySize = atoll(argv[3]) * 1048576;
	// step 2. init Vulkan
	printf("initializing Vulkan...\n");
	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
	};
	VkInstance vmInstance = VK_NULL_HANDLE;
	if (vkCreateInstance(&createInfo, NULL, &vmInstance) != VK_SUCCESS) {
		printf("unable to create Vulkan instance\n");
		return 1;
	}
	// step 3. init more of Vulkan
	printf("initializing even more Vulkan...\n");
	uint32_t dump = gpuId + 16;
	VkPhysicalDevice vmPhysicalDevices[dump];
	vkEnumeratePhysicalDevices(vmInstance, &dump, vmPhysicalDevices);
	if (dump <= gpuId) {
		printf("GPU id was invalid: %i when amount of GPUs was %u\n", gpuId, dump);
		return 1;
	}
	// We need a queue for no good reason.
	printf("initializing, yup, more Vulkan...\n");
	float ignoreMePlease = 1.0f;
	VkDeviceQueueCreateInfo mainQueueCI = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = 0, // Choose queue family index 0, surely it's valid and we don't care what type it is
		.queueCount = 1,
		.pQueuePriorities = &ignoreMePlease
	};
	VkDeviceCreateInfo dCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &mainQueueCI
	};
	VkDevice vmDevice = VK_NULL_HANDLE;
	if (vkCreateDevice(vmPhysicalDevices[gpuId], &dCreateInfo, NULL, &vmDevice) != VK_SUCCESS) {
		printf("Amazingly, the device couldn't be created.\n");
		return 1;
	}
	// step 4. allocate memory
	printf("allocating memory...\n");
	VkMemoryAllocateInfo pAllocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memorySize,
		.memoryTypeIndex = memoryTypeIdx
	};
	VkDeviceMemory vmMemory = VK_NULL_HANDLE;
	if (vkAllocateMemory(vmDevice, &pAllocate, NULL, &vmMemory) != VK_SUCCESS) {
		printf("The memory couldn't be allocated.\n");
		return 1;
	}
	// step 5. map memory
	printf("mapping memory...\n");
	if (vkMapMemory(vmDevice, vmMemory, 0, VK_WHOLE_SIZE, 0, (void **) &memoryPtr) != VK_SUCCESS) {
		printf("The memory couldn't be mapped.\n");
		return 1;
	}
	// step 6. lock
	printf("locking driver memory to hopefully prevent deadlocks...\n");
	// this must be done as late as safely possible, because otherwise the kernel will randomly kill the process if it doesn't have enough "memory lock allowance"
	// basically, standard users have a small memory lock allowance presumably intended for security purposes
	// but if mlockall'ing with MCL_FUTURE, we're not allowed to exceed that allowance, and that will cause SIGSEGV
	if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
		printf("unable to mlockall, you may need sudo\n");
		return 1;
	}
	// step 7. BUSE init
	struct buse_operations buseOp = {
		.read = memoryRead,
		.write = memoryWrite,
		.size = memorySize
	};
	printf("calling buse_main : this is the last thing you will see, the driver is running\n");
	buse_main(argv[4], &buseOp, NULL);
	return 0;
}

