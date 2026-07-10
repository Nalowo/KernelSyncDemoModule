// Обёртки захвата/освобождения блокировок (spinlock / mutex / semaphore).

#include <linux/kernel.h>

#include "kernel_sync_demo.h"

void sync_locks_init(struct sync_ctx *ctx) {
  spin_lock_init(&ctx->slock);
  mutex_init(&ctx->mlock);
  sema_init(&ctx->sem, 1);
}

void sync_lock_acquire(struct sync_ctx *ctx, ktime_t *wait_time,
                       unsigned int *contention) {
  ktime_t t_before = ktime_get();
  switch (ctx->lock_type) {
  case LOCK_TYPE_SPINLOCK:
    spin_lock(&ctx->slock);
    break;
  case LOCK_TYPE_MUTEX:
    mutex_lock(&ctx->mlock);
    break;
  case LOCK_TYPE_SEMAPHORE:
    down(&ctx->sem);
    break;
  default:
    break;
  }
  ktime_t t_after = ktime_get();

  t_after = ktime_sub(t_after, t_before);
  if (ktime_to_ns(t_after) > SD_WAIT_THRESHOLD_NS) {
    *wait_time = ktime_add(*wait_time, t_after);
    ++(*contention);
  }
}

void sync_lock_release(struct sync_ctx *ctx) {
  switch (ctx->lock_type) {
  case LOCK_TYPE_SPINLOCK:
    spin_unlock(&ctx->slock);
    break;
  case LOCK_TYPE_MUTEX:
    mutex_unlock(&ctx->mlock);
    break;
  case LOCK_TYPE_SEMAPHORE:
    up(&ctx->sem);
    break;
  default:
    break;
  }
}
