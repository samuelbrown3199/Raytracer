#pragma once

#include "VulkanTypes.h"


struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;

    void AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags shaderStages);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device);
};

struct DescriptorAllocator
{
    struct PoolSizeRatio
    {
        VkDescriptorType m_type;
        float m_ratio;
    };

    VkDescriptorPool m_pool;

    void InitializePool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void ClearDescriptors(VkDevice device);
    void DestroyPool(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);
};

struct DescriptorAllocatorGrowable
{
public:
    struct PoolSizeRatio
    {
        VkDescriptorType m_type;
        float m_ratio;
    };

    void Initialize(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
    void ClearPools(VkDevice device);
    void DestroyPools(VkDevice device);

    VkDescriptorSet AllocateSet(VkDevice device, VkDescriptorSetLayout layout);

private:
    VkDescriptorPool GetPool(VkDevice device);
    VkDescriptorPool CreatePool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> m_ratios;
    std::vector<VkDescriptorPool> m_fullPools;
    std::vector<VkDescriptorPool> m_readyPools;
    uint32_t m_setsPerPool;
};

struct DescriptorWriter
{
    std::deque<VkDescriptorImageInfo> m_imageInfos;
    std::deque<VkDescriptorBufferInfo> m_bufferInfos;
    std::vector<VkWriteDescriptorSet> m_writes;

    void WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void Clear();
    void UpdateSet(VkDevice device, VkDescriptorSet set);
};