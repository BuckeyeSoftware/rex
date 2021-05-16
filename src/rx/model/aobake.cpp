#include <stdlib.h>

#include "rx/model/aobake.h"
#include "rx/model/voxel.h"

#include "rx/core/log.h"

#include "rx/core/math/constants.h"
#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"
#include "rx/core/math/pow.h"

#include "rx/core/concurrency/scheduler.h"
#include "rx/core/concurrency/wait_group.h"

#include "rx/math/ray.h"

#include "rx/core/prng/mt19937.h"

namespace Rx::Model {

RX_LOG("model/aobake", logger);

// Just the result of Math::sqrt(2.0) for brightness multiplier in |compute_ao|.
static constexpr const auto SQRT_2 = 1.41421356237f;

// Small epsilon to start the ray slightly off the vertex to avoid
// false self-occlusion.
static constexpr const auto ORIGIN_OFFSET = 0.0001f;

static Math::Vec3f random_unit_vec(PRNG::MT19937& _rng) {
  const auto phi = _rng.f32() * Math::TAU<Float32>;
  const auto cos_theta = _rng.f32() * 2.0f - 1.0f;
  const auto theta = Math::acos(cos_theta);
  return {
    Math::sin(theta) * Math::cos(phi),
    Math::sin(theta) * Math::sin(phi),
    Math::cos(theta)
  };
}

float compute_ao(const Vector<float>& _ray_results, Float32 _max_distance, Size _ray_count, Float32 _fall_off) {
  Float32 brightness = 1.0f;
  const auto inv_max_distance = 1.0f / _max_distance;
  _ray_results.each_fwd([&](Float32 _distance) {
    const auto normalized_distance = _distance * inv_max_distance;
    const auto occlusion = 1.0f - Math::pow(normalized_distance, _fall_off);
    brightness -= occlusion / _ray_count;
  });
  return Algorithm::min(1.0f, brightness * SQRT_2);
}

Optional<Vector<Float32>> bake_ao(
  Concurrency::Scheduler& _scheduler,
  const Math::AABB& _aabb,
  const Vector<Math::Vec3f>& _positions,
  const Vector<Uint32>& _elements,
  const AoConfig& _config)
{
  const auto max_distance = Math::length(_aabb.max() - _aabb.min());

  PRNG::MT19937 random;
  random.seed(_config.raytrace_seed);

  auto voxel = Voxel::create(
    _scheduler,
    _positions.allocator(),
    _aabb,
    _positions,
    _elements,
    _config.voxelize_max_voxels_per_dimension,
    _config.voxelize_triangles_per_task);

  if (!voxel) {
    logger->error("failed to voxelize");
    return nullopt;
  }

  const auto n_vertices = _positions.size();
  Vector<float> ao;
  if (!ao.resize(n_vertices)) {
    return nullopt;
  }

  auto n_vertices_per_task = _config.raytrace_vertices_per_task;
  if (n_vertices_per_task == 0) {
    n_vertices_per_task = n_vertices / _scheduler.total_threads();
  }

  const auto tasks = n_vertices / n_vertices_per_task;

  // Kernel function for a single vertex.
  const auto kernel = [&](Size _vertex) {
    const auto& vertex = _positions[_vertex];
    Vector<Float32> results;
    for (Size i = 0; i < _config.raytrace_rays_per_vertex; i++) {
      auto direction = random_unit_vec(random);

      // Should the rays always go "up"?
      if (direction.y < 0.0f) {
        direction.y = -direction.y;
      }

      Math::Ray ray{vertex + direction * ORIGIN_OFFSET, direction};
      if (const auto distance = voxel->ray_cast(ray)) {
        results.push_back(Algorithm::min(*distance, max_distance));
      }
    }
    ao[_vertex] = compute_ao(results, max_distance,
      _config.raytrace_rays_per_vertex, max_distance * _config.fall_off);
  };

  // Distribute the kernel over the thread pool.
  Concurrency::WaitGroup group{tasks};
  for (Size task = 0; task < tasks; task++) {
    (void)_scheduler.add([&, task](Sint32) {
      for (Size vertex = 0; vertex < n_vertices_per_task; vertex++) {
        kernel(task * n_vertices_per_task + vertex);
      }
      group.signal();
    });
  }

  // Handle remainder vertices not handled by the above.
  for (Size vertex = tasks * n_vertices_per_task; vertex < n_vertices; vertex++) {
    kernel(vertex);
  }

  // Wait for all tasks to complete.
  group.wait();

  const auto mix = [](Float32 x, Float32 y, Float32 a) {
    return x * (1.0 - a) + y * a;
  };

  // Denoising pass
  for (Size pass = 0; pass < _config.denoising_passes; pass++) {
    const auto n_elements = _elements.size();
    for (Size element = 0; element < n_elements; element += 3) {
      const auto average =
        (ao[_elements[element + 0]] +
         ao[_elements[element + 1]] +
         ao[_elements[element + 2]]) / 3.0f;

      ao[_elements[element + 0]] = mix(ao[_elements[element + 0]], average, _config.denoising_weight);
      ao[_elements[element + 1]] = mix(ao[_elements[element + 1]], average, _config.denoising_weight);
      ao[_elements[element + 2]] = mix(ao[_elements[element + 2]], average, _config.denoising_weight);
    }
  }

  return ao;
}
}