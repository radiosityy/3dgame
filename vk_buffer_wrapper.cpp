#include "vk_buffer_wrapper.h"

VkBufferWrapper::VkBufferWrapper(VkBufferUsageFlags _usage_flags, bool _host_visible_required, VkFormat _buf_view_format)
    : usage_flags(_usage_flags)
    , host_visible_required(_host_visible_required)
    , buf_view_format(_buf_view_format)
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
{
    other.buf = VK_NULL_HANDLE;
    other.buf_view = VK_NULL_HANDLE;
    other.mem = VK_NULL_HANDLE;
    other.req_size = 0;
    other.size = 0;
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
