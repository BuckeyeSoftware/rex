#include "rx/render/model.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/model/loader.h"

namespace rx::render {

model::model(frontend::interface* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("geometry")}
  , m_buffer{nullptr}
{
}

model::~model() {
  m_frontend->destroy_buffer(RX_RENDER_TAG("model"), m_buffer);
}

bool model::load(const string& _file_name) {
  rx::model::loader model{m_frontend->allocator()};
  if (!model.load(_file_name)) {
    return false;
  }

  m_frontend->destroy_buffer(RX_RENDER_TAG("model"), m_buffer);
  m_buffer = m_frontend->create_buffer(RX_RENDER_TAG("model"));
  m_buffer->record_type(frontend::buffer::type::k_static);

  if (model.is_animated()) {
    using vertex = rx::model::loader::animated_vertex;
    const auto &vertices{model.animated_vertices()};
    m_buffer->record_stride(sizeof(vertex));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 3, offsetof(vertex, position));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 3, offsetof(vertex, normal));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 4, offsetof(vertex, tangent));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, coordinate));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_u8, 4, offsetof(vertex, blend_weights));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_u8, 4, offsetof(vertex, blend_indices));
    m_buffer->write_vertices(vertices.data(), vertices.size() * sizeof(vertex));
  } else {
    using vertex = rx::model::loader::vertex;
    const auto &vertices{model.vertices()};
    m_buffer->record_stride(sizeof(vertex));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 3, offsetof(vertex, position));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 3, offsetof(vertex, normal));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 4, offsetof(vertex, tangent));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, coordinate));
    m_buffer->write_vertices(vertices.data(), vertices.size() * sizeof(vertex));
  }

  const auto &elements{model.elements()};
  m_buffer->record_element_type(frontend::buffer::element_type::k_u32);
  m_buffer->write_elements(elements.data(), elements.size() * sizeof(rx_u32));
  m_frontend->initialize_buffer(RX_RENDER_TAG("model"), m_buffer);

  m_materials.clear();

  // Map all the loaded material::loader's to render::frontend::material's while
  // using indices to refer to them rather than strings.
  map<string, rx_size> material_indices{m_frontend->allocator()};
  model.materials().each([this, &material_indices](rx_size, const string& _name, material::loader& _material) {
    frontend::material material{m_frontend};
    if (material.load(utility::move(_material))) {
      const rx_size material_index{m_materials.size()};
      material_indices.insert(_name, material_index);
      m_materials.push_back(utility::move(material));
    }
  });

  // Resolve all the meshes of the loaded model.
  model.meshes().each_fwd([this, &material_indices](const rx::model::mesh& _mesh) {
    if (auto* find = material_indices.find(_mesh.material)) {
      m_meshes.push_back({_mesh.offset, _mesh.count, *find});
    }
  });

  return true;
}

void model::render(frontend::target* _target, const math::mat4x4f& _model,
  const math::mat4x4f& _view, const math::mat4x4f& _projection)
{
  frontend::state state;
  state.depth.record_test(true);
  state.depth.record_write(true);

  state.cull.record_enable(true);
  state.cull.record_front_face(frontend::cull_state::front_face_type::k_clock_wise);
  state.cull.record_cull_face(frontend::cull_state::cull_face_type::k_back);

  state.blend.record_enable(false);

  state.viewport.record_dimensions(_target->dimensions());

  m_meshes.each_fwd([&](const mesh& _mesh) {
    const auto& material{m_materials[_mesh.material]};

    rx_u64 flags{0};
    if (material.diffuse())    flags |= 1 << 1;
    if (material.normal())     flags |= 1 << 2;
    if (material.metalness())  flags |= 1 << 3;
    if (material.roughness())  flags |= 1 << 4;
    if (material.alpha_test()) flags |= 1 << 5;
    
    // Disable backface culling for alpha-tested geometry.
    state.cull.record_enable(!material.alpha_test());

    frontend::program* program{m_technique->permute(flags)};
    auto& uniforms{program->uniforms()};

    uniforms[0].record_mat4x4f(_model * _view * _projection);
    uniforms[1].record_mat4x4f(_model);
    uniforms[2].record_mat3x3f(material.transform().to_mat3());

    m_frontend->draw(
      RX_RENDER_TAG("model mesh"),
      state,
      _target,
      m_buffer,
      program,
      _mesh.count,
      _mesh.offset,
      render::frontend::primitive_type::k_triangles,
      material.normal() ? "22" : "2",
      material.diffuse(),
      material.normal(),
      material.metalness(),
      material.roughness());
  });
}

} // namespace rx::render