#include "HardwareRenderer.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Useful/Useful.h"
#include "Vulkan/VulkanImages.h"
#include "Vulkan/VulkanInitialisers.h"
#include "Vulkan/VulkanPipelines.h"
#include "Imgui/backends/imgui_impl_sdl3.h"
#include "Imgui/backends/imgui_impl_vulkan.h"
#include "Imgui/implot.h"

#include "../Software/Common.h"

#include "CameraController.h"

void HardwareRenderer::InitializeVulkan()
{
	CreateInstance();
	InitializeDescriptors();
	InitializeSwapchain();
	InitializeCommands();
	InitializeSyncStructures();
	InitializeImgui();
	InitializePipelines();

	m_bInitialized = true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL HardwareRenderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cout << FormatString("%s\n", pCallbackData->pMessage) + "\n";
	return VK_FALSE;
}

void HardwareRenderer::CreateInstance()
{
	vkb::InstanceBuilder builder;

	auto instanceRet = builder.set_app_name("Raytracer")
		.request_validation_layers(m_bEnableValidationLayers)
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
		.require_api_version(1, 3, 0)
		.set_debug_callback(&HardwareRenderer::DebugCallback)
		.set_engine_name("Raytracer")
		.set_engine_version(VK_MAKE_API_VERSION(1, 0, 0, 0)) //TODO: Add proper versioning to this.
		.build();

	vkb::Instance vkbInstance = instanceRet.value();

	m_vulkanInstance = vkbInstance.instance;
	m_debugMessenger = vkbInstance.debug_messenger;

	m_pWindow = new Window("Raytracer", 800, 600);

	if (SDL_Vulkan_CreateSurface(m_pWindow->GetSDLWindow(), m_vulkanInstance, NULL, &m_surface) != true)
	{
		throw std::exception("Failed to create Vulkan surface.");
	}

	//Vulkan 1.3 Features
	VkPhysicalDeviceVulkan13Features features{};
	features.dynamicRendering = true;
	features.synchronization2 = true;

	//Vulkan 1.2 Features
	VkPhysicalDeviceVulkan12Features features12{};
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	//Using VkBootstrap to select a gpu.
	//Supports some 1.3 & 1.2 features and also supplies the surface for writing to.
	vkb::PhysicalDeviceSelector selector{ vkbInstance };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(m_surface)
		.select()
		.value();


	std::vector<vkb::CustomQueueDescription> computeQueueDescriptions;
	auto queue_families = physicalDevice.get_queue_families();
	int computeQueueCount = 0;

	bool foundGraphicsFamily = false;

	for (uint32_t i = 0; i < static_cast<uint32_t>(queue_families.size()); i++)
	{
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && foundGraphicsFamily == false)
		{
			m_graphicsQueueFamily = i;
			foundGraphicsFamily = true;
		}
	}

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	deviceBuilder.custom_queue_setup(computeQueueDescriptions);

	vkb::Device vkbDevice = deviceBuilder.build().value();

	m_device = vkbDevice.device;
	m_physicalDevice = physicalDevice.physical_device;

	//Use VkBootstrap to get a Graphics Queue
	vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = m_physicalDevice;
	allocatorInfo.device = m_device;
	allocatorInfo.instance = m_vulkanInstance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &m_allocator);
}

