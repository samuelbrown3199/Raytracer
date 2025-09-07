#include "HardwareRenderer.h"

#include <numeric>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "../../Useful/Useful.h"
#include "Vulkan/VulkanImages.h"
#include "Vulkan/VulkanInitialisers.h"
#include "Vulkan/VulkanPipelines.h"
#include "Imgui/backends/imgui_impl_sdl3.h"
#include "Imgui/backends/imgui_impl_vulkan.h"
#include "Imgui/implot.h"

#include "../ModelLoader.h"

#include "../../Useful/Useful.h"
#include "../../Interface/RaytracerSettingsUI.hpp"
#include "../../Interface/PerformanceStatsUI.hpp"
#include "../../Interface/SceneEditorUI.hpp"

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
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 50 }
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
		builder.AddBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(10, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(11, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(12, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(13, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(14, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(15, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(16, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.AddBinding(17, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
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

	AABB objectAABB = m_models[obj.modelName].parentBVH.node.aabb;
	glm::vec3 aabbMin = objectAABB.min;
	glm::vec3 aabbMax = objectAABB.max;

	glm::vec3 corners[8] = {
		{aabbMin.x, aabbMin.y, aabbMin.z},
		{aabbMax.x, aabbMin.y, aabbMin.z},
		{aabbMin.x, aabbMax.y, aabbMin.z},
		{aabbMax.x, aabbMax.y, aabbMin.z},
		{aabbMin.x, aabbMin.y, aabbMax.z},
		{aabbMax.x, aabbMin.y, aabbMax.z},
		{aabbMin.x, aabbMax.y, aabbMax.z},
		{aabbMax.x, aabbMax.y, aabbMax.z}
	};

	glm::vec3 newMin(FLT_MAX), newMax(-FLT_MAX);
	for (int i = 0; i < 8; ++i) {
		glm::vec3 transformed = glm::vec3(objectMat * glm::vec4(corners[i], 1.0f));
		newMin = glm::min(newMin, transformed);
		newMax = glm::max(newMax, transformed);
	}

	objectAABB.min = newMin;
	objectAABB.max = newMax;

	gpuObject.inverseTransform = glm::inverse(objectMat);
	gpuObject.materialIndex = obj.materialIndex;

	gpuObject.triangleStartIndex = m_models[obj.modelName].triangleStartIndex;
	gpuObject.triangleCount = m_models[obj.modelName].triangleCount;


	ParentBVHNode parentNode = m_models[obj.modelName].parentBVH;
	parentNode.node.aabb = objectAABB;
	parentNode.objectIndex = m_gpuSceneObjects.size();

	m_gpuSceneObjects.push_back(gpuObject);
	m_parentBVH.push_back(parentNode);
}

void HardwareRenderer::InitializeUIs()
{
	std::shared_ptr<RaytracerSettingsUI> raytraceSettings = std::make_shared<RaytracerSettingsUI>();
	raytraceSettings->SetRenderer(this);
	raytraceSettings->SetPushConstants(&m_pushConstants);
	raytraceSettings->SetCameraSettings(&m_camera);
	raytraceSettings->SetCameraController(&m_cameraController);

	m_toolUIs.emplace(std::make_pair("RaytracerSettings", raytraceSettings));


	std::shared_ptr<PerformanceStatsUI> performanceStats = std::make_shared<PerformanceStatsUI>();
	performanceStats->SetPerformanceStats(&m_performanceStats);
	m_toolUIs.emplace(std::make_pair("PerformanceStats", performanceStats));


	std::shared_ptr<SceneEditorUI> sceneEditor = std::make_shared<SceneEditorUI>();
	sceneEditor->SetRenderer(this);
	m_toolUIs.emplace(std::make_pair("SceneEditor", sceneEditor));
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

	GPUMaterial dullGoldMaterial;
	dullGoldMaterial.albedo = glm::vec3(1.0, 0.71, 0.29);
	dullGoldMaterial.smoothness = 0.8f;
	dullGoldMaterial.fuzziness = 0.4f;
	dullGoldMaterial.emission = 0.0f;
	m_sceneMaterials.push_back(dullGoldMaterial);

	GPUMaterial shinyGoldMaterial;
	shinyGoldMaterial.albedo = glm::vec3(1.0, 0.71, 0.29);
	shinyGoldMaterial.smoothness = 1.0f;
	shinyGoldMaterial.fuzziness = 0.0f;
	shinyGoldMaterial.emission = 0.0f;
	m_sceneMaterials.push_back(shinyGoldMaterial);

	GPUMaterial redMaterial;
	redMaterial.albedo = glm::vec3(0.8, 0.1, 0.1);
	redMaterial.emission = 0.0f;
	m_sceneMaterials.push_back(redMaterial);

	GPUMaterial greenMaterial;
	greenMaterial.albedo = glm::vec3(0.1, 0.8, 0.1);
	greenMaterial.emission = 0.0f;
	m_sceneMaterials.push_back(greenMaterial);

	std::string quadModelPath = GetWorkingDirectory() + "\\Resources\\Models\\flat_quad.obj";
	LoadModel(quadModelPath);

	std::string spherePath = GetWorkingDirectory() + "\\Resources\\Models\\icosphere.obj";
	LoadModel(spherePath);

	std::string dragonPath = GetWorkingDirectory() + "\\Resources\\Models\\dragon.obj";
	LoadModel(dragonPath);

	AddSceneObject(spherePath, glm::vec3(0, 10, 0), glm::vec3(0, 0, 0), glm::vec3(2, 2, 2), emissiveMaterial);
	AddSceneObject(dragonPath, glm::vec3(0, 2, 0), glm::vec3(0, 195, 0), glm::vec3(1, 1, 1), dullGoldMaterial);

	AddSceneObject(quadModelPath, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(5, 5, 5), groundMaterial);
	AddSceneObject(quadModelPath, glm::vec3(0, 10, 0), glm::vec3(0, 0, 180), glm::vec3(5, 5, 5), groundMaterial);
	AddSceneObject(quadModelPath, glm::vec3(0, 5, -5), glm::vec3(90, 0, 0), glm::vec3(5, 5, 5), groundMaterial);
	AddSceneObject(quadModelPath, glm::vec3(-5, 5, 0), glm::vec3(0, 0, 90), glm::vec3(5, 5, 5), redMaterial);
	AddSceneObject(quadModelPath, glm::vec3(5, 5, 0), glm::vec3(0, 0, -90), glm::vec3(5, 5, 5), greenMaterial);
	AddSceneObject(quadModelPath, glm::vec3(0, 5, 5), glm::vec3(-90, 0, 0), glm::vec3(5, 5, 5), groundMaterial);

	BufferSceneData();
}

void HardwareRenderer::BufferSceneData()
{
	m_gpuSceneObjects.clear();
	for (const auto& obj : m_sceneObjects)
	{
		ConvertSceneObjectToGPUObject(obj);
	}

	void* data;

	m_parentBVHBuffer = CreateBuffer(sizeof(ParentBVHNode) * m_parentBVH.size()+1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "SceneAABBBuffer");
	vmaMapMemory(m_allocator, m_parentBVHBuffer.m_allocation, &data);
	memcpy(data, m_parentBVH.data(), sizeof(ParentBVHNode) * m_parentBVH.size());
	vmaUnmapMemory(m_allocator, m_parentBVHBuffer.m_allocation);

	m_sceneObjectBuffer = CreateBuffer(sizeof(GPUObject) * m_gpuSceneObjects.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "SceneObjectBuffer");
	vmaMapMemory(m_allocator, m_sceneObjectBuffer.m_allocation, &data);
	memcpy(data, m_gpuSceneObjects.data(), sizeof(GPUObject) * m_gpuSceneObjects.size());
	vmaUnmapMemory(m_allocator, m_sceneObjectBuffer.m_allocation);

	m_sceneMaterialBuffer = CreateBuffer(sizeof(GPUMaterial) * m_sceneMaterials.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "SceneMaterialBuffer");
	vmaMapMemory(m_allocator, m_sceneMaterialBuffer.m_allocation, &data);
	memcpy(data, m_sceneMaterials.data(), sizeof(GPUMaterial) * m_sceneMaterials.size());
	vmaUnmapMemory(m_allocator, m_sceneMaterialBuffer.m_allocation);

	m_triangleV0Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleV0s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleV0sBuffer");
	vmaMapMemory(m_allocator, m_triangleV0Buffer.m_allocation, &data);
	memcpy(data, m_triangleV0s.data(), sizeof(glm::vec4) * m_triangleV0s.size());
	vmaUnmapMemory(m_allocator, m_triangleV0Buffer.m_allocation);

	m_triangleV1Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleV1s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleV1sBuffer");
	vmaMapMemory(m_allocator, m_triangleV1Buffer.m_allocation, &data);
	memcpy(data, m_triangleV1s.data(), sizeof(glm::vec4) * m_triangleV1s.size());
	vmaUnmapMemory(m_allocator, m_triangleV1Buffer.m_allocation);

	m_triangleV2Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleV2s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleV2sBuffer");
	vmaMapMemory(m_allocator, m_triangleV2Buffer.m_allocation, &data);
	memcpy(data, m_triangleV2s.data(), sizeof(glm::vec4) * m_triangleV2s.size());
	vmaUnmapMemory(m_allocator, m_triangleV2Buffer.m_allocation);

	m_triangleN0Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleN0s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleN0sBuffer");
	vmaMapMemory(m_allocator, m_triangleN0Buffer.m_allocation, &data);
	memcpy(data, m_triangleN0s.data(), sizeof(glm::vec4) * m_triangleN0s.size());
	vmaUnmapMemory(m_allocator, m_triangleN0Buffer.m_allocation);

	m_triangleN1Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleN1s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleN1sBuffer");
	vmaMapMemory(m_allocator, m_triangleN1Buffer.m_allocation, &data);
	memcpy(data, m_triangleN1s.data(), sizeof(glm::vec4) * m_triangleN1s.size());
	vmaUnmapMemory(m_allocator, m_triangleN1Buffer.m_allocation);

	m_triangleN2Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleN2s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleN2sBuffer");
	vmaMapMemory(m_allocator, m_triangleN2Buffer.m_allocation, &data);
	memcpy(data, m_triangleN2s.data(), sizeof(glm::vec4) * m_triangleN2s.size());
	vmaUnmapMemory(m_allocator, m_triangleN2Buffer.m_allocation);

	m_triangleUV0Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleUV0s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleUV0sBuffer");
	vmaMapMemory(m_allocator, m_triangleUV0Buffer.m_allocation, &data);
	memcpy(data, m_triangleUV0s.data(), sizeof(glm::vec4) * m_triangleUV0s.size());
	vmaUnmapMemory(m_allocator, m_triangleUV0Buffer.m_allocation);

	m_triangleUV1Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleUV1s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleUV1sBuffer");
	vmaMapMemory(m_allocator, m_triangleUV1Buffer.m_allocation, &data);
	memcpy(data, m_triangleUV1s.data(), sizeof(glm::vec4) * m_triangleUV1s.size());
	vmaUnmapMemory(m_allocator, m_triangleUV1Buffer.m_allocation);

	m_triangleUV2Buffer = CreateBuffer(sizeof(glm::vec4) * m_triangleUV2s.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "TriangleUV2sBuffer");	
	vmaMapMemory(m_allocator, m_triangleUV2Buffer.m_allocation, &data);
	memcpy(data, m_triangleUV2s.data(), sizeof(glm::vec4) * m_triangleUV2s.size());
	vmaUnmapMemory(m_allocator, m_triangleUV2Buffer.m_allocation);

	m_aabbMinBuffer = CreateBuffer(sizeof(glm::vec4) * m_aabbMins.size()+1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "AABBMinBuffer");
	vmaMapMemory(m_allocator, m_aabbMinBuffer.m_allocation, &data);
	memcpy(data, m_aabbMins.data(), sizeof(glm::vec4) * m_aabbMins.size());
	vmaUnmapMemory(m_allocator, m_aabbMinBuffer.m_allocation);

	m_aabbMaxBuffer = CreateBuffer(sizeof(glm::vec4) * m_aabbMaxs.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "AABBMaxBuffer");
	vmaMapMemory(m_allocator, m_aabbMaxBuffer.m_allocation, &data);
	memcpy(data, m_aabbMaxs.data(), sizeof(glm::vec4) * m_aabbMaxs.size());
	vmaUnmapMemory(m_allocator, m_aabbMaxBuffer.m_allocation);

	m_bvhLeftChildrenBuffer = CreateBuffer(sizeof(int) * m_bvhLeftChildren.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "BVHLeftChildrenBuffer");
	vmaMapMemory(m_allocator, m_bvhLeftChildrenBuffer.m_allocation, &data);
	memcpy(data, m_bvhLeftChildren.data(), sizeof(int) * m_bvhLeftChildren.size());
	vmaUnmapMemory(m_allocator, m_bvhLeftChildrenBuffer.m_allocation);

	m_bvhRightChildrenBuffer = CreateBuffer(sizeof(int) * m_bvhRightChildren.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "BVHRightChildrenBuffer");
	vmaMapMemory(m_allocator, m_bvhRightChildrenBuffer.m_allocation, &data);
	memcpy(data, m_bvhRightChildren.data(), sizeof(int) * m_bvhRightChildren.size());
	vmaUnmapMemory(m_allocator, m_bvhRightChildrenBuffer.m_allocation);

	m_bvhTriangleStartIndicesBuffer = CreateBuffer(sizeof(int) * m_bvhTriangleStartIndices.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "BVHTriangleStartIndicesBuffer");
	vmaMapMemory(m_allocator, m_bvhTriangleStartIndicesBuffer.m_allocation, &data);
	memcpy(data, m_bvhTriangleStartIndices.data(), sizeof(int) * m_bvhTriangleStartIndices.size());
	vmaUnmapMemory(m_allocator, m_bvhTriangleStartIndicesBuffer.m_allocation);

	m_bvhTriangleCountsBuffer = CreateBuffer(sizeof(int) * m_bvhTriangleCounts.size() + 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "BVHTriangleCountsBuffer");
	vmaMapMemory(m_allocator, m_bvhTriangleCountsBuffer.m_allocation, &data);
	memcpy(data, m_bvhTriangleCounts.data(), sizeof(int) * m_bvhTriangleCounts.size());
	vmaUnmapMemory(m_allocator, m_bvhTriangleCountsBuffer.m_allocation);

	DescriptorWriter writer;
	writer.WriteBuffer(0, m_parentBVHBuffer.m_buffer, sizeof(ParentBVHNode) * m_parentBVH.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(1, m_sceneObjectBuffer.m_buffer, sizeof(GPUObject) * m_gpuSceneObjects.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(2, m_sceneMaterialBuffer.m_buffer, sizeof(GPUMaterial) * m_sceneMaterials.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(3, m_triangleV0Buffer.m_buffer, sizeof(glm::vec4) * m_triangleV0s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(4, m_triangleV1Buffer.m_buffer, sizeof(glm::vec4) * m_triangleV1s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(5, m_triangleV2Buffer.m_buffer, sizeof(glm::vec4) * m_triangleV2s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(6, m_triangleN0Buffer.m_buffer, sizeof(glm::vec4) * m_triangleN0s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(7, m_triangleN1Buffer.m_buffer, sizeof(glm::vec4) * m_triangleN1s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(8, m_triangleN2Buffer.m_buffer, sizeof(glm::vec4) * m_triangleN2s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(9, m_triangleUV0Buffer.m_buffer, sizeof(glm::vec4) * m_triangleUV0s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(10, m_triangleUV1Buffer.m_buffer, sizeof(glm::vec4) * m_triangleUV1s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(11, m_triangleUV2Buffer.m_buffer, sizeof(glm::vec4) * m_triangleUV2s.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(12, m_aabbMinBuffer.m_buffer, sizeof(glm::vec4)* m_aabbMins.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(13, m_aabbMaxBuffer.m_buffer, sizeof(glm::vec4)* m_aabbMaxs.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(14, m_bvhLeftChildrenBuffer.m_buffer, sizeof(int)* m_bvhLeftChildren.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(15, m_bvhRightChildrenBuffer.m_buffer, sizeof(int)* m_bvhRightChildren.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(16, m_bvhTriangleStartIndicesBuffer.m_buffer, sizeof(int)* m_bvhTriangleStartIndices.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.WriteBuffer(17, m_bvhTriangleCountsBuffer.m_buffer, sizeof(int)* m_bvhTriangleCounts.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.UpdateSet(m_device, m_sceneDescriptor);
}

void HardwareRenderer::ClearSceneData()
{
	DestroyBuffer(m_parentBVHBuffer);
	//DestroyBuffer(m_childBVHBuffer);
	DestroyBuffer(m_sceneObjectBuffer);
	DestroyBuffer(m_sceneMaterialBuffer);

	m_parentBVH.clear();
	m_gpuSceneObjects.clear();
	//m_sceneTriangles.clear();
	//m_sceneObjects.clear();
	//m_sceneMaterials.clear();
}

void HardwareRenderer::RebufferSceneData()
{
	ClearSceneData();
	BufferSceneData();
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
	m_pushConstants.parentBVHCount = m_parentBVH.size();

	std::vector<VkDescriptorSet> sets;
	sets.push_back(m_drawImageDescriptors);
	sets.push_back(m_sceneDescriptor);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_raytracePipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_raytracePipelineLayout, 0, sets.size(), sets.data(), 0, nullptr);

	vkCmdPushConstants(cmd, m_raytracePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(RaytracePushConstants), &m_pushConstants);

	uint32_t groupCountX = (m_drawExtent.width + 15) / 16;
	uint32_t groupCountY = (m_drawExtent.height + 15) / 16;
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

	while (m_bRun)
	{
		m_performanceStats.StartPerformanceMeasurement("Frame");

		m_inputManager.HandleGeneralInput();
		if (m_pWindow->IsWindowMinimized())
			continue;

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		m_cameraController.Update(this);

		DoInterfaceControls();

		for (auto& toolUI : m_toolUIs)
		{
			if (toolUI.second->m_uiOpen)
				toolUI.second->DoInterface();
		}

		m_pWindow->CheckScreenSizeForUpdate(this);
		RenderFrame();

		m_pushConstants.frame++;

		if(m_bDoRender)
			ProduceRender();

		m_inputManager.ClearFrameInputs();
		m_performanceStats.EndPerformanceMeasurement("Frame");
		m_performanceStats.UpdatePerformanceStats();
	}
}

void HardwareRenderer::DoInterfaceControls()
{
	if (m_inputManager.GetKeyDown(SDLK_F1))
		ToggleUI("RaytracerSettings");

	if (m_inputManager.GetKeyDown(SDLK_F2))
		ToggleUI("PerformanceStats");

	if (m_inputManager.GetKeyDown(SDLK_F3))
		ToggleUI("SceneEditor");
}

void HardwareRenderer::ToggleUI(const std::string& uiName)
{
	m_toolUIs.at(uiName)->m_uiOpen = !m_toolUIs.at(uiName)->m_uiOpen;
}

void HardwareRenderer::ProduceRender()
{
	std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

	float renderPercentage = 0.0f;
	float percentageStep = 100.0f / m_iRenderFrames;

	int previousPercentage = 1;

	for(int i = m_pushConstants.frame; i < m_iRenderFrames; ++i)
	{
		RenderFrame();
		m_pushConstants.frame++;

		renderPercentage = (i+1) * percentageStep;
		int rounded = glm::floor(renderPercentage);

		if (rounded != previousPercentage)
		{
			std::cout << "\rRendering: " << (int)renderPercentage << "%   " << std::flush;
			previousPercentage = rounded;
		}
	}
	std::cout << std::endl;

	WriteDrawImageToFile();
	m_bDoRender = false;

	std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsedSeconds = endTime - startTime;
	std::cout << "Render took " << elapsedSeconds.count() << " seconds." << std::endl;
}

void HardwareRenderer::WriteDrawImageToFile()
{
	const uint32_t width = m_drawImage.m_imageExtent.width;
	const uint32_t height = m_drawImage.m_imageExtent.height;
	const size_t pixelCount = size_t(width) * size_t(height);

	// CORRECT: 4 floats per pixel (R32G32B32A32_SFLOAT)
	const VkDeviceSize floatPixelSize = sizeof(float) * 4;
	const VkDeviceSize bufferSize = floatPixelSize * pixelCount; // <-- no extra *4

	AllocatedBuffer stagingBuffer = CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_TO_CPU,
		"StagingBuffer"
	);

	{
		std::lock_guard<std::mutex> lock(m_immediateSubmitMutex);
		ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				TransitionImage(cmd, m_drawImage.m_image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				CopyImageToBuffer(cmd, m_drawImage.m_image, stagingBuffer.m_buffer, m_drawImage.m_imageExtent);
				TransitionImage(cmd, m_drawImage.m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
			});
	}

	// Ensure GPU writes are visible to CPU if memory is non-coherent
	vmaInvalidateAllocation(m_allocator, stagingBuffer.m_allocation, 0, VK_WHOLE_SIZE);

	void* mappedData = nullptr;
	vmaMapMemory(m_allocator, stagingBuffer.m_allocation, &mappedData);
	const float* floatPixels = reinterpret_cast<const float*>(mappedData);

	std::vector<uint8_t> imageData(pixelCount * 4);

	for (size_t i = 0; i < pixelCount; ++i)
	{
		float r = floatPixels[i * 4 + 0];
		float g = floatPixels[i * 4 + 1];
		float b = floatPixels[i * 4 + 2];
		float a = floatPixels[i * 4 + 3];

		// clamp to [0,1]
		r = std::min(std::max(r, 0.0f), 1.0f);
		g = std::min(std::max(g, 0.0f), 1.0f);
		b = std::min(std::max(b, 0.0f), 1.0f);
		a = std::min(std::max(a, 0.0f), 1.0f);

		// convert to 0..255 with rounding
		imageData[i * 4 + 0] = static_cast<uint8_t>(r * 255.0f + 0.5f);
		imageData[i * 4 + 1] = static_cast<uint8_t>(g * 255.0f + 0.5f);
		imageData[i * 4 + 2] = static_cast<uint8_t>(b * 255.0f + 0.5f);

		// OPTION A: Force opaque so you can see the content in viewers
		imageData[i * 4 + 3] = 255;

		// OPTION B (if you want to keep real alpha): use premultiplied alpha,
		// imageData[i*4 + 0] = static_cast<uint8_t>(r * a * 255.0f + 0.5f);
		// imageData[i*4 + 1] = static_cast<uint8_t>(g * a * 255.0f + 0.5f);
		// imageData[i*4 + 2] = static_cast<uint8_t>(b * a * 255.0f + 0.5f);
		// imageData[i*4 + 3] = static_cast<uint8_t>(a * 255.0f + 0.5f);
	}

	vmaUnmapMemory(m_allocator, stagingBuffer.m_allocation);

	CreateNewDirectory("Renders");
	std::string fileName = "Renders\\render_" + GetDateTimeString() + ".png";

	stbi_write_png(
		fileName.c_str(),
		static_cast<int>(width),
		static_cast<int>(height),
		4,
		imageData.data(),
		static_cast<int>(width) * 4
	);

	vmaDestroyBuffer(m_allocator, stagingBuffer.m_buffer, stagingBuffer.m_allocation);

	std::cout << "Wrote render to " << fileName << std::endl;
}

void HardwareRenderer::InitializeRenderer()
{
	m_inputManager.SetQuitFunction([this]() { this->Quit(); });

	InitializeVulkan();
	InitializeUIs();
	InitializeScene();

	MainLoop();
}

void HardwareRenderer::LoadModel(const std::string& filePath)
{
	if (filePath.substr(filePath.find_last_of(".") + 1) != "obj")
	{
		throw std::exception("Only .obj files are supported.");
	}

	if (m_models.find(filePath) != m_models.end())
	{
		return;
	}

	if (FileExists(filePath) == false)
	{
		std::string error = filePath + " does not exist.";
		throw std::exception(error.c_str());
	}

	Model newModel;
	AABB modelAABB;
	newModel.triangleStartIndex = static_cast<int>(m_triangleV0s.size());

	std::vector<Vertex> vertices;
	std::vector<Triangle> modelTriangles;

	LoadObjFile(filePath, vertices);

	float minX = INT_MAX;
	float minY = INT_MAX;
	float minZ = INT_MAX;
	float maxX = INT_MIN;
	float maxY = INT_MIN;
	float maxZ = INT_MIN;

	int triangleCount = static_cast<int>(vertices.size() / 3);
	for (int i = 0; i < (int)vertices.size(); i += 3)
	{
		Triangle newTriangle;
		newTriangle.v0 = vertices[i].m_position;
		newTriangle.v1 = vertices[i + 1].m_position;
		newTriangle.v2 = vertices[i + 2].m_position;

		newTriangle.triCentroid = (newTriangle.v0 + newTriangle.v1 + newTriangle.v2) * 0.33f;

		if (vertices[i].m_normal != glm::vec3(0.0f) &&
			vertices[i + 1].m_normal != glm::vec3(0.0f) &&
			vertices[i + 2].m_normal != glm::vec3(0.0f))
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

		// update bounding ints
		if (newTriangle.v0.x < minX) minX = static_cast<int>(std::floor(newTriangle.v0.x));
		if (newTriangle.v0.y < minY) minY = static_cast<int>(std::floor(newTriangle.v0.y));
		if (newTriangle.v0.z < minZ) minZ = static_cast<int>(std::floor(newTriangle.v0.z));
		if (newTriangle.v0.x > maxX) maxX = static_cast<int>(std::ceil(newTriangle.v0.x));
		if (newTriangle.v0.y > maxY) maxY = static_cast<int>(std::ceil(newTriangle.v0.y));
		if (newTriangle.v0.z > maxZ) maxZ = static_cast<int>(std::ceil(newTriangle.v0.z));

		if (newTriangle.v1.x < minX) minX = static_cast<int>(std::floor(newTriangle.v1.x));
		if (newTriangle.v1.y < minY) minY = static_cast<int>(std::floor(newTriangle.v1.y));
		if (newTriangle.v1.z < minZ) minZ = static_cast<int>(std::floor(newTriangle.v1.z));
		if (newTriangle.v1.x > maxX) maxX = static_cast<int>(std::ceil(newTriangle.v1.x));
		if (newTriangle.v1.y > maxY) maxY = static_cast<int>(std::ceil(newTriangle.v1.y));
		if (newTriangle.v1.z > maxZ) maxZ = static_cast<int>(std::ceil(newTriangle.v1.z));

		if (newTriangle.v2.x < minX) minX = static_cast<int>(std::floor(newTriangle.v2.x));
		if (newTriangle.v2.y < minY) minY = static_cast<int>(std::floor(newTriangle.v2.y));
		if (newTriangle.v2.z < minZ) minZ = static_cast<int>(std::floor(newTriangle.v2.z));
		if (newTriangle.v2.x > maxX) maxX = static_cast<int>(std::ceil(newTriangle.v2.x));
		if (newTriangle.v2.y > maxY) maxY = static_cast<int>(std::ceil(newTriangle.v2.y));
		if (newTriangle.v2.z > maxZ) maxZ = static_cast<int>(std::ceil(newTriangle.v2.z));

		modelTriangles.push_back(newTriangle);
	}

	// ensure non-zero extents
	float xDiff = maxX - minX;
	float yDiff = maxY - minY;
	float zDiff = maxZ - minZ;
	if (xDiff == 0) { maxX += 1; minX -= 1; }
	if (yDiff == 0) { maxY += 1; minY -= 1; }
	if (zDiff == 0) { maxZ += 1; minZ -= 1; }

	modelAABB.min = glm::vec3(minX, minY, minZ);
	modelAABB.max = glm::vec3(maxX, maxY, maxZ);

	ParentBVHNode parentNode;
	parentNode.node.aabb = modelAABB;
	parentNode.objectIndex = -1;
	parentNode.node.leftChild = -1;
	parentNode.node.rightChild = -1;
	parentNode.node.triangleStartIndex = 0;
	parentNode.node.triangleCount = modelTriangles.size();

	newModel.parentBVH = parentNode;

	std::vector<BVHNode> modelBVH;
	if (!modelTriangles.empty())
		BuildBVH(modelTriangles, modelBVH, parentNode, m_aabbMins.size());
	
	newModel.triangleStartIndex = m_triangleV0s.size();
	newModel.triangleCount = modelTriangles.size();

	// Now convert modelBVH node local starts into global indices and update child indices
	int bvhGlobalOffset = static_cast<int>(m_aabbMins.size());
	int triangleGlobalOffset = static_cast<int>(m_triangleV0s.size());
	for (auto& node : modelBVH)
	{
		node.leftChild = node.leftChild != -1 ? node.leftChild + bvhGlobalOffset : -1;
		node.rightChild = node.rightChild != -1 ? node.rightChild + bvhGlobalOffset : -1;

		node.triangleStartIndex += triangleGlobalOffset;

		m_aabbMins.push_back(glm::vec4(node.aabb.min, 0.0f));
		m_aabbMaxs.push_back(glm::vec4(node.aabb.max, 0.0f));

		m_bvhLeftChildren.push_back(node.leftChild);
		m_bvhRightChildren.push_back(node.rightChild);

		m_bvhTriangleStartIndices.push_back(node.triangleStartIndex);
		m_bvhTriangleCounts.push_back(node.triangleCount);
	}

	parentNode.node.triangleStartIndex += triangleGlobalOffset;

	//Append reordered triangles to global list
	for (auto& tri : modelTriangles)
	{
		m_triangleV0s.push_back(glm::vec4(tri.v0, 0.0));
		m_triangleV1s.push_back(glm::vec4(tri.v1, 0.0));
		m_triangleV2s.push_back(glm::vec4(tri.v2, 0.0));

		m_triangleN0s.push_back(glm::vec4(tri.n0, 0.0));
		m_triangleN1s.push_back(glm::vec4(tri.n1, 0.0));
		m_triangleN2s.push_back(glm::vec4(tri.n2, 0.0));

		//TODO, support UVs
		m_triangleUV0s.push_back(glm::vec4(0.0f));
		m_triangleUV1s.push_back(glm::vec4(0.0f));
		m_triangleUV2s.push_back(glm::vec4(0.0f));
	}

	// store parent BVH and model
	newModel.parentBVH = parentNode;
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