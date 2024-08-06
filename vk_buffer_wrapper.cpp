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
    , transfer_buffer(std::move(other.transfer_buffer))
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
}
