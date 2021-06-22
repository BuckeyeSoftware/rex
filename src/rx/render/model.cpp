#include <stddef.h> // offsetof

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

Model::Model(Frontend::Context* _frontend, Frontend::Technique* _technique)
  : m_frontend{_frontend}
  , m_technique{_technique}
  , m_arena{nullptr}
  , m_materials{_frontend->allocator()}
  , m_opaque_meshes{_frontend->allocator()}
  , m_transparent_meshes{_frontend->allocator()}
  , m_clips{_frontend->allocator()}
{
}

Optional<Model> Model::create(Frontend::Context* _frontend) {
  auto technique = _frontend->find_technique_by_name("geometry");
  if (!technique) {
    return nullopt;
  }
  return Model{_frontend, technique};
}

Model::Model(Model&& model_)
  : m_frontend{Utility::exchange(model_.m_frontend, nullptr)}
  , m_technique{Utility::exchange(model_.m_technique, nullptr)}
  , m_arena{Utility::exchange(model_.m_arena, nullptr)}
  , m_block{Utility::move(model_.m_block)}
  , m_materials{Utility::move(model_.m_materials)}
  , m_opaque_meshes{Utility::move(model_.m_opaque_meshes)}
  , m_transparent_meshes{Utility::move(model_.m_transparent_meshes)}
  , m_skeleton{Utility::move(model_.m_skeleton)}
  , m_animation{Utility::move(model_.m_animation)}
  , m_clips{Utility::move(model_.m_clips)}
  , m_aabb{Utility::move(model_.m_aabb)}
{
}

Model& Model::operator=(Model&& model_) {
  if (this == &model_) {
    return *this;
  }

  m_frontend = Utility::exchange(model_.m_frontend, nullptr);
  m_technique = Utility::exchange(model_.m_technique, nullptr);
  m_arena = Utility::exchange(model_.m_arena, nullptr);
  m_block = Utility::move(model_.m_block);
  m_materials = Utility::move(model_.m_materials);
  m_opaque_meshes = Utility::move(model_.m_opaque_meshes);
  m_transparent_meshes = Utility::move(model_.m_transparent_meshes);
  m_skeleton = Utility::move(model_.m_skeleton);
  m_animation = Utility::move(model_.m_animation);
  m_clips = Utility::move(model_.m_clips);
  m_aabb = Utility::move(model_.m_aabb);

  return *this;
}

Model::~Model() {
}

