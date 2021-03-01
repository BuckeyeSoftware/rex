#ifndef RX_RENDER_MODEL_H
#define RX_RENDER_MODEL_H
#include "rx/render/frontend/material.h"
#include "rx/render/frontend/arena.h"

#include "rx/model/loader.h"
#include "rx/model/animation.h"
#include "rx/model/skeleton.h"

#include "rx/math/mat4x4.h"
#include "rx/math/aabb.h"

#include "rx/core/uninitialized.h"

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Technique;
  struct Buffer;
  struct Target;
  struct Arena;
}

struct Immediate3D;
struct ImageBasedLighting;

struct Model {
  enum {
    NORMALS  = 1 << 0,
    SKELETON = 1 << 1,
    BOUNDS   = 1 << 2
  };

  Model();
  Model(Model&& model_);
  Model& operator=(Model&& model_);
  ~Model();

  static Optional<Model> create(Frontend::Context* _frontend);

  void animate(Size _index, bool _loop);

  void update(Float32 _delta_time);

  void render(Frontend::Target* _target, const Math::Mat4x4f& _model,
              const Math::Mat4x4f& _view, const Math::Mat4x4f& _projection,
              Uint32 _flags, Render::Immediate3D* _immediate = nullptr);

  [[nodiscard]] bool load(Stream* _stream);
  [[nodiscard]] bool load(const String& _file_name);

  const Optional<Rx::Model::Skeleton>& skeleton() const &;
  const Optional<Rx::Model::Animation>& animation() const &;

private:
  Model(Frontend::Context* _frontend, Frontend::Technique* _technique);

  struct Mesh {
    Size offset;
    Size count;
    Size material;
    Vector<Vector<Math::AABB>> bounds;
  };

  void render_normals(const Math::Mat4x4f& _world, Render::Immediate3D* _immediate);
  void render_skeleton(const Math::Mat4x4f& _world, Render::Immediate3D* _immediate);

  [[nodiscard]] bool upload(const Rx::Model::Loader& _loader);

  // Obtains the bounds for a given mesh |_mesh| even if currently animated.
  Math::AABB mesh_bounds(const Mesh& _mesh) const;

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;
  Frontend::Arena* m_arena;
  Frontend::Arena::Block m_block;
  Vector<Frontend::Material> m_materials;
  Vector<Mesh> m_opaque_meshes;
  Vector<Mesh> m_transparent_meshes;
  Optional<Rx::Model::Skeleton> m_skeleton;
  Optional<Rx::Model::Animation> m_animation;
  Vector<Rx::Model::Clip> m_clips;
  Math::AABB m_aabb;
};

inline Model::Model()
  : Model{nullptr, nullptr}
{
}

inline const Optional<Rx::Model::Skeleton>& Model::skeleton() const & {
  return m_skeleton;
}

inline const Optional<Rx::Model::Animation>& Model::animation() const & {
  return m_animation;
}

} // namespace Rx::Render

#endif // RX_RENDER_MODEL_H
