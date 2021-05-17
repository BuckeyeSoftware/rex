#include "rx/model/voxel.h"

#include "rx/core/bitset.h"

#include "rx/core/math/ceil.h"
#include "rx/core/math/abs.h"

#include "rx/core/algorithm/clamp.h"
#include "rx/core/algorithm/saturate.h"

#include "rx/core/concurrency/wait_group.h"
#include "rx/core/concurrency/scheduler.h"
#include "rx/core/concurrency/atomic.h"

#include "rx/math/compare.h"
#include "rx/math/mat3x3.h"
#include "rx/math/plane.h"
#include "rx/math/vec2.h"
#include "rx/math/ray.h"

#include <stdio.h>

namespace Rx::Model {

// Maximum distance to rasterize in [0, 1) range. A value close to 1.0 is ideal.
static constexpr const auto RASTERIZE_MAX_DISTANCE = 0.7f;

// Calculate the distance from point |_p| to a line segment [|_l0|, |_l1|].
static Float32 line_segment_point_distance(const Math::Vec2f& _p,
  const Math::Vec2f& _l0, const Math::Vec2f& _l1)
{
  const auto length = Math::squared_length(_l0 - _l1);
  if (length == 0.0f) {
    return Math::squared_length(_p - _l0);
  }

  const auto time =
    ((_p.x - _l0.x) * (_l1.x - _l0.x) + (_p.y - _l0.y) * (_l1.y - _l0.y)) / length;

  return Math::length(_p - (_l0 + Algorithm::saturate(time) * (_l1 - _l0)));
}

// Checks if a point |_p| intersects the triangle |_t0, _t1, _t2|.
static bool tri_point_intersect(const Math::Vec2f& _p, const Math::Vec2f& _t0,
  const Math::Vec2f& _t1, const Math::Vec2f& _t2)
{
  const auto d1 = (_p.x - _t1.x) * (_t0.y - _t1.y) - (_t0.x - _t1.x) * (_p.y - _t1.y);
  const auto d2 = (_p.x - _t2.x) * (_t1.y - _t2.y) - (_t1.x - _t2.x) * (_p.y - _t2.y);
  const auto d3 = (_p.x - _t0.x) * (_t2.y - _t0.y) - (_t2.x - _t0.x) * (_p.y - _t0.y);

  const auto has_neg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
  const auto has_pos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);

  return !(has_neg && has_pos);
}

// Distance from point |_p| to triangle |_t0, _t1, _t2|.
static Float32 tri_point_distance(const Math::Vec2f& _p, const Math::Vec2f& _t0,
  const Math::Vec2f& _t1, const Math::Vec2f& _t2)
{
  if (tri_point_intersect(_p, _t0, _t1, _t2)) {
    return 0.0f;
  }

  const auto a = line_segment_point_distance(_p, _t0, _t1);
  const auto b = line_segment_point_distance(_p, _t1, _t2);
  const auto c = line_segment_point_distance(_p, _t2, _t0);

  return Algorithm::min(a, b, c);
}

// Rasterizes triangle |_a, _b, _c| within the voxel space bounded by
// [|_min, |_max|) into |plane_set| with voxel size |_voxel_size|.
template<Size I, Size J>
static void voxelize_in_plane(
  const Math::Mat3x3f& _triangle,
  const Math::Vec3z& _min,
  const Math::Vec3z& _max,
  const Math::Vec3z& _count,
  const Math::Vec3f& _min_point,
  Float32 _full_voxel_size,
  Float32 _half_voxel_size,
  Bitset& plane_set_)
{
  const auto x = _min_point[I];
  const auto y = _min_point[J];

  const auto t0x = _triangle.x[I];
  const auto t1x = _triangle.y[I];
  const auto t2x = _triangle.z[I];

  const auto t0y = _triangle.x[J];
  const auto t1y = _triangle.y[J];
  const auto t2y = _triangle.z[J];

  for (auto i = _min[I]; i <= _max[I]; i++) {
    for (auto j = _min[J]; j <= _max[J]; j++) {
      const auto index =
        (i - _min[I]) * _count[J] + (j - _min[J]);

      // Don't bother performing the intersection test if there's a voxel already
      // occuping on this plane set.
      if (plane_set_.test(index)) {
        continue;
      }

      const Math::Vec2f center = {
        x + (i * _full_voxel_size + _half_voxel_size),
        y + (j * _full_voxel_size + _half_voxel_size)
      };

      if (tri_point_distance(center, {t0x, t0y}, {t1x, t1y}, {t2x, t2y}) < _full_voxel_size * RASTERIZE_MAX_DISTANCE) {
        plane_set_.set(index);
      }
    }
  }
}