bool Model::upload(const Rx::Model::Loader& _loader) {
  if (auto clips = Utility::copy(_loader.clips())) {
    m_clips = Utility::move(*clips);
  } else {
    return false;
  }

  if (const auto& skeleton = _loader.skeleton()) {
    if (auto copy = Utility::copy(*skeleton)) {
      m_skeleton = Utility::move(*copy);
    } else {
      return false;
    }
  }

  m_last_transform = nullopt;

  // Clear incase being called multiple times for model changes.
  m_opaque_meshes.clear();
  m_transparent_meshes.clear();

  if (_loader.is_animated()) {
    using Vertex = Rx::Model::Loader::AnimatedVertex;

    Frontend::Buffer::Format format{m_frontend->allocator()};
    format.record_element_type(Frontend::Buffer::ElementType::U32);
    format.record_vertex_stride(sizeof(Vertex));
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, position)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32,   offsetof(Vertex, occlusion)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, normal)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x4, offsetof(Vertex, tangent)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x2, offsetof(Vertex, coordinate)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x4, offsetof(Vertex, blend_weights)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::S32x4, offsetof(Vertex, blend_indices)});
    format.finalize();

    m_arena = m_frontend->arena(format);
    m_block = m_arena;

    const auto &vertices = _loader.animated_vertices();
    const auto size = vertices.size() * sizeof(Vertex);

    m_block.write_vertices(vertices.data(), size);
    m_block.record_vertices_edit(0, size);
  } else {
    using Vertex = Rx::Model::Loader::Vertex;

    Frontend::Buffer::Format format{m_frontend->allocator()};
    format.record_element_type(Frontend::Buffer::ElementType::U32);
    format.record_vertex_stride(sizeof(Vertex));
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, position)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32,   offsetof(Vertex, occlusion)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, normal)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x4, offsetof(Vertex, tangent)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x2, offsetof(Vertex, coordinate)});
    format.finalize();

    m_arena = m_frontend->arena(format);
    m_block = m_arena;

    const auto &vertices = _loader.vertices();
    const auto size = vertices.size() * sizeof(Vertex);
    m_block.write_vertices(vertices.data(), size);
    m_block.record_vertices_edit(0, size);
  }

  const auto &elements = _loader.elements();
  const auto size = elements.size() * sizeof(Uint32);
  m_block.write_elements(elements.data(), size);
  m_block.record_elements_edit(0, size);

  m_frontend->update_buffer(RX_RENDER_TAG("Model"), m_arena->buffer());

  m_materials.clear();

  // Map all the loaded material::loader's to render::frontend::material's while
  // using indices to refer to them rather than strings.
  Map<String, Size> material_indices{m_frontend->allocator()};
  const bool material_load_result =
    _loader.materials().each_pair([this, &material_indices](const String& _name, const Material::Loader& _material) {
      Frontend::Material material{m_frontend};
      if (material.load(_material)) {
        const Size material_index{m_materials.size()};
        return material_indices.insert(_name, material_index) && m_materials.push_back(Utility::move(material));
      }
      return false;
    });

  if (!material_load_result) {
    return false;
  }

  // Resolve all the meshes of the loaded model.
  return _loader.meshes().each_fwd([this, &material_indices](const Rx::Model::Mesh& _mesh) {
    if (auto* find = material_indices.find(_mesh.material)) {
      auto bounds = Utility::copy(_mesh.bounds);
      if (!bounds) {
        // Out of memory.
        return false;
      }
      if (m_materials[*find].has_alpha()) {
        return m_transparent_meshes.emplace_back(_mesh.offset, _mesh.count,
          *find, Utility::move(*bounds));
      } else {
        return m_opaque_meshes.emplace_back(_mesh.offset, _mesh.count,
          *find, Utility::move(*bounds));
      }
    }
    return false;
  });
}

void Model::animate(Size _index, [[maybe_unused]] bool _loop) {
  if (m_skeleton && m_clips.in_range(_index)) {
    m_animation = Rx::Model::Animation::create(m_frontend->allocator(), *m_skeleton, m_clips[_index]);
  } else {
    m_animation = nullopt;
  }
}

void Model::update(Float32 _delta_time) {
  if (m_animation) {
    m_animation->update(_delta_time, true);
  }

  Math::AABB aabb;
  auto expand = [&, this](const Mesh& _mesh) {
    aabb.expand(mesh_bounds(_mesh));
  };

  m_opaque_meshes.each_fwd(expand);
  m_transparent_meshes.each_fwd(expand);

  m_aabb = aabb;
}

Math::AABB Model::mesh_bounds(const Mesh& _mesh) const {
  if (m_animation) {
    // Interpolate between the two frames.
    const auto& interpolant = m_animation->interpolant();
    const auto& bounds = _mesh.bounds[m_animation->clip()->index];
    const auto& aabb1 = bounds[interpolant.frame1];
    const auto& aabb2 = bounds[interpolant.frame2];
    return {
      aabb1.min() * (1.0f - interpolant.offset) + aabb2.min() * interpolant.offset,
      aabb1.max() * (1.0f - interpolant.offset) + aabb2.max() * interpolant.offset
    };
  } else {
    return _mesh.bounds[0][0];
  }
}

