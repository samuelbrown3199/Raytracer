#pragma once 

#include <vulkan/vulkan.h>

#include "VulkanTypes.h"

class HardwareRenderer;

void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize, VkFilter filter = VK_FILTER_LINEAR, int targetLayer=0);
void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

AllocatedImage CreateImage(HardwareRenderer* renderer, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped, std::string imageName, int numLayers=1);
AllocatedImage CreateImage(HardwareRenderer* renderer, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped, std::string imageName, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
void DestroyImage(HardwareRenderer* renderer, AllocatedImage* img);

void UpdateImage(HardwareRenderer* renderer, AllocatedImage* img, void* data, VkExtent3D size, VkImageLayout finalLayout);