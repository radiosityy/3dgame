#ifndef ENGINE_3D_H
#define ENGINE_3D_H

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <list>

#include "collision.h"
#include "platform.h"
#include <vulkan/vulkan.h>
#include "window.h"
#include "render_data.h"
#include "font.h"
#include "camera.h"
#include <queue>
#include <span>
#include "shader_data.h"
#include "vk_buffer_wrapper.h"

using DirLightId = uint32_t;
using PointLightId = uint32_t;

struct VertexBuffer : VkBufferWrapper
{
    VertexBuffer() : VkBufferWrapper(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)
    {}
};

struct VertexBufferAllocation
{
    VertexBuffer* vb = nullptr;
    uint64_t data_offset = 0;
    uint32_t vertex_offset = 0;
};

class Engine3D
{
    struct RenderBatch
    {
        RenderBatch() = default;

        RenderBatch(RenderMode render_mode_, VertexBuffer* vb_, uint32_t vertex_offset_, uint32_t vertex_count_, uint32_t instance_id_, std::optional<Sphere> bounding_sphere_)
            : render_mode(render_mode_)
            , vb(vb_)
            , vertex_offset(vertex_offset_)
            , vertex_count(vertex_count_)
            , instance_id(instance_id_)
            , bounding_sphere(bounding_sphere_)
        {}

        RenderMode render_mode;
        VertexBuffer* vb;
        uint32_t vertex_offset;
        uint32_t vertex_count;
        uint32_t instance_id;
        std::optional<Sphere> bounding_sphere;
    };

    struct RenderBatchUi
    {
        RenderBatchUi() = default;

        RenderBatchUi(RenderModeUi render_mode_, VertexBuffer* vb_, uint32_t vertex_offset_, uint32_t vertex_count_, const Quad& scissor_)
            : render_mode(render_mode_)
            , vb(vb_)
            , vertex_offset(vertex_offset_)
            , vertex_count(vertex_count_)
            , scissor(scissor_)
        {}

        RenderModeUi render_mode;
        VertexBuffer* vb;
        uint32_t vertex_offset;
        uint32_t vertex_count;
        Quad scissor;
    };

    struct VkImageWrapper
    {
        VkImage img = VK_NULL_HANDLE;
        VkImageView img_view = VK_NULL_HANDLE;
        VkDeviceMemory mem = VK_NULL_HANDLE;
    };

    struct RenderTarget
    {
        VkImageWrapper color_img;
        VkImageWrapper depth_img;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;

        VkRenderPassBeginInfo render_pass_begin_info;
    };

    struct Texture
    {
        VkImageWrapper image;
        uint32_t id;
    };

    struct DirShadowMap
    {
        DirShadowMap() = default;
        DirShadowMap(const DirShadowMap&) = delete;
        DirShadowMap& operator=(const DirShadowMap&) = delete;
        DirShadowMap(DirShadowMap&& other) = default;
        DirShadowMap& operator=(DirShadowMap&&) = default;

        VkImageWrapper depth_img;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkRenderPassBeginInfo render_pass_begin_info;

        uint32_t count = 0;
        uint32_t res_x = 0;
        uint32_t res_y = 0;
    };

    struct alignas(16) DirShadowMapData
    {
        mat4x4 P;
        mat4x4 tex_P;
        float z;
    };

    struct PointShadowMap
    {
        PointShadowMap() = default;
        PointShadowMap(const PointShadowMap&) = delete;
        PointShadowMap& operator=(const PointShadowMap&) = delete;
        PointShadowMap(PointShadowMap&& other) = default;
        PointShadowMap& operator=(PointShadowMap&&) = default;

        VkImageWrapper depth_img;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkRenderPassBeginInfo render_pass_begin_info;

        uint32_t res = 0;
    };

    struct alignas(16) PointShadowMapData
    {
        std::array<mat4x4, 6> P;
        vec3 light_pos;
        float max_d;
    };