void Model::render(Frontend::Target* _target, const Math::Mat4x4f& _model,
                   const Math::Mat4x4f& _view, const Math::Mat4x4f& _projection,
                   Uint32 _flags, Immediate3D* _immediate)
{
  const auto& view_projection = _view * _projection;

  Math::Frustum frustum{view_projection};

  RX_PROFILE_CPU("model::render");
  RX_PROFILE_GPU("model::render");

  Frontend::State state;

  // Enable(DEPTH_TEST)
  state.depth.record_test(true);

  // Enable(STENCIL_TEST)
  state.stencil.record_enable(true);

  // StencilMask(0xff)
  state.stencil.record_write_mask(0xff);
  // StencilFunc(ALWAYS, 1, 0xff)
  state.stencil.record_function(Frontend::StencilState::FunctionType::ALWAYS);
  state.stencil.record_reference(1);
  state.stencil.record_mask(0xff);

  state.stencil.record_fail_action(Frontend::StencilState::OperationType::KEEP);
  state.stencil.record_depth_fail_action(Frontend::StencilState::OperationType::REPLACE);
  state.stencil.record_depth_pass_action(Frontend::StencilState::OperationType::REPLACE);

  // Enable(CULL_FACE)
  state.cull.record_enable(true);

  // Disable(BLEND)
  state.blend.record_enable(false);

  // FrontFace(CW)
  state.cull.record_front_face(Frontend::CullState::FrontFaceType::CLOCK_WISE);

  // CullFace(BACK)
  state.cull.record_cull_face(Frontend::CullState::CullFaceType::BACK);

  // DepthMask(TRUE)
  state.depth.record_write(true);

  // Viewport(0, 0, w, h)
  state.viewport.record_dimensions(_target->dimensions());

  auto draw = [&](const Mesh& _mesh, bool _transparent) {
    if (!frustum.is_aabb_inside(mesh_bounds(_mesh).transform(_model))) {
      return false;
    }

    RX_PROFILE_CPU("batch");
    RX_PROFILE_GPU("batch");

    const auto& material{m_materials[_mesh.material]};

    Uint64 flags{0};
    if (material.albedo())     flags |= 1 << 0;
    if (material.normal())     flags |= 1 << 1;
    if (material.metalness())  flags |= 1 << 2;
    if (material.roughness())  flags |= 1 << 3;
    if (material.alpha_test()) flags |= 1 << 4;
    if (material.occlusion())  flags |= 1 << 5;
    if (material.emissive())   flags |= 1 << 6;

    Size configuration = 0;
    if (m_animation) {
      configuration = 2;
      // TODO(DQS)
    }

    Frontend::Program* program{m_technique->configuration(configuration).permute(flags)};
    auto& uniforms = program->uniforms();

    uniforms[0].record_mat4x4f(_model);
    uniforms[1].record_mat4x4f(view_projection);
    if (m_last_transform) {
      uniforms[2].record_mat4x4f(*m_last_transform);
    } else {
      uniforms[2].record_mat4x4f(_model * view_projection);
    }
    if (const auto& transform = material.transform()) {
      uniforms[3].record_mat3x3f(transform->as_mat3());
    }

    uniforms[4].record_float(material.roughness_value());
    uniforms[5].record_float(material.metalness_value());
    uniforms[6].record_float(material.occlusion_value());
    uniforms[7].record_vec3f(material.albedo_color());
    uniforms[8].record_vec3f(material.emission_color());

    // Record all the textures.
    Frontend::Textures draw_textures;
    if (material.albedo())    uniforms[9].record_sampler(draw_textures.add(material.albedo()));
    if (material.normal())    uniforms[10].record_sampler(draw_textures.add(material.normal()));
    if (material.metalness()) uniforms[11].record_sampler(draw_textures.add(material.metalness()));
    if (material.roughness()) uniforms[12].record_sampler(draw_textures.add(material.roughness()));
    if (material.occlusion()) uniforms[13].record_sampler(draw_textures.add(material.occlusion()));
    if (material.emissive())  uniforms[14].record_sampler(draw_textures.add(material.emissive()));

    // For animation
    if (m_animation) {
      // LBS ...
      uniforms[15].record_lb_bones(m_animation->lb_frames(), m_skeleton->joints().size());
      // DQS ...
      uniforms[16].record_dq_bones(m_animation->dq_frames(), m_skeleton->joints().size());
    }

    // Record all the draw buffers.
    Frontend::Buffers draw_buffers;
    draw_buffers.add(0); // gbuffer albedo    (albedo.r,   albedo.g,   albedo.b,   ambient)
    draw_buffers.add(1); // gbuffer normal    (normal.r,   normal.g,   roughness,  metalness)
    draw_buffers.add(2); // gbuffer emission  (emission.r, emission.g, emission.b, 0.0)
    draw_buffers.add(3); // gbuffer velocity  (velocity.x, velocity.y)

    // Only backface cull when neither alpha-testing or transparent.
    state.cull.record_enable(!material.alpha_test() && !_transparent);

    // Only blend when transparent.
    state.blend.record_enable(_transparent);

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
      Render::Frontend::PrimitiveType::TRIANGLES,
      draw_textures);

    if (_flags & BOUNDS) {
      _immediate->frame_queue().record_wire_box(
        {1.0f, 0.0f, 0.0f, 1.0f},
        mesh_bounds(_mesh).transform(_model),
        Immediate3D::DEPTH_TEST | Immediate3D::DEPTH_WRITE);
    }

    return true;
  };

  bool visible = false;
  m_opaque_meshes.each_fwd([&](const Mesh& _mesh) {
    visible |= draw(_mesh, false);
  });

  m_transparent_meshes.each_fwd([&](const Mesh& _mesh) {
    visible |= draw(_mesh, true);
  });

  m_last_transform = _model * view_projection;

  if (!visible) {
    return;
  }

  if (_flags & BOUNDS) {
    _immediate->frame_queue().record_wire_box(
      {0.0f, 0.0f, 1.0f, 1.0f},
      m_aabb.transform(_model),
      Immediate3D::DEPTH_TEST | Immediate3D::DEPTH_WRITE);
  }

  if (_flags & SKELETON) {
    render_skeleton(_model, _immediate);
  }

  if (_flags & NORMALS) {
    render_normals(_model, _immediate);
  }
}

