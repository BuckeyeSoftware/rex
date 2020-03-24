#include "rx/render/model.h"
#include "rx/render/ibl.h"
#include "rx/render/immediate3D.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/math/frustum.h"

#include "rx/core/profiler.h"

namespace rx::render {

model::model(frontend::context* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("geometry")}
  , m_buffer{nullptr}
  , m_materials{m_frontend->allocator()}
  , m_opaque_meshes{m_frontend->allocator()}
  , m_transparent_meshes{m_frontend->allocator()}
  , m_model{m_frontend->allocator()}
{
}

model::~model() {
  m_frontend->destroy_buffer(RX_RENDER_TAG("model"), m_buffer);
}

bool model::upload() {
  m_frontend->destroy_buffer(RX_RENDER_TAG("model"), m_buffer);
  m_buffer = m_frontend->create_buffer(RX_RENDER_TAG("model"));
  m_buffer->record_type(frontend::buffer::type::k_static);

  if (m_model.is_animated()) {
    using vertex = rx::model::loader::animated_vertex;
    const auto &vertices{m_model.animated_vertices()};
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
    const auto &vertices{m_model.vertices()};
    m_buffer->record_stride(sizeof(vertex));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 3, offsetof(vertex, position));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 3, offsetof(vertex, normal));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 4, offsetof(vertex, tangent));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, coordinate));
    m_buffer->write_vertices(vertices.data(), vertices.size() * sizeof(vertex));
  }

  const auto &elements{m_model.elements()};
  m_buffer->record_element_type(frontend::buffer::element_type::k_u32);
  m_buffer->write_elements(elements.data(), elements.size() * sizeof(rx_u32));
  m_frontend->initialize_buffer(RX_RENDER_TAG("model"), m_buffer);

  m_materials.clear();

  // Map all the loaded material::loader's to render::frontend::material's while
  // using indices to refer to them rather than strings.
  map<string, rx_size> material_indices{m_frontend->allocator()};
  const bool material_load_result =
    m_model.materials().each_pair([this, &material_indices](const string& _name, material::loader& material_) {
      frontend::material material{m_frontend};
      if (material.load(utility::move(material_))) {
        const rx_size material_index{m_materials.size()};
        material_indices.insert(_name, material_index);
        m_materials.push_back(utility::move(material));
        return true;
      }
      return false;
    });

  if (!material_load_result) {
    return false;
  }

  // Resolve all the meshes of the loaded model.
  m_model.meshes().each_fwd([this, &material_indices](const rx::model::mesh& _mesh) {
    if (auto* find = material_indices.find(_mesh.material)) {
      if (m_materials[*find].has_alpha()) {
        m_transparent_meshes.push_back({_mesh.offset, _mesh.count, *find, _mesh.bounds});
      } else {
        m_opaque_meshes.push_back({_mesh.offset, _mesh.count, *find, _mesh.bounds});
      }
    }
    m_aabb.expand(_mesh.bounds);
  });

  return true;
}

void model::animate(rx_size _index, [[maybe_unused]] bool _loop) {
  if (m_model.is_animated()) {
    m_animation = {&m_model, _index};
  }
}

void model::update(rx_f32 _delta_time) {
  if (m_animation) {
    m_animation->update(_delta_time, true);
  }
}

