#include <stddef.h> // offsetof
#include <string.h> // memcpy
#include <stdio.h> // printf

#include "rx/render/model.h"
#include "rx/render/image_based_lighting.h"
#include "rx/render/immediate3D.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/arena.h"

#include "rx/math/frustum.h"

#include "rx/core/profiler.h"

namespace Rx::Render {

Model::Model(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("geometry")}
  , m_arena{nullptr}
  , m_materials{m_frontend->allocator()}
  , m_opaque_meshes{m_frontend->allocator()}
  , m_transparent_meshes{m_frontend->allocator()}
  , m_model{m_frontend->allocator().create<Rx::Model::Loader>(m_frontend->allocator())}
{
}


Model::Model(Model&& model_)
  : m_frontend{Utility::exchange(model_.m_frontend, nullptr)}
  , m_technique{Utility::exchange(model_.m_technique, nullptr)}
  , m_arena{Utility::exchange(model_.m_arena, nullptr)}
  , m_block{Utility::move(model_.m_block)}
  , m_materials{Utility::move(model_.m_materials)}
  , m_opaque_meshes{Utility::move(model_.m_opaque_meshes)}
  , m_transparent_meshes{Utility::move(model_.m_transparent_meshes)}
  , m_model{Utility::exchange(model_.m_model, nullptr)}
  , m_animation{Utility::move(model_.m_animation)}
  , m_aabb{Utility::move(model_.m_aabb)}
{
}

Model::~Model() {
  if (m_model) {
    m_frontend->allocator().destroy<Rx::Model::Loader>(m_model);
  }
}

bool Model::upload() {
  // Clear incase being called multiple times for model changes.
  m_materials.clear();
  m_opaque_meshes.clear();
  m_transparent_meshes.clear();

  if (m_model->is_animated()) {
    using Vertex = Rx::Model::Loader::AnimatedVertex;

    Frontend::Buffer::Format format;
    format.record_type(Frontend::Buffer::Type::k_static);
    format.record_element_type(Frontend::Buffer::ElementType::k_u32);
    format.record_vertex_stride(sizeof(Vertex));
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec3f, offsetof(Vertex, position)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec3f, offsetof(Vertex, normal)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec4f, offsetof(Vertex, tangent)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec2f, offsetof(Vertex, coordinate)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec4b, offsetof(Vertex, blend_weights)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec4b, offsetof(Vertex, blend_indices)});
    format.finalize();

    m_arena = m_frontend->arena(format);
    m_block = m_arena;

    const auto &vertices = m_model->animated_vertices();
    const auto size = vertices.size() * sizeof(Vertex);