void Model::render_normals(const Math::Mat4x4f& _world, Render::Immediate3D* _immediate) {
  const auto scale = m_aabb.transform(_world).scale() * 0.25f;

  if (m_animation) {
    const auto& vertices = m_block.vertices().cast<const Rx::Model::Loader::AnimatedVertex>();
    const auto n_vertices = vertices.size();

    for (Size i = 0; i < n_vertices; i++) {
      const auto& vertex = vertices[i];

      const Math::Vec3f point_a = vertex.position;
      const Math::Vec3f point_b = vertex.position + vertex.normal * scale;

      const Math::Vec3f color = vertex.normal * 0.5f + 0.5f;

      // CPU skeletal animation of the lines.
      const auto& frames = m_animation->lb_frames();

      Math::Mat3x4f transform;
      transform  = frames[vertex.blend_indices.x] * vertex.blend_weights.x;
      transform += frames[vertex.blend_indices.y] * vertex.blend_weights.y;
      transform += frames[vertex.blend_indices.z] * vertex.blend_weights.z;
      transform += frames[vertex.blend_indices.w] * vertex.blend_weights.w;

      const Math::Vec3f& x{transform.x.x, transform.y.x, transform.z.x};
      const Math::Vec3f& y{transform.x.y, transform.y.y, transform.z.y};
      const Math::Vec3f& z{transform.x.z, transform.y.z, transform.z.z};
      const Math::Vec3f& w{transform.x.w, transform.y.w, transform.z.w};

      const Math::Mat4x4f& mat{{x.x, x.y, x.z, 0.0f},
                               {y.x, y.y, y.z, 0.0f},
                               {z.x, z.y, z.z, 0.0f},
                               {w.x, w.y, w.z, 1.0f}};

      _immediate->frame_queue().record_line(
              Math::transform_point(point_a, mat * _world),
              Math::transform_point(point_b, mat * _world),
              {color.r, color.g, color.b, 1.0f},
              {color.r, color.g, color.b, 1.0f},
              Immediate3D::DEPTH_TEST | Immediate3D::DEPTH_WRITE);
    };
  } else {
    const auto& vertices = m_block.vertices().cast<const Rx::Model::Loader::Vertex>();
    const auto n_vertices = vertices.size();

    for (Size i = 0; i < n_vertices; i++) {
      const auto& vertex = vertices[i];

      const Math::Vec3f point_a = vertex.position;
      const Math::Vec3f point_b = vertex.position + vertex.normal * scale;

      const Math::Vec3f color = vertex.normal * 0.5f + 0.5f;

      _immediate->frame_queue().record_line(
              Math::transform_point(point_a, _world),
              Math::transform_point(point_b, _world),
              {color.r, color.g, color.b, 1.0f},
              {color.r, color.g, color.b, 1.0f},
              Immediate3D::DEPTH_TEST | Immediate3D::DEPTH_WRITE);
    };
  }
}

