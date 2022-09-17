#include "engine_3d.h"
#include "error_handling.h"

#include <iostream>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <fstream>
#include <list>

#include <png.h>

static VkBool32 debugReportCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

/*---------------- helper methods ----------------*/
static void checkIfLayersAndExtensionsAvailable(const std::vector<const char*>& layers, const std::vector<const char*>& extensions)
{
    uint32_t count;
    VkResult res;

    /*enumerate instance layers and check if required layers are available*/
    res = vkEnumerateInstanceLayerProperties(&count, NULL);
    assertVkSuccess(res, "Failed to enumerate instance layer properties.");

    std::vector<VkLayerProperties> layer_props(count);
    res = vkEnumerateInstanceLayerProperties(&count, layer_props.data());
    assertVkSuccess(res, "Failed to enumerate instance layer properties.");

    for(const char* layer : layers)
    {
        bool found = false;

        for(const auto& lp : layer_props)
        {
            if(!std::strcmp(layer, lp.layerName))
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            std::stringstream error_msg;
            error_msg << "Error: Required instance layer unavailable: " << layer;
            throw std::runtime_error(error_msg.str());
        }
    }

    /*enumerate instance extensions and check if the required ones are available*/
    res = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    assertVkSuccess(res, "Failed to enumerate instance extension properties.");

    std::vector<VkExtensionProperties> ext_props(count);
    res = vkEnumerateInstanceExtensionProperties(NULL, &count, ext_props.data());
    assertVkSuccess(res, "Failed to enumerate instance extension properties.");

    for(const char* ext : extensions)
    {
        bool found = false;

        for(VkExtensionProperties& ep : ext_props)
        {
            if(!std::strcmp(ext, ep.extensionName))
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            std::stringstream error_msg;
            error_msg << "Error: Required instance extension unavailable: " << ext;
            throw std::runtime_error(error_msg.str());
        }
    }
}

VkPipelineShaderStageCreateInfo Engine3D::loadShader(const std::string& filename, VkShaderStageFlagBits stage, VkShaderModule* shader_module, const VkSpecializationInfo* spec_info)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(!file)
    {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }

    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint32_t> buffer(((static_cast<size_t>(size) - 1) / sizeof(uint32_t)) + 1);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    VkShaderModuleCreateInfo module_create_info{};
    module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_create_info.pNext = NULL;
    module_create_info.flags = 0;
    module_create_info.codeSize = static_cast<size_t>(size);
    module_create_info.pCode = buffer.data();

    VkResult res = vkCreateShaderModule(m_device, &module_create_info, NULL, shader_module);
    assertVkSuccess(res, "Failed to create shader module");

    VkPipelineShaderStageCreateInfo stage_create_info{};
    stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.pNext = NULL;
    stage_create_info.flags = 0;
    stage_create_info.stage = stage;
    stage_create_info.module = *shader_module;
    stage_create_info.pName = "main";
    stage_create_info.pSpecializationInfo = spec_info;

    return stage_create_info;
}

std::pair<VkDeviceMemory, bool> Engine3D::allocateMemory(VkMemoryRequirements mem_req, bool host_visible_required) const
{
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkMemoryPropertyFlags preferred = host_visible_required ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    std::vector<uint32_t> preferred_mem_type_ids;
    std::vector<uint32_t> valid_mem_type_ids;

    for(uint32_t i = 0; i < m_physical_device_memory_properties.memoryTypeCount; i++)
    {
        if(mem_req.memoryTypeBits & (1 << i))
        {
            if((m_physical_device_memory_properties.memoryTypes[i].propertyFlags & preferred) == preferred)
            {
                preferred_mem_type_ids.push_back(i);
            }
            else
            {
                valid_mem_type_ids.push_back(i);
            }
        }
    }

    VkMemoryAllocateInfo mem_alloc_info{};
    mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc_info.pNext = NULL;
    mem_alloc_info.allocationSize = mem_req.size;

    for(uint32_t id : preferred_mem_type_ids)
    {
        mem_alloc_info.memoryTypeIndex = id;

        if(VK_SUCCESS == vkAllocateMemory(m_device, &mem_alloc_info, NULL, &mem))
        {
            return {mem, m_physical_device_memory_properties.memoryTypes[id].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT};
        }
    }

    if(!host_visible_required)
    {
        for(uint32_t id : valid_mem_type_ids)
        {
            mem_alloc_info.memoryTypeIndex = id;

            if(VK_SUCCESS == vkAllocateMemory(m_device, &mem_alloc_info, NULL, &mem))
            {
                return {mem, m_physical_device_memory_properties.memoryTypes[id].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT};
            }
        }
    }

    throw std::runtime_error("Failed to allocate memory");
}

void Engine3D::allocateAndBindMemory(VkImageWrapper& image) const
{
    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(m_device, image.img, &mem_req);

    std::tie(image.mem, std::ignore) = allocateMemory(mem_req, false);

    VkResult res = vkBindImageMemory(m_device, image.img, image.mem, 0);
    if(VK_SUCCESS != res)
    {
        vkFreeMemory(m_device, image.mem, NULL);
        throw std::runtime_error(errorMsg("Failed to bind image memory.", res));
    }
}

void Engine3D::allocateAndBindMemory(VkBufferWrapper& buffer) const
{
    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(m_device, buffer.buf, &mem_req);

    std::tie(buffer.mem, buffer.host_visible) = allocateMemory(mem_req, buffer.host_visible_required);

    VkResult res = vkBindBufferMemory(m_device, buffer.buf, buffer.mem, 0);
    if(VK_SUCCESS != res)
    {
        vkFreeMemory(m_device, buffer.mem, NULL);
        throw std::runtime_error(errorMsg("Failed to bind buffer memory.", res));
    }
}

void Engine3D::createImage(VkImageWrapper& image, const VkImageCreateInfo& img_create_info, VkImageViewCreateInfo& img_view_create_info) const
{
    VkResult res;

    res = vkCreateImage(m_device, &img_create_info, NULL, &image.img);
    assertVkSuccess(res, "Failed to create image.");

    try{allocateAndBindMemory(image);} catch(...)
    {
        destroyImage(image);
        throw;
    }

    img_view_create_info.image = image.img;

    res = vkCreateImageView(m_device, &img_view_create_info, NULL, &image.img_view);
    if(VK_SUCCESS != res)
    {
        destroyImage(image);
        throw std::runtime_error(errorMsg("Failed to create image view.", res));
    }
}

Engine3D::VkImageWrapper Engine3D::createImage(const VkImageCreateInfo& img_create_info, VkImageViewCreateInfo& img_view_create_info) const
{
    VkImageWrapper image;

    createImage(image, img_create_info, img_view_create_info);

    return image;
}

void Engine3D::destroyImage(VkImageWrapper& image) const noexcept
{
    vkFreeMemory(m_device, image.mem, NULL);
    vkDestroyImageView(m_device, image.img_view, NULL);
    vkDestroyImage(m_device, image.img, NULL);

    image.img = VK_NULL_HANDLE;
    image.img_view = VK_NULL_HANDLE;
    image.mem = VK_NULL_HANDLE;
}

void Engine3D::createBuffer(VkBufferWrapper& buffer, VkDeviceSize size, bool force_transfer_buffer) const
{
    if(buffer.transfer_buffer)
    {
        destroyBuffer(*buffer.transfer_buffer);
        buffer.transfer_buffer.reset();
    }

    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.size = size;
    create_info.usage = buffer.usage_flags;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &m_queue_family_index;

    VkResult res = vkCreateBuffer(m_device, &create_info, NULL, &buffer.buf);
    assertVkSuccess(res, "Failed to create buffer.");

    try{ allocateAndBindMemory(buffer); } catch(...)
    {
        destroyBuffer(buffer);
        throw;
    }

    if(buffer.buf_view_format != VK_FORMAT_UNDEFINED)
    {
        VkBufferViewCreateInfo buf_view_create_info{};
        buf_view_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        buf_view_create_info.pNext = NULL;
        buf_view_create_info.flags = 0;
        buf_view_create_info.buffer = buffer.buf;
        buf_view_create_info.format = buffer.buf_view_format;
        buf_view_create_info.offset = 0;
        buf_view_create_info.range = VK_WHOLE_SIZE;

        res = vkCreateBufferView(m_device, &buf_view_create_info, NULL, &buffer.buf_view);
        if(VK_SUCCESS != res)
        {
            destroyBuffer(buffer);
            throw std::runtime_error(errorMsg("Failed to create buffer view.", res));
        }
    }

    buffer.size = size;

    if(force_transfer_buffer || (!buffer.host_visible && buffer.size > SMALL_BUFFER_SIZE))
    {
        buffer.transfer_buffer = std::make_unique<VkBufferWrapper>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        createBuffer(*buffer.transfer_buffer, size, false);
    }
}

void Engine3D::destroyBuffer(VkBufferWrapper& buffer) const noexcept
{
    vkFreeMemory(m_device, buffer.mem, NULL);
    vkDestroyBufferView(m_device, buffer.buf_view, NULL);
    vkDestroyBuffer(m_device, buffer.buf, NULL);

    buffer.buf = VK_NULL_HANDLE;
    buffer.buf_view = VK_NULL_HANDLE;
    buffer.mem = VK_NULL_HANDLE;
    buffer.size = 0;
    buffer.req_size = 0;

    if(buffer.transfer_buffer)
    {
        destroyBuffer(*buffer.transfer_buffer);
    }
}

void Engine3D::updateBuffer(VkBufferWrapper& buffer, size_t data_size, const std::function<void(void*)>& copy_fun, VkCommandBuffer cmd_buf)
{
    if(buffer.size < data_size)
    {
        destroyBuffer(buffer);
        createBuffer(buffer, data_size);
    }

    if(buffer.host_visible)
    {
        void* ptr;
        VkResult res = vkMapMemory(m_device, buffer.mem, 0, buffer.size, 0, &ptr);
        assertVkSuccess(res, "Failed to map buffer memory.");

        copy_fun(ptr);

        vkUnmapMemory(m_device, buffer.mem);
    }
    else if(buffer.size <= SMALL_BUFFER_SIZE)
    {
        std::vector<uint8_t> data_buf(buffer.size);
        copy_fun(data_buf.data());

        vkCmdUpdateBuffer(cmd_buf, buffer.buf, 0, data_buf.size(), data_buf.data());
    }
    else
    {
        void* ptr;
        VkResult res = vkMapMemory(m_device, buffer.transfer_buffer->mem, 0, buffer.size, 0, &ptr);
        assertVkSuccess(res, "Failed to map buffer memory.");

        copy_fun(ptr);

        vkUnmapMemory(m_device, buffer.transfer_buffer->mem);

        VkBufferCopy buf_cpy_region{0, 0, buffer.size};

        vkCmdCopyBuffer(cmd_buf, buffer.transfer_buffer->buf, buffer.buf, 1, &buf_cpy_region);
    }

    //TODO: if we vkMapMemory here and the buffer is not host coherent, then we need to flush
    //this may be better handled in bulk in updateAndRender() that updates multiple buffer and could only flush once at the end
    //(also, maybe we can use a barrier instead of a flush in updateAndRender() ?)
}

void Engine3D::updateBuffer(VkBufferWrapper& buffer, size_t data_size, const void* data, VkCommandBuffer cmd_buf)
{
    if(buffer.size < data_size)
    {
        destroyBuffer(buffer);
        createBuffer(buffer, data_size);
    }

    if(buffer.host_visible)
    {
        void* ptr;
        VkResult res = vkMapMemory(m_device, buffer.mem, 0, buffer.size, 0, &ptr);
        assertVkSuccess(res, "Failed to map buffer memory.");

        std::memcpy(ptr, data, data_size);

        vkUnmapMemory(m_device, buffer.mem);
    }
    else if(buffer.size <= SMALL_BUFFER_SIZE)
    {
        vkCmdUpdateBuffer(cmd_buf, buffer.buf, 0, data_size, data);
    }
    else
    {
        void* ptr;
        VkResult res = vkMapMemory(m_device, buffer.transfer_buffer->mem, 0, buffer.size, 0, &ptr);
        assertVkSuccess(res, "Failed to map buffer memory.");

        std::memcpy(ptr, data, data_size);

        vkUnmapMemory(m_device, buffer.transfer_buffer->mem);

        VkBufferCopy buf_cpy_region{0, 0, buffer.size};

        vkCmdCopyBuffer(cmd_buf, buffer.transfer_buffer->buf, buffer.buf, 1, &buf_cpy_region);
    }

    //TODO: if we vkMapMemory here and the buffer is not host coherent, then we need to flush
    //this may be better handled in bulk in updateAndRender() that updates multiple buffer and could only flush once at the end
    //(also, maybe we can use a barrier instead of a flush in updateAndRender() ?)
}

uint32_t Engine3D::loadTexture(std::string_view texture_filename)
{
    return loadTextures({texture_filename})[0];
}

std::vector<uint32_t> Engine3D::loadTextures(const std::vector<std::string_view>& texture_filenames)
{
    return loadTexturesGeneric(texture_filenames, m_textures, true);
}

uint32_t Engine3D::loadNormalMap(std::string_view texture_filename)
{
    return loadNormalMaps({texture_filename})[0];
}

std::vector<uint32_t> Engine3D::loadNormalMaps(const std::vector<std::string_view>& texture_filenames)
{
    return loadTexturesGeneric(texture_filenames, m_normal_maps, true);
}

void Engine3D::deviceWaitIdle()
{
    VkResult res = vkDeviceWaitIdle(m_device);
    assertVkSuccess(res, "vkDeviceWaitIdle error");
}

