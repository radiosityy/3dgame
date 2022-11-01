#include "vk_buffer_wrapper.h"

VkBufferWrapper::VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_)
    : usage_flags(usage_flags_)
    , host_visible_required(host_visible_required_)
    , buf_view_format(VK_FORMAT_UNDEFINED)
    , wait_stage(0)
    , use_transfer_buffer(false)
{}

VkBufferWrapper::VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkPipelineStageFlags wait_stage_)
    : usage_flags(usage_flags_)
    , host_visible_required(host_visible_required_)
    , buf_view_format(VK_FORMAT_UNDEFINED)
    , wait_stage(wait_stage_)
    , use_transfer_buffer(true)
{}

VkBufferWrapper::VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkFormat buf_view_format_)
    : usage_flags(usage_flags_)
    , host_visible_required(host_visible_required_)
    , buf_view_format(buf_view_format_)
    , wait_stage(0)
    , use_transfer_buffer(false)
{}

VkBufferWrapper::VkBufferWrapper(VkBufferUsageFlags usage_flags_, bool host_visible_required_, VkFormat buf_view_format_, VkPipelineStageFlags wait_stage_)
    : usage_flags(usage_flags_)
    , host_visible_required(host_visible_required_)
    , buf_view_format(buf_view_format_)
    , wait_stage(wait_stage_)
    , use_transfer_buffer(true)
{}

VkBufferWrapper::VkBufferWrapper(VkBufferWrapper&& other)
    : buf(other.buf)
    , buf_view(other.buf_view)
    , mem(other.mem)
    , host_visible(other.host_visible)
    , req_size(other.req_size)
    , size(other.size)
    , usage_flags(other.usage_flags)
    , host_visible_required(other.host_visible_required)
    , buf_view_format(other.buf_view_format)
    , wait_stage(other.wait_stage)
    , use_transfer_buffer(other.use_transfer_buffer)
    , cmd_buf(other.cmd_buf)
    , fence(other.fence)
    , semaphore(other.semaphore)
{
    other.buf = VK_NULL_HANDLE;
    other.buf_view = VK_NULL_HANDLE;
    other.mem = VK_NULL_HANDLE;
    other.req_size = 0;
    other.size = 0;
    other.cmd_buf = VK_NULL_HANDLE;
    other.fence = VK_NULL_HANDLE;
    other.semaphore = VK_NULL_HANDLE;
    transfer_buffer = std::move(other.transfer_buffer);
}

uint64_t VkBufferWrapper::allocate(uint64_t alloc_size)
{
    auto itr = m_free_spaces.lower_bound(FreeSpace(0, alloc_size));

    if(itr == m_free_spaces.end())
    {
        const uint64_t offset = req_size;
        req_size += alloc_size;
        return offset;
    }
    else
    {
        const uint64_t offset = itr->offset;
        const uint64_t remaining_size = itr->size - alloc_size;

        m_free_spaces.erase(itr);

        if(remaining_size > 0)
        {
            m_free_spaces.emplace(offset + alloc_size, remaining_size);
        }

        return offset;
    }
}

void VkBufferWrapper::free(uint64_t offset, uint64_t alloc_size)
{
    if((offset + alloc_size) == req_size)
    {
        req_size -= alloc_size;
    }
    else
    {
        m_free_spaces.emplace(offset, alloc_size);
    }
}