void model::render(frontend::target* _target, const math::mat4x4f& _model,
  const math::mat4x4f& _view, const math::mat4x4f& _projection)
{
  math::frustum frustum{_view * _projection};

  profiler::cpu_sample cpu_sample{"model::render"};
  profiler::gpu_sample gpu_sample{"model::render"};

  frontend::state state;

  // Enable(DEPTH_TEST)
  state.depth.record_test(true);

  // Enable(STENCIL_TEST)
  state.stencil.record_enable(true);

  // Enable(CULL_FACE)
  state.cull.record_enable(true);

  // Disable(BLEND)
  state.blend.record_enable(false);

  // FrontFace(CW)
  state.cull.record_front_face(frontend::cull_state::front_face_type::k_clock_wise);

  // CullFace(BACK)
  state.cull.record_cull_face(frontend::cull_state::cull_face_type::k_back);

  // DepthMask(TRUE)
  state.depth.record_write(true);

  // StencilFunc(ALWAYS, 1, 0xFF)
  state.stencil.record_function(frontend::stencil_state::function_type::k_always);
  state.stencil.record_reference(1);
  state.stencil.record_mask(0xFF);

  // StencilMask(0xFF)
  state.stencil.record_write_mask(0xFF);

  // StencilOp(KEEP, REPLACE, REPLACE)
  state.stencil.record_fail_action(frontend::stencil_state::operation_type::k_keep);
  state.stencil.record_depth_fail_action(frontend::stencil_state::operation_type::k_replace);
  state.stencil.record_depth_pass_action(frontend::stencil_state::operation_type::k_replace);

  // Viewport(0, 0, w, h)
  state.viewport.record_dimensions(_target->dimensions());

  m_opaque_meshes.each_fwd([&](const mesh& _mesh) {
    if (!frustum.is_aabb_inside(_mesh.bounds.transform(_model))) {
      return true;
    }

    profiler::cpu_sample cpu_sample{"batch"};
    profiler::gpu_sample gpu_sample{"batch"};

    const auto& material{m_materials[_mesh.material]};

    rx_u64 flags{0};
    if (m_animation)           flags |= 1 << 0;
    if (material.albedo())     flags |= 1 << 1;
    if (material.normal())     flags |= 1 << 2;
    if (material.metalness())  flags |= 1 << 3;
    if (material.roughness())  flags |= 1 << 4;
    if (material.alpha_test()) flags |= 1 << 5;
    if (material.ambient())    flags |= 1 << 6;
    if (material.emissive())   flags |= 1 << 7;

    frontend::program* program{m_technique->permute(flags)};
    auto& uniforms{program->uniforms()};

    uniforms[0].record_mat4x4f(_model);
    uniforms[1].record_mat4x4f(_view);
    uniforms[2].record_mat4x4f(_projection);
    if (const auto transform{material.transform()}) {
      uniforms[3].record_mat3x3f(transform->to_mat3());
    }

    if (m_animation) {
      uniforms[4].record_bones(m_animation->frames(), m_animation->joints());
    }

    uniforms[5].record_vec2f({material.roughness_value(), material.metalness_value()});

    // Record all the textures.
    frontend::textures draw_textures;
    if (material.albedo())    uniforms[ 6].record_sampler(draw_textures.add(material.albedo()));
    if (material.normal())    uniforms[ 7].record_sampler(draw_textures.add(material.normal()));
    if (material.metalness()) uniforms[ 8].record_sampler(draw_textures.add(material.metalness()));
    if (material.roughness()) uniforms[ 9].record_sampler(draw_textures.add(material.roughness()));
    if (material.ambient())   uniforms[10].record_sampler(draw_textures.add(material.ambient()));
    if (material.emissive())  uniforms[11].record_sampler(draw_textures.add(material.emissive()));

    // Record all the draw buffers.
    frontend::buffers draw_buffers;
    draw_buffers.add(0);
    draw_buffers.add(1);
    draw_buffers.add(2);
    draw_buffers.add(3);

    // Disable backface culling for alpha-tested geometry.
    state.cull.record_enable(!material.alpha_test());

    m_frontend->draw(
      RX_RENDER_TAG("model mesh"),
      state,
      _target,
      draw_buffers,
      m_buffer,
      program,
      _mesh.count,
      _mesh.offset,
      render::frontend::primitive_type::k_triangles,
      draw_textures);

    return true;
  });
}