std::vector<uint32_t> Engine3D::loadTexturesGeneric(const std::vector<std::string_view>& texture_filenames_, TextureCollection& tex_col, bool generate_mipmaps)
{
    std::vector<std::string> texture_filenames(texture_filenames_.size());
    for(size_t i = 0; i < texture_filenames_.size(); i++)
    {
        texture_filenames[i] = std::string("assets/textures/") + texture_filenames_[i].data();
    }

    std::vector<uint32_t> ret(texture_filenames.size());
    std::vector<uint32_t> textures_to_load;

    for(size_t i = 0; i < texture_filenames.size(); i++)
    {
        if(tex_col.ids.contains(texture_filenames[i].data()))
        {
            ret[i] = tex_col.ids[texture_filenames[i].data()];
        }
        else
        {
            textures_to_load.emplace_back(i);
        }
    }

    if(textures_to_load.empty())
    {
        return ret;
    }

    m_update_descriptors = true;

    VkResult res;
    std::vector<uvec2> img_sizes(textures_to_load.size());

    VkImageCreateInfo img_create_info{};
    img_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_create_info.pNext = NULL;
    img_create_info.flags = 0;
    img_create_info.imageType = VK_IMAGE_TYPE_2D;
    img_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    img_create_info.mipLevels = 1;
    img_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if(generate_mipmaps)
    {
        img_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    img_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_create_info.queueFamilyIndexCount = 1;
    img_create_info.pQueueFamilyIndices = &m_queue_family_index;
    img_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo img_view_create_info{};
    img_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_create_info.pNext = NULL;
    img_view_create_info.flags = 0;
    img_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    img_view_create_info.format = img_create_info.format;
    img_view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    img_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, 1};

    VkDeviceSize total_size = 0;

    /*for each texture to load*/
    for(size_t i = 0; i < textures_to_load.size(); i++)
    {
        auto file = std::fopen(texture_filenames[textures_to_load[i]].data(), "rb");

        if(!file)
        {
            std::stringstream ss;
            ss << "Failed to open " << texture_filenames[textures_to_load[i]];
            throw std::runtime_error(ss.str());
        }

        uint8_t sig[8];
        fread(sig, 1, 8, file);

        if(auto check = png_check_sig(sig, 8); !check)
        {
            fclose(file);

            std::stringstream ss;
            ss << "Texture file " << texture_filenames[textures_to_load[i]] << "is not a valid .png file.";
            throw std::runtime_error(ss.str());
        }

        auto png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        auto info_ptr = png_create_info_struct(png_ptr);

        png_init_io(png_ptr, file);
        png_set_sig_bytes(png_ptr, 8);
        png_read_info(png_ptr, info_ptr);

        const auto w = png_get_image_width(png_ptr, info_ptr);
        const auto h = png_get_image_height(png_ptr, info_ptr);

        total_size += w*h;
        img_sizes[i].x = w;
        img_sizes[i].y = h;

        /*clean up*/
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(file);
    }

    /*now we have to create a vulkan buffer to store all the image data
    and read all the png files raw data and copy it to the vulkan buffer;
    then we'll have to copy the image data from the vulkan buffer to the
    respective vulkan images for each texture*/
    VkBufferWrapper tex_buf(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    createBuffer(tex_buf, total_size * 4);

    /*having created the texture buffer and allocated and bound memory to it
    we now have to read raw image data and copy it into the buffer*/
    void* tex_buf_ptr;
    vkMapMemory(m_device, tex_buf.mem, 0, VK_WHOLE_SIZE, 0, &tex_buf_ptr);

    size_t base_offset = 0;

    for(size_t t = 0; t < textures_to_load.size(); t++)
    {
        const auto w = img_sizes[t].x;
        const auto h = img_sizes[t].y;

        auto file = std::fopen(texture_filenames[textures_to_load[t]].data(), "rb");

        auto png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        auto info_ptr = png_create_info_struct(png_ptr);

        png_init_io(png_ptr, file);
        png_read_info(png_ptr, info_ptr);

        const auto color_type = png_get_color_type(png_ptr, info_ptr);
        const auto bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        if(16 == bit_depth)
        {
            png_set_strip_16(png_ptr);
        }

        if(color_type == PNG_COLOR_TYPE_PALETTE)
        {
            png_set_palette_to_rgb(png_ptr);
        }

        if((PNG_COLOR_TYPE_GRAY == color_type) && (bit_depth < 8))
        {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }

        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(png_ptr);
        }

        if((PNG_COLOR_TYPE_RGB == color_type) || (PNG_COLOR_TYPE_GRAY == color_type) || (PNG_COLOR_TYPE_PALETTE == color_type))
        {
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
        }

        if((PNG_COLOR_TYPE_GRAY == color_type) || (PNG_COLOR_TYPE_GRAY_ALPHA == color_type))
        {
            png_set_gray_to_rgb(png_ptr);
        }

        png_read_update_info(png_ptr, info_ptr);

        std::vector<uint8_t*> row_ptrs(h);
        for(size_t i = 0; i < h; i++)
        {
            row_ptrs[i] = reinterpret_cast<uint8_t*>(tex_buf_ptr) + base_offset + i*w*4;
        }

        png_read_image(png_ptr, row_ptrs.data());

        base_offset += w*h*4;

        png_read_end(png_ptr, NULL);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

        fclose(file);
    }

    vkUnmapMemory(m_device, tex_buf.mem);

    /*now we'll have to record commands to copy the buffer contents to vulkan images*/
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;

    res = vkBeginCommandBuffer(m_transfer_cmd_buf, &begin_info);
    assertVkSuccess(res, "An error occurred while beginning the transfer command buffer.");

    base_offset = 0;

    for(size_t t = 0; t < textures_to_load.size(); t++)
    {
        uint32_t tex_id = 0;

        if(tex_col.free_ids.empty())
        {
            tex_col.textures.emplace_back();
            tex_id = static_cast<uint32_t>(tex_col.textures.size() - 1);
        }
        else
        {
            tex_id = tex_col.free_ids.front();
            tex_col.free_ids.pop();
        }

        ret[textures_to_load[t]] = tex_id;
        tex_col.ids[texture_filenames[textures_to_load[t]].data()] = tex_id;
        VkImageWrapper& texture = tex_col.textures[tex_id];

        const auto w = img_sizes[t].x;
        const auto h = img_sizes[t].y;

        /*update vulkan create info and create an image for the texture*/
        img_create_info.extent = {w, h, 1};
        img_create_info.arrayLayers = 1;

        img_view_create_info.image = texture.img;
        img_view_create_info.subresourceRange.layerCount = img_create_info.arrayLayers;

        if(generate_mipmaps)
        {
            img_create_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(static_cast<float>(w), static_cast<float>(h))))) + 1;
        }
        else
        {
            img_create_info.mipLevels = 1;
        }

        createImage(texture, img_create_info, img_view_create_info);

        VkImageSubresourceRange img_sub_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, img_create_info.mipLevels, 0, 1};

        VkImageMemoryBarrier img_mem_bar{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, texture.img, img_sub_range};

        vkCmdPipelineBarrier(m_transfer_cmd_buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &img_mem_bar);

        VkBufferImageCopy buf_img_copy{};
        buf_img_copy.bufferOffset = base_offset;
        buf_img_copy.bufferRowLength = 0;
        buf_img_copy.bufferImageHeight = 0;

        buf_img_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buf_img_copy.imageSubresource.mipLevel = 0;
        buf_img_copy.imageSubresource.baseArrayLayer = 0;
        buf_img_copy.imageSubresource.layerCount = 1;

        buf_img_copy.imageOffset = {0, 0, 0};
        buf_img_copy.imageExtent = {w, h, 1};

        vkCmdCopyBufferToImage(m_transfer_cmd_buf, tex_buf.buf, texture.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buf_img_copy);

        base_offset += 4*w*h;

        if(generate_mipmaps && img_create_info.mipLevels > 1)
        {
            for(uint32_t i = 0; i < img_create_info.mipLevels - 1; i++)
            {
                img_sub_range.baseMipLevel = i;
                img_sub_range.levelCount = 1;
                img_mem_bar = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, texture.img, img_sub_range};

                vkCmdPipelineBarrier(m_transfer_cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &img_mem_bar);

                VkImageBlit img_blit{};
                img_blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};
                img_blit.srcOffsets[0] = {0, 0, 0};
                img_blit.srcOffsets[1] = {std::max(1, static_cast<int>(w / std::pow(2, i))), std::max(1, static_cast<int>(h / std::pow(2, i))), 1};
                img_blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i + 1, 0, 1};
                img_blit.dstOffsets[0] = {0, 0, 0};
                img_blit.dstOffsets[1] = {std::max(1, static_cast<int>(w / std::pow(2, i + 1))), std::max(1, static_cast<int>(h / std::pow(2, i + 1))), 1};

                vkCmdBlitImage(m_transfer_cmd_buf, texture.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_blit, VK_FILTER_LINEAR);
            }

            //TODO: put these 2 barriers together into a single vkCmdPipelineBarrier() call
            img_sub_range.baseMipLevel = 0;
            img_sub_range.levelCount = img_create_info.mipLevels - 1;
            img_mem_bar = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, texture.img, img_sub_range};

            vkCmdPipelineBarrier(m_transfer_cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &img_mem_bar);

            img_sub_range.baseMipLevel = img_create_info.mipLevels - 1;
            img_sub_range.levelCount = 1;
            img_mem_bar = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, texture.img, img_sub_range};

            vkCmdPipelineBarrier(m_transfer_cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &img_mem_bar);
        }
        else
        {
            img_mem_bar = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, texture.img, img_sub_range};

            vkCmdPipelineBarrier(m_transfer_cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &img_mem_bar);
        }
    }

    res = vkEndCommandBuffer(m_transfer_cmd_buf);
    assertVkSuccess(res, "An error occurred while ending the transfer command buffer.");

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_transfer_cmd_buf;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    res = vkResetFences(m_device, 1, &m_transfer_cmd_buf_fence);
    assertVkSuccess(res, "An error occurred while reseting transfer cmd buf fence.");

    res = vkQueueSubmit(m_queue, 1, &submit_info, m_transfer_cmd_buf_fence);
    assertVkSuccess(res, "An error occurred while submitting the transfer command buffer.");

    res = vkWaitForFences(m_device, 1, &m_transfer_cmd_buf_fence, VK_TRUE, UINT64_MAX);
    assertVkSuccess(res, "An error occured while waiting for a transfer cmd buf fence.");

    destroyBuffer(tex_buf);

    return ret;
}

/*---------------- main methods ----------------*/

Engine3D::Engine3D(const Window& window, std::string_view app_name)
    : m_render_batches(10000)
    , m_bone_transform_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false)
    , m_terrain_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, true)
{
    try
    {
        createInstance(app_name);
        createDevice();
        createSurface(window);
        createSwapchain(window.width(), window.height());
        createMainRenderPass();
        createShadowMapRenderPass();
        createCommandPool();
        createCommandBuffers();
        createSynchronizationPrimitives();
        createRenderTargets();
        createSamplers();
        createBuffers();

        //shadow maps
        m_shadow_map_clear_value.depthStencil.depth = 1.0f;
    }
    catch(...)
    {
        destroy();
        throw;
    }
}

Engine3D::~Engine3D()
{
    destroy();
}

void Engine3D::destroy() noexcept
{
    if(m_device)
    {
        /*wait for the vulkan device to finish all
        queued tasks before destroying any vulkan objects*/
        deviceWaitIdle();

        destroyShadowMaps();

        destroySamplers();
        destroyFontTextures();
        destroyTextures();

        for(auto& per_frame_data : m_per_frame_data)
        {
            destroyBuffer(*per_frame_data.common_buffer);
            destroyBuffer(*per_frame_data.dir_light_buffer);
            destroyBuffer(*per_frame_data.dir_light_valid_buffer);
            destroyBuffer(*per_frame_data.point_light_buffer);
            destroyBuffer(*per_frame_data.point_light_valid_buffer);
            destroyBuffer(*per_frame_data.dir_shadow_map_buffer);
            destroyBuffer(*per_frame_data.point_shadow_map_buffer);
        }

        for(auto& vb : m_vertex_buffers)
        {
            destroyBuffer(vb.second);
        }

        destroyBuffer(m_per_instance_vertex_buffer);
        destroyBuffer(m_bone_transform_buffer);
        destroyBuffer(m_terrain_buffer);

        destroySynchronizationPrimitives();
        destroyCommandPool();
        destroyPipelines();
        destroyPipelineLayout();
        destroyDescriptorSets();
        destroyRenderTargets();
        destroyRenderPasses();

        destroySwapchain();
    }

    if(m_instance)
    {
        destroySurface();
    }

    destroyDevice();
    destroyInstance();
}

void Engine3D::resizeBuffers()
{
    for(auto& req : m_buffer_update_reqs)
    {
        auto buf = req.first;

        if(buf->req_size > buf->size)
        {
            //TODO: do this first time creation elsewhere?
            if(buf->buf == VK_NULL_HANDLE)
            {
                createBuffer(*buf, buf->req_size);
            }
            else
            {
                VkBufferWrapper* old_buf = new VkBufferWrapper(std::move(*buf));
                createBuffer(*buf, old_buf->req_size);
                buf->req_size = buf->size;

                void* old_data = nullptr;
                VkResult res = vkMapMemory(m_device, old_buf->transfer_buffer->mem, 0, old_buf->size, 0, &old_data);
                assertVkSuccess(res, "Failed to map buffer memory");
                req.second.insert(req.second.begin(), {0, old_buf->size, old_data});

                m_per_frame_data[(m_frame_id + 1) % FRAMES_IN_FLIGHT].bufs_to_destroy.push_back(old_buf);
            }
        }
    }
}

void Engine3D::updateBuffers()
{
    for(auto& mapping : m_buffer_update_reqs)
    {
        if(!mapping.second.empty())
        {
            auto buf = mapping.first;

            void* dst = nullptr;
            VkResult res = vkMapMemory(m_device, buf->transfer_buffer->mem, 0, buf->size, 0, &dst);
            assertVkSuccess(res, "Failed to map buffer memory");

            for(auto& req : mapping.second)
            {
                void* const offset_dst = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(dst) + req.data_offset);
                std::memcpy(offset_dst, req.data, req.data_size);
            }

            vkUnmapMemory(m_device, buf->transfer_buffer->mem);
        }
    }
}

