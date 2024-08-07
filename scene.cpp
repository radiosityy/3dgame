#include "scene.h"
#include "game_utils.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <filesystem>

#include "timer.h"
#include <print>
#include <sstream>
#include <fstream>

Scene::Scene(Renderer& renderer, float aspect_ratio, const Font& font)
    : m_renderer(renderer)
    , m_camera(aspect_ratio, 1.0f, 2000.0f, degToRad(90.0f))
{
    renderer.loadTexture("Ground003_4K_Color.png");
    renderer.loadNormalMap("Ground003_4K_Normal.png");

    /*add sun*/
    m_sun.shadow_map_count = 4;
    m_sun.shadow_map_res_x = 2048;
    m_sun.shadow_map_res_y = 2048;
    renderer.addDirLight(m_sun);

    m_time_of_day = 11.0f * 3600.0f;
    updateSun();

    loadFromFile("scene.scn");

    m_camera.setPos(vec3(0.0f, 66.0f, -70.0f));
    m_camera.pitch(glm::radians(45.0f));
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
        uint64_t obj_count = 0;
        scene_file.read(reinterpret_cast<char*>(&obj_count), sizeof(uint64_t));

        for(uint64_t i = 0; i < obj_count; i++)
        {
            addObject(m_renderer, scene_file);
        }

        uint64_t point_light_count = 0;
        scene_file.read(reinterpret_cast<char*>(&point_light_count), sizeof(uint64_t));

        for(uint64_t i = 0; i < point_light_count; i++)
        {
            PointLight point_light(scene_file);

#if !EDITOR_ENABLE
            addPointLight(point_light);
#else
            addStaticPointLight(point_light);
#endif
        }
    }

    /*add sky dome*/
    addObject(m_renderer, "assets/meshes/sky_dome.3d", RenderMode::Sky, vec3(0.0f), vec3(1000.0f));
#if EDITOR_ENABLE
    m_objects.back().setSerializable(false);
#endif

    /*add player*/
    m_player = std::make_unique<Player>(m_renderer, "assets/meshes/man.3d", RenderMode::Default, vec3(0.0f, 5.0f, -10.0f));
#if EDITOR_ENABLE
    m_player->setSerializable(false);
#endif

    /*add terrain*/
    m_terrain = std::make_unique<Terrain>(m_renderer);
}

void Scene::update(float dt, const InputState& input_state) noexcept
{
    for(auto& obj : m_objects)
    {
        obj.update(m_renderer, dt);
    }
    m_player->update(m_renderer, dt);

    /*advance time of day (wrap around after a whole day has passed)*/
    m_time_of_day += dt * m_time_scale;

    if(m_time_of_day >= m_day_length)
    {
        m_time_of_day -= m_day_length;
    }

    m_time_of_day = 11.0f * 3600.0f;

    updateSun();

    if(m_player_movement)
    {
        playerControls(input_state);

        //TODO:should we rotate before translation or after?
        if(m_player->rotVelocity() != 0.0f)
        {
            tryPlayerRotation(dt);
        }

        tryPlayerMove(dt);

        const vec3 player_camera_reference_point = m_player->pos() + vec3(0.0f, 5.0f, 0.0f);

        m_camera.setPos(player_camera_reference_point + sphericalToCartesian(m_player_camera_radius, m_player_camera_theta, m_player_camera_phi));

        const vec3 cam_forward = normalize(player_camera_reference_point - m_camera.pos());
        const vec3 cam_right = cross(vec3(0.0f, 1.0f, 0.0f), cam_forward);
        const vec3 cam_up = cross(cam_forward, cam_right);

        m_camera.setBasis(cam_forward, cam_up, cam_right);
    }
    else
    {
        const float cam_speed = input_state.keyboard[VKeyLShift] ? 42.0f : 14.0f;

        if(input_state.keyboard[VKeyW])
        {
            m_camera.walk(dt*cam_speed);
        }

        if(input_state.keyboard[VKeyS])
        {
            m_camera.walk(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeyA])
        {
            m_camera.strafe(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeyD])
        {
            m_camera.strafe(dt*cam_speed);
        }

        if(input_state.keyboard[VKeyC])
        {
            m_camera.tilt(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeySpace])
        {
            m_camera.tilt(dt*cam_speed);
        }
    }
}

void Scene::draw(RenderData& render_data) noexcept
{
    render_data.visual_sun_pos = m_visual_sun_pos;
    render_data.effective_sun_pos = m_effective_sun_pos;
    render_data.sun_radius = m_sun_radius;
    render_data.terrain_patch_size = m_terrain->patchSize();

    //TODO: do frustum culling here
    const auto view_frustum_planes = m_camera.viewFrustumPlanesW();

    for(auto& obj : m_objects)
    {
        if(obj.cull(view_frustum_planes))
        {
            obj.draw(m_renderer);
        }
    }
    if(m_player->cull(view_frustum_planes))
    {
        m_player->draw(m_renderer);
    }

    m_terrain->draw(m_renderer);
}

