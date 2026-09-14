#include "core/Application.h"

//Helper functions for Swap chain

/**
* @brief Checks if the monitors supports SRGB color space
**/
VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	//Look through the list of formats with GPU supports
	for (const auto& availableFormat : availableFormats) {

		//We want SRGB color space with 8-bit per channel color format (RGB)
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	//If there isn't just use the first format
	return availableFormats[0];
}

/**
* @brief V-SYnc behavior. Used to render as fast but simply replaces the old frame if the monitor
* isn't ready yet, ensuring ultra low-latency without screen tearing
**/
VkPresentModeKHR Application::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		//Mail box mode allwoing low-latency rendering without tearing
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	//Fifo is the standard V-sync. Locks framerate to monitor's refresh rate.
	//Vulkan spec guarantees that FIFO is always available on every device.
	return VK_PRESENT_MODE_FIFO_KHR;
}

/**
* @brief Calculate the exact pixel dimensions of the images we are creating
**/
VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilites) {
	//If the driver specfies the exact resolution, use it directly
	if (capabilites.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilites.currentExtent;
	}
	else {
		//Otherwise ask GLFW for actual pixel dimensions of the window
		int width{}, height{};

		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width),
		static_cast<uint32_t>(height) };

		actualExtent.width = std::clamp<uint32_t>(actualExtent.width,
			capabilites.minImageExtent.width,
			capabilites.maxImageExtent.width);

		actualExtent.height = std::clamp<uint32_t>(actualExtent.height,
			capabilites.minImageExtent.height,
			capabilites.maxImageExtent.height);

		return actualExtent;
	}

}

/**
* @brief Function to query about the swap frame buffer
*/
SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice& device) {
	SwapChainSupportDetails dets{};

	//1. Get basic surface capabilites
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &dets.capabilities);

	//2. Surface formats
	uint32_t formatCount{};
	//Check first to ensure that there are formats first
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
	if (formatCount != 0) {
		//Once confirmed that there are formats from the surface, proceeed to store them
		dets.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, dets.formats.data());
	}

	//3. Present modes
	//Two call enumerations. First call asks the driver how many entries will it return, then resize, then write the entries into those elements
	uint32_t presentModeCount{};
	//Check if there are present modes present first
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		dets.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, dets.presentModes.data());
	}
	return dets;

}

/**
 * @brief Allocates and configures the virtual swapchain system.
 * Evaluates hardware features to choose optimal image layouts, determines the target size
 * of the canvas bounds, specifies structural queue ownership parameters, allocates the
 * underlying images, and caches configuration properties for the graphics pipeline.
 */
void Application::createSwapChain() {
	//1. Query the physical GPU to see the capabilites that the current monitor/surface support
	SwapChainSupportDetails swapChainSupport{ querySwapChainSupport(m_physicalDevice) }; //m_physicalevce is referring to the selected GPU

	//2. Setting selection
	//Use helper functions to pick the best color format, presentation mode and resolution
	VkSurfaceFormatKHR surfaceFormat{ chooseSwapSurfaceFormat(swapChainSupport.formats) };
	VkPresentModeKHR presentMode{ chooseSwapPresentMode(swapChainSupport.presentModes) };
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	//3. Request one more image than he minimum to prevent waiting on the driver
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//4. Swapchain blueprint
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;

	//Core canvas setting
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//5. Queue syncronaization logic
	//Check if the silicon lane that draws the Graphics is the same lane that shows the image (Present)
	QueueFamilies indices{ findHardwareFamily(m_physicalDevice) };
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		//Different lanes: Image must be explictly shared between queues to avoid memory access violations
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		//Same lane for optimaization for drawing and presenting happening on the same queue
		//Exclusive mode provides the highest performance
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	//6. Presentation & OS Window Settings
	//Thelling the swap chain if we want to physically rotate the image(But we will just use the screen's default)
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

	//Ignore alpha blending with other OS desktop windows behind ours (Creates a solid, opaque window)
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	//Set presentation mode
	createInfo.presentMode = presentMode;

	//ALlow GPU to skip rendering pixels that are obscured by other windows overlapping
	createInfo.clipped = VK_TRUE;

	//If window resizes, a new chain swap must be made. Points to the old one to transfer ownership smoothly
	createInfo.oldSwapchain = VK_NULL_HANDLE;


	//7. EXECUTION
	//Hand the completed blueprint of the information to the driver to allocate the Swap Chain Object
	if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain");
	}

	//8. Retrieve the raw image handles
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);

	//Resize the vector to hold the size
	m_swapChainImages.resize(imageCount);

	//Pull the raw VkImage handles out of the GPU and store them in the vecotr
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

	//8. Cache the pipeline variables
	//Save the setting because the future graphics pipeline needs to know the exact format
	// and size of the canvas it is expected to draw onto
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;
}