void Engine3D::updateAndRender(const RenderData& render_data, Camera& camera)
{
    m_frame_id = (m_frame_id + 1) % FRAMES_IN_FLIGHT;
    auto& per_frame_data = m_per_frame_data[m_frame_id];

    m_common_buffer_data.VP = camera.viewProj();
    m_common_buffer_data.V = camera.view();
    //TODO: this guiT field can only be calculated once I guess, for a given surface size, no need to recalculate it every frame
    m_common_buffer_data.guiT = scale(vec3(1.0f / static_cast<float>(m_surface_width), 1.0f / static_cast<float>(m_surface_height), 1.0f));
    m_common_buffer_data.camera_pos = camera.pos();
    m_common_buffer_data.visual_sun_pos = render_data.visual_sun_pos;
    m_common_buffer_data.effective_sun_pos = render_data.effective_sun_pos;
    m_common_buffer_data.sun_radius = render_data.sun_radius;
    m_common_buffer_data.editor_highlight_color = render_data.editor_highlight_color;
    m_common_buffer_data.cur_pos_terrain = render_data.cur_terrain_pos;
    m_common_buffer_data.cur_terrain_intersection = render_data.cur_terrain_intersection;
    m_common_buffer_data.editor_terrain_tool_inner_radius = render_data.editor_terrain_tool_inner_radius;
    m_common_buffer_data.editor_terrain_tool_outer_radius = render_data.editor_terrain_tool_outer_radius;

    for(auto buf_to_destroy : per_frame_data.bufs_to_destroy)
    {
        destroyBuffer(*buf_to_destroy);
    }
    per_frame_data.bufs_to_destroy.clear();

    //TODO: synch here before resizing and updating buffers
    deviceWaitIdle();
    //TODO: set m_update_descriptors = true if we resized any buffers that we have descriptors for
    resizeBuffers();
    updateBuffers();

    if(m_update_descriptors)
    {
        deviceWaitIdle();

        destroyPipelines();
        destroyPipelineLayout();
        destroyDescriptorSets();

        createDescriptorSets();
        createPipelineLayout();
        createPipelines();
        updateDescriptorSets();

        m_update_descriptors = false;
    }

    std::vector<bool> cull(m_render_batch_count);

    /*--- frustum culling ---*/
    const auto view_frustum_planes = camera.viewFrustumPlanesW();

    for(uint32_t i = 0; i < m_render_batch_count; i++)
    {
        const RenderBatch& render_batch = m_render_batches[i];

        if(RenderMode::Default == render_batch.render_mode)
        {
            cull[i] = !render_batch.bounding_sphere->intersect(view_frustum_planes);
        }
        else
        {
            cull[i] = false;
        }
    }

    //update all dir shadow maps every frame as they depend on the camera and we can assume the camera will change every frame
    for(DirLightId i = 0; i < m_common_buffer_data.dir_light_count; i++)
    {
        if(m_dir_lights_valid[i] && m_dir_lights[i].shadow_map_count)
        {
            updateDirShadowMap(camera, m_dir_lights[i]);
        }
    }

    //only update point shadow maps for the point lights that have changed this frame
    for(PointLightId id : per_frame_data.point_lights_to_update)
    {
        if(m_point_lights_valid[id] && m_point_lights[id].shadow_map_res)
        {
            updatePointShadowMap(m_point_lights[id]);
        }
    }

    const size_t dir_shadow_map_data_size = MAX_DIR_SHADOW_MAP_PARTITIONS * m_dir_shadow_map_count * sizeof(DirShadowMapData);
    const size_t point_shadow_map_data_size = m_point_shadow_map_count * sizeof(PointShadowMapData);

    //dir shadow map data copy callback
    const auto copy_dir_shadow_map_data = [&](void* ptr)
    {
        uint8_t* dst = reinterpret_cast<uint8_t*>(ptr);
        std::memcpy(dst, m_dir_shadow_map_data.data(), dir_shadow_map_data_size);
    };

    //point shadow map data copy callback
    const auto copy_point_shadow_map_data = [&](void* ptr)
    {
        uint8_t* dst = reinterpret_cast<uint8_t*>(ptr);
        std::memcpy(dst, m_point_shadow_map_data.data(), point_shadow_map_data_size);
    };

    VkCommandBuffer cmd_buf = per_frame_data.cmd_buf;

    VkResult res = vkWaitForFences(m_device, 1, &per_frame_data.cmd_buf_ready_fence, VK_TRUE, UINT64_MAX);
    assertVkSuccess(res, "An error occured while waiting for a fence.");

    for(auto id : per_frame_data.dir_shadow_maps_to_destroy)
    {
        destroyDirShadowMap(per_frame_data.dir_shadow_maps[id]);

        if(m_frame_id == m_dir_shadow_map_free_id_frame_ids[id])
        {
            if(id == m_dir_shadow_map_count - 1)
            {
                m_dir_shadow_map_count--;
            }
            else
            {
                m_dir_shadow_maps_free_ids.push(id);
            }
        }
    }
    per_frame_data.dir_shadow_maps_to_destroy.clear();

    for(auto id : per_frame_data.point_shadow_maps_to_destroy)
    {
        destroyPointShadowMap(per_frame_data.point_shadow_maps[id]);

        if(m_frame_id == m_point_shadow_map_free_id_frame_ids[id])
        {
            if(id == m_point_shadow_map_count - 1)
            {
                m_point_shadow_map_count--;
            }
            else
            {
                m_point_shadow_maps_free_ids.push(id);
            }
        }
    }
    per_frame_data.point_shadow_maps_to_destroy.clear();

    uint32_t image_id;
    res = vkAcquireNextImageKHR(m_device, m_swapchain, 0, per_frame_data.image_acquire_semaphore, VK_NULL_HANDLE, &image_id);
    if(res != VK_SUCCESS)
    {
        switch(res)
        {
        case VK_TIMEOUT:
        case VK_NOT_READY:
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            return;
        default:
            throw std::runtime_error(errorMsg("Failed to aqcuire swapchain image.", res));
        }
    }

    /*--------------------- command recording begin ---------------------*/
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;

    res = vkBeginCommandBuffer(cmd_buf, &begin_info);
    assertVkSuccess(res, "An error occurred while begining a command buffer.");

    //TODO: group all the transfer barriers together and use them in a single call
    /*--- update buffers ---*/
    {
        VkBuffer common_buf = per_frame_data.common_buffer->buf;
        vkCmdUpdateBuffer(cmd_buf, common_buf, 0, sizeof(m_common_buffer_data), &m_common_buffer_data);
        VkBufferMemoryBarrier buf_mem_bar = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, NULL, 0, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, common_buf, 0, sizeof(m_common_buffer_data)};
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &buf_mem_bar, 0, NULL);
    }

    for(const auto& req : m_buffer_update_reqs)
    {
        auto buffer = req.first;

        VkBufferCopy buf_cpy_region{0, 0, buffer->size};
        vkCmdCopyBuffer(cmd_buf, buffer->transfer_buffer->buf, buffer->buf, 1, &buf_cpy_region);

        //TODO: add appropriate barriers here! barriers should depend on buffer type/usage so will have to figure something out here
        //might for instance store the relevant pipeline stages/accesses used in a barrier inside the VkBufferWrapper struct and generically
        //do barriers for reach buffer here
        //ALSO: the buffer transfer here will need to be synchronized between frames (with access to the buffers in the subsequent frames)
        //and so we'll need to add some synchronization code here, possibly even move the transfer to a separate command buffer
        //(in that case the barriers won't be necessary?)
    }
    m_buffer_update_reqs.clear();

    if(!per_frame_data.dir_lights_to_update.empty())
    {
        for(DirLightId id : per_frame_data.dir_lights_to_update)
        {
            vkCmdUpdateBuffer(cmd_buf, per_frame_data.dir_light_buffer->buf, id * sizeof(DirLightShaderData), sizeof(DirLightShaderData), &m_dir_lights[id]);
        }

        VkBufferMemoryBarrier buf_mem_bar = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, NULL, 0, VK_ACCESS_SHADER_READ_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, per_frame_data.dir_light_buffer->buf, 0, m_common_buffer_data.dir_light_count * sizeof(DirLightShaderData)};
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 1, &buf_mem_bar, 0, NULL);
    }

    if(!per_frame_data.point_lights_to_update.empty())
    {
        for(PointLightId id : per_frame_data.point_lights_to_update)
        {
            vkCmdUpdateBuffer(cmd_buf, per_frame_data.point_light_buffer->buf, id * sizeof(PointLightShaderData), sizeof(PointLightShaderData), &m_point_lights[id]);
        }

        VkBufferMemoryBarrier buf_mem_bar = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, NULL, 0, VK_ACCESS_SHADER_READ_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, per_frame_data.point_light_buffer->buf, 0, m_common_buffer_data.point_light_count * sizeof(PointLightShaderData)};
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 1, &buf_mem_bar, 0, NULL);
    }

    if(dir_shadow_map_data_size != 0)
    {
        VkBufferWrapper& shadow_map_buffer = *per_frame_data.dir_shadow_map_buffer;

        updateBuffer(shadow_map_buffer, dir_shadow_map_data_size, copy_dir_shadow_map_data, cmd_buf);
        VkBufferMemoryBarrier buf_mem_bar = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, NULL, 0, VK_ACCESS_SHADER_READ_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, shadow_map_buffer.buf, 0, dir_shadow_map_data_size};
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, 0, 0, NULL, 1, &buf_mem_bar, 0, NULL);
    }

    //TODO:only update point shadow maps for point lights that got updated this frame
    if(point_shadow_map_data_size != 0)
    {
        VkBufferWrapper& shadow_map_buffer = *per_frame_data.point_shadow_map_buffer;

        updateBuffer(shadow_map_buffer, point_shadow_map_data_size, copy_point_shadow_map_data, cmd_buf);
        VkBufferMemoryBarrier buf_mem_bar = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, NULL, 0, VK_ACCESS_SHADER_READ_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, shadow_map_buffer.buf, 0, point_shadow_map_data_size};
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, 0, 0, NULL, 1, &buf_mem_bar, 0, NULL);
    }

    //TODO: set some flag if we need to update light valid buffers if we actually change any lights
    if(m_common_buffer_data.dir_light_count != 0)
    {
        const VkDeviceSize data_size = roundUp(m_common_buffer_data.dir_light_count, 4u);

        vkCmdUpdateBuffer(cmd_buf, per_frame_data.dir_light_valid_buffer->buf, 0, data_size, m_dir_lights_valid.data());
        VkBufferMemoryBarrier buf_mem_bar = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, NULL, 0, VK_ACCESS_SHADER_READ_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, per_frame_data.dir_light_valid_buffer->buf, 0, data_size};
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, 0, 0, NULL, 1, &buf_mem_bar, 0, NULL);
    }

    if(m_common_buffer_data.point_light_count != 0)
    {
        const VkDeviceSize data_size = roundUp(m_common_buffer_data.point_light_count, 4u);

        vkCmdUpdateBuffer(cmd_buf, per_frame_data.point_light_valid_buffer->buf, 0, data_size, m_point_lights_valid.data());
        VkBufferMemoryBarrier buf_mem_bar = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, NULL, 0, VK_ACCESS_SHADER_READ_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, per_frame_data.point_light_valid_buffer->buf, 0, data_size};
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, 0, 0, NULL, 1, &buf_mem_bar, 0, NULL);
    }

    per_frame_data.dir_lights_to_update.clear();
    per_frame_data.point_lights_to_update.clear();

    /*bind vertex buffer*/
    const VkDeviceSize vb_offset = 0;
    vkCmdBindVertexBuffers(cmd_buf, 1, 1, &m_per_instance_vertex_buffer.buf, &vb_offset);

    /*bind descriptor sets*/
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &per_frame_data.descriptor_set, 0, NULL);

    /*shadow map rendering*/
    if(m_dir_shadow_map_count != 0)
    {
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[static_cast<uint32_t>(RenderMode::DirShadowMap)]);
        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_vertex_buffers[sizeof(VertexDefault)].buf, &vb_offset);

        uint32_t shadow_map_offset = 0;
        uint32_t prev_viewport_width = 0;
        uint32_t prev_viewport_height = 0;

        for(uint32_t dir_shadow_map_id = 0; dir_shadow_map_id < m_dir_shadow_map_count; dir_shadow_map_id++)
        {
            if(!m_dir_shadow_maps_valid[dir_shadow_map_id])
            {
                continue;
            }

            const auto& shadow_map = per_frame_data.dir_shadow_maps[dir_shadow_map_id];

            if((prev_viewport_width != shadow_map.res_x) || (prev_viewport_height != shadow_map.res_y))
            {
                VkViewport viewport;
                viewport.x = 0.0f;
                viewport.y = static_cast<float>(shadow_map.res_y);
                viewport.width = static_cast<float>(shadow_map.res_x);
                viewport.height = -static_cast<float>(shadow_map.res_y);
                viewport.maxDepth = 1.0f;
                viewport.minDepth = 0.0f;

                vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
            }

            push_const.shadow_map_count = shadow_map.count;
            push_const.shadow_map_offset = shadow_map_offset;
            vkCmdPushConstants(cmd_buf, m_pipeline_layout, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(push_const), &push_const);

            vkCmdBeginRenderPass(cmd_buf, &shadow_map.render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            for(uint32_t i = 0; i < m_render_batch_count; i++)
            {
                const auto& rb = m_render_batches[i];

                if(rb.render_mode == RenderMode::Default)
                {
                    vkCmdDraw(cmd_buf, rb.vertex_count, 1, rb.vertex_offset, rb.instance_id);
                }
            }

            vkCmdEndRenderPass(cmd_buf);

            //TODO: is the barrier necessary? or does the end of the render pass do what we want?
            VkImageSubresourceRange subres_range = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, shadow_map.count};
            VkImageMemoryBarrier img_mem_bar = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, shadow_map.depth_img.img, subres_range};
            vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &img_mem_bar);

            shadow_map_offset += shadow_map.count;
        }
    }

    if(m_point_shadow_map_count != 0)
    {
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[static_cast<uint32_t>(RenderMode::PointShadowMap)]);
        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_vertex_buffers[sizeof(VertexDefault)].buf, &vb_offset);

        uint32_t shadow_map_offset = 0;
        uint32_t prev_viewport_res = 0;

        //TODO: add frustum culling for point shadow map rendering?
        for(uint32_t point_shadow_map_id = 0; point_shadow_map_id < m_point_shadow_map_count; point_shadow_map_id++)
        {
            if(!m_point_shadow_maps_valid[point_shadow_map_id])
            {
                continue;
            }

            const auto& shadow_map = per_frame_data.point_shadow_maps[point_shadow_map_id];

            if(prev_viewport_res != shadow_map.res)
            {
                VkViewport viewport;
                viewport.x = 0.0f;
                viewport.y = static_cast<float>(shadow_map.res);
                viewport.width = static_cast<float>(shadow_map.res);
                viewport.height = -static_cast<float>(shadow_map.res);
                viewport.maxDepth = 1.0f;
                viewport.minDepth = 0.0f;

                vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
            }

            push_const.shadow_map_offset = shadow_map_offset;
            vkCmdPushConstants(cmd_buf, m_pipeline_layout, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(push_const), &push_const);

            vkCmdBeginRenderPass(cmd_buf, &shadow_map.render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            for(uint32_t i = 0; i < m_render_batch_count; i++)
            {
                const auto& rb = m_render_batches[i];

                if(rb.render_mode == RenderMode::Default)
                {
                    vkCmdDraw(cmd_buf, rb.vertex_count, 1, rb.vertex_offset, rb.instance_id);
                }
            }

            vkCmdEndRenderPass(cmd_buf);

            //TODO: is the barrier necessary? or does the end of the render pass do what we want?
            VkImageSubresourceRange subres_range = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 6};
            VkImageMemoryBarrier img_mem_bar = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, shadow_map.depth_img.img, subres_range};
            vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &img_mem_bar);

            shadow_map_offset ++;
        }
    }

    /*main render pass*/
    vkCmdBeginRenderPass(cmd_buf, &m_render_targets[image_id].render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    if(m_render_batch_count != 0)
    {
        for(uint32_t i = 0; i < m_render_batch_count; i++)
        {
            const auto& rb = m_render_batches[i];

            if(!cull[i])
            {
                vkCmdPushConstants(cmd_buf, m_pipeline_layout, push_const_ranges[0].stageFlags, 0, sizeof(push_const), &push_const);
                vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[static_cast<uint32_t>(rb.render_mode)]);
                vkCmdBindVertexBuffers(cmd_buf, 0, 1, &rb.vb->buf, &vb_offset);

                if(RenderMode::Ui == rb.render_mode || RenderMode::Font == rb.render_mode)
                {
                    VkRect2D scissor;

                    if(rb.scissor)
                    {
                        scissor.offset.x = static_cast<int>(std::clamp<float>(rb.scissor->x, 0, static_cast<float>(m_surface_width)));
                        scissor.offset.y = static_cast<int>(std::clamp<float>(rb.scissor->y, 0, static_cast<float>(m_surface_height)));
                        scissor.extent.width = static_cast<uint32_t>(std::clamp<float>(rb.scissor->width, 0, static_cast<float>(m_surface_width)));
                        scissor.extent.height = static_cast<uint32_t>(std::clamp<float>(rb.scissor->height, 0, static_cast<float>(m_surface_height)));
                    }
                    else
                    {
                        scissor.offset = {0, 0};
                        scissor.extent = {m_surface_width, m_surface_height};
                    }

                    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
                }

                vkCmdDraw(cmd_buf, rb.vertex_count, 1, rb.vertex_offset, rb.instance_id);
            }
        }

        m_render_batch_count = 0;
    }

    vkCmdEndRenderPass(cmd_buf);

    res = vkEndCommandBuffer(cmd_buf);
    assertVkSuccess(res, "Failed to record command buffer.");
    /*--------------------- command recording end ---------------------*/

    VkPipelineStageFlags submit_wait_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &per_frame_data.image_acquire_semaphore;
    submit_info.pWaitDstStageMask = &submit_wait_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &per_frame_data.rendering_finished_semaphore;

    res = vkResetFences(m_device, 1, &per_frame_data.cmd_buf_ready_fence);
    assertVkSuccess(res, "An error occurred while reseting fences.");

    res = vkQueueSubmit(m_queue, 1, &submit_info, per_frame_data.cmd_buf_ready_fence);
    assertVkSuccess(res, "Failed to submit commands.");

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &per_frame_data.rendering_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &m_swapchain;
    present_info.pImageIndices = &image_id;
    present_info.pResults = NULL;

    res = vkQueuePresentKHR(m_queue, &present_info);

    switch(res)
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        return;
    default:
        throw std::runtime_error(errorMsg("Failed to queue present.", res));
    }
}

void Engine3D::onWindowResize(uint32_t width, uint32_t height)
{
    createSwapchain(width, height);
}

void Engine3D::onSceneLoad(const SceneInitData& scene_init_data)
{
    //TODO: do this wait better (wait on fence or sth?)
    deviceWaitIdle();

    m_common_buffer_data.terrain_patch_size_x = scene_init_data.terrain_patch_size_x;
    m_common_buffer_data.terrain_patch_size_z = scene_init_data.terrain_patch_size_z;

    loadFonts(scene_init_data.fonts);

    createDescriptorSets();
    createPipelineLayout();
    createPipelines();
    updateDescriptorSets();
}

void Engine3D::loadFonts(const std::vector<const Font*>& fonts)
{
    m_font_count = static_cast<uint32_t>(fonts.size());
    m_font_textures.resize(fonts.size());

    VkResult res;

    VkImageCreateInfo img_create_info{};
    img_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_create_info.pNext = NULL;
    img_create_info.flags = 0;
    img_create_info.imageType = VK_IMAGE_TYPE_2D;
    img_create_info.format = VK_FORMAT_R8_UNORM;
    img_create_info.mipLevels = 1;
    img_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    img_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_create_info.queueFamilyIndexCount = 1;
    img_create_info.pQueueFamilyIndices = &m_queue_family_index;
    img_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo img_view_create_info{};
    img_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_create_info.pNext = NULL;
    img_view_create_info.flags = 0;
    img_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    img_view_create_info.format = img_create_info.format;
    img_view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    img_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;

    //TODO: create a seperate cmd buffer for transfer? or maybe wait for this one to be idle before using it???
    res = vkBeginCommandBuffer(m_transfer_cmd_buf, &begin_info);
    assertVkSuccess(res, "An error occurred while beginning the transfer command buffer.");

    std::vector<VkBufferWrapper> tex_buffers;

    for(auto& font : fonts)
    {
        const auto bitmaps = font->bitmaps();
        const uint32_t tex_width = font->texWidth();
        const uint32_t tex_height = font->texHeight();

        auto& tex_buf = tex_buffers.emplace_back(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        createBuffer(tex_buf, bitmaps.size() * tex_width * tex_height);

        /*having created the texture buffer and allocated and bound memory to it
        we now have to read raw image data and copy it into the buffer*/
        void* tex_buf_ptr;
        vkMapMemory(m_device, tex_buf.mem, 0, VK_WHOLE_SIZE, 0, &tex_buf_ptr);

        std::memcpy(tex_buf_ptr, bitmaps.data(), bitmaps.size());

        vkUnmapMemory(m_device, tex_buf.mem);

        const uint32_t layer_count = font->charCount();

        /*update vulkan create info and create an image for the texture*/
        Texture& texture = m_font_textures.back();
        texture.id = 0;

        img_create_info.extent = {tex_width, tex_height, 1};
        img_create_info.arrayLayers = layer_count;

        img_view_create_info.image = texture.image.img;
        img_view_create_info.subresourceRange.layerCount = img_create_info.arrayLayers;

        createImage(texture.image, img_create_info, img_view_create_info);

        VkImageSubresourceRange img_sub_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layer_count};

        VkImageMemoryBarrier img_mem_bar{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, texture.image.img, img_sub_range};

        vkCmdPipelineBarrier(m_transfer_cmd_buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, NULL, 0, NULL, 1, &img_mem_bar);

        VkBufferImageCopy buf_img_copy{};
        buf_img_copy.bufferOffset = 0;
        buf_img_copy.bufferRowLength = 0;
        buf_img_copy.bufferImageHeight = 0;

        buf_img_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buf_img_copy.imageSubresource.mipLevel = 0;
        buf_img_copy.imageSubresource.baseArrayLayer = 0;
        buf_img_copy.imageSubresource.layerCount = layer_count;

        buf_img_copy.imageOffset = {0, 0, 0};
        buf_img_copy.imageExtent = {tex_width, tex_height, 1};

        vkCmdCopyBufferToImage(m_transfer_cmd_buf, tex_buf.buf, texture.image.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buf_img_copy);

        img_mem_bar = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, texture.image.img, img_sub_range};

        vkCmdPipelineBarrier(m_transfer_cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, NULL, 0, NULL, 1, &img_mem_bar);
    }

    res = vkEndCommandBuffer(m_transfer_cmd_buf);
    assertVkSuccess(res, "An error occurred while ending the transfer command buffer.");

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_transfer_cmd_buf;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    res = vkResetFences(m_device, 1, &m_transfer_cmd_buf_fence);
    assertVkSuccess(res, "An error occurred while reseting transfer cmd buf fence.");

    res = vkQueueSubmit(m_queue, 1, &submit_info, m_transfer_cmd_buf_fence);
    assertVkSuccess(res, "An error occurred while submitting the transfer command buffer.");

    res = vkWaitForFences(m_device, 1, &m_transfer_cmd_buf_fence, VK_TRUE, UINT64_MAX);
    assertVkSuccess(res, "An error occured while waiting for a transfer cmd buf fence.");

    for(auto& tex_buf : tex_buffers)
    {
        destroyBuffer(tex_buf);
    }
}

void Engine3D::destroyTextures() noexcept
{
    for(auto& texture : m_textures.textures)
    {
        destroyImage(texture);
    }

    m_textures.clear();

    for(auto& normal_map : m_normal_maps.textures)
    {
        destroyImage(normal_map);
    }

    m_normal_maps.clear();
}

void Engine3D::destroyFontTextures() noexcept
{
    for(auto& font_texture : m_font_textures)
    {
        destroyImage(font_texture.image);
    }
}

void Engine3D::setSampleCount(VkSampleCountFlagBits sample_count)
{
    /*if the sample count doesn't change return immediately*/
    if(sample_count == m_sample_count)
    {
        return;
    }

    /*update the sample count member variable*/
    m_sample_count = sample_count;

    deviceWaitIdle();

    /*if the sample count changes we need to recreate the render pass,
    render targets, and the pipelines (if any already exist)*/
    destroyRenderTargets();
    destroyRenderPasses();

    createMainRenderPass();
    createRenderTargets();

    if(!m_pipelines.empty() && (m_pipelines[0] != VK_NULL_HANDLE))
    {
        destroyPipelines();
        createPipelines();
    }
}

bool Engine3D::enableVsync(bool vsync)
{
    if(!m_vsync_disable_support)
    {
        return false;
    }

    if(vsync != m_vsync)
    {
        m_vsync = vsync;

        createSwapchain(m_surface_width, m_surface_height);
    }

    return true;
}

uint32_t Engine3D::requestPerInstanceVertexBufferAllocation()
{
    const uint32_t instance_id = m_per_instance_vertex_buffer.req_size / sizeof(InstanceVertexData);
    m_per_instance_vertex_buffer.req_size += sizeof(InstanceVertexData);
    return instance_id;
}

uint32_t Engine3D::requestBoneTransformBufferAllocation(uint32_t bone_count)
{
    const uint32_t bone_transform_id = m_bone_transform_buffer.req_size / sizeof(mat4x4);
    m_bone_transform_buffer.req_size += bone_count * sizeof(mat4x4);
    return bone_transform_id;
}

void Engine3D::requestTerrainBufferAllocation(uint64_t size)
{
    m_terrain_buffer.req_size = size;
}

void Engine3D::freeVertexBufferAllocation(VertexBuffer* vb, uint64_t offset, uint64_t size)
{

}

void Engine3D::updateVertexData(VertexBuffer* vb, uint64_t data_offset, uint64_t data_size, const void* data)
{
    m_buffer_update_reqs[vb].emplace_back(data_offset, data_size, data);
}

void Engine3D::updatePerInstanceVertexData(uint32_t instance_id, const void* data)
{
    m_buffer_update_reqs[&m_per_instance_vertex_buffer].emplace_back(instance_id * sizeof(InstanceVertexData), sizeof(InstanceVertexData), data);
}