void HardwareRenderer::InitializeSwapchain()
{
	glm::ivec2 windowSize = m_pWindow->GetWindowSize();
	CreateSwapchain(windowSize.x, windowSize.y);

	//TODO: experiment with downscaling this for a pixelated effect on entities of any rotation
	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		windowSize.x,
		windowSize.y,
		1
	};

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

	m_drawImage = CreateImage(this, drawImageExtent, VK_FORMAT_R32G32B32A32_SFLOAT, drawImageUsages, false, "DrawImage");
	m_accumulationImage = CreateImage(this, drawImageExtent, VK_FORMAT_R32G32B32A32_SFLOAT, drawImageUsages, false, "AccumulationImage");

	DescriptorWriter writer;
	writer.WriteImage(0, m_drawImage.m_imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.WriteImage(1, m_accumulationImage.m_imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.UpdateSet(m_device, m_drawImageDescriptors);
}

SwapChainSupportDetails HardwareRenderer::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR HardwareRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR HardwareRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

void HardwareRenderer::CreateSwapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder{ m_physicalDevice, m_device, m_surface };
	m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	SwapChainSupportDetails swapchainSupport = QuerySwapChainSupport(m_physicalDevice);

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{ .format = m_swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(ChooseSwapPresentMode(swapchainSupport.presentModes))
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	m_swapchainExtent = vkbSwapchain.extent;
	m_swapchain = vkbSwapchain.swapchain;
	m_swapchainImages = vkbSwapchain.get_images().value();
	m_swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void HardwareRenderer::DestroySwapchain()
{
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

	for (int i = 0; i < m_swapchainImageViews.size(); i++)
	{
		vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
	}
}

void HardwareRenderer::RecreateSwapchain()
{
	vkDeviceWaitIdle(m_device);
	m_bInitialized = false;

	DestroyImage(this, &m_drawImage);
	DestroyImage(this, &m_accumulationImage);

	DestroySwapchain();
	InitializeSwapchain();

	m_bRefreshAccumulation = true;
	m_bInitialized = true;
}

void HardwareRenderer::InitializeCommands()
{
	//Create a command pool for commands submitted to the graphics queue.
	//We also want the pool to allow for resetting of individual command buffers.
	VkCommandPoolCreateInfo commandPoolInfo = CommandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i].m_commandPool) != VK_SUCCESS)
			throw std::exception();

		VkCommandBufferAllocateInfo cmdAllocInfo = CommandBufferAllocateInfo(m_frames[i].m_commandPool, 1);
		if (vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i].m_mainCommandBuffer) != VK_SUCCESS)
			throw std::exception();
	}

	if (vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_immediateCommandPool) != VK_SUCCESS)
		throw std::exception();

	VkCommandBufferAllocateInfo cmdAllocInfo = CommandBufferAllocateInfo(m_immediateCommandPool, 1);
	if (vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_immediateCommandBuffer) != VK_SUCCESS)
		throw std::exception();

	std::lock_guard<std::mutex> lock(m_deletionQueueMutex);
	m_mainDeletionQueue.push_function([=]() {
		vkDestroyCommandPool(m_device, m_immediateCommandPool, nullptr);
	});
}

void HardwareRenderer::InitializeSyncStructures()
{
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceCreateInfo = FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = SemaphoreCreateInfo();

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frames[i].m_renderFence) != VK_SUCCESS)
			throw std::exception();

		if (vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].m_swapchainSemaphore) != VK_SUCCESS)
			throw std::exception();
		if (vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].m_renderSemaphore) != VK_SUCCESS)
			throw std::exception();
	}

	if (vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immediateFence) != VK_SUCCESS)
		throw std::exception();

	std::lock_guard<std::mutex> lock(m_deletionQueueMutex);
	m_mainDeletionQueue.push_function([=]() { vkDestroyFence(m_device, m_immediateFence, nullptr); });
}

void HardwareRenderer::InitializeDescriptors()
{
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 5 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5 }
	};

	m_globalDescriptorAllocator.InitializePool(m_device, 10, sizes);

	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

		m_drawImageDescriptorLayout = builder.Build(m_device);
	}

	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		m_sceneDescriptorLayout = builder.Build(m_device);
	}

	m_drawImageDescriptors = m_globalDescriptorAllocator.Allocate(m_device, m_drawImageDescriptorLayout);
	m_sceneDescriptor = m_globalDescriptorAllocator.Allocate(m_device, m_sceneDescriptorLayout);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frameSizes =
		{
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		m_frames[i].m_frameDescriptors = DescriptorAllocatorGrowable{};
		m_frames[i].m_frameDescriptors.Initialize(m_device, 1000, frameSizes);

		std::lock_guard<std::mutex> lock(m_deletionQueueMutex);
		m_mainDeletionQueue.push_function([&, i]()
			{
				m_frames[i].m_frameDescriptors.DestroyPools(m_device);
			});
	}

	std::lock_guard<std::mutex> lock(m_deletionQueueMutex);
	m_mainDeletionQueue.push_function([=]()
		{
			vkDestroyDescriptorSetLayout(m_device, m_drawImageDescriptorLayout, nullptr);

			m_globalDescriptorAllocator.ClearDescriptors(m_device);
			m_globalDescriptorAllocator.DestroyPool(m_device);
		});
}

