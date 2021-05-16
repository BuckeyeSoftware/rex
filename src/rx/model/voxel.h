#ifndef RX_MODEL_VOXEL_H
#define RX_MODEL_VOXEL_H
#include "rx/core/vector.h"
#include "rx/math/aabb.h"

namespace Rx::Concurrency {
  struct Scheduler;
} // namespace Rx::Concurrency

namespace Rx::Math {
  struct Ray;
} // namespace Rx::Math

namespace Rx::Model {

struct Voxel {
  static Optional<Voxel> create(
    Concurrency::Scheduler& _schedler,
    Memory::Allocator& _allocator,
    const Math::AABB& _aabb,
    const Vector<Math::Vec3f>& _positions,
    const Vector<Uint32>& _elements,
    Size _max_voxels,
    Size _triangles_per_task);

  Optional<Float32> ray_cast(const Math::Ray& _ray) const;

  const Math::Vec3f voxel_origin(const Math::Vec3z& _voxel) const;

  Voxel(Voxel&& voxel_);
  Voxel(const Math::AABB& _bounds, const Math::Vec3z& _voxel_count,
    Float32 _full_voxel_size, Float32 _half_voxel_size, Vector<Uint8>&& voxels_);

private:
  Math::AABB m_bounds;
  Math::Vec3z m_voxel_count;
  Float32 m_full_voxel_size;
  Float32 m_half_voxel_size;
  Vector<Uint8> m_voxels;
};

inline Voxel::Voxel(Voxel&& voxel_)
  : m_bounds{Utility::exchange(voxel_.m_bounds, Math::AABB{})}
  , m_voxel_count{Utility::exchange(voxel_.m_voxel_count, Math::Vec3z{})}
  , m_full_voxel_size{Utility::exchange(voxel_.m_full_voxel_size, 0.0f)}
  , m_half_voxel_size{Utility::exchange(voxel_.m_half_voxel_size, 0.0f)}
  , m_voxels{Utility::move(voxel_.m_voxels)}
{
}

inline Voxel::Voxel(const Math::AABB& _bounds, const Math::Vec3z& _voxel_count,
  Float32 _full_voxel_size, Float32 _half_voxel_size, Vector<Uint8>&& voxels_)
  : m_bounds{_bounds}
  , m_voxel_count{_voxel_count}
  , m_full_voxel_size{_full_voxel_size}
  , m_half_voxel_size{_half_voxel_size}
  , m_voxels{Utility::move(voxels_)}
{
}

inline const Math::Vec3f Voxel::voxel_origin(const Math::Vec3z& _voxel) const {
  return m_bounds.min() + (m_full_voxel_size * _voxel.cast<Float32>()) + m_half_voxel_size;
}

};

#endif // RX_MODEL_VOXELIZE_H