void Engine3D::updateBoneTransformData(uint32_t bone_offset, uint32_t bone_count, const mat4x4* data)
{
    m_buffer_update_reqs[&m_bone_transform_buffer].emplace_back(bone_offset * sizeof(mat4x4), bone_count * sizeof(mat4x4), data);
}

void Engine3D::updateTerrainData(void* data, uint64_t offset, uint64_t size)
{
    m_buffer_update_reqs[&m_terrain_buffer].emplace_back(offset, size, data);
}

void Engine3D::draw(RenderMode render_mode, VertexBuffer* vb, uint32_t vertex_offset, uint32_t vertex_count, uint32_t instance_id, std::optional<Sphere> bounding_sphere, std::optional<Quad> scissor)
{
    m_render_batches[m_render_batch_count] = RenderBatch(render_mode, vb, vertex_offset, vertex_count, instance_id, bounding_sphere, scissor);
    m_render_batch_count++;
}

DirLightId Engine3D::addDirLight(const DirLight& dir_light)
{
    DirLightId id;

    if(!m_dir_light_free_ids.empty())
    {
        id = m_dir_light_free_ids.front();
        m_dir_light_free_ids.pop();
    }
    else
    {
        id = m_common_buffer_data.dir_light_count;
        m_common_buffer_data.dir_light_count++;
    }

    m_dir_lights_valid[id] = 1;
    for(auto& per_frame_data : m_per_frame_data)
    {
        per_frame_data.dir_lights_to_update.push_back(id);
    }

    m_dir_lights[id] = dir_light;

    if(dir_light.shadow_map_count != 0)
    {
        m_dir_lights[id].shadow_map_id = createDirShadowMap(dir_light);
    }

    return id;
}

void Engine3D::updateDirLight(DirLightId id, const DirLight& dir_light)
{
    if((dir_light.shadow_map_count != m_dir_lights[id].shadow_map_count) ||
       (dir_light.shadow_map_res_x != m_dir_lights[id].shadow_map_res_x) ||
       (dir_light.shadow_map_res_y != m_dir_lights[id].shadow_map_res_y))
    {
        if(m_dir_lights[id].shadow_map_count != 0)
        {
            markDirShadowMapForDestroy(m_dir_lights[id].shadow_map_id);
        }

        if(dir_light.shadow_map_count != 0)
        {
            m_dir_lights[id].shadow_map_id = createDirShadowMap(dir_light);
        }
    }

    m_dir_lights[id] = dir_light;

    for(auto& per_frame_data : m_per_frame_data)
    {
        per_frame_data.dir_lights_to_update.push_back(id);
    }
}

void Engine3D::removeDirLight(DirLightId id)
{
    m_dir_lights_valid[id] = 0;

    if(m_dir_lights[id].shadow_map_count != 0)
    {
        markDirShadowMapForDestroy(m_dir_lights[id].shadow_map_id);
    }

    if(id == m_common_buffer_data.dir_light_count - 1)
    {
        m_common_buffer_data.dir_light_count--;
    }
    else
    {
        m_dir_light_free_ids.emplace(id);
    }
}

PointLightId Engine3D::addPointLight(const PointLight& point_light)
{
    PointLightId id;

    if(!m_point_light_free_ids.empty())
    {
        id = m_point_light_free_ids.front();
        m_point_light_free_ids.pop();
    }
    else
    {
        id = m_common_buffer_data.point_light_count;
        m_common_buffer_data.point_light_count++;
    }

    m_point_lights_valid[id] = 1;
    for(auto& per_frame_data : m_per_frame_data)
    {
        per_frame_data.point_lights_to_update.push_back(id);
    }

    m_point_lights[id] = point_light;

    if(point_light.shadow_map_res != 0)
    {
        m_point_lights[id].shadow_map_id = createPointShadowMap(point_light);
        updatePointShadowMap(m_point_lights[id]);
    }

    return id;
}

void Engine3D::updatePointLight(PointLightId id, const PointLight& point_light)
{
    if(point_light.shadow_map_res != m_point_lights[id].shadow_map_res)
    {
        if(m_point_lights[id].shadow_map_res != 0)
        {
            markPointShadowMapForDestroy(m_point_lights[id].shadow_map_id);
        }

        if(point_light.shadow_map_res != 0)
        {
            m_point_lights[id].shadow_map_id = createPointShadowMap(point_light);
        }
    }

    m_point_lights[id] = point_light;

    for(auto& per_frame_data : m_per_frame_data)
    {
        per_frame_data.point_lights_to_update.push_back(id);
    }

    //TODO: updating point shadow maps is pretty expensive
    //and it only has to be done if either the position or max_d of the light has changed
    //other changes don't require the shadow map update
    //maybe it would be worth checking if those specific values have changed and only then update the shadow map
    //this should be profiled in the future once we have specific use cases to check what's optimal
    if(m_point_lights[id].shadow_map_res != 0)
    {
        updatePointShadowMap(m_point_lights[id]);
    }
}

void Engine3D::removePointLight(PointLightId id)
{
    m_point_lights_valid[id] = 0;

    if(m_point_lights[id].shadow_map_res != 0)
    {
        markPointShadowMapForDestroy(m_point_lights[id].shadow_map_id);
    }

    if(id == m_common_buffer_data.point_light_count - 1)
    {
        m_common_buffer_data.point_light_count--;
    }
    else
    {
        m_point_light_free_ids.emplace(id);
    }
}

void Engine3D::updateDescriptorSets() noexcept
{
    std::vector<VkDescriptorBufferInfo> common_buf_infos(FRAMES_IN_FLIGHT);
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        common_buf_infos[i] = {m_per_frame_data[i].common_buffer->buf, 0, VK_WHOLE_SIZE};
    }

    std::vector<VkDescriptorImageInfo> tex_img_infos(m_textures.textures.size());
    for(size_t i = 0; i < m_textures.textures.size(); i++)
    {
        tex_img_infos[i] = {VK_NULL_HANDLE, m_textures.textures[i].img_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    std::vector<VkDescriptorImageInfo> font_img_infos(m_font_desc_count);
    for(size_t i = 0; i < m_font_desc_count; i++)
    {
        font_img_infos[i] = {VK_NULL_HANDLE, m_font_textures[i].image.img_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    std::vector<VkDescriptorBufferInfo> dir_light_buf_infos(FRAMES_IN_FLIGHT);
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        dir_light_buf_infos[i] = {m_per_frame_data[i].dir_light_buffer->buf, 0, VK_WHOLE_SIZE};
    }

    std::vector<VkDescriptorBufferInfo> point_light_buf_infos(FRAMES_IN_FLIGHT);
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        point_light_buf_infos[i] = {m_per_frame_data[i].point_light_buffer->buf, 0, VK_WHOLE_SIZE};
    }

    std::vector<VkDescriptorBufferInfo> dir_shadow_map_buf_infos(FRAMES_IN_FLIGHT);
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        dir_shadow_map_buf_infos[i] = {m_per_frame_data[i].dir_shadow_map_buffer->buf, 0, VK_WHOLE_SIZE};
    }

    std::vector<std::vector<VkDescriptorImageInfo>> dir_shadow_map_img_infos(FRAMES_IN_FLIGHT);
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        dir_shadow_map_img_infos[i].resize(m_dir_shadow_map_count);
        for(uint32_t j = 0; j < dir_shadow_map_img_infos[i].size(); j++)
        {
            dir_shadow_map_img_infos[i][j] = {VK_NULL_HANDLE, m_per_frame_data[i].dir_shadow_maps[j].depth_img.img_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        }
    }

    std::vector<VkDescriptorBufferInfo> point_shadow_map_buf_infos(FRAMES_IN_FLIGHT);
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        point_shadow_map_buf_infos[i] = {m_per_frame_data[i].point_shadow_map_buffer->buf, 0, VK_WHOLE_SIZE};
    }

    std::vector<std::vector<VkDescriptorImageInfo>> point_shadow_map_img_infos(FRAMES_IN_FLIGHT);
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        point_shadow_map_img_infos[i].resize(m_point_shadow_map_count);
        for(uint32_t j = 0; j < point_shadow_map_img_infos[i].size(); j++)
        {
            point_shadow_map_img_infos[i][j] = {VK_NULL_HANDLE, m_per_frame_data[i].point_shadow_maps[j].depth_img.img_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        }
    }

    std::vector<VkDescriptorImageInfo> normal_map_img_infos(m_normal_maps.textures.size());
    for(size_t i = 0; i < m_normal_maps.textures.size(); i++)
    {
        normal_map_img_infos[i] = {VK_NULL_HANDLE, m_normal_maps.textures[i].img_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    VkDescriptorBufferInfo terrain_buf_info = {m_terrain_buffer.buf, 0, m_terrain_buffer.size};
    VkDescriptorBufferInfo bone_transform_buf_info = {m_bone_transform_buffer.buf, 0, m_bone_transform_buffer.size};

    std::vector<VkWriteDescriptorSet> desc_set_writes;
    for(size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, COMMON_BUF_BINDING, 0, m_common_buf_desc_count, m_common_buf_desc_type, NULL, &common_buf_infos[i], NULL});

        const uint32_t valid_tex_desc_count = static_cast<uint32_t>(tex_img_infos.size());
        if(valid_tex_desc_count > 0)
        {
            desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, TEX_BINDING, 0, valid_tex_desc_count, m_tex_desc_type, tex_img_infos.data(), NULL, NULL});
        }

        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, FONT_BINDING,                0, m_font_desc_count,           m_font_desc_type, font_img_infos.data(), NULL, NULL});
        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, DIR_LIGHTS_BINDING,          0, m_dir_lights_desc_count,     m_dir_lights_desc_type, NULL, &dir_light_buf_infos[i], NULL});
        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, DIR_LIGHTS_VALID_BINDING,    0, m_dir_lights_valid_desc_count, m_dir_lights_valid_desc_type, NULL, NULL, &m_per_frame_data[i].dir_light_valid_buffer->buf_view});
        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, POINT_LIGHTS_BINDING,        0, m_point_lights_desc_count,   m_point_lights_desc_type, NULL, &point_light_buf_infos[i], NULL});
        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, POINT_LIGHTS_VALID_BINDING,  0, m_point_lights_valid_desc_count, m_point_lights_valid_desc_type, NULL, NULL, &m_per_frame_data[i].point_light_valid_buffer->buf_view});
        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, DIR_SM_BUF_BINDING,          0, m_dir_sm_buf_desc_count,     m_dir_sm_buf_desc_type, NULL, &dir_shadow_map_buf_infos[i], NULL});

        const uint32_t valid_dir_sm_desc_count = static_cast<uint32_t>(dir_shadow_map_img_infos[i].size());
        if(valid_dir_sm_desc_count > 0)
        {
            desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, DIR_SM_BINDING, 0, valid_dir_sm_desc_count, m_dir_sm_desc_type, dir_shadow_map_img_infos[i].data(), NULL, NULL});
        }

        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, POINT_SM_BUF_BINDING, 0, m_point_sm_buf_desc_count, m_point_sm_buf_desc_type, NULL, &point_shadow_map_buf_infos[i], NULL});

        const uint32_t valid_point_sm_desc_count = static_cast<uint32_t>(point_shadow_map_img_infos[i].size());
        if(valid_point_sm_desc_count > 0)
        {
            desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, POINT_SM_BINDING, 0, valid_point_sm_desc_count, m_point_sm_desc_type, point_shadow_map_img_infos[i].data(), NULL, NULL});
        }

        const uint32_t valid_normal_map_desc_count = static_cast<uint32_t>(normal_map_img_infos.size());
        if(valid_normal_map_desc_count > 0)
        {
            desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, NORMAL_MAP_BINDING, 0, valid_normal_map_desc_count, m_normal_map_desc_type, normal_map_img_infos.data(), NULL, NULL});
        }

        if(m_terrain_buffer.buf)
        {
            desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, TERRAIN_BINDING, 0, m_terrain_desc_count, m_terrain_desc_type, NULL, &terrain_buf_info, NULL});
        }
        desc_set_writes.emplace_back(VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, m_per_frame_data[i].descriptor_set, BONE_TRANSFORM_BUF_BINDING, 0, m_bone_transform_buf_desc_count, m_bone_transform_buf_desc_type, NULL, &bone_transform_buf_info, NULL});
    }

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(desc_set_writes.size()), desc_set_writes.data(), 0, NULL);
}

/*---------------- create methods ----------------*/

void Engine3D::createInstance(std::string_view app_name)
{
    const std::vector<const char*> req_layer_names =
    {
#if VALIDATION_ENABLE
        "VK_LAYER_KHRONOS_validation"
#endif
    };

    const std::vector<const char*> req_ext_names =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_PLATFORM_SURFACE_EXTENSION_NAME
#if VALIDATION_ENABLE
        , VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };

    checkIfLayersAndExtensionsAvailable(req_layer_names, req_ext_names);

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = app_name.data();
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "EngineName"; //TODO: set engine name
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pNext = NULL;
    ici.flags = 0;
    ici.pApplicationInfo = &app_info;
    ici.enabledLayerCount = static_cast<uint32_t>(req_layer_names.size());
    ici.ppEnabledLayerNames = req_layer_names.data();
    ici.enabledExtensionCount = static_cast<uint32_t>(req_ext_names.size());
    ici.ppEnabledExtensionNames = req_ext_names.data();

#if VALIDATION_ENABLE
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    create_info.pfnUserCallback = debugReportCallback;
    create_info.pUserData =NULL;

    ici.pNext = &create_info;
#endif

    VkResult res = vkCreateInstance(&ici, NULL, &m_instance);
    assertVkSuccess(res, "Failed to create VkInstance");

#if VALIDATION_ENABLE
    createDebugCallback(create_info);
#endif
}

void Engine3D::createDevice()
{
    VkResult res;

    /*first enumerate physical devices*/
    uint32_t count;

    res = vkEnumeratePhysicalDevices(m_instance, &count, NULL);
    assertVkSuccess(res, "Failed to enumerate physical devices.");

    std::vector<VkPhysicalDevice> physical_devices(count);

    res = vkEnumeratePhysicalDevices(m_instance, &count, physical_devices.data());
    assertVkSuccess(res, "Failed to enumerate physical devices.");

    /*look for a discrete gpu and choose it as the physical device to be used*/
    //@TODO: if a discrete gpu is not present, choose a different device
    for(VkPhysicalDevice& pd : physical_devices)
    {
        vkGetPhysicalDeviceProperties(pd, &m_physical_device_properties);

        if(m_physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            m_physical_device = pd;
            break;
        }
    }

    /*get the physical device's available features and properties*/
    m_physical_device_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_physical_device_12_features.pNext = NULL;

    VkPhysicalDeviceFeatures2 phy_dev_feat2{};
    phy_dev_feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    phy_dev_feat2.pNext = &m_physical_device_12_features;

    vkGetPhysicalDeviceFeatures2(m_physical_device, &phy_dev_feat2);

    /*only enable the required features, not all the supported ones*/
    //TODO: review features; remove geometry shader???
    if((phy_dev_feat2.features.geometryShader == VK_FALSE)
       || (phy_dev_feat2.features.sampleRateShading == VK_FALSE)
       || (phy_dev_feat2.features.shaderSampledImageArrayDynamicIndexing == VK_FALSE)
       || (phy_dev_feat2.features.fillModeNonSolid == VK_FALSE)
       || (phy_dev_feat2.features.samplerAnisotropy == VK_FALSE)
       || (phy_dev_feat2.features.tessellationShader == VK_FALSE)
       || (m_physical_device_12_features.descriptorBindingPartiallyBound == VK_FALSE))
    {
        throw std::runtime_error("Required physical device feature not supported.");
    }

    m_physical_device_features = {};
    m_physical_device_features.fillModeNonSolid = VK_TRUE;
    m_physical_device_features.geometryShader = VK_TRUE;
    m_physical_device_features.sampleRateShading = VK_TRUE;
    m_physical_device_features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
    m_physical_device_features.samplerAnisotropy = VK_TRUE;
    m_physical_device_features.tessellationShader = VK_TRUE;

    m_physical_device_12_features = {};
    m_physical_device_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_physical_device_12_features.pNext = NULL;
    m_physical_device_12_features.descriptorBindingPartiallyBound = VK_TRUE;

    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_physical_device_memory_properties);

    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &count, NULL);
    std::vector<VkQueueFamilyProperties> queue_family_properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &count, queue_family_properties.data());

    for(uint32_t i = 0; i < queue_family_properties.size(); i++)
    {
        if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
           m_queue_family_index = i;
           break;
        }
    }

    float queue_priorities = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pNext = NULL;
    queue_create_info.flags = 0;
    queue_create_info.queueFamilyIndex = m_queue_family_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priorities;

    std::vector<const char*> device_extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    //@TODO: check if extensions are available

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &m_physical_device_12_features;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();
    device_create_info.pEnabledFeatures = &m_physical_device_features;

    res = vkCreateDevice(m_physical_device, &device_create_info, NULL, &m_device);
    assertVkSuccess(res, "Failed to create device.");

    vkGetDeviceQueue(m_device, m_queue_family_index, 0, &m_queue);
}

void Engine3D::createSurface(const Window& window)
{
#ifdef PLATFORM_WIN32
    VkWin32SurfaceCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.hwnd = window.getParams().hwnd;
    create_info.hinstance = window.getParams().hinstance;

    VkResult res = vkCreateWin32SurfaceKHR(m_instance, &create_info, NULL, &m_surface);
    assertVkSuccess(res, "Failed to create Xcb surface.");
#elif defined(PLATFORM_XCB)
    VkXcbSurfaceCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.connection = window.getParams().connection;
    create_info.window = window.getParams().window;

    VkResult res = vkCreateXcbSurfaceKHR(m_instance, &create_info, NULL, &m_surface);
    assertVkSuccess(res, "Failed to create Xcb surface.");
#endif

    VkBool32 surface_support;
    res = vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, m_queue_family_index, m_surface, &surface_support);
    assertVkSuccess(res, "Failed to get physical device surface support.");

    if(surface_support == VK_FALSE)
    {
        throw std::runtime_error("Surface not supported by the physical device.");
    }
}