    m_block.write_vertices(vertices.data(), size);
    m_block.record_vertices_edit(0, size);
  } else {
    using Vertex = Rx::Model::Loader::Vertex;

    Frontend::Buffer::Format format;
    format.record_type(Frontend::Buffer::Type::k_static);
    format.record_element_type(Frontend::Buffer::ElementType::k_u32);
    format.record_vertex_stride(sizeof(Vertex));
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec3f, offsetof(Vertex, position)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec3f, offsetof(Vertex, normal)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec4f, offsetof(Vertex, tangent)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::k_vec2f, offsetof(Vertex, coordinate)});
    format.finalize();

    m_arena = m_frontend->arena(format);
    m_block = m_arena;

    const auto &vertices = m_model->vertices();
    const auto size = vertices.size() * sizeof(Vertex);
    m_block.write_vertices(vertices.data(), size);
    m_block.record_vertices_edit(0, size);
  }

  const auto &elements = m_model->elements();
  const auto size = elements.size() * sizeof(Uint32);
  m_block.write_elements(elements.data(), size);
  m_block.record_elements_edit(0, size);

  m_frontend->update_buffer(RX_RENDER_TAG("Model"), m_arena->buffer());

  m_materials.clear();

  // Map all the loaded material::loader's to render::frontend::material's while
  // using indices to refer to them rather than strings.
  Map<String, Size> material_indices{m_frontend->allocator()};
  const bool material_load_result =
    m_model->materials().each_pair([this, &material_indices](const String& _name, Material::Loader& material_) {
      Frontend::Material material{m_frontend};
      if (material.load(Utility::move(material_))) {
        const Size material_index{m_materials.size()};
        material_indices.insert(_name, material_index);
        m_materials.push_back(Utility::move(material));
        return true;
      }
      return false;
    });

  if (!material_load_result) {
    return false;
  }

  // Resolve all the meshes of the loaded model.
  m_model->meshes().each_fwd([this, &material_indices](const Rx::Model::Mesh& _mesh) {
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

void Model::animate(Size _index, [[maybe_unused]] bool _loop) {
  if (_index != -1_z && m_model->is_animated()) {
    m_animation = {m_model, _index};
  } else {
    m_animation = nullopt;
  }
}

void Model::update(Float32 _delta_time) {
  if (m_animation) {
    m_animation->update(_delta_time, true);
  }
}

void Model::render(Frontend::Target* _target, const Math::Mat4x4f& _model,
                   const Math::Mat4x4f& _view, const Math::Mat4x4f& _projection)
{
  Math::Frustum frustum{_view * _projection};

  RX_PROFILE_CPU("model::render");
  RX_PROFILE_GPU("model::render");

  Frontend::State state;

  // Enable(DEPTH_TEST)
  state.depth.record_test(true);

  // Enable(CULL_FACE)
  state.cull.record_enable(false);

  // Disable(BLEND)
  state.blend.record_enable(false);

  // FrontFace(CW)
  state.cull.record_front_face(Frontend::CullState::FrontFaceType::k_clock_wise);

  // CullFace(BACK)
  state.cull.record_cull_face(Frontend::CullState::CullFaceType::k_back);

  // DepthMask(TRUE)
  state.depth.record_write(true);

  // Viewport(0, 0, w, h)
  state.viewport.record_dimensions(_target->dimensions());

  auto draw = [&](const Mesh& _mesh, bool _allow_cull) {
    if (!frustum.is_aabb_inside(_mesh.bounds.transform(_model))) {
      return true;
    }

    RX_PROFILE_CPU("batch");
    RX_PROFILE_GPU("batch");

    const auto& material{m_materials[_mesh.material]};

    Uint64 flags{0};
    if (m_animation)           flags |= 1 << 0;
    if (material.albedo())     flags |= 1 << 1;
    if (material.normal())     flags |= 1 << 2;
    if (material.metalness())  flags |= 1 << 3;
    if (material.roughness())  flags |= 1 << 4;
    if (material.alpha_test()) flags |= 1 << 5;
    if (material.ambient())    flags |= 1 << 6;
    if (material.emissive())   flags |= 1 << 7;

    Frontend::Program* program{m_technique->permute(flags)};
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

    uniforms[5].record_float(material.roughness_value());
    uniforms[6].record_float(material.metalness_value());
    uniforms[7].record_float(material.occlusion_value());
    uniforms[8].record_vec3f(material.albedo_color());
    uniforms[9].record_vec3f(material.emission_color());

    // Record all the textures.
    Frontend::Textures draw_textures;
    if (material.albedo())    uniforms[10].record_sampler(draw_textures.add(material.albedo()));
    if (material.normal())    uniforms[11].record_sampler(draw_textures.add(material.normal()));
    if (material.metalness()) uniforms[12].record_sampler(draw_textures.add(material.metalness()));
    if (material.roughness()) uniforms[13].record_sampler(draw_textures.add(material.roughness()));
    if (material.ambient())   uniforms[14].record_sampler(draw_textures.add(material.ambient()));
    if (material.emissive())  uniforms[15].record_sampler(draw_textures.add(material.emissive()));

    // Record all the draw buffers.
    Frontend::Buffers draw_buffers;
    draw_buffers.add(0); // gbuffer albedo    (albedo.r,   albedo.g,   albedo.b,   ambient)
    draw_buffers.add(1); // gbuffer normal    (normal.r,   normal.g,   roughness,  metalness)
    draw_buffers.add(2); // gbuffer emission  (emission.r, emission.g, emission.b, 0.0)

    // Disable backface culling for alpha-tested geometry.
    if (_allow_cull) {
      state.cull.record_enable(!material.alpha_test());
    } else {
      state.cull.record_enable(false);
    }

    m_frontend->draw(
      RX_RENDER_TAG("model mesh"),
      state,
      _target,
      draw_buffers,
      m_arena->buffer(),
      program,
      _mesh.count,
      m_block.base_element() + _mesh.offset,
      0,
      m_block.base_vertex(),
      m_block.base_instance(),
      Render::Frontend::PrimitiveType::k_triangles,
      draw_textures);

    return true;
  };

  m_opaque_meshes.each_fwd([&](const Mesh& _mesh) { draw(_mesh, true); });

/*
  state.blend.record_enable(true);
  state.blend.record_blend_factors(
    Render::Frontend::BlendState::FactorType::k_src_alpha,
    Render::Frontend::BlendState::FactorType::k_one_minus_src_alpha);*/
  m_transparent_meshes.each_fwd([&](const Mesh& _mesh) { draw(_mesh, false); });
}

void Model::render_normals(const Math::Mat4x4f& _world, Render::Immediate3D* _immediate) {
  const auto scale = m_aabb.transform(_world).scale() * 0.25f;

  if (m_model->is_animated()) {
    m_model->animated_vertices().each_fwd([&](const Rx::Model::Loader::AnimatedVertex& _vertex) {
      const Math::Vec3f point_a{_vertex.position};
      const Math::Vec3f point_b{_vertex.position + _vertex.normal * scale};

      const Math::Vec3f color{_vertex.normal * 0.5f + 0.5f};

      if (m_animation) {
        // CPU skeletal animation of the lines.
        const auto& frames{m_animation->frames()};

        Math::Mat3x4f transform;
        transform  = frames[_vertex.blend_indices.x] * (_vertex.blend_weights.x / 255.0f);
        transform += frames[_vertex.blend_indices.y] * (_vertex.blend_weights.y / 255.0f);
        transform += frames[_vertex.blend_indices.z] * (_vertex.blend_weights.z / 255.0f);
        transform += frames[_vertex.blend_indices.w] * (_vertex.blend_weights.w / 255.0f);

        const Math::Vec3f& x{transform.x.x, transform.y.x, transform.z.x};
        const Math::Vec3f& y{transform.x.y, transform.y.y, transform.z.y};
        const Math::Vec3f& z{transform.x.z, transform.y.z, transform.z.z};
        const Math::Vec3f& w{transform.x.w, transform.y.w, transform.z.w};

        const Math::Mat4x4f& mat{{x.x, x.y, x.z, 0.0f},
                                 {y.x, y.y, y.z, 0.0f},
                                 {z.x, z.y, z.z, 0.0f},
                                 {w.x, w.y, w.z, 1.0f}};

        _immediate->frame_queue().record_line(
                Math::Mat4x4f::transform_point(point_a, mat * _world),
                Math::Mat4x4f::transform_point(point_b, mat * _world),
                {color.r, color.g, color.b, 1.0f},
                Immediate3D::k_depth_test | Immediate3D::k_depth_write);
      } else {
        _immediate->frame_queue().record_line(
                Math::Mat4x4f::transform_point(point_a, _world),
                Math::Mat4x4f::transform_point(point_b, _world),
                {color.r, color.g, color.b, 1.0f},
                Immediate3D::k_depth_test | Immediate3D::k_depth_write);
      }
    });
  } else {
    m_model->vertices().each_fwd([&](const Rx::Model::Loader::Vertex& _vertex) {
      const Math::Vec3f point_a{_vertex.position};
      const Math::Vec3f point_b{_vertex.position + _vertex.normal * scale};

      const Math::Vec3f color{_vertex.normal * 0.5f + 0.5f};

      _immediate->frame_queue().record_line(
              Math::Mat4x4f::transform_point(point_a, _world),
              Math::Mat4x4f::transform_point(point_b, _world),
              {color.r, color.g, color.b, 1.0f},
              Immediate3D::k_depth_test | Immediate3D::k_depth_write);
    });
  }
}

void Model::render_skeleton(const Math::Mat4x4f& _world, Render::Immediate3D* _immediate) {
  if (!m_animation) {
    return;
  }

  const auto& joints = m_model->joints();

#if 0
  // Render all the joints.
  for (Size i{0}; i < joints.size(); i++) {
    const Math::Mat3x4f& frame = m_animation->frames()[i] * joints[i].frame;

    const Math::Vec3f& x = Math::normalize({frame.x.x, frame.y.x, frame.z.x});
    const Math::Vec3f& y = Math::normalize({frame.x.y, frame.y.y, frame.z.y});
    const Math::Vec3f& z = Math::normalize({frame.x.z, frame.y.z, frame.z.z});
    const Math::Vec3f& w{frame.x.w, frame.y.w, frame.z.w};

    const Math::Mat4x4f& joint{{x.x, x.y, x.z, 0.0f},
                               {y.x, y.y, y.z, 0.0f},
                               {z.x, z.y, z.z, 0.0f},
                               {w.x, w.y, w.z, 1.0f}};

    const auto scale = m_aabb.scale().max_element() * 0.01f;

    _immediate->frame_queue().record_solid_sphere(
      {16.0f, 16.0f},
      {0.5f, 0.5f, 1.0f, 1.0f},
      Math::Mat4x4f::scale({scale, scale, scale}) * joint * _world,
      0);
  }
#endif

  // Render the skeleton.
  for (Size i{0}; i < joints.size(); i++) {
    const Math::Mat3x4f& frame{m_animation->frames()[i] * joints[i].frame};
    const Math::Vec3f& w{frame.x.w, frame.y.w, frame.z.w};
    const Sint32 parent{joints[i].parent};

    if (parent >= 0) {
      const Math::Mat3x4f& parent_joint{joints[parent].frame};
      const Math::Mat3x4f& parent_frame{m_animation->frames()[parent] * parent_joint};
      const Math::Vec3f& parent_position{parent_frame.x.w, parent_frame.y.w, parent_frame.z.w};

      _immediate->frame_queue().record_line(
        Math::Mat4x4f::transform_point(w, _world),
        Math::Mat4x4f::transform_point(parent_position, _world),
        {0.5f, 0.5f, 1.0f, 1.0f},
        0);
    }
  }
}

} // namespace rx::render
