#ifndef VK_BUFFER_WRAPPER_H
#define VK_BUFFER_WRAPPER_H

#include <vulkan/vulkan.h>
#include <memory>

struct VkBufferWrapper
{
    VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_);
    VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkPipelineStageFlags wait_stage_);
    VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkFormat buf_view_format_);
    VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkFormat buf_view_format_, VkPipelineStageFlags wait_stage_);
    VkBufferWrapper(VkBufferWrapper&& other);

    VkBuffer buf = VK_NULL_HANDLE;
    VkBufferView buf_view = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;

    bool host_visible;
    VkDeviceSize req_size = 0;
    VkDeviceSize size = 0;

    const VkBufferUsageFlags usage_flags;
    const bool host_visible_required;
    const VkFormat buf_view_format;
    const VkPipelineStageFlags wait_stage;
    const bool use_transfer_buffer;

    std::unique_ptr<VkBufferWrapper> transfer_buffer;

    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore semaphore = VK_NULL_HANDLE;
};

#endif //VK_BUFFER_WRAPPER_H
