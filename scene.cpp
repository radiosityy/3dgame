#include "scene.h"
#include "game_utils.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <filesystem>

#include <print>
#include <fstream>

Scene::Scene(Renderer& renderer)
    : m_renderer(renderer)
{
    renderer.loadTexture("Ground003_4K_Color.png");
    renderer.loadNormalMap("Ground003_4K_Normal.png");

    loadFromFile("scene.scn");
}

void Scene::loadFromFile(std::string_view filename)
{
    std::ifstream scene_file(filename.data(), std::ios::binary);

    if(!scene_file)
    {
        std::println("No scene.scn file found. Creating new scene...");
    }
    else
    {
        uint64_t mesh_data_size = 0;
        scene_file.read(reinterpret_cast<char*>(&mesh_data_size), sizeof(uint64_t));

        m_renderer.initStaticVB(mesh_data_size);

        uint32_t mesh_count = 0;
        scene_file.read(reinterpret_cast<char*>(&mesh_count), sizeof(uint32_t));
        for(uint32_t mesh_id = 0; mesh_id < mesh_count; mesh_id++)
        {
            uint8_t mesh_filename_length = 0;
            scene_file.read(reinterpret_cast<char*>(&mesh_filename_length), sizeof(uint8_t));
            std::string mesh_filename;
            mesh_filename.resize(mesh_filename_length);
            scene_file.read(reinterpret_cast<char*>(mesh_filename.data()), mesh_filename_length);
            m_mesh_manager.loadMeshData(m_renderer, mesh_filename);
        }

        m_renderer.finalizeStaticVB();

        uint32_t obj_count = 0;
        scene_file.read(reinterpret_cast<char*>(&obj_count), sizeof(uint32_t));

        for(uint32_t i = 0; i < obj_count; i++)
        {
            addObject(m_renderer, scene_file);
        }

        uint32_t point_light_count = 0;
        scene_file.read(reinterpret_cast<char*>(&point_light_count), sizeof(uint32_t));

        for(uint32_t i = 0; i < point_light_count; i++)
        {
            PointLight point_light(scene_file);

#if !EDITOR_ENABLE
            addPointLight(point_light);
#else
            addStaticPointLight(point_light);
#endif
        }
    }

    /*add terrain*/
    m_terrain = std::make_unique<Terrain>(m_renderer);
}

void Scene::update(float dt, const InputState& input_state) noexcept
{
    for(auto& obj : m_objects)
    {
        obj.update(m_renderer, dt);
    }
}

void Scene::draw(const Camera& camera, RenderData& render_data) noexcept
{
    render_data.terrain_patch_size = m_terrain->patchSize();

    //TODO: do frustum culling here
    const auto view_frustum_planes = camera.viewFrustumPlanesW();

    for(auto& obj : m_objects)
    {
        if(obj.cull(view_frustum_planes))
        {
            obj.draw(m_renderer);
        }
    }

    m_terrain->draw(m_renderer);
}

void Scene::removeObject(Object* obj)
{
    for(auto it = m_objects.begin(); it != m_objects.end(); it++)
    {
        if(&(*it) == obj)
        {
            m_objects.erase(it);
            return;
        }
    }
}

PointLightId Scene::addPointLight(const PointLight& point_light) const
{
    return m_renderer.addPointLight(point_light);
}

void Scene::updatePointLight(PointLightId id, const PointLight& point_light) const
{
    m_renderer.updatePointLight(id, point_light);
}

PointLightId Scene::addDirLight(const DirLight& dir_light) const
{
    return m_renderer.addDirLight(dir_light);
}

void Scene::updateDirLight(DirLightId id, const DirLight& dir_light) const
{
    m_renderer.updateDirLight(id, dir_light);
}

Terrain& Scene::terrain() noexcept
{
    return *m_terrain;
}


#if EDITOR_ENABLE

Object* Scene::pickObject(const Ray& rayW, float& d)
{
    d = std::numeric_limits<float>::max();
    Object* closest_obj = nullptr;

    for(auto& obj : m_objects)
    {
        float _d;
        if(obj.rayIntersetion(rayW, d, _d))
        {
            d = std::min(d, _d);
            closest_obj = &obj;
        }
    }

    // float _d;
    // if(m_player->rayIntersetion(rayW, d, _d))
    // {
    //     d = std::min(d, _d);
    //     closest_obj = m_player.get();
    // }

    return closest_obj;
}

void Scene::addStaticPointLight(const PointLight& point_light)
{
    m_static_point_light_ids.push_back(addPointLight(point_light));
    m_static_point_lights.push_back(point_light);
}

void Scene::removeStaticPointLight(uint32_t id)
{
    m_renderer.removePointLight(m_static_point_light_ids[id]);
    m_static_point_lights.erase(m_static_point_lights.begin() + id);
    m_static_point_light_ids.erase(m_static_point_light_ids.begin() + id);
}

void Scene::updateStaticPointLight(uint32_t id, const PointLight& point_light)
{
    m_static_point_lights[id] = point_light;
    updatePointLight(m_static_point_light_ids[id], point_light);
}

std::vector<PointLight>& Scene::staticPointLights()
{
    return m_static_point_lights;
}

void Scene::saveToFile(std::string_view filename) const
{
    if(std::filesystem::exists(filename))
    {
        std::filesystem::copy_file(filename, std::string(filename) + ".bak", std::filesystem::copy_options::overwrite_existing);
    }

    if(std::filesystem::exists("terrain.dat"))
    {
        std::filesystem::copy_file("terrain.dat", "terrain.dat.bak", std::filesystem::copy_options::overwrite_existing);
    }

    std::ofstream outfile(filename.data(), std::ios::binary);

    if(!outfile)
    {
        error(std::format("Failed to open file: {}", filename));
    }

    const uint64_t mesh_data_size = m_mesh_manager.meshDataSize();
    outfile.write(reinterpret_cast<const char*>(&mesh_data_size), sizeof(uint64_t));

    const auto& mesh_filenames = m_mesh_manager.meshFilenames();
    const uint32_t mesh_count = mesh_filenames.size();
    outfile.write(reinterpret_cast<const char*>(&mesh_count), sizeof(uint32_t));

    for(const auto& mesh_filename : mesh_filenames)
    {
        const uint8_t mesh_filename_lenght = mesh_filename.size();
        outfile.write(reinterpret_cast<const char*>(&mesh_filename_lenght), sizeof(uint8_t));
        outfile.write(reinterpret_cast<const char*>(mesh_filename.data()), sizeof(char) * mesh_filename_lenght);
    }

    uint32_t obj_count = 0;

    for(const auto& obj : m_objects)
    {
        if(obj.isSerializable())
        {
            obj_count++;
        }
    }

    outfile.write(reinterpret_cast<const char*>(&obj_count), sizeof(uint32_t));

    for(const auto& obj : m_objects)
    {
        if(obj.isSerializable())
        {
            obj.serialize(outfile);
        }
    }

    const uint32_t point_light_count = m_static_point_lights.size();
    outfile.write(reinterpret_cast<const char*>(&point_light_count), sizeof(uint32_t));

    for(const auto& point_light : m_static_point_lights)
    {
        point_light.serialize(outfile);
    }

    m_terrain->saveToFile();
}
#endif