void Engine3D::createSwapchain(uint32_t width, uint32_t height)
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &surface_capabilities);
    assertVkSuccess(res, "Failed to get physical device surface capabilities.");

    uint32_t count = 0;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &count, NULL);
    assertVkSuccess(res, "Failed to get physical device surface formats.");
    std::vector<VkSurfaceFormatKHR> surface_formats(count);
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &count, surface_formats.data());
    assertVkSuccess(res, "Failed to get physical device surface formats.");

    res = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &count, NULL);
    assertVkSuccess(res, "Failed to get physical device surface present modes.");
    std::vector<VkPresentModeKHR> present_modes(count);
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &count, present_modes.data());
    assertVkSuccess(res, "Failed to get physical device surface present modes.");

    for(auto present_mode : present_modes)
    {
        if(present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            m_vsync_disable_support = true;
            break;
        }
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.surface = m_surface;
    create_info.minImageCount = surface_capabilities.maxImageCount > surface_capabilities.minImageCount ? surface_capabilities.minImageCount + 1 : surface_capabilities.maxImageCount;
    create_info.imageFormat = surface_formats[0].format;
    create_info.imageColorSpace = surface_formats[0].colorSpace;

    if((surface_capabilities.currentExtent.width != 0xffffffff) && (surface_capabilities.currentExtent.height != 0xffffffff))
    {
        create_info.imageExtent = surface_capabilities.currentExtent;
    }
    else
    {
        create_info.imageExtent.width = std::clamp(width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
        create_info.imageExtent.height = std::clamp(height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    }

    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &m_queue_family_index;
    create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = (!m_vsync && m_vsync_disable_support) ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = m_swapchain;

    VkSwapchainKHR new_swapchain = VK_NULL_HANDLE;

    res = vkCreateSwapchainKHR(m_device, &create_info, NULL, &new_swapchain);
    assertVkSuccess(res, "Failed to create swapchain.");

    deviceWaitIdle();

    destroySwapchain();

    m_swapchain = new_swapchain;

    m_surface_width = create_info.imageExtent.width;
    m_surface_height = create_info.imageExtent.height;
    m_swapchain_format = create_info.imageFormat;

    /*get new swapchain images*/
    res = vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, NULL);
    assertVkSuccess(res, "Failed to get swapchain images.");
    m_swapchain_images.resize(count);
    res = vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, m_swapchain_images.data());
    assertVkSuccess(res, "Failed to get swapchain images.");

    /*create image views for the new swapchain images*/
    VkImageViewCreateInfo iv_create_info{};
    iv_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv_create_info.pNext = NULL;
    iv_create_info.flags = 0;
    iv_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv_create_info.format = create_info.imageFormat;
    iv_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    iv_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    m_swapchain_image_views.resize(m_swapchain_images.size());
    for(size_t i = 0; i < m_swapchain_image_views.size(); i++)
    {
        iv_create_info.image = m_swapchain_images[i];
        res = vkCreateImageView(m_device, &iv_create_info, NULL, &m_swapchain_image_views[i]);
        assertVkSuccess(res, "Failed to create swapchain image views.");
    }

    /*recreate any existing objects that depend on swapchain*/
    if(!m_render_targets.empty())
    {
        destroyRenderTargets();
        createRenderTargets();
    }

    if((!m_pipelines.empty()) && (m_pipelines[0] != VK_NULL_HANDLE))
    {
        destroyPipelines();
        createPipelines();
    }
}

void Engine3D::createCommandPool()
{
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = NULL;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = m_queue_family_index;

    VkResult res = vkCreateCommandPool(m_device, &pool_create_info, NULL, &m_command_pool);
    assertVkSuccess(res, "Failed to create command pool.");
}

void Engine3D::createMainRenderPass()
{
    std::vector<VkAttachmentDescription> att_desc;

    VkAttachmentDescription color_att_desc{};
    color_att_desc.flags = 0;
    color_att_desc.format = m_swapchain_format;
    color_att_desc.samples = m_sample_count;
    color_att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_att_desc.storeOp = m_sample_count == VK_SAMPLE_COUNT_1_BIT ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //ignored
    color_att_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //ignored
    color_att_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_att_desc.finalLayout = m_sample_count == VK_SAMPLE_COUNT_1_BIT ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    att_desc.push_back(color_att_desc);

    VkAttachmentDescription depth_att_desc{};
    depth_att_desc.flags = 0;
    depth_att_desc.format = VK_FORMAT_D32_SFLOAT;
    depth_att_desc.samples = m_sample_count;
    depth_att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_att_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //ignored
    depth_att_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //ignored
    depth_att_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_att_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    att_desc.push_back(depth_att_desc);

    if(m_sample_count != VK_SAMPLE_COUNT_1_BIT)
    {
        VkAttachmentDescription resolve_att_desc{};
        resolve_att_desc.flags = 0;
        resolve_att_desc.format = m_swapchain_format;
        resolve_att_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        resolve_att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        resolve_att_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolve_att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //ignored
        resolve_att_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //ignored
        resolve_att_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolve_att_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        att_desc.push_back(resolve_att_desc);
    }

    VkAttachmentReference col_att_ref{};
    col_att_ref.attachment = 0;
    col_att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_att_ref{};
    depth_att_ref.attachment = 1;
    depth_att_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference res_att_ref{};
    res_att_ref.attachment = 2;
    res_att_ref.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    VkSubpassDescription subpass_desc{};
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = NULL;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &col_att_ref;
    subpass_desc.pResolveAttachments = m_sample_count == VK_SAMPLE_COUNT_1_BIT ? NULL : &res_att_ref;
    subpass_desc.pDepthStencilAttachment = &depth_att_ref;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.attachmentCount = static_cast<uint32_t>(att_desc.size());
    create_info.pAttachments = att_desc.data();
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass_desc;
    create_info.dependencyCount = 0;
    create_info.pDependencies = NULL;

    VkResult res = vkCreateRenderPass(m_device, &create_info, NULL, &m_main_render_pass);
    assertVkSuccess(res, "Failed to create render pass.");
#if VALIDATION_ENABLE
    setDebugObjectName(m_main_render_pass, "MainRenderPass");
#endif
}

void Engine3D::createShadowMapRenderPass()
{
    VkAttachmentDescription depth_att_desc{};
    depth_att_desc.flags = 0;
    depth_att_desc.format = VK_FORMAT_D16_UNORM;
    depth_att_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_att_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //ignored
    depth_att_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //ignored
    depth_att_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_att_desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depth_att_ref{};
    depth_att_ref.attachment = 0;
    depth_att_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc{};
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = NULL;
    subpass_desc.colorAttachmentCount = 0;
    subpass_desc.pColorAttachments = NULL;
    subpass_desc.pResolveAttachments = NULL;
    subpass_desc.pDepthStencilAttachment = &depth_att_ref;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.attachmentCount = 1;
    create_info.pAttachments = &depth_att_desc;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass_desc;
    create_info.dependencyCount = 0;
    create_info.pDependencies = NULL;

    VkResult res = vkCreateRenderPass(m_device, &create_info, NULL, &m_shadow_map_render_pass);
    assertVkSuccess(res, "Failed to create shadow map render pass.");
#if VALIDATION_ENABLE
    setDebugObjectName(m_main_render_pass, "ShadowMapRenderPass");
#endif
}

void Engine3D::createDescriptorSets()
{
    VkResult res;

    m_common_buf_desc_count = 1;
    m_tex_desc_count = std::max<uint32_t>(1, static_cast<uint32_t>(m_textures.textures.size()));
    m_normal_map_desc_count = std::max<uint32_t>(1, static_cast<uint32_t>(m_normal_maps.textures.size()));
    m_font_desc_count = static_cast<uint32_t>(m_font_textures.size());
    m_dir_lights_desc_count = 1;
    m_dir_lights_valid_desc_count = 1;
    m_point_lights_desc_count = 1;
    m_point_lights_valid_desc_count = 1;
    m_dir_sm_buf_desc_count = 1;
    //TODO: should we immediately create descriptors for max possible shadow maps or start low and scale up if we need to?
    m_dir_sm_desc_count = std::max<uint32_t>(1, static_cast<uint32_t>(MAX_DIR_SHADOW_MAP_COUNT));
    m_point_sm_buf_desc_count = 1;
    //TODO: should we immediately create descriptors for max possible shadow maps or start low and scale up if we need to?
    m_point_sm_desc_count = std::max<uint32_t>(1, static_cast<uint32_t>(MAX_POINT_SHADOW_MAP_COUNT));
    m_bone_transform_buf_desc_count = 1;
    m_terrain_desc_count = 1;

    std::vector<VkSampler> tex_samplers(m_tex_desc_count, m_sampler);
    std::vector<VkSampler> font_samplers(m_font_desc_count, m_sampler);
    std::vector<VkSampler> dir_shadow_map_samplers(m_dir_sm_desc_count, m_shadow_map_sampler);
    //TODO: which sampler should be used for point shadow maps?
    std::vector<VkSampler> point_shadow_map_samplers(m_point_sm_desc_count, m_shadow_map_sampler);
    std::vector<VkSampler> normal_map_samplers(m_normal_map_desc_count, m_sampler);

    std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings =
    {
          {COMMON_BUF_BINDING, m_common_buf_desc_type, m_common_buf_desc_count, VK_SHADER_STAGE_ALL_GRAPHICS, NULL} //common buffer
        , {TEX_BINDING, m_tex_desc_type, m_tex_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, tex_samplers.data()} //textures
        , {NORMAL_MAP_BINDING, m_normal_map_desc_type, m_normal_map_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, normal_map_samplers.data()} //normal_maps
        , {FONT_BINDING, m_font_desc_type, m_font_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, font_samplers.data()} //font textures
        , {DIR_LIGHTS_BINDING, m_dir_lights_desc_type, m_dir_lights_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, NULL} // directional lights
        , {DIR_LIGHTS_VALID_BINDING, m_dir_lights_valid_desc_type, m_dir_lights_valid_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, NULL} // directional lights valid
        , {POINT_LIGHTS_BINDING, m_point_lights_desc_type, m_point_lights_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, NULL} // point lights
        , {POINT_LIGHTS_VALID_BINDING, m_point_lights_valid_desc_type, m_point_lights_valid_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, NULL} // point lights valid
        , {DIR_SM_BUF_BINDING, m_dir_sm_buf_desc_type, m_dir_sm_buf_desc_count, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL} // dir shadow map data
        , {DIR_SM_BINDING, m_dir_sm_desc_type, m_dir_sm_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, dir_shadow_map_samplers.data()} // dir shadow maps
        , {POINT_SM_BUF_BINDING, m_point_sm_buf_desc_type, m_point_sm_buf_desc_count, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL} // point shadow map data
        , {POINT_SM_BINDING, m_point_sm_desc_type, m_point_sm_desc_count, VK_SHADER_STAGE_FRAGMENT_BIT, point_shadow_map_samplers.data()} // point shadow maps
        , {BONE_TRANSFORM_BUF_BINDING, m_bone_transform_buf_desc_type, m_bone_transform_buf_desc_count, VK_SHADER_STAGE_VERTEX_BIT, NULL} // bone transform buffer
        , {TERRAIN_BINDING, m_terrain_desc_type, m_terrain_desc_count, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, NULL} // terrain per vertex data
    };

    std::vector<VkDescriptorBindingFlags> desc_binding_flags =
    {
        0,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        0,
        0,
        0,
        0,
        0,
        0,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        0,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        0,
        0
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo desc_set_layout_binding_flags{};
    desc_set_layout_binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    desc_set_layout_binding_flags.pNext = NULL;
    desc_set_layout_binding_flags.bindingCount = static_cast<uint32_t>(desc_binding_flags.size());
    desc_set_layout_binding_flags.pBindingFlags = desc_binding_flags.data();

    VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info{};
    desc_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    desc_set_layout_create_info.pNext = &desc_set_layout_binding_flags;
    desc_set_layout_create_info.flags = 0;
    desc_set_layout_create_info.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
    desc_set_layout_create_info.pBindings = desc_set_layout_bindings.data();

    res = vkCreateDescriptorSetLayout(m_device, &desc_set_layout_create_info, NULL, &m_descriptor_set_layout);
    assertVkSuccess(res, "Failed to create descriptor set layout.");

    //TODO: figure out how these sizes work, I can put 1 and 1 here and it still works...
    std::vector<VkDescriptorPoolSize> desc_pool_sizes =
    {
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (m_tex_desc_count + m_font_desc_count + m_dir_sm_desc_count + m_point_sm_desc_count + m_normal_map_desc_count) * FRAMES_IN_FLIGHT}
        , {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (m_common_buf_desc_count + m_dir_lights_desc_count + m_point_lights_desc_count + m_dir_sm_buf_desc_count + m_point_sm_buf_desc_count) * FRAMES_IN_FLIGHT}
        , {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, (m_dir_lights_valid_desc_count + m_point_lights_valid_desc_count) * FRAMES_IN_FLIGHT}
        , {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_terrain_desc_count * FRAMES_IN_FLIGHT + m_bone_transform_buf_desc_count}
    };

    VkDescriptorPoolCreateInfo desc_pool_create_info{};
    desc_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_create_info.pNext = NULL;
    desc_pool_create_info.flags = 0;
    desc_pool_create_info.maxSets = FRAMES_IN_FLIGHT;
    desc_pool_create_info.poolSizeCount = static_cast<uint32_t>(desc_pool_sizes.size());
    desc_pool_create_info.pPoolSizes = desc_pool_sizes.data();

    res = vkCreateDescriptorPool(m_device, &desc_pool_create_info, NULL, &m_descriptor_pool);
    assertVkSuccess(res, "Failed to create descriptor pool.");

    /*allocate a descriptor set*/
    std::vector<VkDescriptorSetLayout> desc_set_layouts(FRAMES_IN_FLIGHT, m_descriptor_set_layout);

    VkDescriptorSetAllocateInfo desc_set_allocate_info{};
    desc_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_set_allocate_info.pNext = NULL;
    desc_set_allocate_info.descriptorPool = m_descriptor_pool;
    desc_set_allocate_info.descriptorSetCount = static_cast<uint32_t>(desc_set_layouts.size());
    desc_set_allocate_info.pSetLayouts = desc_set_layouts.data();

    std::vector<VkDescriptorSet> descriptor_sets(FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    res = vkAllocateDescriptorSets(m_device, &desc_set_allocate_info, descriptor_sets.data());
    assertVkSuccess(res, "Failed to allocate descriptor sets.");

    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_per_frame_data[i].descriptor_set = descriptor_sets[i];
#if VALIDATION_ENABLE
        setDebugObjectName(m_per_frame_data[i].descriptor_set, "DescriptorSet_" + std::to_string(i));
#endif
    }
}

void Engine3D::createPipelineLayout()
{
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = NULL;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &m_descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_const_ranges.size());
    pipeline_layout_create_info.pPushConstantRanges = push_const_ranges.data();

    VkResult res = vkCreatePipelineLayout(m_device, &pipeline_layout_create_info, NULL, &m_pipeline_layout);
    assertVkSuccess(res, "Failed to create pipeline layout.");
}

void Engine3D::createPipelines()
{
    m_pipelines.resize(static_cast<size_t>(RENDER_MODE_COUNT));

    /*--- Ui ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        VkVertexInputBindingDescription vertex_binding_desc{};
        vertex_binding_desc.binding = 0;
        vertex_binding_desc.stride = sizeof(VertexQuad);
        vertex_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_desc;
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_quad_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_quad_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        /*----------------------- viewport state -----------------------*/
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_surface_width);
        viewport.height = static_cast<float>(m_surface_height);
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_surface_width, m_surface_height};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 0;
        rasterization_state_create_info.lineWidth = 1.0f;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = m_sample_count;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_FALSE;
        depth_stencil_state.depthWriteEnable = VK_FALSE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_NEVER;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 0;

        /*----------------------- color blend state -----------------------*/
        VkPipelineColorBlendAttachmentState color_blend_att_state{};
        color_blend_att_state.blendEnable = VK_TRUE;
        color_blend_att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = NULL;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_att_state;
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;

        /*------------------------- dynamic state -------------------------*/
        VkDynamicState dynamic_state = VK_DYNAMIC_STATE_SCISSOR;
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 1;
        dynamic_state_create_info.pDynamicStates = &dynamic_state;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_main_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        uint32_t tex_count = static_cast<uint32_t>(m_textures.textures.size());
        VkSpecializationMapEntry spec_info_map_entry{};
        spec_info_map_entry.constantID = 0;
        spec_info_map_entry.offset = 0;
        spec_info_map_entry.size = sizeof(tex_count);

        VkSpecializationInfo spec_info{};
        spec_info.pData = &tex_count;
        spec_info.dataSize = sizeof(tex_count);
        spec_info.pMapEntries = &spec_info_map_entry;
        spec_info.mapEntryCount = 1;

        std::vector<VkShaderModule> shader_modules(3, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_QUAD_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(GS_UI_FILENAME, VK_SHADER_STAGE_GEOMETRY_BIT, &shader_modules[1]));
        shader_stage_infos.emplace_back(loadShader(FS_UI_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[2], &spec_info));
