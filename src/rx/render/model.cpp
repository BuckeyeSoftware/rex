#include "rx/render/model.h"
#include "rx/render/ibl.h"
#include "rx/render/immediate3D.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/model/loader.h"

#include "rx/math/frustum.h"

#include "rx/core/profiler.h"

namespace rx::render {

model::model(frontend::interface* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("geometry")}
  , m_buffer{nullptr}
  , m_materials{m_frontend->allocator()}
  , m_opaque_meshes{m_frontend->allocator()}
  , m_transparent_meshes{m_frontend->allocator()}
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
  model.materials().each_pair([this, &material_indices](const string& _name, material::loader& material_) {
    frontend::material material{m_frontend};
    if (material.load(utility::move(material_))) {
      const rx_size material_index{m_materials.size()};
      material_indices.insert(_name, material_index);
      m_materials.push_back(utility::move(material));
    }
  });

  // Resolve all the meshes of the loaded model.
  model.meshes().each_fwd([this, &material_indices](const rx::model::mesh& _mesh) {
    if (auto* find = material_indices.find(_mesh.material)) {
      if (m_materials[*find].has_alpha()) {
        m_transparent_meshes.push_back({_mesh.offset, _mesh.count, *find, _mesh.bounds});
      } else {
        m_opaque_meshes.push_back({_mesh.offset, _mesh.count, *find, _mesh.bounds});
      }
    }
  });

  return true;
}

void model::render(ibl* _ibl, frontend::target* _target, const math::mat4x4f& _model,
  const math::mat4x4f& _view, const math::mat4x4f& _projection)
{
  math::frustum frustum{_view * _projection};

  profiler::cpu_sample cpu_sample{"model::render"};
  profiler::gpu_sample gpu_sample{"model::render"};

  frontend::state state;
  state.depth.record_test(true);
  state.depth.record_write(true);

  state.cull.record_enable(true);
  state.cull.record_front_face(frontend::cull_state::front_face_type::k_clock_wise);
  state.cull.record_cull_face(frontend::cull_state::cull_face_type::k_back);

  state.blend.record_enable(false);

  state.viewport.record_dimensions(_target->dimensions());

  m_opaque_meshes.each_fwd([&](const mesh& _mesh) {
    if (!frustum.is_aabb_inside(_mesh.bounds.transform(_model))) {
      return true;
    }

    profiler::cpu_sample cpu_sample{"batch"};
    profiler::gpu_sample gpu_sample{"batch"};

    const auto& material{m_materials[_mesh.material]};

    rx_u64 flags{0};
    if (material.diffuse())    flags |= 1 << 1;
    if (material.normal())     flags |= 1 << 2;
    if (material.metalness())  flags |= 1 << 3;
    if (material.roughness())  flags |= 1 << 4;
    if (material.alpha_test()) flags |= 1 << 5;
    if (material.ambient())    flags |= 1 << 6;
    if (material.emission())   flags |= 1 << 7;

    frontend::program* program{m_technique->permute(flags)};
    auto& uniforms{program->uniforms()};

    const auto& camera{math::mat4x4f::invert(_view).w};
    uniforms[0].record_mat4x4f(_model);
    uniforms[1].record_mat4x4f(_view);
    uniforms[2].record_mat4x4f(_projection);
    uniforms[3].record_vec3f({camera.x, camera.y, camera.z});
    if (const auto transform{material.transform()}) {
      uniforms[4].record_mat3x3f(transform->to_mat3());
    }

    // TODO: skeletal.

    // Record all the textures.
    frontend::draw_textures textures;
    uniforms[6].record_sampler(textures.add(_ibl->irradiance()));
    uniforms[7].record_sampler(textures.add(_ibl->prefilter()));
    uniforms[8].record_sampler(textures.add(_ibl->scale_bias()));
    if (material.diffuse())   uniforms[ 9].record_sampler(textures.add(material.diffuse()));
    if (material.normal())    uniforms[10].record_sampler(textures.add(material.normal()));
    if (material.metalness()) uniforms[11].record_sampler(textures.add(material.metalness()));
    if (material.roughness()) uniforms[12].record_sampler(textures.add(material.roughness()));
    if (material.ambient())   uniforms[13].record_sampler(textures.add(material.ambient()));
    if (material.emission())  uniforms[14].record_sampler(textures.add(material.emission()));

    // Disable backface culling for alpha-tested geometry.
    state.cull.record_enable(!material.alpha_test());

    void* const* handles{textures.handles()};

    m_frontend->draw(
      RX_RENDER_TAG("model mesh"),
      state,
      _target,
      "0123",
      m_buffer,
      program,
      _mesh.count,
      _mesh.offset,
      render::frontend::primitive_type::k_triangles,
      textures.specification(),
      handles[0],
      handles[1],
      handles[2],
      handles[3],
      handles[4],
      handles[5],
      handles[6],
      handles[7],
      handles[8]);

    return true;
  });
}

void model::render_normals(const math::mat4x4f& _world, render::immediate3D* _immediate) {
  const vector<rx_byte>& vertices{m_buffer->vertices()};
  const rx_size stride{m_buffer->stride()};
  const rx_size n_vertices{vertices.size() / stride};
  const rx_byte* vertex{vertices.data()};

  for (rx_size i = 0; i < n_vertices; i++) {
    const auto* position{reinterpret_cast<const math::vec3f*>(vertex)};
    const auto* normal{reinterpret_cast<const math::vec3f*>(vertex + sizeof *position)};

    const math::vec3f point_a{math::mat4x4f::transform_point(*position, _world)};
    const math::vec3f point_b{math::mat4x4f::transform_point(*position + *normal * 0.05f, _world)};

    const math::vec3f color{*normal * 0.5f + 0.5f};
    _immediate->frame_queue().record_line(
      point_a,
      point_b,
      {color.r, color.g, color.b, 1.0f},
      immediate3D::k_depth_test | immediate3D::k_depth_write);

    vertex += stride;
  }
}

} // namespace rx::render
