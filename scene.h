#ifndef SCENE_H
#define SCENE_H

#include "camera.h"
#include "player.h"
#include "event.h"
#include "terrain.h"

#include <vector>
#include <memory>
#include "label.h"
#include "engine_3d.h"

class Scene
{
public:
    Scene(Engine3D& engine3d, float aspect_ratio, const Font& font);

    Scene(const Scene&) = delete;
    Scene(const Scene&&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene& operator=(const Scene&&) = delete;

    void loadFromFile(std::string_view filename);

    void update(float dt, const InputState& input_state) noexcept;
    void draw(RenderData&) noexcept;
    void onInputEvent(const Event&, const InputState&);
    void onWindowResize(float aspect_ratio) noexcept;

    void addObject(std::unique_ptr<Object>&&);
    void removeObject(Object*);
    PointLightId addPointLight(const PointLight& point_light) const;
    void updatePointLight(PointLightId id, const PointLight& point_light) const;

    Camera& camera() noexcept;
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
    void playerControls(const InputState&);

    void tryPlayerRotation(float dt);
    void tryPlayerMove(float dt);

    bool objectRotateCollision(const Object*, const quat&);
    bool objectMoveCollision(const Object*, vec3);

    void updateSun();

    template<class Collider>
    bool objectCollision(const Object*, const Collider&);

    Engine3D& m_engine3d;

    std::unique_ptr<Terrain> m_terrain;

    std::vector<std::unique_ptr<Object>> m_objects;

    std::unique_ptr<Camera> m_camera;

    std::unique_ptr<Label> m_time_label;
    static constexpr float m_time_scale = 60.0f * 60.0f; //one minute in game is one sec in real life
    static constexpr float m_day_length = 24.0f * 60.0f * 60.0f;
    static constexpr float m_sun_radius = 0.1f;
    static constexpr float m_lat = degToRad(52.33f);
    static constexpr float m_decl = degToRad(23.0f);
    static constexpr float m_utc_offset = 2.0f;
    static constexpr float m_lst_offset = (12.0f + m_utc_offset) * 3600.0f;
    float m_time_of_day = 0.0f;

    DirLight m_sun;
    vec3 m_visual_sun_pos;
    vec3 m_effective_sun_pos;

    /*gameplay*/
    Player* m_player = nullptr;
    bool m_player_movement = true;
    float m_player_camera_radius = 5.0f;
    float m_player_camera_theta = pi_2; //inclination
    float m_player_camera_phi = -pi_2; //azimuth

#if EDITOR_ENABLE
    std::vector<PointLight> m_static_point_lights;
    std::vector<PointLightId> m_static_point_light_ids;
#endif
};

#endif // SCENE_H