#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "UiVS");
        setDebugObjectName(shader_modules[1], "UiGS");
        setDebugObjectName(shader_modules[2], "UiFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::Ui)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::Ui)], "PipelineUi");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- Font ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        VkVertexInputBindingDescription vertex_binding_desc{};
        vertex_binding_desc.binding = 0;
        vertex_binding_desc.stride = sizeof(VertexQuad);
        vertex_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_desc;
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_quad_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_quad_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        /*----------------------- viewport state -----------------------*/
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_surface_width);
        viewport.height = static_cast<float>(m_surface_height);
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_surface_width, m_surface_height};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 0;
        rasterization_state_create_info.lineWidth = 1.0f;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = m_sample_count;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_FALSE;
        depth_stencil_state.depthWriteEnable = VK_FALSE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_NEVER;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 0;

        /*----------------------- color blend state -----------------------*/
        VkPipelineColorBlendAttachmentState color_blend_att_state{};
        color_blend_att_state.blendEnable = VK_TRUE;
        color_blend_att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = NULL;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_att_state;
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;

        /*------------------------- dynamic state -------------------------*/
        VkDynamicState dynamic_state = VK_DYNAMIC_STATE_SCISSOR;
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 1;
        dynamic_state_create_info.pDynamicStates = &dynamic_state;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_main_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        VkSpecializationMapEntry spec_info_map_entry{};
        spec_info_map_entry.constantID = 0;
        spec_info_map_entry.offset = 0;
        spec_info_map_entry.size = sizeof(uint32_t);

        VkSpecializationInfo spec_info{};
        spec_info.pData = &m_font_count;
        spec_info.dataSize = sizeof(uint32_t);
        spec_info.pMapEntries = &spec_info_map_entry;
        spec_info.mapEntryCount = 1;

        std::vector<VkShaderModule> shader_modules(3, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_QUAD_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(GS_UI_FILENAME, VK_SHADER_STAGE_GEOMETRY_BIT, &shader_modules[1]));
        shader_stage_infos.emplace_back(loadShader(FS_FONT_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[2], &spec_info));
#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "FontVS");
        setDebugObjectName(shader_modules[1], "FontGS");
        setDebugObjectName(shader_modules[2], "FontFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::Font)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::Font)], "PipelineFont");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- Default ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        std::vector<VkVertexInputBindingDescription> vertex_binding_desc =
        {
            {0, sizeof(VertexDefault), VK_VERTEX_INPUT_RATE_VERTEX},
            {1, sizeof(InstanceVertexData), VK_VERTEX_INPUT_RATE_INSTANCE}
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_desc.size());
        vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_desc.data();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_default_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_default_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        /*----------------------- viewport state -----------------------*/
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(m_surface_height);
        viewport.width = static_cast<float>(m_surface_width);
        viewport.height = -static_cast<float>(m_surface_height);
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_surface_width, m_surface_height};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 0;
        rasterization_state_create_info.lineWidth = 1.0;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = m_sample_count;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 1000.0f;

        /*----------------------- color blend state -----------------------*/
        VkPipelineColorBlendAttachmentState color_blend_att_state{};
        color_blend_att_state.blendEnable = VK_FALSE;
        color_blend_att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = NULL;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_att_state;
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;

        /*------------------------- dynamic state -------------------------*/
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 0;
        dynamic_state_create_info.pDynamicStates = NULL;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_main_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        std::vector<uint32_t> spec_data =
        {
            static_cast<uint32_t>(m_tex_desc_count),
            static_cast<uint32_t>(m_normal_map_desc_count),
            static_cast<uint32_t>(m_dir_sm_desc_count),
            static_cast<uint32_t>(m_point_sm_desc_count)
        };

        std::vector<VkSpecializationMapEntry> spec_info_map_entries =
        {
            {0, 0, sizeof(uint32_t)},
            {1, sizeof(uint32_t), sizeof(uint32_t)},
            {2, 2 * sizeof(uint32_t), sizeof(uint32_t)},
            {3, 3 * sizeof(uint32_t), sizeof(uint32_t)},
        };

        VkSpecializationInfo spec_info{};
        spec_info.pData = spec_data.data();
        spec_info.dataSize = spec_data.size() * sizeof(uint32_t);
        spec_info.pMapEntries = spec_info_map_entries.data();
        spec_info.mapEntryCount = static_cast<uint32_t>(spec_info_map_entries.size());

        std::vector<VkShaderModule> shader_modules(2, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_DEFAULT_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(FS_DEFAULT_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[1], &spec_info));
#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "DefaultVS");
        setDebugObjectName(shader_modules[1], "DefaultFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::Default)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::Default)], "PipelineDefault");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- Terrain ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        std::vector<VkVertexInputBindingDescription> vertex_binding_desc =
        {
            {0, sizeof(VertexTerrain), VK_VERTEX_INPUT_RATE_INSTANCE},
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_desc.size());
        vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_desc.data();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_terrain_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_terrain_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        /*----------------------- viewport state -----------------------*/
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(m_surface_height);
        viewport.width = static_cast<float>(m_surface_width);
        viewport.height = -static_cast<float>(m_surface_height);
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_surface_width, m_surface_height};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 0;
        rasterization_state_create_info.lineWidth = 1.0;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = m_sample_count;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 1000.0f;

        /*----------------------- color blend state -----------------------*/
        VkPipelineColorBlendAttachmentState color_blend_att_state{};
        color_blend_att_state.blendEnable = VK_FALSE;
        color_blend_att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = NULL;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_att_state;
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;

        /*------------------------- dynamic state -------------------------*/
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 0;
        dynamic_state_create_info.pDynamicStates = NULL;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_main_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        std::vector<uint32_t> spec_data =
        {
            static_cast<uint32_t>(m_tex_desc_count),
            static_cast<uint32_t>(m_normal_map_desc_count),
            static_cast<uint32_t>(m_dir_sm_desc_count),
            static_cast<uint32_t>(m_point_sm_desc_count)
        };

        std::vector<VkSpecializationMapEntry> spec_info_map_entries =
        {
            {0, 0, sizeof(uint32_t)},
            {1, sizeof(uint32_t), sizeof(uint32_t)},
            {2, 2*sizeof(uint32_t), sizeof(uint32_t)},
            {3, 3*sizeof(uint32_t), sizeof(uint32_t)},
        };

        VkSpecializationInfo spec_info{};
        spec_info.pData = spec_data.data();
        spec_info.dataSize = spec_data.size() * sizeof(uint32_t);
        spec_info.pMapEntries = spec_info_map_entries.data();
        spec_info.mapEntryCount = static_cast<uint32_t>(spec_info_map_entries.size());

        std::vector<VkShaderModule> shader_modules(2, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_TERRAIN_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(FS_TERRAIN_EDITOR_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[1], &spec_info));

#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "TerrainVS");
        setDebugObjectName(shader_modules[1], "TerrainFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::Terrain)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::Terrain)], "PipelineTerrain");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- Terrain Wireframe ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        std::vector<VkVertexInputBindingDescription> vertex_binding_desc =
        {
            {0, sizeof(VertexTerrain), VK_VERTEX_INPUT_RATE_INSTANCE},
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_desc.size());
        vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_desc.data();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_terrain_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_terrain_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        /*----------------------- viewport state -----------------------*/
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(m_surface_height);
        viewport.width = static_cast<float>(m_surface_width);
        viewport.height = -static_cast<float>(m_surface_height);
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_surface_width, m_surface_height};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_LINE;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 0;
        rasterization_state_create_info.lineWidth = 1.0;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = m_sample_count;
        multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 1000.0f;

        /*----------------------- color blend state -----------------------*/
        VkPipelineColorBlendAttachmentState color_blend_att_state{};
        color_blend_att_state.blendEnable = VK_FALSE;
        color_blend_att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = NULL;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_att_state;
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;

        /*------------------------- dynamic state -------------------------*/
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 0;
        dynamic_state_create_info.pDynamicStates = NULL;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_main_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        std::vector<VkShaderModule> shader_modules(2, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_TERRAIN_WIREFRAME_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(FS_COLOR_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[1]));

#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "TerrainWireframeVS");
        setDebugObjectName(shader_modules[1], "TerrainWireframeFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::TerrainWireframe)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::TerrainWireframe)], "PipelineTerrainWireframe");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- Sky ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        std::vector<VkVertexInputBindingDescription> vertex_binding_desc =
        {
            //TODO: sky should probably use a different vertex type
            {0, sizeof(VertexDefault), VK_VERTEX_INPUT_RATE_VERTEX},
            {1, sizeof(InstanceVertexData), VK_VERTEX_INPUT_RATE_INSTANCE}
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_desc.size());
        vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_desc.data();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_default_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_default_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        /*----------------------- viewport state -----------------------*/
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(m_surface_height);
        viewport.width = static_cast<float>(m_surface_width);
        viewport.height = -static_cast<float>(m_surface_height);
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_surface_width, m_surface_height};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 0;
        rasterization_state_create_info.lineWidth = 1.0;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = m_sample_count;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 1000.0f;

        /*----------------------- color blend state -----------------------*/
        VkPipelineColorBlendAttachmentState color_blend_att_state{};
        color_blend_att_state.blendEnable = VK_FALSE;
        color_blend_att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = NULL;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_att_state;
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;

        /*------------------------- dynamic state -------------------------*/
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 0;
        dynamic_state_create_info.pDynamicStates = NULL;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_main_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        std::vector<VkShaderModule> shader_modules(2, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_SKY_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(FS_SKY_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[1]));
#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "SkyVS");
        setDebugObjectName(shader_modules[1], "SkyFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::Sky)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::Sky)], "PipelineSky");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- DirShadowMap ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        std::vector<VkVertexInputBindingDescription> vertex_binding_desc =
        {
            {0, sizeof(VertexDefault), VK_VERTEX_INPUT_RATE_VERTEX},
            {1, sizeof(InstanceVertexData), VK_VERTEX_INPUT_RATE_INSTANCE}
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_desc.size());
        vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_desc.data();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_default_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_default_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        /*----------------------- viewport state -----------------------*/
        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_physical_device_properties.limits.maxViewportDimensions[0],
                           m_physical_device_properties.limits.maxViewportDimensions[1]};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = NULL; //will be set dynamically
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_TRUE;
        rasterization_state_create_info.depthBiasConstantFactor = 1.0f;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 5.0f;
        rasterization_state_create_info.lineWidth = 1.0;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 1000.0f;

        /*------------------------- dynamic state -------------------------*/
        VkDynamicState dynamic_state = VK_DYNAMIC_STATE_VIEWPORT;
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 1;
        dynamic_state_create_info.pDynamicStates = &dynamic_state;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = NULL;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_shadow_map_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        std::vector<VkShaderModule> shader_modules(3, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_SHADOWMAP_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(GS_DIR_SHADOW_MAP_FILENAME, VK_SHADER_STAGE_GEOMETRY_BIT, &shader_modules[1]));
        shader_stage_infos.emplace_back(loadShader(FS_DIR_SHADOW_MAP_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[2]));
#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "DirShadowMapVS");
        setDebugObjectName(shader_modules[1], "DirShadowMapGS");
        setDebugObjectName(shader_modules[2], "DirShadowMapFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::DirShadowMap)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::DirShadowMap)], "PipelineDirShadowMap");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- PointShadowMap ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        std::vector<VkVertexInputBindingDescription> vertex_binding_desc =
        {
            {0, sizeof(VertexDefault), VK_VERTEX_INPUT_RATE_VERTEX},
            {1, sizeof(InstanceVertexData), VK_VERTEX_INPUT_RATE_INSTANCE}
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_desc.size());
        vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_desc.data();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_default_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_default_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        /*----------------------- viewport state -----------------------*/
        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_physical_device_properties.limits.maxViewportDimensions[0],
                           m_physical_device_properties.limits.maxViewportDimensions[1]};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = NULL; //will be set dynamically
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 1.0f;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 5.0f;
        rasterization_state_create_info.lineWidth = 1.0;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 1000.0f;

        /*------------------------- dynamic state -------------------------*/
        VkDynamicState dynamic_state = VK_DYNAMIC_STATE_VIEWPORT;
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 1;
        dynamic_state_create_info.pDynamicStates = &dynamic_state;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = NULL;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_shadow_map_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        std::vector<VkShaderModule> shader_modules(3, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_SHADOWMAP_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(GS_POINT_SHADOW_MAP_FILENAME, VK_SHADER_STAGE_GEOMETRY_BIT, &shader_modules[1]));
        shader_stage_infos.emplace_back(loadShader(FS_POINT_SHADOW_MAP_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[2]));
#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "PointShadowMapVS");
        setDebugObjectName(shader_modules[1], "PointShadowMapGS");
        setDebugObjectName(shader_modules[2], "PointShadowMapFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::PointShadowMap)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::PointShadowMap)], "PipelinePointShadowMap");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- Highlight ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        std::vector<VkVertexInputBindingDescription> vertex_binding_desc =
        {
            {0, sizeof(VertexDefault), VK_VERTEX_INPUT_RATE_VERTEX},
            {1, sizeof(InstanceVertexData), VK_VERTEX_INPUT_RATE_INSTANCE}
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_desc.size());
        vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_desc.data();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_default_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_default_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        /*----------------------- viewport state -----------------------*/
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(m_surface_height);
        viewport.width = static_cast<float>(m_surface_width);
        viewport.height = -static_cast<float>(m_surface_height);
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_surface_width, m_surface_height};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 0;
        rasterization_state_create_info.lineWidth = 1.0;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = m_sample_count;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_FALSE;
        depth_stencil_state.depthWriteEnable = VK_FALSE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 0;

        /*----------------------- color blend state -----------------------*/
        VkPipelineColorBlendAttachmentState color_blend_att_state{};
        color_blend_att_state.blendEnable = VK_TRUE;
        color_blend_att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = NULL;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_att_state;
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;

        /*------------------------- dynamic state -------------------------*/
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 0;
        dynamic_state_create_info.pDynamicStates = NULL;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_main_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        std::vector<VkShaderModule> shader_modules(2, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_HIGHLIGHT_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(FS_COLOR_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[1]));
#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "HighlightVS");
        setDebugObjectName(shader_modules[1], "HighlightFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::Highlight)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::Highlight)], "PipelineHighlight");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }

    /*--- Billboard ---*/
    {
        /*----------------------- vertex input state -----------------------*/
        VkVertexInputBindingDescription vertex_binding_desc{};
        vertex_binding_desc.binding = 0;
        vertex_binding_desc.stride = sizeof(VertexQuad);
        vertex_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = NULL;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_desc;
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_quad_attr_desc.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_quad_attr_desc.data();

        /*----------------------- input assembly state -----------------------*/
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
        input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_create_info.pNext = NULL;
        input_assembly_state_create_info.flags = 0;
        input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        /*----------------------- viewport state -----------------------*/
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(m_surface_height);
        viewport.width = static_cast<float>(m_surface_width);
        viewport.height = -static_cast<float>(m_surface_height);
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors;
        scissors.offset = {0, 0};
        scissors.extent = {m_surface_width, m_surface_height};

        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = NULL;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissors;

        /*----------------------- rasterization state -----------------------*/
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = NULL;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0;
        rasterization_state_create_info.depthBiasClamp = 0;
        rasterization_state_create_info.depthBiasSlopeFactor = 0;
        rasterization_state_create_info.lineWidth = 1.0f;

        /*----------------------- multisample state -----------------------*/
        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = NULL;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = m_sample_count;
        multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        multisample_state_create_info.minSampleShading = 0.0;
        multisample_state_create_info.pSampleMask = NULL;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        /*---------------------- depth stencil state ----------------------*/
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = NULL;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_FALSE;
        depth_stencil_state.depthWriteEnable = VK_FALSE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_NEVER;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0;
        depth_stencil_state.maxDepthBounds = 0;

        /*----------------------- color blend state -----------------------*/
        VkPipelineColorBlendAttachmentState color_blend_att_state{};
        color_blend_att_state.blendEnable = VK_TRUE;
        color_blend_att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = NULL;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_att_state;
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;

        /*------------------------- dynamic state -------------------------*/
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = NULL;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 0;
        dynamic_state_create_info.pDynamicStates = NULL;

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = NULL;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = NULL;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = m_pipeline_layout;
        pipeline_create_info.renderPass = m_main_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        /*---------------------------- shaders ----------------------------*/
        uint32_t tex_count = static_cast<uint32_t>(m_textures.textures.size());
        VkSpecializationMapEntry spec_info_map_entry{};
        spec_info_map_entry.constantID = 0;
        spec_info_map_entry.offset = 0;
        spec_info_map_entry.size = sizeof(tex_count);

        VkSpecializationInfo spec_info{};
        spec_info.pData = &tex_count;
        spec_info.dataSize = sizeof(tex_count);
        spec_info.pMapEntries = &spec_info_map_entry;
        spec_info.mapEntryCount = 1;

        std::vector<VkShaderModule> shader_modules(3, VK_NULL_HANDLE);
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

        shader_stage_infos.emplace_back(loadShader(VS_QUAD_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, &shader_modules[0]));
        shader_stage_infos.emplace_back(loadShader(GS_BILLBOARD_FILENAME, VK_SHADER_STAGE_GEOMETRY_BIT, &shader_modules[1]));
        shader_stage_infos.emplace_back(loadShader(FS_UI_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, &shader_modules[2], &spec_info));
#if VALIDATION_ENABLE
        setDebugObjectName(shader_modules[0], "BillboardVS");
        setDebugObjectName(shader_modules[1], "BillboardGS");
        setDebugObjectName(shader_modules[2], "BillboardFS");
#endif

        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_infos.size());
        pipeline_create_info.pStages = shader_stage_infos.data();

        VkResult res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &m_pipelines[static_cast<size_t>(RenderMode::Billboard)]);
#if VALIDATION_ENABLE
        setDebugObjectName(m_pipelines[static_cast<size_t>(RenderMode::Billboard)], "PipelineBillboard");
#endif

        for(auto& sm : shader_modules)
        {
            vkDestroyShaderModule(m_device, sm, NULL);
        }

        assertVkSuccess(res, "Failed to create graphics pipeline.");
    }
}

void Engine3D::createCommandBuffers()
{
    VkCommandBufferAllocateInfo cmd_buf_alloc_info{};
    cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_alloc_info.pNext = NULL;
    cmd_buf_alloc_info.commandPool = m_command_pool;
    cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_alloc_info.commandBufferCount = FRAMES_IN_FLIGHT;

    std::vector<VkCommandBuffer> command_buffers(FRAMES_IN_FLIGHT);
    VkResult res = vkAllocateCommandBuffers(m_device, &cmd_buf_alloc_info, command_buffers.data());
    assertVkSuccess(res, "Failed to allocate command buffers.");
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_per_frame_data[i].cmd_buf = command_buffers[i];
#if VALIDATION_ENABLE
        setDebugObjectName(m_per_frame_data[i].cmd_buf, "CmdBuf_" + std::to_string(i));
#endif
    }

    cmd_buf_alloc_info.commandBufferCount = 1;
    res = vkAllocateCommandBuffers(m_device, &cmd_buf_alloc_info, &m_transfer_cmd_buf);
    assertVkSuccess(res, "Failed to allocate transfer command buffer.");
