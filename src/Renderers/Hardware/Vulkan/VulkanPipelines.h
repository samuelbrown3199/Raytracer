#pragma once

#include "VulkanTypes.h"

#include <vector>
#include <memory>


bool LoadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);

class PipelineBuilder
{
    public:
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
    VkPipelineRasterizationStateCreateInfo m_rasterizer;
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo m_multisampling;
    VkPipelineLayout m_pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo m_depthStencil;
    VkPipelineRenderingCreateInfo m_renderInfo;
    VkFormat m_colorAttachmentFormat;

    PipelineBuilder() { Clear(); }

    void Clear();

    void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void SetComputeShader(VkShaderModule computeShader);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void SetMultisamplingNone();

    void DisableBlending();
    void EnableBlendingAdditive();
    void EnableBlendingAlphaBlend();

    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthFormat(VkFormat format);
    void DisableDepthTest();
    void EnableDepthTest(bool depthWriteEnable, VkCompareOp op);

    VkPipeline BuildPipeline(VkDevice device);
    VkPipeline BuildComputePipeline(VkDevice device);
};