Optional<Voxel> Voxel::create(Concurrency::Scheduler& _scheduler,
  Memory::Allocator& _allocator, const Math::AABB& _aabb,
  const Vector<Math::Vec3f>& _positions, const Vector<Uint32>& _elements,
  Size _max_voxels, Size _triangles_per_task)
{
  const auto& corner_to_corner = _aabb.max() - _aabb.min();

  const auto full_voxel_size = corner_to_corner.max_element() / static_cast<Float32>(_max_voxels);
  const auto half_voxel_size = full_voxel_size * 0.5f;

  const auto voxel_count = (corner_to_corner / full_voxel_size)
    .map([](Float32 _value) { return Math::ceil(_value); })
    .cast<Size>();

  const auto& min = _aabb.min();

  const Math::AABB bounds = {
    min,
    min + voxel_count.cast<Float32>() * full_voxel_size
  };

  Vector<Concurrency::Atomic<Uint8>> matrix;
  if (!matrix.resize(voxel_count.area())) {
    return nullopt;
  }

  // Zero initialize since Atomic<Uint8> has no zero initialization.
  matrix.each_fwd([](auto& atomic_) { atomic_.store(0); });

  const auto n_triangles = _elements.size() / 3;

  if (_triangles_per_task == 0) {
    _triangles_per_task = n_triangles / _scheduler.total_threads();
  }

  const auto n_tasks = n_triangles / _triangles_per_task;

  // Kernel function called for a given triangle index.
  const auto kernel = [&](Size _triangle) {
    // Read three vertices of the triangle |_triangle|.
    const auto& a = _positions[_elements[_triangle * 3 + 0]];
    const auto& b = _positions[_elements[_triangle * 3 + 1]];
    const auto& c = _positions[_elements[_triangle * 3 + 2]];

    // Calculate facet normal of triangle.
    const auto normal = Math::cross(b - a, c - a);

    // Calculate axis-aligned bounding-box of the triangle.
    Math::AABB bb;
    bb.expand(a);
    bb.expand(b);
    bb.expand(c);

    const auto& min_coord = ((bb.min() - min) / full_voxel_size).cast<Sint32>();
    const auto& max_coord = ((bb.max() - min) / full_voxel_size).cast<Sint32>();

    // Determine the minimum and maximum coordinates in voxel-space, bounding
    // the triangle's axis-aligned bounding-box, this is the area that will be
    // rastized to.
    const Math::Vec3z min_coords = {
      Size(Algorithm::clamp(min_coord.x, 0, Sint32(voxel_count.x - 1))),
      Size(Algorithm::clamp(min_coord.y, 0, Sint32(voxel_count.y - 1))),
      Size(Algorithm::clamp(min_coord.z, 0, Sint32(voxel_count.z - 1)))
    };

    const Math::Vec3z max_coords = {
      Size(Algorithm::clamp(max_coord.x, 0, Sint32(voxel_count.x - 1))),
      Size(Algorithm::clamp(max_coord.y, 0, Sint32(voxel_count.y - 1))),
      Size(Algorithm::clamp(max_coord.z, 0, Sint32(voxel_count.z - 1)))
    };

    const auto count = max_coords - min_coords + 1_z;

    // Create 3x planes representing "two dimensional" voxel-space in the x-y,
    // x-z, and z-y planes. This separable approach to filling voxel space
    // reduces a lot of duplicate work and is more cache-efficient.
    auto xy_mat = Bitset::create(_allocator, count.x * count.y);
    auto xz_mat = Bitset::create(_allocator, count.x * count.z);
    auto zy_mat = Bitset::create(_allocator, count.z * count.y);

    // Out of memory.
    if (RX_HINT_UNLIKELY(!xy_mat || !xz_mat || !zy_mat)) {
       return false;
     }

    // Render the voxels for triangle |_triangle| in each plane.
    const Math::Mat3x3f triangle{a, b, c};
    voxelize_in_plane<0, 1>(triangle, min_coords, max_coords, count, min,
      full_voxel_size, half_voxel_size, *xy_mat);
    voxelize_in_plane<0, 2>(triangle, min_coords, max_coords, count, min,
      full_voxel_size, half_voxel_size, *xz_mat);
    voxelize_in_plane<2, 1>(triangle, min_coords, max_coords, count, min,
      full_voxel_size, half_voxel_size, *zy_mat);

    // Look at each individual plane separately, if a voxel is in all three
    // planes and the plane formed by the triangle is not too far away from
    // center of the voxel, then reasterize it into the 3D volume.
    for (Size i = min_coords.x; i <= max_coords.x; i++) {
      const auto ix = i - min_coords.x;

      for (Size j = min_coords.y; j <= max_coords.y; j++) {
        const auto jy = j - min_coords.y;

        for (Size k = min_coords.z; k <= max_coords.z; k++) {
          const auto kz = k - min_coords.z;

          const auto center = min
            + Math::Vec3z(i, j, k).cast<Float32>() * full_voxel_size
            + half_voxel_size;

          const auto index =
            i * voxel_count.y * voxel_count.z + j * voxel_count.z + k;

          // Check plane distance first to avoid testing the planes, as those
          // tests have an associated decode cost.
          const auto plane = Math::Plane{normal, center - a};
          const auto hit =
            Math::abs(plane.distance()) < full_voxel_size * RASTERIZE_MAX_DISTANCE
              && xy_mat->test(ix * count.y + jy)
              && xz_mat->test(ix * count.z + kz)
              && zy_mat->test(kz * count.y + jy);

          // Note that the voxelization is threaded in terms of triangles per
          // task, overlapping triangles or shared-edge triangles can trample
          // each other when building the volume, use atomic fetch_or here.
          matrix[index].fetch_or(hit);
        }
      }
    }
    return true;
  };

  // Schedule |n_tasks| tasks on |_scheduler|.
  Concurrency::WaitGroup group{n_tasks};
  Concurrency::Atomic<Size> success = 0;
  for (Size task = 0; task < n_tasks; task++) {
    auto voxelize_triangles = [&, task](Sint32) {
      for (Size triangle = 0; triangle < _triangles_per_task; triangle++) {
        if (RX_HINT_UNLIKELY(!kernel(task * _triangles_per_task + triangle))) {
          group.signal();
          return;
        }
      }
      success++;
      group.signal();
    };

    if (RX_HINT_UNLIKELY(!_scheduler.add(voxelize_triangles))) {
      return nullopt;
    }
  }

  // Handle remainder triangles here.
  for (Size triangle = n_tasks * _triangles_per_task; triangle < n_triangles; triangle++) {
    kernel(triangle);
  }

  // Wait for all the tasks to complete.
  group.wait();

  // When not all of them were successful, we ran out of memory.
  if (RX_HINT_UNLIKELY(success != n_tasks)) {
    return nullopt;
  }

  // Here we just disown the contents into a Vector<Uint8> since we no longer
  // require it to be atomic.
  return Voxel {
    bounds,
    voxel_count,
    full_voxel_size,
    half_voxel_size,
    { matrix.disown() }
  };
}