#if VALIDATION_ENABLE
    setDebugObjectName(m_transfer_cmd_buf, "TransferCmdBuf");
#endif
}

void Engine3D::createSynchronizationPrimitives()
{
    VkResult res;

    VkSemaphoreCreateInfo sem_create_info{};
    sem_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_create_info.pNext = NULL;
    sem_create_info.flags = 0;

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        res = vkCreateFence(m_device, &fence_create_info, NULL, &m_per_frame_data[i].cmd_buf_ready_fence);
        assertVkSuccess(res, "Failed to create fence.");

        res = vkCreateSemaphore(m_device, &sem_create_info, NULL, &m_per_frame_data[i].image_acquire_semaphore);
        assertVkSuccess(res, "Failed to create semaphore.");

        res = vkCreateSemaphore(m_device, &sem_create_info, NULL, &m_per_frame_data[i].rendering_finished_semaphore);
        assertVkSuccess(res, "Failed to create semaphore.");

#if VALIDATION_ENABLE
        setDebugObjectName(m_per_frame_data[i].cmd_buf_ready_fence, "CmdBufReadyFence_" + std::to_string(i));
        setDebugObjectName(m_per_frame_data[i].image_acquire_semaphore, "ImgAcquireSemaphore_" + std::to_string(i));
        setDebugObjectName(m_per_frame_data[i].rendering_finished_semaphore, "RenderFinishedSemaphore_" + std::to_string(i));
#endif
    }

    fence_create_info.flags = 0;
    res = vkCreateFence(m_device, &fence_create_info, NULL, &m_transfer_cmd_buf_fence);
    assertVkSuccess(res, "Failed to create fence.");
#if VALIDATION_ENABLE
    setDebugObjectName(m_transfer_cmd_buf_fence, "TransferCmdBufFence");
#endif
}

void Engine3D::createRenderTargets()
{
    VkResult res;

    VkFramebufferCreateInfo framebuffer_create_info{};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = NULL;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = m_main_render_pass;
    framebuffer_create_info.attachmentCount = (m_sample_count == VK_SAMPLE_COUNT_1_BIT) ? 2 : 3;
    framebuffer_create_info.width = m_surface_width;
    framebuffer_create_info.height = m_surface_height;
    framebuffer_create_info.layers = 1;

    /*image and image view for color attachment (multisampling only)*/
    VkImageCreateInfo img_create_info{};
    img_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_create_info.pNext = NULL;
    img_create_info.flags = 0;
    img_create_info.imageType = VK_IMAGE_TYPE_2D;
    img_create_info.format = m_swapchain_format;
    img_create_info.extent = {m_surface_width, m_surface_height, 1};
    img_create_info.mipLevels = 1;
    img_create_info.arrayLayers = 1;
    img_create_info.samples = m_sample_count;
    img_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    img_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_create_info.queueFamilyIndexCount = 1;
    img_create_info.pQueueFamilyIndices = &m_queue_family_index;
    img_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo img_view_create_info{};
    img_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_create_info.pNext = NULL;
    img_view_create_info.flags = 0;
    img_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    img_view_create_info.format = img_create_info.format;
    img_view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    img_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    /*image and image view for depth attachment*/
    VkImageCreateInfo depth_img_create_info{};
    depth_img_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_img_create_info.pNext = NULL;
    depth_img_create_info.flags = 0;
    depth_img_create_info.imageType = VK_IMAGE_TYPE_2D;
    depth_img_create_info.format = VK_FORMAT_D32_SFLOAT;
    depth_img_create_info.extent = {m_surface_width, m_surface_height, 1};
    depth_img_create_info.mipLevels = 1;
    depth_img_create_info.arrayLayers = 1;
    depth_img_create_info.samples = m_sample_count;
    depth_img_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_img_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_img_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depth_img_create_info.queueFamilyIndexCount = 1;
    depth_img_create_info.pQueueFamilyIndices = &m_queue_family_index;
    depth_img_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo depth_img_view_create_info{};
    depth_img_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_img_view_create_info.pNext = NULL;
    depth_img_view_create_info.flags = 0;
    depth_img_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_img_view_create_info.format = depth_img_create_info.format;
    depth_img_view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    depth_img_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    std::vector<VkImageView> attatchments(framebuffer_create_info.attachmentCount, VK_NULL_HANDLE);

    m_render_targets.resize(m_swapchain_images.size());
    m_render_target_clear_values.resize(framebuffer_create_info.attachmentCount);

    VkClearValue depth_clear_value{};
    depth_clear_value.depthStencil.depth = 1.0f;

    m_render_target_clear_values[0] = {};
    m_render_target_clear_values[1] = depth_clear_value;

    if(m_sample_count != VK_SAMPLE_COUNT_1_BIT)
    {
        m_render_target_clear_values[2] = {};
    }

    for(size_t i = 0; i < m_render_targets.size(); i++)
    {
        m_render_targets[i].depth_img = createImage(depth_img_create_info, depth_img_view_create_info);
#if VALIDATION_ENABLE
        setDebugObjectName(m_render_targets[i].depth_img.img, "MainDepthImg_" + std::to_string(i));
        setDebugObjectName(m_render_targets[i].depth_img.img_view, "MainDepthImgView_" + std::to_string(i));
        setDebugObjectName(m_render_targets[i].depth_img.mem, "MainDepthImgMem_" + std::to_string(i));
#endif

        if(m_sample_count != VK_SAMPLE_COUNT_1_BIT)
        {
            m_render_targets[i].color_img = createImage(img_create_info, img_view_create_info);
#if VALIDATION_ENABLE
        setDebugObjectName(m_render_targets[i].color_img.img, "MainColorImg_" + std::to_string(i));
        setDebugObjectName(m_render_targets[i].color_img.img_view, "MainColorImgView_" + std::to_string(i));
        setDebugObjectName(m_render_targets[i].color_img.mem, "MainColorImgMem_" + std::to_string(i));
#endif

            attatchments[0] = m_render_targets[i].color_img.img_view;
            attatchments[1] = m_render_targets[i].depth_img.img_view;
            attatchments[2] = m_swapchain_image_views[i];
        }
        else
        {
            attatchments[0] = m_swapchain_image_views[i];
            attatchments[1] = m_render_targets[i].depth_img.img_view;
        }

        framebuffer_create_info.pAttachments = attatchments.data();

        res = vkCreateFramebuffer(m_device, &framebuffer_create_info, NULL, &m_render_targets[i].framebuffer);
        assertVkSuccess(res, "Failed to create framebuffer.");
#if VALIDATION_ENABLE
        setDebugObjectName(m_render_targets[i].framebuffer, "MainFramebuffer_" + std::to_string(i));
#endif

        m_render_targets[i].render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        m_render_targets[i].render_pass_begin_info.pNext = NULL;
        m_render_targets[i].render_pass_begin_info.renderPass = m_main_render_pass;
        m_render_targets[i].render_pass_begin_info.framebuffer = m_render_targets[i].framebuffer;
        m_render_targets[i].render_pass_begin_info.renderArea.extent = {m_surface_width, m_surface_height};
        m_render_targets[i].render_pass_begin_info.renderArea.offset = {0, 0};
        m_render_targets[i].render_pass_begin_info.clearValueCount = static_cast<uint32_t>(m_render_target_clear_values.size());
        m_render_targets[i].render_pass_begin_info.pClearValues = m_render_target_clear_values.data();
    }
}

void Engine3D::createSamplers()
{
    VkSamplerCreateInfo sampler_create_info{};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = NULL;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16.0f;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    VkResult res = vkCreateSampler(m_device, &sampler_create_info, NULL, &m_sampler);
    assertVkSuccess(res, "Failed to create sampler.");
#if VALIDATION_ENABLE
    setDebugObjectName(m_sampler, "MainSampler");
#endif

    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = NULL;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = VK_FILTER_NEAREST;
    sampler_create_info.minFilter = VK_FILTER_NEAREST;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.maxAnisotropy = 0.0f;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 1.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    res = vkCreateSampler(m_device, &sampler_create_info, NULL, &m_shadow_map_sampler);
    assertVkSuccess(res, "Failed to create shadow map sampler.");
#if VALIDATION_ENABLE
    setDebugObjectName(m_shadow_map_sampler, "ShadowMapSampler");
#endif
}

void Engine3D::createBuffers()
{
    //create buffers
    for(size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_per_frame_data[i].common_buffer = std::make_unique<VkBufferWrapper>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
        createBuffer(*m_per_frame_data[i].common_buffer , sizeof(m_common_buffer_data));

        m_per_frame_data[i].dir_light_buffer = std::make_unique<VkBufferWrapper>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
        createBuffer(*m_per_frame_data[i].dir_light_buffer , SMALL_BUFFER_SIZE);

        m_per_frame_data[i].dir_light_valid_buffer = std::make_unique<VkBufferWrapper>(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false, VK_FORMAT_R8_UINT);
        createBuffer(*m_per_frame_data[i].dir_light_valid_buffer , MAX_DIR_LIGHT_COUNT);

        m_per_frame_data[i].point_light_buffer = std::make_unique<VkBufferWrapper>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
        createBuffer(*m_per_frame_data[i].point_light_buffer , SMALL_BUFFER_SIZE);

        m_per_frame_data[i].point_light_valid_buffer = std::make_unique<VkBufferWrapper>(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false, VK_FORMAT_R8_UINT);
        createBuffer(*m_per_frame_data[i].point_light_valid_buffer , MAX_POINT_LIGHT_COUNT);

        m_per_frame_data[i].dir_shadow_map_buffer = std::make_unique<VkBufferWrapper>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
        createBuffer(*m_per_frame_data[i].dir_shadow_map_buffer , SMALL_BUFFER_SIZE);

        m_per_frame_data[i].point_shadow_map_buffer = std::make_unique<VkBufferWrapper>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
        createBuffer(*m_per_frame_data[i].point_shadow_map_buffer , SMALL_BUFFER_SIZE);

        //TODO: when buffers are later destroyed and created anew when they need to be resized, we lose these debug names
        //should find a way to make sure we can set the debug names even after we recreate them later
#if VALIDATION_ENABLE
        setDebugObjectName(m_per_frame_data[i].common_buffer->buf, "CommonBuffer_" + std::to_string(i));
        setDebugObjectName(m_per_frame_data[i].dir_light_buffer->buf, "DitLightBuffer_" + std::to_string(i));
        setDebugObjectName(m_per_frame_data[i].dir_light_valid_buffer->buf, "DirLightValidBuffer_" + std::to_string(i));
        setDebugObjectName(m_per_frame_data[i].point_light_buffer->buf, "PointLightBuffer_" + std::to_string(i));
        setDebugObjectName(m_per_frame_data[i].point_light_valid_buffer->buf, "PointLightValidBuffer_" + std::to_string(i));
        setDebugObjectName(m_per_frame_data[i].dir_shadow_map_buffer->buf, "DirShadowMapBuffer_" + std::to_string(i));
        setDebugObjectName(m_per_frame_data[i].point_shadow_map_buffer->buf, "PointShadowMapBuffer_" + std::to_string(i));
#endif
    }

#if VALIDATION_ENABLE
//        setDebugObjectName(m_terrain_buffer->buf, "TerrainBuffer");
#endif

    //initialize buffers
    std::vector<uint8_t> zero_data(std::max(m_per_frame_data[0].dir_light_valid_buffer->size, m_per_frame_data[0].point_light_valid_buffer->size), 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;

    VkResult res = vkBeginCommandBuffer(m_transfer_cmd_buf, &begin_info);
    assertVkSuccess(res, "An error occurred while begining the transfer command buffer.");

    for(size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        vkCmdUpdateBuffer(m_transfer_cmd_buf, m_per_frame_data[i].dir_light_valid_buffer->buf, 0, m_per_frame_data[i].dir_light_valid_buffer->size, zero_data.data());
        vkCmdUpdateBuffer(m_transfer_cmd_buf, m_per_frame_data[i].point_light_valid_buffer->buf, 0, m_per_frame_data[i].point_light_valid_buffer->size, zero_data.data());
    }

    //TODO: this is only here because we force all 3d object vertices to have a bone transform currently, even if they have no animation
    //for that reason we put an identity matrix at index 0 in the buffer for all non-animated objects to use
    //we should later separate animated and non-animated objects and have separate vertex types for them, but for now this trick will do
    {
        const auto identity = glm::identity<mat4x4>();
        m_bone_transform_buffer.req_size = sizeof(mat4x4);
        createBuffer(m_bone_transform_buffer, sizeof(mat4x4), true);
        void* data;
        vkMapMemory(m_device, m_bone_transform_buffer.transfer_buffer->mem, 0, sizeof(mat4x4), 0, &data);
        std::memcpy(data, &identity, sizeof(mat4x4));
        vkUnmapMemory(m_device, m_bone_transform_buffer.transfer_buffer->mem);
        vkCmdUpdateBuffer(m_transfer_cmd_buf, m_bone_transform_buffer.buf, 0, sizeof(mat4x4), &identity);
    }

    vkEndCommandBuffer(m_transfer_cmd_buf);
    assertVkSuccess(res, "An error occurred while ending the transfer command buffer.");

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_transfer_cmd_buf;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    res = vkResetFences(m_device, 1, &m_transfer_cmd_buf_fence);
    assertVkSuccess(res, "An error occurred while reseting transfer cmd buf fence.");

    res = vkQueueSubmit(m_queue, 1, &submit_info, m_transfer_cmd_buf_fence);
    assertVkSuccess(res, "An error occurred while submitting the transfer command buffer.");

    res = vkWaitForFences(m_device, 1, &m_transfer_cmd_buf_fence, VK_TRUE, UINT64_MAX);
    assertVkSuccess(res, "An error occured while waiting for a transfer cmd buf fence.");
}

uint32_t Engine3D::createDirShadowMap(const DirLight& light)
{
    //TODO: only do this check in debug build?
    if(light.shadow_map_count > MAX_DIR_SHADOW_MAP_PARTITIONS)
    {
        std::stringstream ss;
        ss << "shadow_map_count of directional light = " << light.shadow_map_count << " . Max = " << MAX_DIR_SHADOW_MAP_PARTITIONS;
        throw std::runtime_error(ss.str());
    }

    VkFramebufferCreateInfo framebuffer_create_info{};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = NULL;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = m_shadow_map_render_pass;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.width = light.shadow_map_res_x;
    framebuffer_create_info.height = light.shadow_map_res_y;
    framebuffer_create_info.layers = light.shadow_map_count;

    VkImageCreateInfo depth_img_create_info{};
    depth_img_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_img_create_info.pNext = NULL;
    depth_img_create_info.flags = 0;
    depth_img_create_info.imageType = VK_IMAGE_TYPE_2D;
    depth_img_create_info.format = VK_FORMAT_D16_UNORM;
    depth_img_create_info.extent = {light.shadow_map_res_x, light.shadow_map_res_y, 1};
    depth_img_create_info.mipLevels = 1;
    depth_img_create_info.arrayLayers = light.shadow_map_count;
    depth_img_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_img_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_img_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    depth_img_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depth_img_create_info.queueFamilyIndexCount = 1;
    depth_img_create_info.pQueueFamilyIndices = &m_queue_family_index;
    depth_img_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo depth_img_view_create_info{};
    depth_img_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_img_view_create_info.pNext = NULL;
    depth_img_view_create_info.flags = 0;
    depth_img_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    depth_img_view_create_info.format = depth_img_create_info.format;
    depth_img_view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    depth_img_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, light.shadow_map_count};

    uint32_t shadow_map_id = 0;

    if(m_dir_shadow_maps_free_ids.empty())
    {
        shadow_map_id = m_dir_shadow_map_count;
        m_dir_shadow_map_count++;
    }
    else
    {
        shadow_map_id = m_dir_shadow_maps_free_ids.front();
        m_dir_shadow_maps_free_ids.pop();
    }

    m_update_descriptors = true;

    for(auto& per_frame_data : m_per_frame_data)
    {
        DirShadowMap& shadow_map = per_frame_data.dir_shadow_maps[shadow_map_id];

        shadow_map.count = light.shadow_map_count;
        shadow_map.res_x = light.shadow_map_res_x;
        shadow_map.res_y = light.shadow_map_res_y;

        shadow_map.depth_img = createImage(depth_img_create_info, depth_img_view_create_info);

        framebuffer_create_info.pAttachments = &shadow_map.depth_img.img_view;

        VkResult res = vkCreateFramebuffer(m_device, &framebuffer_create_info, NULL, &shadow_map.framebuffer);
        assertVkSuccess(res, "Failed to create shadow map framebuffer.");

        shadow_map.render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        shadow_map.render_pass_begin_info.pNext = NULL;
        shadow_map.render_pass_begin_info.renderPass = m_shadow_map_render_pass;
        shadow_map.render_pass_begin_info.framebuffer = shadow_map.framebuffer;
        shadow_map.render_pass_begin_info.renderArea.extent = {light.shadow_map_res_x, light.shadow_map_res_y};
        shadow_map.render_pass_begin_info.renderArea.offset = {0, 0};
        shadow_map.render_pass_begin_info.clearValueCount = 1;
        shadow_map.render_pass_begin_info.pClearValues = &m_shadow_map_clear_value;
    }

#if VALIDATION_ENABLE
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        setDebugObjectName(m_per_frame_data[i].dir_shadow_maps[shadow_map_id].framebuffer, "DirShadowMapFramebuffer_" + std::to_string(i) + "_" + std::to_string(shadow_map_id));
        setDebugObjectName(m_per_frame_data[i].dir_shadow_maps[shadow_map_id].depth_img.img, "DirShadowMapImg_" + std::to_string(i) + "_" + std::to_string(shadow_map_id));
        setDebugObjectName(m_per_frame_data[i].dir_shadow_maps[shadow_map_id].depth_img.img_view, "DirShadowMapImgView_" + std::to_string(i) + "_" + std::to_string(shadow_map_id));
        setDebugObjectName(m_per_frame_data[i].dir_shadow_maps[shadow_map_id].depth_img.mem, "DirShadowMapImgMem_" + std::to_string(i) + "_" + std::to_string(shadow_map_id));
    }