    struct TextureCollection
    {
        void clear()
        {
            textures.clear();
            ids.clear();
            free_ids = std::queue<uint32_t>();
        }

        std::vector<VkImageWrapper> textures;
        std::unordered_map<std::string, uint32_t> ids;
        std::queue<uint32_t> free_ids;
    };

    struct PerFrameData
    {
        PerFrameData() = default;
        PerFrameData(const PerFrameData&) = delete;
        PerFrameData& operator=(const PerFrameData&) = delete;

        VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        VkSemaphore image_acquire_semaphore = VK_NULL_HANDLE;
        VkSemaphore rendering_finished_semaphore = VK_NULL_HANDLE;
        VkFence cmd_buf_ready_fence = VK_NULL_HANDLE;
        std::array<DirShadowMap, MAX_DIR_SHADOW_MAP_COUNT> dir_shadow_maps;
        std::array<PointShadowMap, MAX_POINT_SHADOW_MAP_COUNT> point_shadow_maps;
        //TODO: could use a better structure for lights_to_update than std::vector?
        std::vector<DirLightId> dir_lights_to_update;
        std::vector<PointLightId> point_lights_to_update;
        std::vector<uint32_t> dir_shadow_maps_to_destroy;
        std::vector<uint32_t> point_shadow_maps_to_destroy;
        std::vector<VkBufferWrapper*> bufs_to_destroy;

        /*buffers*/
        std::unique_ptr<VkBufferWrapper> common_buffer;
        //TODO: we should only have one on each of these (like VBs now), not one per frame in flight
        //unless we can tell that some will be fully updated every frame (like dir shadow map buffer?)(?)
        std::unique_ptr<VkBufferWrapper> dir_light_buffer;
        std::unique_ptr<VkBufferWrapper> dir_light_valid_buffer;
        std::unique_ptr<VkBufferWrapper> point_light_buffer;
        std::unique_ptr<VkBufferWrapper> point_light_valid_buffer;
    };

    struct BufferUpdateReq
    {
        BufferUpdateReq(uint64_t data_offset_, uint64_t data_size_, const void* data_)
            : data_offset(data_offset_)
            , data_size(data_size_)
            , data(data_)
        {}

        uint64_t data_offset;
        uint64_t data_size;
        const void* data;
    };

public:
    Engine3D(const Window& window, std::string_view app_name);

    ~Engine3D();
    Engine3D(const Engine3D&) = delete;
    Engine3D(const Engine3D&&) = delete;
    Engine3D& operator=(const Engine3D&) = delete;
    Engine3D& operator=(const Engine3D&&) = delete;

    void updateAndRender(const RenderData&, Camera& camera);

    void onWindowResize(uint32_t width, uint32_t height);
    void onSceneLoad(const SceneInitData&);
    void setSampleCount(VkSampleCountFlagBits);
    bool enableVsync(bool vsync);

    template<class VertexType>
    VertexBufferAllocation requestVertexBufferAllocation(uint32_t vertex_count)
    {
        VertexBufferAllocation alloc;
        alloc.vb = &m_vertex_buffers[sizeof(VertexType)];

        alloc.data_offset = alloc.vb->allocate(vertex_count * sizeof(VertexType));
        alloc.vertex_offset = alloc.data_offset / sizeof(VertexType);

        return alloc;
    }
    uint32_t requestInstanceVertexBufferAllocation(uint32_t instance_count);
    uint32_t requestBoneTransformBufferAllocation(uint32_t bone_count);
    void requestTerrainBufferAllocation(uint64_t size);

    void freeVertexBufferAllocation(VertexBuffer* vb, uint64_t offset, uint64_t size);
    void freeInstanceVertexBufferAllocation(uint32_t instance_id, uint32_t instance_count);
    void freeBoneTransformBufferAllocation(uint32_t bone_id, uint32_t bone_count);
    void freeTerrainBufferAllocation();