// Implementation of DDA ray cast as outlined in
//  https://www.researchgate.net/publication/2611491_A_Fast_Voxel_Traversal_Algorithm_for_Ray_Tracing
//
// TODO(dweiler): Improve coherency of casted rays as solid / non-solid branch is dominating factor.
Optional<Float32> Voxel::ray_cast(const Math::Ray& _ray) const {
  const auto step = _ray.direction()
    .map([](Float32 _value) { return _value > 0.0f ? 1.0f : -1.0f; })
    .cast<Sint32>();

  const auto count = m_voxel_count - 1_z;

  Math::Vec3z voxel;
  if (m_bounds.is_point_inside(_ray.point())) {
    voxel = ((_ray.point() - m_bounds.min()) / m_full_voxel_size).cast<Size>();
  } else {
    if (auto point = m_bounds.ray_intersect(_ray)) {
      const auto indices = ((*point - m_bounds.min()) / m_full_voxel_size).cast<Size>();
      // Clamp the indices so they do not go out of bounds.
      voxel.x = Algorithm::min(indices.x, count.x);
      voxel.y = Algorithm::min(indices.y, count.y);
      voxel.z = Algorithm::min(indices.z, count.z);
    } else {
      return nullopt;
    }
  }

  const auto center = voxel_origin(voxel);
  const auto next_boundaries = center + step.cast<Float32>() * m_half_voxel_size;
  const auto delta = (m_full_voxel_size / _ray.direction())
    .map([](Float32 _value) { return Math::abs(_value); });

  // March through the voxel scene.
  auto in_air = false;
  auto max = (next_boundaries - _ray.point()) / _ray.direction();
  for (;;) {
    if (max.x < max.y) {
      if (max.x < max.z) {
        if ((voxel.x += step.x) > count.x) {
          return nullopt;
        }
        max.x += delta.x;
      } else {
        if ((voxel.z += step.z) > count.z) {
          return nullopt;
        }
        max.z += delta.z;
      }
    } else {
      if (max.y < max.z) {
        if ((voxel.y += step.y) > count.y) {
          return nullopt;
        }
        max.y += delta.y;
      } else {
        if ((voxel.z += step.z) > count.z) {
          return nullopt;
        }
        max.z += delta.z;
      }
    }

    const auto index =
      voxel.x * m_voxel_count.y * m_voxel_count.z + voxel.y * m_voxel_count.z + voxel.z;

    if (!m_voxels[index] && !in_air) {
      in_air = true;
    }

    if (m_voxels[index] && in_air) {
      // Distance to origin of voxel, not a corner point.
      return Math::length(_ray.point() - voxel_origin(voxel));
    }
  }
}

} // namespace Rx::Model