void model::render_normals(const math::mat4x4f& _world, render::immediate3D* _immediate) {
  static constexpr rx_f32 k_size{0.1f};

  if (m_model.is_animated()) {
    m_model.animated_vertices().each_fwd([&](const rx::model::loader::animated_vertex& _vertex) {
      const math::vec3f point_a{_vertex.position};
      const math::vec3f point_b{_vertex.position + _vertex.normal * k_size};

      const math::vec3f color{_vertex.normal * 0.5f + 0.5f};

      if (m_animation) {
        // CPU skeletal animation of the lines.
        const auto& frames{m_animation->frames()};

        math::mat3x4f transform;
        transform  = frames[_vertex.blend_indices.x] * (_vertex.blend_weights.x / 255.0f);
        transform += frames[_vertex.blend_indices.y] * (_vertex.blend_weights.y / 255.0f);
        transform += frames[_vertex.blend_indices.z] * (_vertex.blend_weights.z / 255.0f);
        transform += frames[_vertex.blend_indices.w] * (_vertex.blend_weights.w / 255.0f);

        const math::vec3f& x{transform.x.x, transform.y.x, transform.z.x};
        const math::vec3f& y{transform.x.y, transform.y.y, transform.z.y};
        const math::vec3f& z{transform.x.z, transform.y.z, transform.z.z};
        const math::vec3f& w{transform.x.w, transform.y.w, transform.z.w};

        const math::mat4x4f& mat{{x.x, x.y, x.z, 0.0f},
                                 {y.x, y.y, y.z, 0.0f},
                                 {z.x, z.y, z.z, 0.0f},
                                 {w.x, w.y, w.z, 1.0f}};

        _immediate->frame_queue().record_line(
          math::mat4x4f::transform_point(point_a, mat * _world),
          math::mat4x4f::transform_point(point_b, mat * _world),
          {color.r, color.g, color.b, 1.0f},
          immediate3D::k_depth_test | immediate3D::k_depth_write);
      } else {
        _immediate->frame_queue().record_line(
          math::mat4x4f::transform_point(point_a, _world),
          math::mat4x4f::transform_point(point_b, _world),
          {color.r, color.g, color.b, 1.0f},
          immediate3D::k_depth_test | immediate3D::k_depth_write);
      }
    });
  } else {
    m_model.vertices().each_fwd([&](const rx::model::loader::vertex& _vertex) {
      const math::vec3f point_a{_vertex.position};
      const math::vec3f point_b{_vertex.position + _vertex.normal * k_size};

      const math::vec3f color{_vertex.normal * 0.5f + 0.5f};

      _immediate->frame_queue().record_line(
        math::mat4x4f::transform_point(point_a, _world),
        math::mat4x4f::transform_point(point_b, _world),
        {color.r, color.g, color.b, 1.0f},
        immediate3D::k_depth_test | immediate3D::k_depth_write);
    });
  }
}

void model::render_skeleton(const math::mat4x4f& _world, render::immediate3D* _immediate) {
  if (!m_animation) {
    return;
  }

  const auto& joints{m_model.joints()};

  // Render all the joints.
  for (rx_size i{0}; i < joints.size(); i++) {
    const math::mat3x4f& frame{m_animation->frames()[i] * joints[i].frame};

    const math::vec3f& x{math::normalize({frame.x.x, frame.y.x, frame.z.x})};
    const math::vec3f& y{math::normalize({frame.x.y, frame.y.y, frame.z.y})};
    const math::vec3f& z{math::normalize({frame.x.z, frame.y.z, frame.z.z})};
    const math::vec3f& w{frame.x.w, frame.y.w, frame.z.w};

    const math::mat4x4f& joint{{x.x, x.y, x.z, 0.0f},
                               {y.x, y.y, y.z, 0.0f},
                               {z.x, z.y, z.z, 0.0f},
                               {w.x, w.y, w.z, 1.0f}};

    _immediate->frame_queue().record_solid_sphere(
      {16.0f, 16.0f},
      {0.5f, 0.5f, 1.0f, 1.0f},
      math::mat4x4f::scale(m_aabb.scale() * 0.01f) * joint * _world,
      0);
  }

  // Render the skeleton.
  for (rx_size i{0}; i < joints.size(); i++) {
    const math::mat3x4f& frame{m_animation->frames()[i] * joints[i].frame};
    const math::vec3f& w{frame.x.w, frame.y.w, frame.z.w};
    const rx_s32 parent{joints[i].parent};

    if (parent >= 0) {
      const math::mat3x4f& parent_joint{joints[parent].frame};
      const math::mat3x4f& parent_frame{m_animation->frames()[parent] * parent_joint};
      const math::vec3f& parent_position{parent_frame.x.w, parent_frame.y.w, parent_frame.z.w};

      _immediate->frame_queue().record_line(
        math::mat4x4f::transform_point(w, _world),
        math::mat4x4f::transform_point(parent_position, _world),
        {0.5f, 0.5f, 1.0f, 1.0f},
        0);
    }
  }
}

} // namespace rx::render