#endif

    m_dir_shadow_maps_valid[shadow_map_id] = true;

    return shadow_map_id;
}

void Engine3D::updateDirShadowMap(Camera& camera, const DirLightShaderData& light)
{
    auto& shadow_map_data = m_dir_shadow_map_data[light.shadow_map_id];

    const std::array<vec3, 8> view_frustum_points = camera.viewFrustumPointsW();

    const uint32_t shadow_map_count = light.shadow_map_count;
    std::vector<std::array<vec3, 4>> frustum_walls(shadow_map_count - 1);

    for(uint32_t i = 0; i < shadow_map_count; i++)
    {
        const vec3 light_pos = -1000.0f * light.dir;

        const quat q = normalize(glm::rotation(light.dir, vec3(0.0f, 0.0f, 1.0f)));
        const auto world_to_light = mat4_cast(q) * translate(-light_pos);

        const vec3* near_wall;

        if(0 == i)
        {
            near_wall = view_frustum_points.data();
        }
        else
        {
            near_wall = frustum_walls[i - 1].data();
        }

        vec3 bb_min = world_to_light * vec4(near_wall[0], 1.0f);
        vec3 bb_max = bb_min;

        for(uint32_t j = 1; j < 4; j++)
        {
            const vec3 p = world_to_light * vec4(near_wall[j], 1.0f);

            bb_min = min(bb_min, p);
            bb_max = max(bb_max, p);
        }

        const vec3* far_wall;

        if(shadow_map_count - 1 == i)
        {
            far_wall = &view_frustum_points[4];
            shadow_map_data[i].z = camera.far();
        }
        else
        {
            const float lambda = 0.95f;
            const float i_over_N = static_cast<float>(i + 1) / static_cast<float>(shadow_map_count);
            const float z = lambda * camera.near() * std::pow(camera.far() / camera.near(), i_over_N) + (1.0f - lambda) * (camera.near() + i_over_N * (camera.far() - camera.near()));
            const float d = (z - camera.near()) / (camera.far() - camera.near());

            for(uint32_t j = 0; j < 4; j++)
            {
                frustum_walls[i][j] = (1.0f - d) * view_frustum_points[j] + d * view_frustum_points[4 + j];
            }

            far_wall = frustum_walls[i].data();
            shadow_map_data[i].z = z;
        }

        for(uint32_t j = 0; j < 4; j++)
        {
            const vec3 p = world_to_light * vec4(far_wall[j], 1.0f);

            bb_min = min(bb_min, p);
            bb_max = max(bb_max, p);
        }

        const auto orto = glm::orthoLH_ZO(bb_min.x, bb_max.x, bb_min.y, bb_max.y, 0.0f, bb_max.z);

        shadow_map_data[i].P = orto * world_to_light;

        constexpr mat4x4 to_tex_coords = mat4x4(0.5f, 0.0f, 0.0f, 0.0f,
                                            0.0f, -0.5f, 0.0f, 0.0f,
                                            0.0f, 0.0f, 1.0f, 0.0f,
                                            0.5f, 0.5f, 0.0f, 1.0f);

        shadow_map_data[i].tex_P = to_tex_coords * shadow_map_data[i].P;
    }
}

uint32_t Engine3D::createPointShadowMap(const PointLight& light)
{
    VkFramebufferCreateInfo framebuffer_create_info{};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = NULL;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = m_shadow_map_render_pass;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.width = light.shadow_map_res;
    framebuffer_create_info.height = light.shadow_map_res;
    framebuffer_create_info.layers = 6;

    VkImageCreateInfo depth_img_create_info{};
    depth_img_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_img_create_info.pNext = NULL;
    depth_img_create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    depth_img_create_info.imageType = VK_IMAGE_TYPE_2D;
    depth_img_create_info.format = VK_FORMAT_D16_UNORM;
    depth_img_create_info.extent = {light.shadow_map_res, light.shadow_map_res, 1};
    depth_img_create_info.mipLevels = 1;
    depth_img_create_info.arrayLayers = 6;
    depth_img_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_img_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_img_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    depth_img_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depth_img_create_info.queueFamilyIndexCount = 1;
    depth_img_create_info.pQueueFamilyIndices = &m_queue_family_index;
    depth_img_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo depth_img_view_create_info{};
    depth_img_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_img_view_create_info.pNext = NULL;
    depth_img_view_create_info.flags = 0;
    depth_img_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    depth_img_view_create_info.format = depth_img_create_info.format;
    depth_img_view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    depth_img_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 6};

    uint32_t shadow_map_id = 0;

    if(m_point_shadow_maps_free_ids.empty())
    {
        shadow_map_id = m_point_shadow_map_count;
        m_point_shadow_map_count++;
    }
    else
    {
        shadow_map_id = m_point_shadow_maps_free_ids.front();
        m_point_shadow_maps_free_ids.pop();
    }

    m_update_descriptors = true;

    for(auto& per_frame_data : m_per_frame_data)
    {
        PointShadowMap& shadow_map = per_frame_data.point_shadow_maps[shadow_map_id];

        shadow_map.res = light.shadow_map_res;

        shadow_map.depth_img = createImage(depth_img_create_info, depth_img_view_create_info);

        framebuffer_create_info.pAttachments = &shadow_map.depth_img.img_view;

        VkResult res = vkCreateFramebuffer(m_device, &framebuffer_create_info, NULL, &shadow_map.framebuffer);
        assertVkSuccess(res, "Failed to create shadow map framebuffer.");

        shadow_map.render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        shadow_map.render_pass_begin_info.pNext = NULL;
        shadow_map.render_pass_begin_info.renderPass = m_shadow_map_render_pass;
        shadow_map.render_pass_begin_info.framebuffer = shadow_map.framebuffer;
        shadow_map.render_pass_begin_info.renderArea.extent = {light.shadow_map_res, light.shadow_map_res};
        shadow_map.render_pass_begin_info.renderArea.offset = {0, 0};
        shadow_map.render_pass_begin_info.clearValueCount = 1;
        shadow_map.render_pass_begin_info.pClearValues = &m_shadow_map_clear_value;
    }

#if VALIDATION_ENABLE
    for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        setDebugObjectName(m_per_frame_data[i].point_shadow_maps[shadow_map_id].framebuffer, "PointShadowMapFramebuffer_" + std::to_string(i) + "_" + std::to_string(shadow_map_id));
        setDebugObjectName(m_per_frame_data[i].point_shadow_maps[shadow_map_id].depth_img.img, "PointShadowMapImg_" + std::to_string(i) + "_" + std::to_string(shadow_map_id));
        setDebugObjectName(m_per_frame_data[i].point_shadow_maps[shadow_map_id].depth_img.img_view, "PointShadowMapImgView_" + std::to_string(i) + "_" + std::to_string(shadow_map_id));
        setDebugObjectName(m_per_frame_data[i].point_shadow_maps[shadow_map_id].depth_img.mem, "PointShadowMapImgMem_" + std::to_string(i) + "_" + std::to_string(shadow_map_id));
    }
#endif

    m_point_shadow_maps_valid[shadow_map_id] = true;

    return shadow_map_id;
}

void Engine3D::markDirShadowMapForDestroy(uint32_t id)
{
    for(auto& per_frame_data : m_per_frame_data)
    {
        per_frame_data.dir_shadow_maps_to_destroy.push_back(id);
        m_dir_shadow_map_free_id_frame_ids[id] = m_frame_id;
        m_dir_shadow_maps_valid[id] = false;
    }
}

void Engine3D::markPointShadowMapForDestroy(uint32_t id)
{
    for(auto& per_frame_data : m_per_frame_data)
    {
        per_frame_data.point_shadow_maps_to_destroy.push_back(id);
        m_point_shadow_map_free_id_frame_ids[id] = m_frame_id;
        m_point_shadow_maps_valid[id] = false;
    }
}

void Engine3D::destroyDirShadowMap(DirShadowMap& shadow_map)
{
    destroyImage(shadow_map.depth_img);
    vkDestroyFramebuffer(m_device, shadow_map.framebuffer, NULL);

    shadow_map.framebuffer = VK_NULL_HANDLE;
}

void Engine3D::destroyPointShadowMap(PointShadowMap& shadow_map)
{
    destroyImage(shadow_map.depth_img);
    vkDestroyFramebuffer(m_device, shadow_map.framebuffer, NULL);

    shadow_map.framebuffer = VK_NULL_HANDLE;
}

void Engine3D::updatePointShadowMap(const PointLightShaderData& light)
{
    PointShadowMapData& shadow_map_data = m_point_shadow_map_data[light.shadow_map_id];

    static const auto proj = glm::perspectiveLH_ZO(degToRad(90), 1.0f, 1.0f, light.max_d);

    shadow_map_data.light_pos = light.pos;
    shadow_map_data.max_d = light.max_d;

    shadow_map_data.P[0] = proj * glm::lookAtLH(light.pos, light.pos + vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    shadow_map_data.P[1] = proj * glm::lookAtLH(light.pos, light.pos + vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    shadow_map_data.P[2] = proj * glm::lookAtLH(light.pos, light.pos + vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f));
    shadow_map_data.P[3] = proj * glm::lookAtLH(light.pos, light.pos + vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
    shadow_map_data.P[4] = proj * glm::lookAtLH(light.pos, light.pos + vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f));
    shadow_map_data.P[5] = proj * glm::lookAtLH(light.pos, light.pos + vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
}

/*---------------- destroy methods ----------------*/

void Engine3D::destroyInstance() noexcept
{
#if VALIDATION_ENABLE
    destroyDebugCallback();
#endif

    vkDestroyInstance(m_instance, NULL);
    m_instance = VK_NULL_HANDLE;
}

void Engine3D::destroyDevice() noexcept
{
    vkDestroyDevice(m_device, NULL);
    m_device = VK_NULL_HANDLE;
}

void Engine3D::destroySurface() noexcept
{
    vkDestroySurfaceKHR(m_instance, m_surface, NULL);
    m_surface = VK_NULL_HANDLE;
}

void Engine3D::destroySwapchain() noexcept
{
    for(auto& siv : m_swapchain_image_views)
    {
        vkDestroyImageView(m_device, siv, NULL);
    }

    m_swapchain_image_views.clear();
    m_swapchain_images.clear();

    vkDestroySwapchainKHR(m_device, m_swapchain, NULL);
    m_swapchain = VK_NULL_HANDLE;
}

void Engine3D::destroyCommandPool() noexcept
{
    vkDestroyCommandPool(m_device, m_command_pool, NULL);
    m_command_pool = VK_NULL_HANDLE;
}

void Engine3D::destroyRenderPasses() noexcept
{
    vkDestroyRenderPass(m_device, m_main_render_pass, NULL);
    m_main_render_pass = VK_NULL_HANDLE;

    vkDestroyRenderPass(m_device, m_shadow_map_render_pass, NULL);
    m_shadow_map_render_pass = VK_NULL_HANDLE;
}

void Engine3D::destroyDescriptorSets() noexcept
{
    vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, NULL);
    m_descriptor_set_layout = VK_NULL_HANDLE;

    vkDestroyDescriptorPool(m_device, m_descriptor_pool, NULL);
    m_descriptor_pool = VK_NULL_HANDLE;
}

void Engine3D::destroyPipelineLayout() noexcept
{
    vkDestroyPipelineLayout(m_device, m_pipeline_layout, NULL);
    m_pipeline_layout = VK_NULL_HANDLE;
}

void Engine3D::destroyPipelines() noexcept
{
    for(auto& p : m_pipelines)
    {
        vkDestroyPipeline(m_device, p, NULL);
    }

    m_pipelines.clear();
}

void Engine3D::destroySynchronizationPrimitives() noexcept
{
    for(auto& per_frame_data : m_per_frame_data)
    {
        vkDestroyFence(m_device, per_frame_data.cmd_buf_ready_fence, NULL);
        vkDestroySemaphore(m_device, per_frame_data.image_acquire_semaphore, NULL);
        vkDestroySemaphore(m_device, per_frame_data.rendering_finished_semaphore, NULL);
    }

    vkDestroyFence(m_device, m_transfer_cmd_buf_fence, NULL);
}

void Engine3D::destroyRenderTargets() noexcept
{
    for(auto& rt : m_render_targets)
    {
        destroyImage(rt.color_img);
        destroyImage(rt.depth_img);
        vkDestroyFramebuffer(m_device, rt.framebuffer, NULL);
    }

    m_render_targets.clear();
}

void Engine3D::destroySamplers() noexcept
{
    vkDestroySampler(m_device, m_sampler, NULL);
    m_sampler = VK_NULL_HANDLE;

    vkDestroySampler(m_device, m_shadow_map_sampler, NULL);
    m_shadow_map_sampler = VK_NULL_HANDLE;
}

void Engine3D::destroyShadowMaps()
{
    for(auto& per_frame_data : m_per_frame_data)
    {
        for(auto& dir_shadow_map : per_frame_data.dir_shadow_maps)
        {
            destroyDirShadowMap(dir_shadow_map);
        }

        for(auto& point_shadow_map : per_frame_data.point_shadow_maps)
        {
            destroyPointShadowMap(point_shadow_map);
        }
    }

    while(!m_dir_shadow_maps_free_ids.empty())
    {
        m_dir_shadow_maps_free_ids.pop();
    }

    while(!m_point_shadow_maps_free_ids.empty())
    {
        m_point_shadow_maps_free_ids.pop();
    }
}

/*---------------- debug callback ----------------*/

#if VALIDATION_ENABLE
VkBool32 debugReportCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
{
    if(!(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) && !(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT))
    {
        std::cout << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

void Engine3D::createDebugCallback(const VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
    PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
    if(!pfnCreateDebugUtilsMessenger)
    {
        throw std::runtime_error(errorMsg("Failed to load vkCreateDebugUtilsMessengerEXT function."));
    }

    m_destroy_debug_utils_messenger_function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
    if(!m_destroy_debug_utils_messenger_function)
    {
        throw std::runtime_error(errorMsg("Failed to load vkDestroyDebugUtilsMessengerEXT function."));
    }

    VkResult res = pfnCreateDebugUtilsMessenger(m_instance, &create_info, NULL, &m_debug_report_callback);
    assertVkSuccess(res, "Failed to create debug report callback");

    m_set_debug_utils_object_name_function = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(m_instance, "vkSetDebugUtilsObjectNameEXT"));
    if(!m_set_debug_utils_object_name_function)
    {
        throw std::runtime_error(errorMsg("Failed to load vkSetDebugUtilsObjectNameEXT function."));
    }
}

void Engine3D::destroyDebugCallback()
{
    if(m_debug_report_callback)
    {
        if(!m_instance)
        {
            throw std::runtime_error("Attempting to destroy debug report callback, which is not NULL, but VkInstance is already NULL!");
        }
        else
        {
            m_destroy_debug_utils_messenger_function(m_instance, m_debug_report_callback, NULL);
            m_debug_report_callback = VK_NULL_HANDLE;
        }
    }
}
#endif

void Engine3D::setDebugObjectName(VkObjectType object_type, uint64_t object_handle, std::string_view object_name)
{
#if VALIDATION_ENABLE
    VkDebugUtilsObjectNameInfoEXT name_info{};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.pNext = NULL;
    name_info.objectType = object_type;
    name_info.objectHandle = object_handle;
    name_info.pObjectName = object_name.data();

    VkResult res = m_set_debug_utils_object_name_function(m_device, &name_info);
    assertVkSuccess(res, "Failed to set debug name.");
#endif
}

void Engine3D::setDebugObjectName(VkSemaphore semaphore, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(semaphore), name);
}

void Engine3D::setDebugObjectName(VkCommandBuffer command_buffer, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(command_buffer), name);
}

void Engine3D::setDebugObjectName(VkFence fence, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_FENCE, reinterpret_cast<uint64_t>(fence), name);
}

void Engine3D::setDebugObjectName(VkDeviceMemory device_memory, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_DEVICE_MEMORY, reinterpret_cast<uint64_t>(device_memory), name);
}

void Engine3D::setDebugObjectName(VkBuffer buffer, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(buffer), name);
}

void Engine3D::setDebugObjectName(VkImage image, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(image), name);
}

void Engine3D::setDebugObjectName(VkBufferView buffer_view, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_BUFFER_VIEW, reinterpret_cast<uint64_t>(buffer_view), name);
}

void Engine3D::setDebugObjectName(VkImageView image_view, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(image_view), name);
}

void Engine3D::setDebugObjectName(VkShaderModule shader_module, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(shader_module), name);
}

void Engine3D::setDebugObjectName(VkPipelineLayout pipeline_layout, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(pipeline_layout), name);
}

void Engine3D::setDebugObjectName(VkRenderPass render_pass, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_RENDER_PASS, reinterpret_cast<uint64_t>(render_pass), name);
}

void Engine3D::setDebugObjectName(VkPipeline pipeline, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(pipeline), name);
}

void Engine3D::setDebugObjectName(VkDescriptorSetLayout descriptor_set_layout, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(descriptor_set_layout), name);
}

void Engine3D::setDebugObjectName(VkSampler sampler, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(sampler), name);
}

void Engine3D::setDebugObjectName(VkDescriptorPool descriptor_pool, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(descriptor_pool), name);
}

void Engine3D::setDebugObjectName(VkDescriptorSet descriptor_set, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(descriptor_set), name);
}

void Engine3D::setDebugObjectName(VkFramebuffer framebuffer, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_FRAMEBUFFER, reinterpret_cast<uint64_t>(framebuffer), name);
}

void Engine3D::setDebugObjectName(VkCommandPool command_pool, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<uint64_t>(command_pool), name);
}

void Engine3D::setDebugObjectName(VkSurfaceKHR surface, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_SURFACE_KHR, reinterpret_cast<uint64_t>(surface), name);
}

void Engine3D::setDebugObjectName(VkSwapchainKHR swapchain, std::string_view name)
{
    setDebugObjectName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, reinterpret_cast<uint64_t>(swapchain), name);
}