void HardwareRenderer::InitializeImgui()
{
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	if (vkCreateDescriptorPool(m_device, &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
		throw std::exception();

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	m_imguiContext = ImGui::CreateContext();
	ImPlot::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL3_InitForVulkan(m_pWindow->GetSDLWindow());

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_vulkanInstance;
	init_info.PhysicalDevice = m_physicalDevice;
	init_info.Device = m_device;
	init_info.Queue = m_graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineRenderingCreateInfo pipelineCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR, .colorAttachmentCount = 1, .pColorAttachmentFormats = &m_swapchainImageFormat };
	init_info.PipelineRenderingCreateInfo = pipelineCreateInfo;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();

	std::lock_guard<std::mutex> lock(m_deletionQueueMutex);
	m_mainDeletionQueue.push_function([=]()
		{
			ImPlot::DestroyContext();
			ImGui_ImplVulkan_Shutdown();

			vkDestroyDescriptorPool(m_device, imguiPool, nullptr);
		});
}

void HardwareRenderer::InitializePipelines()
{
	{
		std::vector<VkDescriptorSetLayout> layouts;
		layouts.push_back(m_drawImageDescriptorLayout);
		layouts.push_back(m_sceneDescriptorLayout);

		VkPushConstantRange pushConstant = VkPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(RaytracePushConstants));
		std::vector<VkPushConstantRange> pushConstants;
		pushConstants.push_back(pushConstant);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PipelineLayoutCreateInfo();
		pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
		pipelineLayoutInfo.pSetLayouts = layouts.data();
		pipelineLayoutInfo.setLayoutCount = layouts.size();
		if (vkCreatePipelineLayout(GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_raytracePipelineLayout) != VK_SUCCESS)
			throw std::exception(FormatString("Failed to create pipeline layout for pipeline.").c_str());

		PipelineBuilder pipelineBuilder;

		std::string computePath = GetWorkingDirectory() + "\\Resources\\Shaders\\raytrace.spv";
		VkShaderModule computeShader;
		LoadShaderModule(computePath.c_str(), m_device, &computeShader);

		pipelineBuilder.SetComputeShader(computeShader);
		pipelineBuilder.m_pipelineLayout = m_raytracePipelineLayout;
		m_raytracePipeline = pipelineBuilder.BuildComputePipeline(GetLogicalDevice());

		vkDestroyShaderModule(m_device, computeShader, nullptr);
	}
}

void HardwareRenderer::Quit()
{
	m_bRun = false;
}

int HardwareRenderer::GetMaterialIndex(const GPUMaterial& material)
{
	for(int i = 0; i < m_sceneMaterials.size(); ++i)
	{
		if (m_sceneMaterials[i] == material)
			return i;
	}

	return -1;
}

void HardwareRenderer::AddSceneObject(std::string modelPath, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, const GPUMaterial& material)
{
	SceneObject newObject;
	newObject.modelName = modelPath;
	newObject.position = position;
	newObject.rotation = rotation;
	newObject.scale = scale;
	newObject.materialIndex = GetMaterialIndex(material);

	m_sceneObjects.push_back(newObject);
}