    void requestBufferUpdate(VkBufferWrapper* buf, uint64_t data_offset, uint64_t data_size, const void* data);
    //TODO: rename these to request*Update
    void updateVertexData(VertexBuffer* vb, uint64_t data_offset, uint64_t data_size, const void* data);
    void updateInstanceVertexData(uint32_t instance_id, uint32_t instance_count, const void* data);
    void updateBoneTransformData(uint32_t bone_offset, uint32_t bone_count, const mat4x4* data);
    void updateTerrainData(void* data, uint64_t offset, uint64_t size);
    std::vector<uint32_t> requestTerrainHeightmaps(std::span<std::pair<float*, uint32_t>> heightmap_data);

    void draw(RenderMode render_mode, VertexBuffer* vb, uint32_t vertex_offset, uint32_t vertex_count, uint32_t instance_id, std::optional<Sphere> bounding_sphere);
    void drawUi(RenderModeUi render_mode, VertexBuffer* vb, uint32_t vertex_offset, uint32_t vertex_count, const Quad& scissor);

    DirLightId addDirLight(const DirLight& dir_light);
    void updateDirLight(DirLightId id, const DirLight& dir_light);
    void removeDirLight(DirLightId id);
    PointLightId addPointLight(const PointLight& point_light);
    void updatePointLight(PointLightId id, const PointLight& point_light);
    void removePointLight(PointLightId id);

    uint32_t loadTexture(std::string_view texture_filename);
    std::vector<uint32_t> loadTextures(const std::vector<std::string_view>& texture_filenames);
    uint32_t loadNormalMap(std::string_view texture_filename);
    std::vector<uint32_t> loadNormalMaps(const std::vector<std::string_view>& texture_filenames);

private:
    void resizeBuffers();
    void updateBuffers();

    /*------------------ helper methods ------------------*/
    void deviceWaitIdle();

    //TODO: change VkShaderModule parameter to not be a pointer (VkShaderModule is a pointer in itself so we don't need to pass a pointer to it I guess?)
    VkPipelineShaderStageCreateInfo loadShader(const std::string& filename, VkShaderStageFlagBits, VkShaderModule*, const VkSpecializationInfo* = nullptr);

    std::pair<VkDeviceMemory, bool> allocateMemory(VkMemoryRequirements, bool host_visible_required) const;
    void allocateAndBindMemory(VkImageWrapper&) const;
    void allocateAndBindMemory(VkBufferWrapper&) const;

    void createImage(VkImageWrapper&, const VkImageCreateInfo&, VkImageViewCreateInfo&) const;
    VkImageWrapper createImage(const VkImageCreateInfo&, VkImageViewCreateInfo&) const;
    void destroyImage(VkImageWrapper&) const noexcept;

    void createBuffer(VkBufferWrapper&, VkDeviceSize size);
    void destroyBuffer(VkBufferWrapper&) const noexcept;

    void updateBuffer(VkBufferWrapper&, size_t data_size, const std::function<void(void*)>&, VkCommandBuffer = VK_NULL_HANDLE);
    void updateBuffer(VkBufferWrapper&, size_t data_size, const void* data, VkCommandBuffer = VK_NULL_HANDLE);

    VkPipeline& getPipeline(RenderMode) noexcept;
    VkPipeline& getPipeline(RenderModeUi) noexcept;

    /*------------------ asset methods -------------------*/
    std::vector<uint32_t> loadTexturesGeneric(const std::vector<std::string_view>& texture_filenames, TextureCollection& texture_collection, bool generate_mipmaps);
    void loadFonts(const std::vector<const Font*>& fonts);

    void destroyTextures() noexcept;
    void destroyFontTextures() noexcept;

    void updateDescriptorSets() noexcept;

