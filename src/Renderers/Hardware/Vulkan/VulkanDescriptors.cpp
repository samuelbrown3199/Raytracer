#include "VulkanDescriptors.h"

void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags shaderStages)
{
	VkDescriptorSetLayoutBinding newBind = {};
	newBind.binding = binding;
	newBind.descriptorCount = 1;
	newBind.descriptorType = type;
	newBind.stageFlags = shaderStages;

	m_bindings.push_back(newBind);
}

void DescriptorLayoutBuilder::Clear()
{
	m_bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device)
{
	VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	info.pNext = nullptr;

	info.pBindings = m_bindings.data();
	info.bindingCount = (uint32_t)m_bindings.size();
	info.flags = 0;

	VkDescriptorSetLayout set;
	if (vkCreateDescriptorSetLayout(device, &info, nullptr, &set) != VK_SUCCESS)
		throw std::exception("Failed to create descriptor set layout.");

	return set;
}


void DescriptorAllocator::InitializePool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (PoolSizeRatio ratio : poolRatios)
	{
		poolSizes.push_back(VkDescriptorPoolSize{ .type = ratio.m_type, .descriptorCount = uint32_t(ratio.m_ratio * maxSets) });
	}

	VkDescriptorPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.flags = 0;
	poolInfo.maxSets = maxSets;
	poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();

	vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_pool);
}

void DescriptorAllocator::ClearDescriptors(VkDevice device)
{
	vkResetDescriptorPool(device, m_pool, 0);
}

void DescriptorAllocator::DestroyPool(VkDevice device)
{
	vkDestroyDescriptorPool(device, m_pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet ds;
	if (vkAllocateDescriptorSets(device, &allocInfo, &ds) != VK_SUCCESS)
		throw std::exception();

	return ds;
}



VkDescriptorPool DescriptorAllocatorGrowable::GetPool(VkDevice device)
{
	VkDescriptorPool newPool;
	if (m_readyPools.size() != 0)
	{
		newPool = m_readyPools.back();
		m_readyPools.pop_back();
	}
	else
	{
		//need to create a new pool
		newPool = CreatePool(device, m_setsPerPool, m_ratios);

		m_setsPerPool = m_setsPerPool * 1.5;
		if (m_setsPerPool > 4092) {
			m_setsPerPool = 4092;
		}
	}

	return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::CreatePool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (PoolSizeRatio ratio : poolRatios)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = ratio.m_type,
			.descriptorCount = uint32_t(ratio.m_ratio * setCount)
			});
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = setCount;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
	return newPool;
}

void DescriptorAllocatorGrowable::Initialize(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
	m_ratios.clear();

	for (auto r : poolRatios)
	{
		m_ratios.push_back(r);
	}

	VkDescriptorPool newPool = CreatePool(device, maxSets, poolRatios);

	m_setsPerPool = maxSets * 1.5; //grow it next allocation

	m_readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::ClearPools(VkDevice device)
{
	for (auto p : m_readyPools)
	{
		vkResetDescriptorPool(device, p, 0);
	}
	for (auto p : m_fullPools)
	{
		vkResetDescriptorPool(device, p, 0);
		m_readyPools.push_back(p);
	}
	m_fullPools.clear();
}

void DescriptorAllocatorGrowable::DestroyPools(VkDevice device)
{
	for (auto p : m_readyPools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	m_readyPools.clear();
	for (auto p : m_fullPools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	m_fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::AllocateSet(VkDevice device, VkDescriptorSetLayout layout)
{
	//get or create a pool to allocate from
	VkDescriptorPool poolToUse = GetPool(device);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = nullptr;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = poolToUse;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet ds;
	VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

	//allocation failed. Try again
	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
	{

		m_fullPools.push_back(poolToUse);

		poolToUse = GetPool(device);
		allocInfo.descriptorPool = poolToUse;

		if (vkAllocateDescriptorSets(device, &allocInfo, &ds) != VK_SUCCESS)
			throw std::exception("Failed to allocate descriptor set.");
	}

	m_readyPools.push_back(poolToUse);
	return ds;
}



void DescriptorWriter::WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
	VkDescriptorImageInfo& info = m_imageInfos.emplace_back(VkDescriptorImageInfo{
		.sampler = sampler,
		.imageView = image,
		.imageLayout = layout
		});

	VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding = binding;
	write.dstSet = VK_NULL_HANDLE; //left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &info;

	m_writes.push_back(write);
}

void DescriptorWriter::WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
	VkDescriptorBufferInfo& info = m_bufferInfos.emplace_back(VkDescriptorBufferInfo{
		.buffer = buffer,
		.offset = offset,
		.range = size
		});

	VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding = binding;
	write.dstSet = VK_NULL_HANDLE; //left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &info;

	m_writes.push_back(write);
}

void DescriptorWriter::Clear()
{
	m_imageInfos.clear();
	m_writes.clear();
	m_bufferInfos.clear();
}

void DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set)
{
	for (VkWriteDescriptorSet& write : m_writes) {
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(device, (uint32_t)m_writes.size(), m_writes.data(), 0, nullptr);
}