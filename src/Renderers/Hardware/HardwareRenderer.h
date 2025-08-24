#pragma once

#include <deque>
#include <functional>
#include <mutex>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/hash.hpp>

#include "Vulkan/VulkanTypes.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Window.h"
#include "InputManager.h"
#include "PerformanceStats.h"
#include "Imgui/ImGui.h"

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

struct CameraSettings
{
	glm::vec3 cameraPosition = glm::vec3(0, 0, 0);
	glm::vec3 cameraLookDirection = glm::vec3(0,0,-1);
	float cameraFov = 90.0f;
	float focusDistance = 10.0f;
	float defocusAngle = 0.0;

	glm::vec3 defocusDiskU;
	glm::vec3 defocusDiskV;
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

	ImGuiContext* m_imguiContext;

	CameraSettings m_camera;
	RaytracePushConstants m_pushConstants;

	VkPipeline m_raytracePipeline;
	VkPipelineLayout m_raytracePipelineLayout;

	PerformanceStats m_performanceStats;
	InputManager m_inputManager;

	void InitializeVulkan();
	void CreateInstance();
	void InitializeSwapchain();
	void InitializeCommands();
	void InitializeSyncStructures();
	void InitializeDescriptors();
	void InitializeImgui();
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

	void Quit();

	void DispatchRayTracingCommands(VkCommandBuffer cmd);
	void RenderImGui(VkCommandBuffer cmd, VkImage targetImage, VkImageView targetImageView);
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