    /*------------------ create methods ------------------*/
    void createInstance(std::string_view app_name);
    void createDevice();
    void pickPhysicalDevice();
    void createSurface(const Window&);
    void createSwapchain(uint32_t width, uint32_t height);
    void createCommandPool();
    void createMainRenderPass();
    void createShadowMapRenderPass();
    void createDescriptorSets();
    void createPipelineLayout();
    void createPipelines();
    void createCommandBuffers();
    void createSynchronizationPrimitives();
    void createRenderTargets();
    void createSamplers();
    void createBuffers();

    uint32_t createDirShadowMap(const DirLight& light);
    uint32_t createPointShadowMap(const PointLight& light);
    void markDirShadowMapForDestroy(uint32_t id);
    void markPointShadowMapForDestroy(uint32_t id);
    void destroyDirShadowMap(DirShadowMap& shadow_map);
    void destroyPointShadowMap(PointShadowMap& shadow_map);

    void updateDirShadowMap(Camera& camera, const DirLightShaderData& dir_light);
    void updatePointShadowMap(const PointLightShaderData& point_light);

    /*----------------- destroy methods ------------------*/
    void destroy() noexcept;
    void destroyInstance() noexcept;
    void destroyDevice() noexcept;
    void destroySurface() noexcept;
    void destroySwapchain() noexcept;
    void destroyCommandPool() noexcept;
    void destroyRenderPasses() noexcept;
    void destroyDescriptorSets() noexcept;
    void destroyPipelineLayout() noexcept;
    void destroyPipelines() noexcept;
    void destroySynchronizationPrimitives() noexcept;
    void destroyRenderTargets() noexcept;
    void destroySamplers() noexcept;

    void destroyShadowMaps();

    /*------------------ vulkan handles ------------------*/
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
    std::vector<VkPipeline> m_pipelines;
    std::vector<VkPipeline> m_pipelines_ui;

    VkCommandBuffer m_transfer_cmd_buf = VK_NULL_HANDLE;
    VkFence m_transfer_cmd_buf_fence = VK_NULL_HANDLE;

    VkSampler m_sampler = VK_NULL_HANDLE;
    VkSampler m_shadow_map_sampler = VK_NULL_HANDLE;
    VkSampler m_terrain_heightmap_sampler = VK_NULL_HANDLE;

    /*-------------------- descriptors -------------------*/
    VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;

    uint32_t m_common_buf_desc_count = 0;
    const VkDescriptorType m_common_buf_desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    uint32_t m_tex_desc_count = 0;
    const VkDescriptorType m_tex_desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    uint32_t m_normal_map_desc_count = 0;
    const VkDescriptorType m_normal_map_desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    uint32_t m_font_desc_count = 0;
    const VkDescriptorType m_font_desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    uint32_t m_dir_lights_desc_count = 0;
    const VkDescriptorType m_dir_lights_desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    uint32_t m_dir_lights_valid_desc_count = 0;
    const VkDescriptorType m_dir_lights_valid_desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

    uint32_t m_point_lights_desc_count = 0;
    const VkDescriptorType m_point_lights_desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    uint32_t m_point_lights_valid_desc_count = 0;
    const VkDescriptorType m_point_lights_valid_desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

    uint32_t m_dir_sm_buf_desc_count = 0;
    const VkDescriptorType m_dir_sm_buf_desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    uint32_t m_dir_sm_desc_count = 0;
    const VkDescriptorType m_dir_sm_desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    uint32_t m_point_sm_buf_desc_count = 0;
    const VkDescriptorType m_point_sm_buf_desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    uint32_t m_point_sm_desc_count = 0;
    const VkDescriptorType m_point_sm_desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    uint32_t m_terrain_buf_desc_count = 0;
    const VkDescriptorType m_terrain_buf_desc_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    uint32_t m_terrain_heightmap_desc_count = 0;
    const VkDescriptorType m_terrain_heightmap_desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    uint32_t m_bone_transform_buf_desc_count = 0;
    const VkDescriptorType m_bone_transform_buf_desc_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    /*---------------------- device ----------------------*/
    VkPhysicalDeviceProperties m_physical_device_properties;
    VkPhysicalDeviceFeatures m_physical_device_features;
    VkPhysicalDeviceVulkan12Features m_physical_device_12_features;
    VkPhysicalDeviceMemoryProperties m_physical_device_memory_properties;

