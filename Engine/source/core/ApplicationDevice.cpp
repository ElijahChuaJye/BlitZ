/**
* @file Application.cpp
* @brief Main engine orchestration and Vulkan pipeline implementation.
** @author Elijah Chua Jye Kang (elijahchua64@gmail.com)
* @date Started : September 2025
* *@description
* This file serves as the heart of the Blitz engine, responsible for the
* orchestration of the Vulkan graphics API. It manages the lifecycle of the
* application, including window management, hardware interrogation,
* logical device initialization, and the setup of the rendering pipeline.
* */

// 1. FIRST: Include your application header. 
// WHY: This pulls in 'volk.h' first, ensuring all Vulkan structures and 
// functions are fully loaded in memory before VMA tries to use them.
#include "core/Application.h"

// 2. SECOND: Set up the VMA implementation macro.
#define VMA_IMPLEMENTATION

// 3. THIRD: Tell MSVC to temporarily turn off all warnings.
// WHY: VMA is a highly generic, cross-platform library. Wrapping the include 
// in a warning push/pop block tells the Microsoft compiler to drop its warning 
// level to '0' (complete silence) ONLY for this header, preventing our strict 
// '/WX' (warnings-as-errors) flag from triggering on AMD's code.
#if defined(_MSC_VER)
#pragma warning(push, 0) 
#endif

// 4. FOURTH: Include the allocator code now that warnings are muted.
#include <vk_mem_alloc.h>

// 5. FIFTH: Instantly restore our strict '/W4 /WX' warning levels.
// WHY: This ensures that any code we write below this line is still strictly 
// scrutinized by the compiler, maintaining our high code-quality standards.
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/**
* @brief Picking the physical GPU that will be used for Vulkan
*
* @return Nothing but it picks the phsical GPU that vulkan will use for the grahics
*/
void Application::pickPhysicalGPU() {
	uint32_t deviceCount{}; //Stores the total count of phtsical device discovered

	// 1. Ask Vulkan how many gpus are there in this local machine first
	vkEnumeratePhysicalDevices(m_vulkinstance, &deviceCount, nullptr);

	//If it has zero GPU, vulkan won't be able to run
	if (deviceCount == 0) {
		throw std::runtime_error("Zero GPU detected in local device!");
	}

	// 2. Allocate an array(vector) big enough to hold all the GPU discovered
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_vulkinstance, &deviceCount, devices.data());

	// 3. Create a multimap to automatically sort our GPU based on perfromance score
	// std::multimap sorts key n ascending order automatically (based on performance)
	std::multimap<int, VkPhysicalDevice> candidates;

	// 4. Interrogate every single GPU found on the machine
	for (auto const& device : devices) {
		int score = rateGPUSuitability(device);

		candidates.insert(std::make_pair(score, device));
	}

	//If the any of the candidate score is more than 0
	if (candidates.rbegin()->first > 0) {
		m_physicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("Failed to find a matching GPU for the engine.");
	}

}

/**
 * @brief Initializes the driver-linked logical device context interface.
 * Queries unique execution queues needed for presentation and graphics pipelines, enables
 * low-level hardware structures, binds core extension array dependencies (like the swapchain),
 * triggers device initialization, and fetches queue processing handles via Volk.
 */
void Application::createLogicalDevice() {

	// 1. Find the queue families indices first supported by our chosen GPU
	QueueFamilies indices = findHardwareFamily(m_physicalDevice);

	// 2. Set the unique queue families to prevent duplication from happening
	std::vector<VkDeviceQueueCreateInfo> createQueueInfo;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	// 3. Assigning performance priority (0.0 - 1.0) to our queues
	float queuePiority{ 1.0f };
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePiority;

		createQueueInfo.push_back(queueCreateInfo);

	}

	// 4. Speicfy the low-level for hardware features we want to activate
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.geometryShader = VK_TRUE;

	//This is for dynamic rendering
	VkPhysicalDeviceVulkan13Features features13{};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE; // Explicitly request dynamic rendering
	features13.pNext = nullptr;

	// 5. Fill the primary logical device configuration device
	VkDeviceCreateInfo createInfo{};

	createInfo.pNext = &features13;
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	//Fix for the exception thrown
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(createQueueInfo.size());
	createInfo.pQueueCreateInfos = createQueueInfo.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	// 6. Pass queue number setup and to enable extensions
	std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	//7. If the validations is enabled, validate with older drivers
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//8. Actual creation of the logical device 
	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	// Load device-specific function pointers through Volk
	volkLoadDevice(m_logicalDevice);

	//9. Retrival: Snatch the handles back	to command queues
	vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue); //  Corrected!

	// WHY: We initialize VMA immediately after the logical device is created. 
	// VMA acts as our smart sub-allocator, so we must feed it the pointers to 
	// our physical device, logical device, and our Vulkan instance.
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = m_physicalDevice;
	allocatorInfo.device = m_logicalDevice;
	allocatorInfo.instance = m_vulkinstance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3; // Matches your API version in createInstance

	// WHY: Because we are using Volk (dynamic function loading), we must explicitly
	// tell VMA how to call Vulkan commands. We pass pointers directly from Volk's 
	// global table so VMA doesn't try to link statically to raw Vulkan libraries.
	VmaVulkanFunctions vulkanFunctions{};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	allocatorInfo.pVulkanFunctions = &vulkanFunctions;

	// Create the allocator context on the GPU
	if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Memory Allocator!");
	}

	std::cout << "VMA: Allocator successfully initialized with Vulkan 1.3 pointer tables." << std::endl;

}


/********************************************
*
* HELPER FUNCTIONS
*
********************************************/