void HardwareRenderer::ConvertSceneObjectToGPUObject(const SceneObject& obj)
{
	GPUObject gpuObject;

	glm::mat4 objectMat = glm::mat4(1.0f);
	objectMat = glm::translate(objectMat, obj.position);
	objectMat = glm::rotate(objectMat, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
	objectMat = glm::rotate(objectMat, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
	objectMat = glm::rotate(objectMat, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
	objectMat = glm::scale(objectMat, obj.scale);

	GPUAABB objectAABB = m_models[obj.modelName].boundingBox;
	objectAABB.min = glm::vec4(objectMat * glm::vec4(objectAABB.min, 1.0f));
	objectAABB.max = glm::vec4(objectMat * glm::vec4(objectAABB.max, 1.0f));
	objectAABB.objectIndex = m_gpuSceneObjects.size();

	gpuObject.inverseTransform = glm::inverse(objectMat);
	gpuObject.materialIndex = obj.materialIndex;

	gpuObject.triangleStartIndex = m_models[obj.modelName].triangleStartIndex;
	gpuObject.triangleCount = m_models[obj.modelName].triangleCount;

	m_gpuSceneObjects.push_back(gpuObject);

	m_sceneAABBs.push_back(objectAABB);
}

void HardwareRenderer::InitializeScene()
{
	GPUMaterial groundMaterial;
	groundMaterial.albedo = glm::vec3(0.5, 0.5, 0.5);
	groundMaterial.smoothness = 0.0;
	groundMaterial.emission = 0.0f;
	m_sceneMaterials.push_back(groundMaterial);

	GPUMaterial diffuseMaterial;
	diffuseMaterial.albedo = glm::vec3(0.8, 0.8, 0.8);
	diffuseMaterial.emission = 0.0f;
	m_sceneMaterials.push_back(diffuseMaterial);

	GPUMaterial glassMaterial;
	glassMaterial.albedo = glm::vec3(1.0, 1.0, 1.0);
	glassMaterial.refractiveIndex = 1.5f;
	glassMaterial.emission = 0.0f;
	m_sceneMaterials.push_back(glassMaterial);

	GPUMaterial reflectiveMaterial;
	reflectiveMaterial.albedo = glm::vec3(1.0, 1.0, 1.0);
	reflectiveMaterial.smoothness = 1.0;
	reflectiveMaterial.fuzziness = 0.0f;
	reflectiveMaterial.emission = 0.0f;
	m_sceneMaterials.push_back(reflectiveMaterial);

	GPUMaterial emissiveMaterial;
	emissiveMaterial.albedo = glm::vec3(1.0, 1.0, 1.0);
	emissiveMaterial.emission = 15.0f;
	m_sceneMaterials.push_back(emissiveMaterial);

	std::string testModelPath = GetWorkingDirectory() + "\\Resources\\Models\\flat_quad.obj";
	LoadModel(testModelPath);

	std::string spherePath = GetWorkingDirectory() + "\\Resources\\Models\\icosphere.obj";
	LoadModel(spherePath);

	AddSceneObject(spherePath, glm::vec3(-4, 1.1, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), reflectiveMaterial);
	AddSceneObject(spherePath, glm::vec3(0, 1.1, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), glassMaterial);
	AddSceneObject(spherePath, glm::vec3(4, 1.1, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), diffuseMaterial);
	AddSceneObject(spherePath, glm::vec3(0, 5, 0), glm::vec3(0, 0, 0), glm::vec3(0.5, 0.5, 0.5), emissiveMaterial);

	AddSceneObject(testModelPath, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1000, 1000, 1000), groundMaterial);

	BufferSceneData();
}

void HardwareRenderer::BufferSceneData()
{
	m_gpuSceneObjects.clear();
	for (const auto& obj : m_sceneObjects)
	{
		ConvertSceneObjectToGPUObject(obj);
	}

	m_sceneAABBBuffer = CreateBuffer(sizeof(GPUAABB) * m_sceneAABBs.size()+1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "SceneAABBBuffer");
	void* data;
	vmaMapMemory(m_allocator, m_sceneAABBBuffer.m_allocation, &data);
	memcpy(data, m_sceneAABBs.data(), sizeof(GPUAABB) * m_sceneAABBs.size());
	vmaUnmapMemory(m_allocator, m_sceneAABBBuffer.m_allocation);

	m_sceneTriangleBuffer = CreateBuffer(sizeof(GPUTriangle) * m_sceneTriangles.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "SceneSphereBuffer");
	vmaMapMemory(m_allocator, m_sceneTriangleBuffer.m_allocation, &data);
	memcpy(data, m_sceneTriangles.data(), sizeof(GPUTriangle) * m_sceneTriangles.size());
	vmaUnmapMemory(m_allocator, m_sceneTriangleBuffer.m_allocation);

	m_sceneObjectBuffer = CreateBuffer(sizeof(GPUObject) * m_gpuSceneObjects.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "SceneObjectBuffer");
	vmaMapMemory(m_allocator, m_sceneObjectBuffer.m_allocation, &data);
	memcpy(data, m_gpuSceneObjects.data(), sizeof(GPUObject) * m_gpuSceneObjects.size());
	vmaUnmapMemory(m_allocator, m_sceneObjectBuffer.m_allocation);

	m_sceneMaterialBuffer = CreateBuffer(sizeof(GPUMaterial) * m_sceneMaterials.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "SceneMaterialBuffer");
	vmaMapMemory(m_allocator, m_sceneMaterialBuffer.m_allocation, &data);
	memcpy(data, m_sceneMaterials.data(), sizeof(GPUMaterial) * m_sceneMaterials.size());
	vmaUnmapMemory(m_allocator, m_sceneMaterialBuffer.m_allocation);

	DescriptorWriter writer;
	writer.WriteBuffer(0, m_sceneAABBBuffer.m_buffer, sizeof(GPUAABB) * m_sceneAABBs.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(1, m_sceneTriangleBuffer.m_buffer, sizeof(GPUTriangle) * m_sceneTriangles.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(2, m_sceneObjectBuffer.m_buffer, sizeof(GPUObject) * m_gpuSceneObjects.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(3, m_sceneMaterialBuffer.m_buffer, sizeof(GPUMaterial) * m_sceneMaterials.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.UpdateSet(m_device, m_sceneDescriptor);
}

void HardwareRenderer::DispatchRayTracingCommands(VkCommandBuffer cmd)
{
	float pixelSampleScale = 1.0f / static_cast<float>(m_pushConstants.raysPerPixel);
	auto theta = glm::radians(m_camera.cameraFov);
	auto h = std::tan(theta / 2.0);
	float viewportHeight = 2 * h * m_camera.focusDistance;
	float viewportWidth = viewportHeight * (double(m_drawExtent.width) / m_drawExtent.height);

	glm::vec3 up = glm::vec3(0, 1, 0);
	glm::vec3 w = glm::normalize(-m_camera.cameraLookDirection);
	glm::vec3 u = glm::normalize(glm::cross(up, w));
	glm::vec3 v = glm::cross(w, u);

	auto viewportU = viewportWidth * u;
	auto viewportV = viewportHeight * -v;

	glm::vec3 pixelDeltaU = viewportU / static_cast<float>(m_drawExtent.width);
	glm::vec3 pixelDeltaV = viewportV / static_cast<float>(m_drawExtent.height);

	auto viewportUpperLeft = m_camera.cameraPosition - (m_camera.focusDistance * w) - viewportU / 2.0f - viewportV / 2.0f;
	glm::vec3 pixel00Location = viewportUpperLeft + 0.5 * (pixelDeltaU + pixelDeltaV);

	auto defocusRadius = m_camera.focusDistance * std::tan(glm::radians(m_camera.defocusAngle / 2.0));

	m_pushConstants.pixel00Location = pixel00Location;
	m_pushConstants.pixelDeltaU = pixelDeltaU;
	m_pushConstants.pixelDeltaV = pixelDeltaV;
	m_pushConstants.cameraPosition = m_camera.cameraPosition;
	m_pushConstants.defocusAngle = m_camera.defocusAngle;
	m_pushConstants.defocusDiskU = defocusRadius * u;
	m_pushConstants.defocusDiskV = defocusRadius * v;
	m_pushConstants.parentAABBCount = m_sceneAABBs.size();

	std::vector<VkDescriptorSet> sets;
	sets.push_back(m_drawImageDescriptors);
	sets.push_back(m_sceneDescriptor);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_raytracePipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_raytracePipelineLayout, 0, sets.size(), sets.data(), 0, nullptr);

	vkCmdPushConstants(cmd, m_raytracePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(RaytracePushConstants), &m_pushConstants);

	uint32_t groupCountX = (m_drawExtent.width + 31) / 32;
	uint32_t groupCountY = (m_drawExtent.height + 31) / 32;
	vkCmdDispatch(cmd, groupCountX, groupCountY, 1);
}

void HardwareRenderer::RefreshAccumulation(VkCommandBuffer cmd)
{
	VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	VkImageSubresourceRange clearRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdClearColorImage(cmd, m_accumulationImage.m_image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);

	m_pushConstants.frame = 0;
	m_bRefreshAccumulation = false;
}

void HardwareRenderer::RenderImGui(VkCommandBuffer cmd, VkImage targetImage, VkImageView targetImageView)
{
	ImGui::Render();

	VkRenderingAttachmentInfo colorAttachment = AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingInfo renderInfo = RenderingInfo(m_swapchainExtent, &colorAttachment, nullptr);

	TransitionImage(cmd, targetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	vkCmdEndRendering(cmd);
}

void HardwareRenderer::RenderFrame()
{
	if (!m_bInitialized)
		return;

	Uint32 windowFlags = m_pWindow->GetWindowFlags();
	if (windowFlags & SDL_WINDOW_MINIMIZED || windowFlags & SDL_WINDOW_HIDDEN)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(m_renderMutex);
	std::lock_guard<std::mutex> immediateLock(m_immediateSubmitMutex);

	if (vkWaitForFences(m_device, 1, &GetCurrentFrame().m_renderFence, true, 1000000000) != VK_SUCCESS)
		throw std::exception("Failed to wait for fence.");

	GetCurrentFrame().m_deletionQueue.flush();
	GetCurrentFrame().m_frameDescriptors.ClearPools(m_device);

	if (vkResetFences(m_device, 1, &GetCurrentFrame().m_renderFence) != VK_SUCCESS)
		throw std::exception("Failed to reset fence.");

	uint32_t swapchainImageIndex;
	if (vkAcquireNextImageKHR(m_device, m_swapchain, 100000000, GetCurrentFrame().m_swapchainSemaphore, nullptr, &swapchainImageIndex) != VK_SUCCESS)
		throw std::exception("Failed to acquire next image.");

	VkCommandBuffer cmd = GetCurrentFrame().m_mainCommandBuffer;
	if (vkResetCommandBuffer(cmd, 0) != VK_SUCCESS)
		throw std::exception("Failed to reset command buffer.");

	VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	if (vkBeginCommandBuffer(cmd, &cmdBeginInfo) != VK_SUCCESS)
		throw std::exception("Failed to begin command buffer.");

	TransitionImage(cmd, m_drawImage.m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	TransitionImage(cmd, m_accumulationImage.m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	m_drawExtent.width = m_drawImage.m_imageExtent.width;
	m_drawExtent.height = m_drawImage.m_imageExtent.height;

	VkViewport viewport = {};
	viewport.width = m_drawExtent.width;
	viewport.height = m_drawExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = m_drawExtent.width;
	scissor.extent.height = m_drawExtent.height;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	if(m_bRefreshAccumulation)
		RefreshAccumulation(cmd);

	DispatchRayTracingCommands(cmd);

	TransitionImage(cmd, m_drawImage.m_image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	TransitionImage(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyImageToImage(cmd, m_drawImage.m_image, m_swapchainImages[swapchainImageIndex], m_drawExtent, m_swapchainExtent, m_drawFilter);
	TransitionImage(cmd, m_drawImage.m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	RenderImGui(cmd, m_swapchainImages[swapchainImageIndex], m_swapchainImageViews[swapchainImageIndex]);

	TransitionImage(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
		throw std::exception("Failed to end command buffer.");

	VkCommandBufferSubmitInfo cmdInfo = CommandBufferSubmitInfo(cmd);

	VkSemaphoreSubmitInfo waitInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().m_swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().m_renderSemaphore);

	VkSubmitInfo2 submit = SubmitInfo(&cmdInfo, &signalInfo, &waitInfo);

	if (vkQueueSubmit2(m_graphicsQueue, 1, &submit, GetCurrentFrame().m_renderFence) != VK_SUCCESS)
		throw std::exception("Failed to submit queue.");

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &GetCurrentFrame().m_renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VkResult result = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapchain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::exception("Failed to present swap chain image.");
	}

	vkDeviceWaitIdle(m_device);
	m_iCurrentFrame = (m_iCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HardwareRenderer::MainLoop()
{
	ImGuiIO& io = ImGui::GetIO();
	CameraController cameraController;

	while (m_bRun)
	{
		m_performanceStats.StartPerformanceMeasurement("Frame");

		m_inputManager.HandleGeneralInput();
		if (m_pWindow->IsWindowMinimized())
			continue;

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		cameraController.Update(this);

		if(!cameraController.m_bLockedMouse)
		{
			bool resetAccumulation = false;

			ImGui::Begin("Raytracer Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::SeparatorText("Camera Settings");
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			std::string cameraPosStr = FormatString("Camera Position: (%.2f, %.2f, %.2f)", m_camera.cameraPosition.x, m_camera.cameraPosition.y, m_camera.cameraPosition.z);
			ImGui::Text(cameraPosStr.c_str());

			if(ImGui::DragFloat("Field of View", &m_camera.cameraFov, 0.1f, 1.0f, 179.0f))
				resetAccumulation = true;

			if (ImGui::DragFloat("Focus Distance", &m_camera.focusDistance, 0.1f, 0.1f, 1000.0f))
				resetAccumulation = true;

			if(ImGui::DragFloat("Defocus Angle", &m_camera.defocusAngle, 0.1f, 0.0f, 90.0f))
				resetAccumulation = true;

			ImGui::DragFloat("Camera Speed", &cameraController.m_fMoveSpeed, 0.1f, 1.0f, 100.0f, "%.1f");

			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::SeparatorText("Render Settings");
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			if(ImGui::DragInt("Rays Per Pixel", &m_pushConstants.raysPerPixel, 1, 1, 100))
				resetAccumulation = true;

			if (ImGui::DragInt("Max Bounces", &m_pushConstants.maxBounces, 1, 1, 100))
				resetAccumulation = true;

			static bool accumlateFrames = true;
			if(ImGui::Checkbox("Accumulate Frames", &accumlateFrames))
			{
				resetAccumulation = true;
			}
			m_pushConstants.accumulateFrames = accumlateFrames;

			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			static float sunlightColour[3] = { m_pushConstants.sunColour[0], m_pushConstants.sunColour[1], m_pushConstants.sunColour[2] };
			if (ImGui::DragFloat3("Sunlight Colour", sunlightColour, 0.01f, 0.0f, 1.0f))
				resetAccumulation = true;

			m_pushConstants.sunColour = glm::vec3(sunlightColour[0], sunlightColour[1], sunlightColour[2]);

			static float sunlightDirection[3] = { m_pushConstants.sunDirection[0], m_pushConstants.sunDirection[1], m_pushConstants.sunDirection[2] };
			if(ImGui::DragFloat3("Sunlight Direction", sunlightDirection, 0.01f, -1.0f, 1.0f))
				resetAccumulation = true;

			m_pushConstants.sunDirection = glm::normalize(glm::vec3(sunlightDirection[0], sunlightDirection[1], sunlightDirection[2]));

			if (ImGui::DragFloat("Sunlight Intensity", &m_pushConstants.sunIntensity, 0.01f, 0.0f, 10.0f))
				resetAccumulation = true;

			m_bRefreshAccumulation = m_bRefreshAccumulation || resetAccumulation;

			ImGui::End();
		}

		{
			ImGui::Begin("Performance Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			
			ImGui::Text("Frame Time: %.2f ms", m_performanceStats.GetPerformanceMeasurement("Frame")->GetPerformanceMeasurementInMilliseconds());
			ImGui::Text("FPS: %.1f", m_performanceStats.GetFPS());
			ImGui::Text("AVG FPS: %.1f", m_performanceStats.GetAvgFPS());

			ImGui::End();
		}

		m_pWindow->CheckScreenSizeForUpdate(this);
		RenderFrame();

		m_pushConstants.frame++;

		m_inputManager.ClearFrameInputs();
		m_performanceStats.EndPerformanceMeasurement("Frame");
		m_performanceStats.UpdatePerformanceStats();
	}
}

void HardwareRenderer::InitializeRenderer()
{
	m_inputManager.SetQuitFunction([this]() { this->Quit(); });

	InitializeVulkan();
	InitializeScene();

	MainLoop();
}

void HardwareRenderer::LoadModel(const std::string& filePath)
{
	if(filePath.substr(filePath.find_last_of(".") + 1) != "obj")
	{
		throw std::exception("Only .obj files are supported.");
	}

	if(m_models.find(filePath) != m_models.end())
	{
		return;
	}

	if(FileExists(filePath) == false)
	{
		std::string error = filePath + " does not exist.";
		throw std::exception(error.c_str());
	}

	Model newModel;
	GPUAABB modelAABB;
	newModel.triangleStartIndex = m_sceneTriangles.size();

	std::vector<Vertex> vertices;

	LoadObjFile(filePath, vertices);

	float minX = INT_MAX;
	float minY = INT_MAX;
	float minZ = INT_MAX;
	float maxX = INT_MIN;
	float maxY = INT_MIN;
	float maxZ = INT_MIN;

	int triangleCount = vertices.size() / 3;
	for(int i = 0; i < vertices.size(); i += 3)
	{
		GPUTriangle newTriangle;
		newTriangle.v0 = vertices[i].m_position;
		newTriangle.v1 = vertices[i + 1].m_position;
		newTriangle.v2 = vertices[i + 2].m_position;

		if(vertices[i].m_normal != glm::vec3(0.0f) && vertices[i + 1].m_normal != glm::vec3(0.0f) && vertices[i + 2].m_normal != glm::vec3(0.0f))
		{
			newTriangle.n0 = glm::normalize(vertices[i].m_normal);
			newTriangle.n1 = glm::normalize(vertices[i + 1].m_normal);
			newTriangle.n2 = glm::normalize(vertices[i + 2].m_normal);
		}
		else
		{
			glm::vec3 edge1 = newTriangle.v1 - newTriangle.v0;
			glm::vec3 edge2 = newTriangle.v2 - newTriangle.v0;
			glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

			newTriangle.n0 = faceNormal;
			newTriangle.n1 = faceNormal;
			newTriangle.n2 = faceNormal;
		}

		//Check all current bound values and update if needed
		if(newTriangle.v0.x < minX)
			minX = static_cast<int>(std::floor(newTriangle.v0.x));
		if (newTriangle.v0.y < minY)
			minY = static_cast<int>(std::floor(newTriangle.v0.y));
		if (newTriangle.v0.z < minZ)
			minZ = static_cast<int>(std::floor(newTriangle.v0.z));
		if (newTriangle.v0.x > maxX)
			maxX = static_cast<int>(std::ceil(newTriangle.v0.x));
		if (newTriangle.v0.y > maxY)
			maxY = static_cast<int>(std::ceil(newTriangle.v0.y));
		if (newTriangle.v0.z > maxZ)
			maxZ = static_cast<int>(std::ceil(newTriangle.v0.z));

		if (newTriangle.v1.x < minX)
			minX = static_cast<int>(std::floor(newTriangle.v1.x));
		if (newTriangle.v1.y < minY)
			minY = static_cast<int>(std::floor(newTriangle.v1.y));
		if (newTriangle.v1.z < minZ)
			minZ = static_cast<int>(std::floor(newTriangle.v1.z));
		if (newTriangle.v1.x > maxX)
			maxX = static_cast<int>(std::ceil(newTriangle.v1.x));
		if (newTriangle.v1.y > maxY)
			maxY = static_cast<int>(std::ceil(newTriangle.v1.y));
		if (newTriangle.v1.z > maxZ)
			maxZ = static_cast<int>(std::ceil(newTriangle.v1.z));

		if (newTriangle.v2.x < minX)
			minX = static_cast<int>(std::floor(newTriangle.v2.x));
		if (newTriangle.v2.y < minY)
			minY = static_cast<int>(std::floor(newTriangle.v2.y));
		if (newTriangle.v2.z < minZ)
			minZ = static_cast<int>(std::floor(newTriangle.v2.z));
		if (newTriangle.v2.x > maxX)
			maxX = static_cast<int>(std::ceil(newTriangle.v2.x));
		if (newTriangle.v2.y > maxY)
			maxY = static_cast<int>(std::ceil(newTriangle.v2.y));
		if (newTriangle.v2.z > maxZ)
			maxZ = static_cast<int>(std::ceil(newTriangle.v2.z));

		m_sceneTriangles.push_back(newTriangle);
	}

	//check the difference in the axis and make sure they are not 0
	float xDiff = maxX - minX;
	float yDiff = maxY - minY;
	float zDiff = maxZ - minZ;
	if (xDiff == 0)
	{
		maxX += 1;
		minX -= 1;
	}
	if (yDiff == 0)
	{
		maxY += 1;
		minY -= 1;
	}
	if (zDiff == 0)
	{
		maxZ += 1;
		minZ -= 1;
	}

	modelAABB.min = glm::vec3(minX, minY, minZ);
	modelAABB.max = glm::vec3(maxX, maxY, maxZ);

	newModel.triangleCount = triangleCount;
	newModel.boundingBox = modelAABB;

	m_models[filePath] = newModel;
}

void HardwareRenderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	if (vkResetFences(m_device, 1, &m_immediateFence) != VK_SUCCESS)
		throw std::exception("Failed to reset immediate fence.");
	if (vkResetCommandBuffer(m_immediateCommandBuffer, 0) != VK_SUCCESS)
		throw std::exception("Failed to reset immediate command buffer.");

	VkCommandBuffer cmd = m_immediateCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	if (vkBeginCommandBuffer(cmd, &cmdBeginInfo) != VK_SUCCESS)
		throw std::exception("Failed to begin immediate command buffer.");

	function(cmd);

	if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
		throw std::exception("Failed to end immediate command buffer.");

	VkCommandBufferSubmitInfo cmdInfo = CommandBufferSubmitInfo(cmd);
	VkSubmitInfo2 submit = SubmitInfo(&cmdInfo, nullptr, nullptr);

	if (vkQueueSubmit2(m_graphicsQueue, 1, &submit, m_immediateFence) != VK_SUCCESS)
		throw std::exception("Failed to submit immediate queue.");

	if (vkWaitForFences(m_device, 1, &m_immediateFence, true, 99999999) != VK_SUCCESS)
		throw std::exception("Failed to wait for immediate fence.");
}

AllocatedBuffer HardwareRenderer::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, std::string allocationName)
{
	VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = memoryUsage;
	vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
	AllocatedBuffer newBuffer;

	if (vmaCreateBuffer(m_allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.m_buffer, &newBuffer.m_allocation, &newBuffer.m_info) != VK_SUCCESS)
		throw std::exception("Failed to create buffer.");

	newBuffer.m_allocation->SetName(m_allocator, allocationName.c_str());

	return newBuffer;
}

void HardwareRenderer::DestroyBuffer(const AllocatedBuffer& buffer)
{
	vmaDestroyBuffer(m_allocator, buffer.m_buffer, buffer.m_allocation);
}