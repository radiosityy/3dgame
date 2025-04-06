#ifndef SCENE_H
#define SCENE_H

#include "camera.h"
#include "player.h"
#include "input_state.h"
#include "terrain.h"

#include <vector>
#include <memory>
#include "label.h"
#include "renderer.h"

class Scene
{
public:
    Scene(Renderer& renderer);

    Scene(const Scene&) = delete;
    Scene(const Scene&&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene& operator=(const Scene&&) = delete;

    void update(float dt, const InputState& input_state) noexcept;
    void draw(const Camera&, RenderData&) noexcept;

    void addObject(auto&&... args)
    {
        m_objects.emplace_back(std::forward<decltype(args)>(args)...);
    }

    void removeObject(Object*);
    PointLightId addPointLight(const PointLight& point_light) const;
    void updatePointLight(PointLightId id, const PointLight& point_light) const;
    DirLightId addDirLight(const DirLight& dir_light) const;
    void updateDirLight(DirLightId id, const DirLight& dir_light) const;

    Terrain& terrain() noexcept;

#if EDITOR_ENABLE
    Object* pickObject(const Ray& rayW, float& d);

    void addStaticPointLight(const PointLight& point_light);
    void removeStaticPointLight(uint32_t id);
    void updateStaticPointLight(uint32_t id, const PointLight& point_light);
    std::vector<PointLight>& staticPointLights();
    void serialize(std::string_view filename) const;
#endif

private:
    void loadFromFile(std::string_view filename);

    Renderer& m_renderer;

    std::unique_ptr<Terrain> m_terrain;

    std::vector<Object> m_objects;

#if EDITOR_ENABLE
    std::vector<PointLight> m_static_point_lights;
    std::vector<PointLightId> m_static_point_light_ids;
#endif
};

#endif // SCENE_H