/**
* @brief Create image view for all swap chain images.
* * Iterates through the raw VkImage handles acquired from the Swap Chain
* and wraps them VkImageView objects. These views act as "lenses" that
* instruct the GPU's graphics pipeline to treat the raw memory as 2D color
* textures, allowing the engine to safely render pixels to the screen buffer.
*
*/
void Application::createImageViews() {
	//1. Resize the container to match the exact number of swap chain
	m_swapChainImageViews.resize(m_swapChainImages.size());

	//2. Iterate and create view for raw image
	for (size_t idx{}; idx < m_swapChainImages.size(); ++idx) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapChainImages[idx];

		//Treat images as standard 2D texture
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		//Use the format cached during the swap chain creation
		createInfo.format = m_swapChainImageFormat;

		//3. Component mapping using swizzling
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//4/ Subresource range
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		// No mipmapping or multiple layers needed for a standard 2D swap chain
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		// 5. Execution
		if (vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[idx]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views!");
		}
	}
}

void Application::createDepthResources(){
	m_depthImageInitialized = false;

// 1. Create image information
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = m_swapChainExtent.width;
	imageInfo.extent.height = m_swapChainExtent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = m_depthFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	if (vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_depthImage, &m_depthImageAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate depth image via VMA!");
	}

	// 2. Image View Create Info
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_logicalDevice, &viewInfo, nullptr, &m_depthImageView) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create depth image view!");
	}

}

void Application::framebufferResizeCallBack(GLFWwindow* window, int  width, int height) {
	(void)width; //Unused variable for now
	(void)height; //Unused variable for now

	auto* application{ static_cast<Application*>(glfwGetWindowUserPointer(window)) };
	if (application != nullptr) {	//If there isn't any pointers
		application->m_framebufferResized = true;
	}
}

void Application::recreateSwapChain() {
	int width{};
	int height{};
	glfwGetFramebufferSize(m_window, &width, &height);

	while ((width == 0 || height == 0) && !glfwWindowShouldClose(m_window)) {
		glfwWaitEvents();
		glfwGetFramebufferSize(m_window, &width, &height);
	}

	if (glfwWindowShouldClose(m_window)) { //Request to close has been made
		return;
	}

	if (vkDeviceWaitIdle(m_logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("Failed to wait for device before swapchain recreation");
	}
	
	//Saves the old format first
	const VkFormat oldFormat = m_swapChainImageFormat;

	// The GPU is idle. Release resources tied to the old extent and image indices
	// before querying the surface and creating the replacement swapchain.
	cleanupSwapChain();

	// Dynamic viewport/scissor handle size changes, but the pipeline's color
	// attachment format is fixed at creation and must match the new swapchain.
	createSwapChain();
	if (m_swapChainImageFormat != oldFormat) {
		vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
		m_graphicsPipeline = VK_NULL_HANDLE;

		vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;

		createGraphicsPipeline();
	}
	createImageViews();
	createDepthResources();
	createRenderFinishedSemaphores();
	m_framebufferResized = false;
}

void Application::cleanupSwapChain() {
	//The clean up
	for (VkImageView imageView : m_swapChainImageViews) {
		if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(m_logicalDevice, imageView, nullptr);	//Passed nullptr because there is no use of custom Vulkan allocation callbacks
		}
	}
	m_swapChainImageViews.clear();

	if (m_depthImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(m_logicalDevice, m_depthImageView, nullptr);
		m_depthImageView = VK_NULL_HANDLE;
	}

	if (m_depthImage != VK_NULL_HANDLE) {
		vmaDestroyImage(m_allocator, m_depthImage, m_depthImageAllocation); //In order to allocate and destroy both of them at the same time so VMA can handle it
		m_depthImage = VK_NULL_HANDLE;
		m_depthImageAllocation = VK_NULL_HANDLE;
	}

	//Next we need to remove the semaphores that were associated with the old image indices
	for (VkSemaphore semaphore : m_renderFinishedSemaphores) {
		if (semaphore != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_logicalDevice, semaphore, nullptr);
		}
	}
	m_renderFinishedSemaphores.clear();

	//Then finally clear the parent swapchains
	if (m_swapChain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
		m_swapChain = VK_NULL_HANDLE;
	}
	m_swapChainImages.clear();
}
