#include "core/Application.h"
#include <stdexcept>
#include <chrono>

//This helper functions helps to tell Vulkan what resources (Uniform buffers, Samplers, Storage buffers) the shader expects as each binding slot
//Purpose: Defines the shader-visiable interface schema (bindings, types, pipeline stages)
void Application::createDescriptorSetLayout() {
	// 1. Describing the binding for our UniformBufferObject (Binding 0 in shader.vert)
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;											// Matches the layout(binding = 0) in shader.vert
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// COnstant buffer with fast read cache
	uboLayoutBinding.descriptorCount = 1;									// Only consumed by the Vertex Shader
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //Only accessed by Vertex Shader

	// 2. Create the layout handle
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	//Param for the vkCreateDescriptorSetLayout(Logical Device creating and owning this layout,
	//											Layout configuration struct,
	//											Optional custom memory allocator callbacks,
	// 											Output handle to store the created layout)			
	if (vkCreateDescriptorSetLayout(m_logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	}

}

// DESCRIPTOR POOL CREATION
// Allocates the GPU memory pool from which individual descriptor sets are drawn
void Application::createDescriptorPool() {
	//Specify how many descriptors of each tpye this pool can provide
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	//Configuration of the pool to hold sets for all frames-in-flight
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(m_logicalDevice, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}

// DESCRIPTOR SET ALLOCATION & POPULATION
// Allocates sets from the pool and writes the specific buffer handles to binding 0
void Application::createDescriptorSets() {
	// Allocation of one set per frame-in-flight using the same layout blueprint
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_logicalDevice, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	// Populate each descriptor set with it's correspoonding frame's uniform buffer
	for (size_t frameIndex{}; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[frameIndex];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[frameIndex];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		//Commit the write to update the GPU Driver
		vkUpdateDescriptorSets(m_logicalDevice, 1, &descriptorWrite, 0, nullptr);
	}
}


//Unifiorm buffer allocation & persistant mapping
//Allocation of the actual memoryu to the GPU which will hold the Model, View and Projection matrices.
void Application::createUniformBuffers(){
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBuffersAllocation.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t idx{}; idx < MAX_FRAMES_IN_FLIGHT; ++idx) {
		// 1. Creating the CPU to GPU buffer using custom helper, which is createBuffer
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			m_uniformBuffers[idx],
			m_uniformBuffersAllocation[idx]
		);

		// 2. Persistenly mapping the buffer so we can write to it every frame via memcpy
		vmaMapMemory(m_allocator, m_uniformBuffersAllocation[idx], &m_uniformBuffersMapped[idx]);
	}
}

//Function calculates the rotation angles and projection matrices, copies the struct into
//mapped memory for active frame index.
void Application::updateUniformBuffer(uint32_t currentImage){
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};

	// Rotate across X, Y, and Z axes simultaneously
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.5f, 1.0f, 0.0f));

	// Place the camera elevated and backed away from the cube
	ubo.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.5f), // Eye
		glm::vec3(0.0f, 0.0f, 0.0f), // Target
		glm::vec3(0.0f, 0.0f, 1.0f)  // Z-Up
	);
	// 3. Projection Matrix: 45 degree vertical FOV
	float aspect = m_swapChainExtent.width / static_cast<float>(m_swapChainExtent.height);
	ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

	// GLM was designed for OpenGL where Y is inverted relative to Vulkan clip coordinates
	ubo.proj[1][1] *= -1.0f;	//Invert Y for Vulkan clip coordinates

	//Fast copy
	memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));


}