void Model::render_skeleton(const Math::Mat4x4f& _world, Render::Immediate3D* _immediate) {
  if (!m_skeleton) {
    return;
  }

  const auto& joints = m_skeleton->joints();
  const auto n_joints = joints.size();

  // Render all the joints.
  for (Size i = 0; i < n_joints; i++) {
    const auto& frame =
      m_animation
        ? m_animation->lb_frames()[i] * joints[i].frame : joints[i].frame;

    const Math::Mat4x4f& joint{{frame.x.x, frame.y.x, frame.z.x, 0.0f},
                               {frame.x.y, frame.y.y, frame.z.y, 0.0f},
                               {frame.x.z, frame.y.z, frame.z.z, 0.0f},
                               {frame.x.w, frame.y.w, frame.z.w, 1.0f}};

    const auto scale = m_aabb.scale().max_element() * 0.01f;

    _immediate->frame_queue().record_solid_sphere(
      {16.0f, 16.0f},
      {0.5f, 0.5f, 1.0f, 1.0f},
      Math::Mat4x4f::scale({scale, scale, scale}) * joint * _world,
      0);
  }

  // Render the skeleton.
  for (Size i = 0; i < n_joints; i++) {
    const auto& frame =
      m_animation ? m_animation->lb_frames()[i] * joints[i].frame : joints[i].frame;

    const auto parent = joints[i].parent;

    if (parent < 0) {
      continue;
    }

    const auto& parent_joint = joints[parent].frame;
    const auto& parent_frame =
      m_animation ? m_animation->lb_frames()[parent] * parent_joint : parent_joint;

    const Math::Vec3f parent_position = {
      parent_frame.x.w,
      parent_frame.y.w,
      parent_frame.z.w
    };

    const Math::Vec3f w{
      frame.x.w,
      frame.y.w,
      frame.z.w
    };

    _immediate->frame_queue().record_line(
      Math::transform_point(w, _world),
      Math::transform_point(parent_position, _world),
      {0.5f, 0.5f, 1.0f, 1.0f},
      {0.5f, 0.5f, 1.0f, 1.0f},
      0);
  }
}

bool Model::load(Stream::UntrackedStream& _stream) {
  Rx::Model::Loader loader{m_frontend->allocator()};
  return loader.load(_stream) && upload(loader);
}

bool Model::load(const String& _file_name) {
  Rx::Model::Loader loader{m_frontend->allocator()};
  return loader.load(_file_name) && upload(loader);
}

} // namespace Rx::Render