void Scene::tryPlayerRotation(float dt)
{
    const float rot_angle = m_player->rotVelocity() * dt;
    const quat rot = normalize(rotate(m_player->rot(), rot_angle, vec3(0.0f, 1.0f, 0.0f)));

    if(!objectRotateCollision(m_player.get(), rot))
    {
        m_player->rotateY(rot_angle);
    }
}

void Scene::tryPlayerMove(float dt)
{
    const float max_dh = 0.5f;
    float total_dh = max_dh;
    vec3 player_velocity = m_player->velocity() + m_player->acceleration() * dt;

    for(uint8_t axis = 0; axis < 3; axis++)
    {
        vec3 s = {0.0f, 0.0f, 0.0f};
        s[axis] = player_velocity[axis] * dt;

        AABB aabb = m_player->aabbs()[0];
        aabb.transform(m_player->pos() + s, m_player->scale());
        const float dh  = m_terrain->collision(aabb, max_dh);

        if(dh > 0.0f)
        {
            if(dh > max_dh) // collision
            {
                if(axis == 1)
                {
                    if(player_velocity.y < 0.0f)
                    {
                        m_player->setAcceleration(vec3(0.0f, 0.0f, 0.0f));
                    }
                    player_velocity.y = 0.0f;
                }
                continue;
            }
            else
            {
                s.y += dh;
                total_dh -=dh;
            }
        }
        else
        {
            if(dh < -max_dh)
            {
                //first, if player has no vertical acceleration, check if the player is airborne
                if(m_player->acceleration().y == 0.0f)
                {
                    if(!objectMoveCollision(m_player.get(), vec3(0.0f, -0.001f, 0.0f)))
                    {
                        m_player->setAcceleration(vec3(0.0f, -9.8f, 0.0f));
                    }
                }
            }
            else
            {
                s.y += dh;
                total_dh -=dh;
            }
        }

        if(axis != 1)
        {
            bool collision = false;

            while(objectMoveCollision(m_player.get(), s))
            {
                s.y += 0.05f;

                if(s.y > total_dh)
                {
                    collision = true;
                    break;
                }
            }

            if(!collision)
            {
                m_player->move(s);
            }
        }
        else if(!objectMoveCollision(m_player.get(), s))
        {
            m_player->move(s);
        }
        else
        {
            if(player_velocity.y < 0.0f)
            {
                m_player->setAcceleration(vec3(0.0f, 0.0f, 0.0f));
            }

            player_velocity.y = 0.0f;
        }

    }

    m_player->setVelocity(player_velocity);
}

bool Scene::objectRotateCollision(const Object* object, const quat& rot)
{
    //TODO: what if object has AABBs?
    for(auto bb : object->bbs())
    {
        bb.transform(object->pos(), rot, object->scale());

        if(objectCollision(object, bb))
        {
            return true;
        }
    }

    return false;
}

bool Scene::objectMoveCollision(const Object* obj, vec3 s)
{
    for(Sphere sphere : obj->spheres())
    {
        sphere.transform(obj->pos() + s, obj->scale());

        if(objectCollision(obj, sphere))
        {
            return true;
        }
    }

    //TODO: what if there's rotation?
    for(AABB aabb : obj->aabbs())
    {
        aabb.transform(obj->pos() + s, obj->scale());

        if(objectCollision(obj, aabb))
        {
            return true;
        }
    }

    for(BoundingBox bb : obj->bbs())
    {
        bb.transform(obj->pos() + s, obj->rot(), obj->scale());

        if(objectCollision(obj, bb))
        {
            return true;
        }
    }

    return false;
}

void Scene::updateSun()
{
    const float t = m_time_of_day / m_day_length;

    const float azimuth = pi_2 - 2.0f * pi * t;
    float inclination = pi_2 - std::asin(std::sin(m_lat) * std::sin(m_decl) + std::cos(m_lat) * std::cos(m_decl) * std::cos(pi / 12.0f * ((m_time_of_day - m_lst_offset) / 3600.0f)));

    m_visual_sun_pos = vec3(std::cos(azimuth) * std::sin(inclination),
                        std::cos(inclination),
                        std::sin(azimuth) * std::sin(inclination));

    if(m_visual_sun_pos.y < m_sun_radius)
    {
        const float alpha = std::acos(std::clamp(-m_visual_sun_pos.y/m_sun_radius, -1.0f, 1.0f));
        const float visible_sun_area = (2.0f*alpha - std::sin(2.0f*alpha)) / (2.0f * pi);

        inclination -= (4.0f * m_sun_radius * std::pow(std::sin(alpha), 3.0f)) / (3.0f * (2.0f*alpha - std::sin(2.0f*alpha)));

        m_effective_sun_pos = vec3(std::cos(azimuth) * std::sin(inclination),
                                   std::cos(inclination),
                                   std::sin(azimuth) * std::sin(inclination));

        m_sun.color = vec3(0.5f) * visible_sun_area;
    }
    else
    {
        m_effective_sun_pos = m_visual_sun_pos;
        m_sun.color = vec3(0.5f);
    }

    m_sun.dir = -m_effective_sun_pos;

    m_renderer.updateDirLight(0, m_sun);
}