/**
* Helper function to find the hardware within the local machine to find the correct hardware
*
**/
QueueFamilies Application::findHardwareFamily(VkPhysicalDevice& device) {
	QueueFamilies indicies;

	//1. Get the number of queue family supported by this GPU
	uint32_t queueFamilyCount{};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	//2. Allocate space and retrieve properties of all queue families
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int idx{};

	for (const auto& queueMembers : queueFamilies) {

		//Check if this specific family gear supports GRAPHICS command
		if (queueMembers.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indicies.graphicsFamily = idx;
		}

		//Check if this specific queue family supports PRESENTATION(Showing image to our window)
		//Requires m_surface to be intiailized first
		VkBool32 presentSupport{ false };
		vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, m_surface, &presentSupport);

		if (presentSupport) {
			indicies.presentFamily = idx;
		}

		if (indicies.isComplete()) {
			break;
		}

		++idx;
	}

	return indicies;
}

/**
* Helper function for the picking of GPU device based on score
*
*
**/
int Application::rateGPUSuitability(VkPhysicalDevice device) {
	// 1. Asking for high-level GPU's name, type, properties and limit
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// 2. Asking GPU for low-level GPU features (Geomtert Shaders
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// 3. Find the available hardware departments (Queues)
	QueueFamilies indices = findHardwareFamily(device);

	//CRITICAL RULE:
	//If it lacks graphics department, a display presentation department,
	// or geometry shaders, it is immidately scored as 0 as they those are very important.
	if (!indices.graphicsFamily || !indices.presentFamily || !deviceFeatures.geometryShader) {
		return 0;

	}

	int score{};

	//PERFORMACE PREFERENCE: Discrete GPUs gets a massive point boost.
	//Dedicated GPU card will score higher than integrated Intel GPU card
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 2212;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	return score;

}

/// <summary>
/// This function is used as generic helper to create any kind of GPU buffer (vertex, index etc) using VMA.
/// Wrapping Vulkan's verbose allocation into one function to be clean and simple
/// </summary>
/// <param name="size"> Size of the buffer in bytes </param>
/// <param name="usage"> Vulkan structural flags </param>
/// <param name="memUsage"> VMA-specifc memory usage hint indicating where to store</param>
/// <param name="buffer"> Reference to the VkBUffer handle to be populated </param>
/// <param name="allocation"> Reference to VmaAllocation handle to be populated </param>
void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, VkBuffer& buffer, VmaAllocation& allocation) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only accesse by our graphics card

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memUsage;

	//vmCreateBuffer handles querying the GPU's memory types, finding a sutiable pool,
	// allocating the memory, the binding it to the buffer handle all in one shot
	if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer via VMA");
	}

}

/// <summary>
/// Copies data from one buffer to another entirely on GPU
/// Use this move vertex data from CPU-visable staging buffer to our priavate GPU-local buffer
/// </summary>
/// <param name="srcBufer"> The source buffer </param>
/// <param name="dstBuffer"> The destination buffer </param>
/// <param name="size"> How much to copy </param>
void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	//For GPU to copy memory, must record a command buffer and submit the command
	//Alocate a temp "one-time subite" command buffer of our exisiting pool.
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer tempCommandBuffer;
	vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &tempCommandBuffer);

	//Starting the recording
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	//Tells the driver that this will only be executed once
	vkBeginCommandBuffer(tempCommandBuffer, &beginInfo);

	//This is to define the copy regin, copying the entiire size of the buffer
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	//Queue the hardware copy command
	vkCmdCopyBuffer(tempCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	//Stop recoring and submit to GPU 
	vkEndCommandBuffer(tempCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tempCommandBuffer;

	//Submit the graphics queue and wait for completion (syncrhonou wait)
	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	//Free the temp command buffer
	vkFreeCommandBuffers(m_logicalDevice, m_commandPool, 1, &tempCommandBuffer);

}

void Application::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size(); //20Bytes * 3 vertices = 60 bytes

	//Create a temp staging buffer such that CPU can see
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,	//This buffer is a "transferred source" that we read from it
		VMA_MEMORY_USAGE_CPU_TO_GPU,		//Tells VMA to make this memory visable to CPU
		stagingBuffer,
		stagingAllocation
	);

	//Copy C++ vertex array into staging buffer
	void* data;
	vmaMapMemory(m_allocator, stagingAllocation, &data);	//Map GPU memory to a C++ pointer
	memcpy(data, m_vertices.data(), (size_t)bufferSize);	//Copy raw bytes
	vmaUnmapMemory(m_allocator, stagingAllocation);			//Unmmaping immidately

	//Create permanent vertex buffer
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,	//Is a transfer destination and a vertex buffer
		VMA_MEMORY_USAGE_GPU_ONLY, //Highly Optimized, complete private GPU memory. *Private GPU meaning only GPU have access to it which the CPU can't touch it
		m_vertexBuffer,
		m_vertexAllocation
	);

	//Tell the GPU to copy the data from the staging buffer to vertex buffer
	copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	//Cleaning up the staging buffer
	//We don't need the staging buffer now that vertices are safely copied to VRAM
	vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

	std::cout << "Engine Buffer : Vertex buffer successfully loaded into private VRAM using VMA. " << std::endl;

}

void Application::createIndexBuffer(){
	VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

	//Temp staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		stagingBuffer,
		stagingAllocation
	);

	//Copy index vector data to staging buffer
	void* data;
	vmaMapMemory(m_allocator, stagingAllocation, &data);
	memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
	vmaUnmapMemory(m_allocator, stagingAllocation);

	//GPU only index buffer
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		m_indexBuffer,
		m_indexAllocation
	);

	copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

	std::cout << "Engine Buffer : Index buffer successfully loaded into private VRAM using VMA." << std::endl;
}
