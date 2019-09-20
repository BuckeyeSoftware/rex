#include "rx/core/profiler.h"
#include "rx/core/log.h"
#include "rx/core/hint.h"

#include "lib/remotery.h"

namespace rx {

RX_LOG("profiler", logger);

profiler::profiler()
{
#if 0
  rmtSettings* settings{rmt_Settings()};
  settings->reuse_open_port = RMT_TRUE;
  settings->limit_connections_to_localhost = RMT_TRUE;
  settings->malloc = &allocate;
  settings->realloc = &reallocate;
  settings->free = &deallocate;
  settings->mm_context = reinterpret_cast<void*>(&m_allocator);
  settings->maxNbMessagesPerUpdate = 128;

  const rmtError result{rmt_CreateGlobalInstance(reinterpret_cast<Remotery**>(&m_impl))};
  if (result != RMT_ERROR_NONE) {
    logger(log::level::k_error, "%s", remotery_error(result));
  } else {
    logger(log::level::k_info, "started");
  }

  set_thread_name("Main");
#endif
}

profiler::~profiler() {
#if 0
  if (m_impl) {
    rmt_DestroyGlobalInstance(reinterpret_cast<Remotery*>(m_impl));
  }

  logger(log::level::k_info, "stopped");

  const auto& stats{m_allocator.stats()};
  logger(log::level::k_verbose, "allocations:             %zu", stats.allocations);
  logger(log::level::k_verbose, "requested reallocations: %zu", stats.request_reallocations);
  logger(log::level::k_verbose, "actual reallocations:    %zu", stats.actual_reallocations);
  logger(log::level::k_verbose, "deallocations:           %zu", stats.deallocations);
  logger(log::level::k_verbose, "peak request memory:     %s",  string::human_size_format(stats.peak_request_bytes));
  logger(log::level::k_verbose, "peak actual bytes:       %s",  string::human_size_format(stats.peak_actual_bytes));
#endif
}


RX_GLOBAL<profiler> profiler::s_profiler{"profiler"};

} // namespace rx