template<class Collider>
bool Scene::objectCollision(const Object* object, const Collider& collider)
{
    for(const auto& obj : m_objects)
    {
        if(&obj == object)
        {
            continue;
        }

        for(Sphere sphere : obj.spheres())
        {
            sphere.transform(obj.pos(), obj.scale());

            if(intersect(collider, sphere))
            {
                return true;
            }
        }

        for(AABB aabb : obj.aabbs())
        {
            aabb.transform(obj.pos(), obj.scale());

            if(intersect(collider, aabb))
            {
                return true;
            }
        }

        for(BoundingBox bb : obj.bbs())
        {
            bb.transform(obj.pos(), obj.rot(), obj.scale());

            if(intersect(collider, bb))
            {
                return true;
            }
        }
    }

    return false;
}

void Scene::onWindowResize(float aspect_ratio) noexcept
{
    m_camera.setAspectRatio(aspect_ratio);
}

Camera& Scene::camera() noexcept
{
    return m_camera;
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

Terrain& Scene::terrain() noexcept
{
    return *m_terrain;
}

void Scene::playerControls(const InputState& input_state)
{
    vec3 v = vec3(0.0f, 0.0f, 0.0f);

    vec3 forward = m_camera.forward();
    forward.y = 0;

    vec3 right = m_camera.right();
    right.y = 0;

    bool walk = false;

    if(input_state.keyboard[VKeyW] && !input_state.keyboard[VKeyS])
    {
        v += forward;
        walk = true;
    }
    else if(input_state.keyboard[VKeyS] && !input_state.keyboard[VKeyW])
    {
        v -= forward;
        walk = true;
    }

    if(input_state.keyboard[VKeyD] && !input_state.keyboard[VKeyA])
    {
        v += right;
        walk = true;
    }
    else if(input_state.keyboard[VKeyA] && !input_state.keyboard[VKeyD])
    {
        v -= right;
        walk = true;
    }

    if(walk)
    {
        m_player->walk(normalize(v));
    }
    else
    {
        m_player->stop();
    }
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

    float _d;
    if(m_player->rayIntersetion(rayW, d, _d))
    {
        d = std::min(d, _d);
        closest_obj = m_player.get();
    }

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

void Scene::serialize(std::string_view filename) const
{
    //backup existing scene and terrain files
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

    uint64_t obj_count = 0;

    for(const auto& obj : m_objects)
    {
        if(obj.isSerializable())
        {
            obj_count++;
        }
    }

    outfile.write(reinterpret_cast<const char*>(&obj_count), sizeof(uint64_t));

    for(const auto& obj : m_objects)
    {
        if(obj.isSerializable())
        {
            obj.serialize(outfile);
        }
    }

    const uint64_t point_light_count = m_static_point_lights.size();
    outfile.write(reinterpret_cast<const char*>(&point_light_count), sizeof(uint64_t));

    for(const auto& point_light : m_static_point_lights)
    {
        point_light.serialize(outfile);
    }

    m_terrain->saveToFile();
}
#endif

/*--- input handling ---*/
void Scene::onKeyPressed(Key key, const InputState&)
{
    switch(key)
    {
    case VKeyM:
        m_player_movement = !m_player_movement;
        break;
    case VKeyF:
        m_player->wave();
        break;
    case VKeyR:
        m_player->setPos(vec3(0.0f, 5.0f, -10.0f));
        m_player->walkBack(false);
        m_player->walkForward(false);
        m_player->walkRight(false);
        m_player->walkLeft(false);
        m_player->setVelocity(vec3(0.0f, 0.0f, 0.0f));
        m_player->setAcceleration(vec3(0.0f, 0.0f, 0.0f));
        break;
    case VKeySpace:
        if(m_player->velocity().y == 0)
        {
            m_player->jump();
        }
        break;
    default:
        break;
    }
}

void Scene::onMouseMoved(vec2 cursor_delta, const InputState& input_state)
{
    if(!input_state.mouse)
    {
        if(!m_player_movement)
        {
            m_camera.rotate(cursor_delta.x * 0.005f);
            m_camera.pitch(cursor_delta.y * 0.005f);
        }
        else
        {
            m_player_camera_phi -= cursor_delta.x * 0.01f;
            if(m_player_camera_phi >= 2.0f * pi)
            {
                m_player_camera_phi -= 2.0f * pi;
            }
            else if(m_player_camera_phi <= 0.0f)
            {
                m_player_camera_phi += 2.0f * pi;
            }

            m_player_camera_theta = std::clamp(m_player_camera_theta - cursor_delta.y * 0.01f, 0.001f, pi - 0.001f);
        }
    }
}

void Scene::onMouseScrolledDown(const InputState&)
{
    m_player_camera_radius += 1.0f;

    if(m_player_camera_radius > 10.0f)
    {
        m_player_camera_radius = 10.0f;
    }
}

void Scene::onMouseScrolledUp(const InputState&)
{
    m_player_camera_radius -= 1.0f;

    if(m_player_camera_radius < 3.0f)
    {
        m_player_camera_radius = 3.0f;
    }
}
