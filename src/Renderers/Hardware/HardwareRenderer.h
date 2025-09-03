#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <unordered_map>

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
#include "RaytracerTypes.h"
#include "CameraController.h"
#include "Imgui/ImGui.h"

#include "../../Interface/ToolUI.h"

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
	glm::vec3 cameraPosition = glm::vec3(0, 1, 5);
	glm::vec3 cameraLookDirection = glm::vec3(0,0,-1);
	float cameraFov = 90.0f;
	float focusDistance = 10.0f;
	float defocusAngle = 0.0f;
};

struct Model
{
	int triangleStartIndex = -1;
	int triangleCount = 0;

	ParentBVHNode parentBVH;
};

struct SceneObject
{
	std::string modelName;
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 rotation = glm::vec3(0, 0, 0);
	glm::vec3 scale = glm::vec3(1, 1, 1);
	int materialIndex = -1;
};

class HardwareRenderer
{
	friend class Window;
	friend class CameraController;
	friend class SceneEditorUI;

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

	bool m_bRefreshAccumulation = true;
	bool m_bDoRender = false;
	int m_iRenderFrames = 0;

	uint32_t m_iCurrentFrame = 0;
	const static int MAX_FRAMES_IN_FLIGHT = 2;
	FrameData m_frames[MAX_FRAMES_IN_FLIGHT];

	AllocatedImage m_accumulationImage;
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
	CameraController m_cameraController;
	RaytracePushConstants m_pushConstants;

	VkPipeline m_raytracePipeline;
	VkPipelineLayout m_raytracePipelineLayout;

	PerformanceStats m_performanceStats;
	InputManager m_inputManager;

	std::unordered_map<std::string, Model> m_models;
	std::vector<SceneObject> m_sceneObjects;

	//Scene Data and buffers

	std::vector<GPUObject> m_gpuSceneObjects;
	AllocatedBuffer m_sceneObjectBuffer;
	std::vector<GPUTriangle> m_sceneTriangles;
	AllocatedBuffer m_sceneTriangleBuffer;

	std::vector<ParentBVHNode> m_parentBVH;
	AllocatedBuffer m_parentBVHBuffer;

	std::vector<GPUBVHNode> m_childBVH;
	AllocatedBuffer m_childBVHBuffer;

	std::vector<GPUMaterial> m_sceneMaterials;
	AllocatedBuffer m_sceneMaterialBuffer;

	VkDescriptorSet m_sceneDescriptor;
	VkDescriptorSetLayout m_sceneDescriptorLayout;

	std::unordered_map<std::string, std::shared_ptr<ToolUI>> m_toolUIs;

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

	int GetMaterialIndex(const GPUMaterial& material);
	void AddSceneObject(std::string modelPath, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, const GPUMaterial& material);
	void ConvertSceneObjectToGPUObject(const SceneObject& obj);

	void InitializeUIs();
	void InitializeScene();
	void BufferSceneData();
	void ClearSceneData();
	void RebufferSceneData();

	void DispatchRayTracingCommands(VkCommandBuffer cmd);
	void RefreshAccumulation(VkCommandBuffer cmd);
	void RenderImGui(VkCommandBuffer cmd, VkImage targetImage, VkImageView targetImageView);
	virtual void RenderFrame();
	void MainLoop();
	void DoInterfaceControls();

	void ToggleUI(const std::string& uiName);

	void ProduceRender();
	void WriteDrawImageToFile();

public:

	void InitializeRenderer();
	void LoadModel(const std::string& filePath);

	void SetDoRender() { m_bDoRender = true; }
	void SetRenderFrames(int frames) { m_iRenderFrames = frames; }
	void SetRefreshAccumulation() { m_bRefreshAccumulation = true; }

	InputManager* GetInputManager() { return &m_inputManager; }
	PerformanceStats* GetPerformanceStats() { return &m_performanceStats; }
	Window* GetWindow() { return m_pWindow; }

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