    uint32_t m_queue_family_index;
    VkQueue m_queue;

    /*---------------- surface/swapchain ----------------*/
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;

    uint32_t m_surface_width;
    uint32_t m_surface_height;
    VkFormat m_swapchain_format;

    /*----------------- render targets -------------------*/
    VkRenderPass m_main_render_pass = VK_NULL_HANDLE;

    std::vector<RenderTarget> m_render_targets;
    std::vector<VkClearValue> m_render_target_clear_values = {};

    /*------------------- shadow maps --------------------*/
    VkRenderPass m_shadow_map_render_pass = VK_NULL_HANDLE;
    VkClearValue m_shadow_map_clear_value = {};

    uint32_t m_dir_shadow_map_count = 0;
    uint32_t m_point_shadow_map_count = 0;
    std::array<bool, MAX_DIR_SHADOW_MAP_COUNT> m_dir_shadow_maps_valid = {};
    std::array<bool, MAX_POINT_SHADOW_MAP_COUNT> m_point_shadow_maps_valid = {};
    std::queue<uint32_t> m_dir_shadow_maps_free_ids;
    std::queue<uint32_t> m_point_shadow_maps_free_ids;
    std::array<uint8_t, MAX_DIR_SHADOW_MAP_COUNT> m_dir_shadow_map_free_id_frame_ids;
    std::array<uint8_t, MAX_POINT_SHADOW_MAP_COUNT> m_point_shadow_map_free_id_frame_ids;

    std::array<std::array<DirShadowMapData, MAX_DIR_SHADOW_MAP_PARTITIONS>, MAX_DIR_SHADOW_MAP_COUNT> m_dir_shadow_map_data;
    std::array<PointShadowMapData, MAX_POINT_SHADOW_MAP_COUNT> m_point_shadow_map_data;

    bool m_update_descriptors = false;

    /*-------------------- resources ---------------------*/
    std::array<PerFrameData, FRAMES_IN_FLIGHT> m_per_frame_data;

    std::vector<RenderBatch> m_render_batches;
    uint32_t m_render_batch_count = 0;
    std::vector<RenderBatchUi> m_render_batches_ui;
    uint32_t m_render_batch_ui_count = 0;
    std::unordered_map<VkBufferWrapper*, std::vector<BufferUpdateReq>> m_buffer_update_reqs;

    /*--- vertex buffers ---*/
    std::unordered_map<uint32_t, VertexBuffer> m_vertex_buffers;
    VertexBuffer m_instance_vertex_buffer;

    /*--- buffers ---*/
    VkBufferWrapper m_dir_shadow_map_buffer;
    VkBufferWrapper m_point_shadow_map_buffer;
    VkBufferWrapper m_bone_transform_buffer;

    /*--- dir lights ---*/
    std::queue<DirLightId> m_dir_light_free_ids;
    std::array<DirLightShaderData, MAX_DIR_LIGHT_COUNT> m_dir_lights;
    std::array<uint8_t, MAX_DIR_LIGHT_COUNT> m_dir_lights_valid = {};
    /*--- point lights ---*/
    std::queue<PointLightId> m_point_light_free_ids;
    std::array<PointLightShaderData, MAX_POINT_LIGHT_COUNT> m_point_lights;
    std::array<uint8_t, MAX_POINT_LIGHT_COUNT> m_point_lights_valid = {};
    /*--- terrain ---*/
    VkBufferWrapper m_terrain_buffer;

    /*---------------------- assets ----------------------*/
    TextureCollection m_textures;
    TextureCollection m_normal_maps;
    std::vector<VkImageWrapper> m_terrain_heightmaps;

