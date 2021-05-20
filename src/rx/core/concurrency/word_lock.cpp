#include "rx/core/concurrency/word_lock.h"
#include "rx/core/concurrency/yield.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"

#include "rx/core/hints/likely.h"
#include "rx/core/assert.h"

// ThreadSanitizer annotations
#if defined(RX_TSAN)
extern "C"
{
  void __tsan_acquire(void*);
  void __tsan_release(void*);
};
# define tsan_acquire(x) __tsan_acquire(x)
# define tsan_release(x) __tsan_release(x)
#else
# define tsan_acquire(x)
# define tsan_release(x)
#endif

namespace Rx::Concurrency {

static constexpr const UintPtr IS_LOCKED_BIT = 1;
static constexpr const UintPtr IS_QUEUE_LOCKED_BIT = 2;
static constexpr const UintPtr QUEUE_HEAD_MASK = 3;

struct ThreadData {
  bool should_park = false;
  Mutex park_mutex;
  ConditionVariable park_condition;
  ThreadData* next = nullptr;
  ThreadData* tail = nullptr;
};

void WordLock::lock() {
  tsan_acquire(&m_word);

  UintPtr expected = 0;
  if (RX_HINT_LIKELY(m_word.compare_exchange_weak(expected, IS_LOCKED_BIT, MemoryOrder::ACQUIRE))) {
    // Lock acquired.
    return;
  }

  lock_slow();
}

void WordLock::unlock() {
  UintPtr expected = IS_LOCKED_BIT;
  if (RX_HINT_LIKELY(m_word.compare_exchange_weak(expected, 0, MemoryOrder::RELEASE))) {
    // Lock released.
    tsan_release(&m_word);
    return;
  }

  unlock_slow();
  tsan_release(&m_word);
}

void WordLock::lock_slow() {
  static constexpr const Size MAX_SPINS = 40;

  Size spins = 0;
  for (;;) {
    auto current_word = m_word.load();

    if (!(current_word & IS_LOCKED_BIT)) {
      // It's not possible for another thread to hold the queue lock while the
      // WordLock itself is no longer held.
      RX_ASSERT(!(current_word & IS_QUEUE_LOCKED_BIT),
        "another thread holds queue lock");

      if (m_word.compare_exchange_weak(current_word, current_word | IS_LOCKED_BIT)) {
        // Acquired the lock.
        return;
      }
    }

    // When no queue is present, just spin up to |MAX_SPINS| times.
    if (!(current_word & ~QUEUE_HEAD_MASK) && spins < MAX_SPINS) {
      spins++;
      yield();
      continue;
    }

    // Need to put this thread on the queue. Create the queue if one does not
    // exist. This requires owning the queue for a short window.
    ThreadData self;

    // Reload the current word since some time may have passed.
    current_word = m_word.load();

    // Only proceed if the queue lock is not held, the WordLock is not held, and
    // the queue lock is acquired.
    if ((current_word & IS_QUEUE_LOCKED_BIT)
      || !(current_word & IS_LOCKED_BIT)
      || !m_word.compare_exchange_weak(current_word, current_word | IS_QUEUE_LOCKED_BIT))
    {
      yield();
      continue;
    }

    self.should_park = true;

    // Queue is now acquired. No other thread can enqueue or deqeue it until
    // this thread is done. It's also not possible to release the WordLock when
    // the queue lock is held.
    ThreadData* head = reinterpret_cast<ThreadData*>(current_word & ~QUEUE_HEAD_MASK);
    if (head) {
      // Put this thread at the end of the queue.
      head->tail->next = &self;
      head->tail = &self;

      // Release the queue lock.
      current_word = m_word.load();
      RX_ASSERT(current_word & ~QUEUE_HEAD_MASK, "inconsistent state");
      RX_ASSERT(current_word & IS_QUEUE_LOCKED_BIT, "queue not locked");
      RX_ASSERT(current_word & IS_LOCKED_BIT, "not locked");
      m_word.store(current_word & ~IS_QUEUE_LOCKED_BIT);
    } else {
      // Make |self| be the queue head.
      head = &self;
      self.tail = &self;

      // Release the queue lock and install |self| as the head. No need for
      // a CAS loop since the queue lock is held.
      current_word = m_word.load();
      RX_ASSERT(~(current_word & ~QUEUE_HEAD_MASK), "inconsistent state");
      RX_ASSERT(current_word & IS_QUEUE_LOCKED_BIT, "queue not locked");
      RX_ASSERT(current_word & IS_LOCKED_BIT, "not locked");
      UintPtr word = current_word;
      word |= reinterpret_cast<UintPtr>(head);
      word &= ~IS_QUEUE_LOCKED_BIT;
      m_word.store(word);
    }

    // At this point other threads that acquire the queue lock will see this
    // thread on the queue and any thread who acquires this WordLock will see
    // that this thread wants to park itself.
    {
      ScopeLock lock{self.park_mutex};
      self.park_condition.wait(lock, [&] { return !self.should_park; });
    }

    RX_ASSERT(!self.should_park, "should no longer be parked");
    RX_ASSERT(!self.next, "queue not empty");
    RX_ASSERT(!self.tail, "queue not empty");

    // Reattempt lock acquisition.
  }
}

void WordLock::unlock_slow() {
  // The fast path can fail because of spurious CAS failure, or because a thread
  // got put on the queue, or the queue lock is currently held. When the queue
  // lock is held, it can only be when something *will* enqueue a thread onto
  // the queue.

  // Acquire the queue lock, or release the lock. This loop handles both lock
  // release in case the fast path's weak CAS spuriously failed and it handles
  // acquisition of the queue lock when there is a thread on the queue.
  for (;;) {
    auto current_word = m_word.load();
    RX_ASSERT(current_word & IS_LOCKED_BIT, "not locked");

    if (current_word == IS_LOCKED_BIT) {
      UintPtr expected = IS_LOCKED_BIT;
      if (m_word.compare_exchange_weak(expected, 0)) {
        // Fast path's weak CAS has spuriously failed and now succeeded.
        return;
      }
      // Loop around and try again.
      yield();
      continue;
    }

    // Queue is still locked, try again.
    if (current_word & IS_QUEUE_LOCKED_BIT) {
      yield();
      continue;
    }

    // Was neither a spurious CAS failure or locked queue, so a thread exists
    // on the queue.
    RX_ASSERT(current_word & ~QUEUE_HEAD_MASK, "inconsistent state");

    if (m_word.compare_exchange_weak(current_word, current_word | IS_QUEUE_LOCKED_BIT)) {
      break;
    }
  }

  auto current_word = m_word.load();

  // After acquiring the queue lock, the WordLock still must be held and the
  // queue must be non-empty. The queue must be non-empty since only lock_slow()
  // could've held the queue lock and if it did then it only releases it after
  // putting a thread on the queue.
  RX_ASSERT(current_word & IS_LOCKED_BIT, "not locked");
  RX_ASSERT(current_word & IS_QUEUE_LOCKED_BIT, "queue not locked");

  auto head = reinterpret_cast<ThreadData*>(current_word & ~QUEUE_HEAD_MASK);
  auto next = head->next;

  // Either this was the only thread on the queue, in which case the queue can
  // be dropped, or there are still more threads on the queue, in which case
  // the head is replaced.
  if (next) {
    next->tail = head->tail;
  }

  // Change the queue head, possibly removing it if no further thread. No need
  // for a CAS loop since the queue lock and WordLock are both held here.
  current_word = m_word.load();
  RX_ASSERT(current_word & IS_LOCKED_BIT, "not locked");
  RX_ASSERT(current_word & IS_QUEUE_LOCKED_BIT, "queue not locked");
  RX_ASSERT((current_word & ~QUEUE_HEAD_MASK) == reinterpret_cast<UintPtr>(head), "inconsistent state");

  auto word = current_word;
  word &= ~IS_LOCKED_BIT; // Release WordLock
  word &= ~IS_QUEUE_LOCKED_BIT; // Release queue lock.
  word &= QUEUE_HEAD_MASK; // Clear out queue head.
  word |= reinterpret_cast<UintPtr>(next);
  m_word.store(word);

  // The lock is avaialble for acquisition, wakeup the thread indicated by head.
  head->next = nullptr;
  next->tail = nullptr;

  // This can run either before or during the critical section in lock_slow(),
  // so be very careful here.
  {
    // Hold lock across call to |signal| because a spurious wakeup could cause
    // the thread at the head of the queue to exit and drop the head.
    ScopeLock lock{head->park_mutex};
    head->should_park = false;

    // Doesn't matter if signal or broadcast because only one thread that could
    // be waiting is the queue head.
    head->park_condition.signal();
  }
}

} // namespace Rx::Concurrency