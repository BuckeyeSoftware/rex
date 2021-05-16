
#ifndef RX_MODEL_AOBAKE_H
#define RX_MODEL_AOBAKE_H
#include "rx/core/vector.h"

namespace Rx::Concurrency {

struct Scheduler;

} // namespace Rx::Concurrency

namespace Rx::Math {

struct AABB;

template<typename> struct Vec3;
using Vec3f = Vec3<Float32>;

} // namespace Rx::Math

namespace Rx::Model {

struct AoConfig {
  // Number of triangles per thread when voxelizing the geometry.
  // A value of 0 attempts to utilize all available threads on the scheduler.
  Size voxelize_triangles_per_task = 0;

  // The maximum number of voxels in any dimension.
  Size voxelize_max_voxels_per_dimension = 150;

  // The number of rays to trace per vertex.
  Size raytrace_rays_per_vertex = 200;

  // The number of vertices per thread.
  // A value of 0 attempts to utilize all available threads on the scheduler.
  Size raytrace_vertices_per_task = 0;

  // Seed for random generation of rays.
  Uint32 raytrace_seed = 0xdeadbeef;

  // Soft attenuation / fall-off for the AO as an exponent.
  Float32 fall_off = 6.0f;

  // Denoising passes to cleanup noise. The more passes the softer.
  Size denoising_passes = 2;
  Float32 denoising_weight = 0.2f; // In range [0, 1]
};

Optional<Vector<Float32>> bake_ao(
  Concurrency::Scheduler& _scheduler,
  const Math::AABB& _aabb,
  const Vector<Math::Vec3f>& _positions,
  const Vector<Uint32>& _elements,
  const AoConfig& _config);

} // namespace Rx::Model

#endif // RX_MODEL_AOBAKE_H