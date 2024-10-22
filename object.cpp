#include "object.h"

Object::Object(Renderer& renderer, std::ifstream& scene_file)
{
    uint8_t mesh_filename_length = 0;
    scene_file.read(reinterpret_cast<char*>(&mesh_filename_length), sizeof(uint8_t));
    std::string model_filename;
    model_filename.resize(mesh_filename_length);
    scene_file.read(reinterpret_cast<char*>(model_filename.data()), mesh_filename_length);
    scene_file.read(reinterpret_cast<char*>(&m_render_mode), sizeof(RenderMode));
    scene_file.read(reinterpret_cast<char*>(&m_pos), sizeof(vec3));
    scene_file.read(reinterpret_cast<char*>(&m_scale), sizeof(vec3));
    scene_file.read(reinterpret_cast<char*>(&m_rot), sizeof(quat));

    m_model = std::make_unique<Model>(renderer, model_filename);
    m_instance_data.resize(m_model->mehes().size());
    m_instance_id = renderer.reqInstanceVBAlloc(m_instance_data.size());

    for(auto& instance_data : m_instance_data)
    {
        instance_data.bone_offset = m_model->boneOffset();
    }

#if EDITOR_ENABLE
    m_model_filename = model_filename;
#endif
}

Object::Object(Renderer& renderer, std::string_view model_filename, RenderMode render_mode, vec3 pos, vec3 scale, quat rot)
    : m_pos(pos)
    , m_scale(scale)
    , m_rot(rot)
    , m_render_mode(render_mode)
#if EDITOR_ENABLE
    , m_model_filename(model_filename)
#endif

{
    m_model = std::make_unique<Model>(renderer, model_filename);
    m_instance_data.resize(m_model->mehes().size());
    m_instance_id = renderer.reqInstanceVBAlloc(m_instance_data.size());

    for(auto& instance_data : m_instance_data)
    {
        instance_data.bone_offset = m_model->boneOffset();
    }
}

bool Object::cull(const std::array<vec4, 6>& frustum_planes_W)
{
    // Sphere s = m_model->boundingSphere();
    // s.transform(m_pos, m_scale);
    return true;
}

void Object::update(Renderer& renderer, float dt)
{
    m_model->animationUpdate(renderer, dt);
}

void Object::draw(Renderer& renderer)
{
    for(uint32_t i = 0; i < m_model->mehes().size(); i++)
    {
       const auto& mesh = m_model->mehes()[i];

       //TODO: only update instance data if the instance data has really changed (measure if any performance boost)
       m_instance_data[i].W = glm::translate(m_pos) * mat4_cast(m_rot) * glm::scale(m_scale);
       m_instance_data[i].tex_id = mesh.textureId();
       m_instance_data[i].normal_map_id = mesh.normalMapId();

        renderer.draw(m_render_mode, mesh.vertexBuffer(), mesh.vertexBufferOffset(), mesh.vertexCount(), m_instance_id + i);
    }

    renderer.updateInstanceVertexData(m_instance_id, m_instance_data.size(), m_instance_data.data());
}

const std::vector<AABB>& Object::aabbs() const
{
    return m_model->aabbs();
}

const std::vector<BoundingBox>& Object::bbs() const
{
    return m_model->bbs();
}

const std::vector<Sphere>& Object::spheres() const
{
    return m_model->spheres();
}

#if 0
AABB Object::aabb()
{
    updateVertexGroup();

    AABB res = m_aabb;
    res.p_min = m_vertex_group.per_instance_data.W * vec4(res.p_min, 1.0f);
    res.p_max = m_vertex_group.per_instance_data.W * vec4(res.p_max, 1.0f);

    return res;
}
#endif

vec3 Object::pos() const
{
    return m_pos;
}

vec3 Object::scale() const
{
    return m_scale;
}

const quat& Object::rot() const
{
    return m_rot;
}

vec3 Object::acceleration() const
{
    return m_acc;
}

vec3 Object::velocity() const
{
    return m_velocity;
}

float Object::rotVelocity() const
{
    return m_rot_velocity;
}

void Object::setPos(vec3 pos)
{
    m_pos = pos;
}

void Object::move(vec3 d)
{
    m_pos += d;
}

void Object::setRotation(quat rot)
{
    m_rot = rot;
}

void Object::rotate(vec3 axis, float a)
{
    m_rot = normalize(glm::rotate(m_rot, a, axis));
}

void Object::rotateX(float a)
{
    rotate(vec3(1.0f, 0.0f, 0.0f), a);
}

void Object::rotateY(float a)
{
    rotate(vec3(0.0f, 1.0f, 0.0f), a);
}

void Object::rotateZ(float a)
{
    rotate(vec3(0.0f, 0.0f, 1.0f), a);
}

void Object::setScale(vec3 scale)
{
    m_scale = scale;
}

void Object::setVelocity(vec3 v)
{
    m_velocity = v;
}

void Object::setAcceleration(vec3 a)
{
    m_acc = a;
}

void Object::setVisible(bool visible)
{
    m_visible = visible;
}

bool Object::isVisible() const
{
    return m_visible;
}

void Object::playAnimation(std::string_view anim_name)
{
    m_model->playAnimation(anim_name);
}

void Object::stopAnimation()
{
    m_model->stopAnimation();
}

void Object::setPose(std::string_view pose_name)
{
    m_model->setPose(pose_name);
}

#if EDITOR_ENABLE

bool Object::isSerializable() const
{
    return m_serialiable;
}

void Object::setSerializable(bool serializable)
{
    m_serialiable = serializable;
}

void Object::serialize(std::ofstream& outfile) const
{
    const uint8_t mesh_filename_length = static_cast<uint8_t>(m_model_filename.size());
    outfile.write(reinterpret_cast<const char*>(&mesh_filename_length), sizeof(uint8_t));
    outfile.write(reinterpret_cast<const char*>(m_model_filename.data()), mesh_filename_length);
    outfile.write(reinterpret_cast<const char*>(&m_render_mode), sizeof(RenderMode));
    outfile.write(reinterpret_cast<const char*>(&m_pos), sizeof(vec3));
    outfile.write(reinterpret_cast<const char*>(&m_scale), sizeof(vec3));
    outfile.write(reinterpret_cast<const char*>(&m_rot), sizeof(quat));
}

void Object::drawHighlight(Renderer& renderer)
{
    for(const auto& mesh : m_model->mehes())
    {
        renderer.draw(RenderMode::Highlight, mesh.vertexBuffer(), mesh.vertexBufferOffset(), mesh.vertexCount(), m_instance_id);
    }
}

bool Object::rayIntersetion(const Ray& rayW, float min_d, float& d) const
{
    //TODO: maybe we already have this calculated and don't have to recalulate here?
    const auto rot = mat4_cast(m_rot);
    const mat4x4 W = glm::translate(m_pos) * rot * glm::scale(m_scale);

    //TODO:if we're going to calculate W here than we can directly calculate the inverse
    //by inverting the order and negating arguments to scale(), translate() etc.
    const Ray rayL = rayW.transformed(glm::inverse(W));

    return m_model->rayIntersetion(rayL, min_d, d);
}
#endif
