#pragma once

#include <deque>
#include <functional>
#include <mutex>

#include "Vulkan/VulkanTypes.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Window.h"

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) 
	{
		deletors.push_back(function);
	}

	void flush()
	{
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call functors
		}

		deletors.clear();
	}
};

struct FrameData
{
	VkCommandPool m_commandPool;
	VkCommandBuffer m_mainCommandBuffer;

	VkSemaphore m_swapchainSemaphore, m_renderSemaphore;
	VkFence m_renderFence;

	DeletionQueue m_deletionQueue;
	DescriptorAllocatorGrowable m_frameDescriptors;

	VkDescriptorSet m_sceneDescriptor;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class HardwareRenderer
{
	friend class Window;

private:

	std::mutex m_renderMutex;
	std::mutex m_immediateSubmitMutex;
	std::mutex m_deletionQueueMutex;

	#ifdef NDEBUG
		const bool m_bEnableValidationLayers = false;
	#else
		const bool m_bEnableValidationLayers = true;
	#endif

	Window* m_pWindow = nullptr;

	VkInstance m_vulkanInstance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkSurfaceKHR m_surface;

	bool m_bRun = true;
	bool m_bInitialized = false;

	uint32_t m_iCurrentFrame = 0;
	const static int MAX_FRAMES_IN_FLIGHT = 2;
	FrameData m_frames[MAX_FRAMES_IN_FLIGHT];

	AllocatedImage m_drawImage;
	VkExtent2D m_drawExtent;
	VkFilter m_drawFilter = VK_FILTER_NEAREST;

	VkQueue m_graphicsQueue;
	uint32_t m_graphicsQueueFamily;

	DeletionQueue m_mainDeletionQueue;
	VmaAllocator m_allocator;

	DescriptorAllocator m_globalDescriptorAllocator;

	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchainImageFormat;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkExtent2D m_swapchainExtent;

	VkFence m_immediateFence;
	VkCommandBuffer m_immediateCommandBuffer;
	VkCommandPool m_immediateCommandPool;

	VkDescriptorSet m_drawImageDescriptors;
	VkDescriptorSetLayout m_drawImageDescriptorLayout;
	VkDescriptorSetLayout m_storageImageDescriptorLayout;

	VkPipeline m_raytracePipeline;
	VkPipelineLayout m_raytracePipelineLayout;

	void InitializeVulkan();
	void CreateInstance();
	void InitializeSwapchain();
	void InitializeCommands();
	void InitializeSyncStructures();
	void InitializeDescriptors();
	void InitializePipelines();

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();
	void RecreateSwapchain();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void DispatchRayTracingCommands(VkCommandBuffer cmd);
	virtual void RenderFrame();
	void MainLoop();

public:

	void InitializeRenderer();

	int GetMaxFramesInFlight() { return MAX_FRAMES_IN_FLIGHT; }
	int GetFrameIndex() { return m_iCurrentFrame % MAX_FRAMES_IN_FLIGHT; }
	FrameData& GetCurrentFrame() { return m_frames[GetFrameIndex()]; }

	VmaAllocator GetAllocator() const { return m_allocator; }
	VkQueue GetGraphicsQueue() { return m_graphicsQueue; }
	VkInstance GetVulkanInstance() { return m_vulkanInstance; }
	VkPhysicalDevice GetPhysicalDevice() { return m_physicalDevice; }
	VkDevice GetLogicalDevice() { return m_device; }
	DescriptorAllocator GetGlobalDescriptorAllocator() { return m_globalDescriptorAllocator; }

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, std::string allocationName);
	void DestroyBuffer(const AllocatedBuffer& buffer);

	VkFormat GetDrawImageFormat() { return m_drawImage.m_imageFormat; }
	AllocatedImage* GetDrawImage() { return &m_drawImage; }
};