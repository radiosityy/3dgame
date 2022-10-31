#ifndef VK_BUFFER_WRAPPER_H
#define VK_BUFFER_WRAPPER_H

#include <vulkan.h>
#include <memory>
#include <set>

class VkBufferWrapper
{
public:
    VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_);
    VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkPipelineStageFlags wait_stage_);
    VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkFormat buf_view_format_);
    VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkFormat buf_view_format_, VkPipelineStageFlags wait_stage_);
    VkBufferWrapper(VkBufferWrapper&& other);

    uint64_t allocate(uint64_t size);
    void free(uint64_t offset, uint64_t size);

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

private:
    struct FreeSpace
    {
        FreeSpace(uint64_t offset_, uint64_t size_)
            : offset(offset_)
            , size(size_)
        {}

        friend bool operator<(const FreeSpace& lhs, const FreeSpace& rhs)
        {
            return lhs.size < rhs.size;
        }

        uint64_t offset = 0;
        uint64_t size = 0;
    };

    std::set<FreeSpace> m_free_spaces;
};

#endif //VK_BUFFER_WRAPPER_H