    std::vector<Texture> m_font_textures;

    /*------------------- render params ------------------*/
    uint8_t m_frame_id = 0;
    VkSampleCountFlagBits m_sample_count = VK_SAMPLE_COUNT_1_BIT;
    bool m_vsync_disable_support = false;
    bool m_vsync = true;

    std::vector<VkSemaphore> m_wait_semaphores;
    std::vector<VkPipelineStageFlags> m_submit_wait_flags;

/*---------------------------------------------------------------------------------------------------------*/
/*------------------------------------------ render mode params -------------------------------------------*/
    uint32_t m_font_count = 0;

    struct
    {
        mat4x4 VP;
        mat4x4 V;
        alignas(16) vec3 camera_pos;
        alignas(16) vec3 camera_up;
        uint32_t dir_light_count = 0;
        uint32_t point_light_count = 0;
        alignas(16) vec3 visual_sun_pos;
                    float sun_radius;
        alignas(16) vec3 effective_sun_pos;
        alignas(16) vec3 cur_pos_terrain;
                    uint32_t cur_terrain_intersection;
        alignas(16) vec2 ui_scale;
                    float terrain_patch_size;
        alignas(16) vec4 editor_highlight_color;
                    float editor_terrain_tool_inner_radius;
                    float editor_terrain_tool_outer_radius;
    } m_common_buffer_data;

    /*------------------- push constants -----------------*/
    struct PushConstants
    {
        uint32_t shadow_map_count;
        uint32_t shadow_map_offset;
    } push_const;

    const std::vector<VkPushConstantRange> push_const_ranges
    {
        {VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(push_const)},
    };

    /*---------------------- debug -----------------------*/
#if VULKAN_VALIDATION_ENABLE
    PFN_vkSetDebugUtilsObjectNameEXT m_set_debug_utils_object_name_function = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debug_report_callback = VK_NULL_HANDLE;
    PFN_vkDestroyDebugUtilsMessengerEXT m_destroy_debug_utils_messenger_function = nullptr;

    void createDebugCallback(const VkDebugUtilsMessengerCreateInfoEXT&);
    void destroyDebugCallback();
#endif

    void setDebugObjectName(VkObjectType object_type, uint64_t object_handle, std::string_view object_name);
    void setDebugObjectName(VkSemaphore semaphore, std::string_view name);
    void setDebugObjectName(VkCommandBuffer command_buffer, std::string_view name);
    void setDebugObjectName(VkFence fence, std::string_view name);
    void setDebugObjectName(VkDeviceMemory device_memory, std::string_view name);
    void setDebugObjectName(VkBuffer buffer, std::string_view name);
    void setDebugObjectName(VkImage image, std::string_view name);
    void setDebugObjectName(VkBufferView buffer_view, std::string_view name);
    void setDebugObjectName(VkImageView image_view, std::string_view name);
    void setDebugObjectName(VkShaderModule shader_module, std::string_view name);
    void setDebugObjectName(VkPipelineLayout pipeline_layout, std::string_view name);
    void setDebugObjectName(VkRenderPass render_pass, std::string_view name);
    void setDebugObjectName(VkPipeline pipeline, std::string_view name);
    void setDebugObjectName(VkDescriptorSetLayout descriptor_set_layout, std::string_view name);
    void setDebugObjectName(VkSampler sampler, std::string_view name);
    void setDebugObjectName(VkDescriptorPool descriptor_pool, std::string_view name);
    void setDebugObjectName(VkDescriptorSet descriptor_set, std::string_view name);
    void setDebugObjectName(VkFramebuffer framebuffer, std::string_view name);
    void setDebugObjectName(VkCommandPool command_pool, std::string_view name);
    void setDebugObjectName(VkSurfaceKHR surface, std::string_view name);
    void setDebugObjectName(VkSwapchainKHR swapchain, std::string_view name);
};

#endif // ENGINE_3D_H
