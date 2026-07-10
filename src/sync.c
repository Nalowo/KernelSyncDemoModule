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
  /* TODO:
   * 1. t_before = ktime_get();
   * 2. Захватить блокировку в зависимости от ctx->lock_type:
   *      LOCK_TYPE_SPINLOCK:  spin_lock(&ctx->slock);
   *      LOCK_TYPE_MUTEX:     mutex_lock(&ctx->mlock);
   *      LOCK_TYPE_SEMAPHORE: down(&ctx->sem);
   * 3. t_after = ktime_get();
   * 4. Если ktime_to_ns(ktime_sub(t_after, t_before)) > SD_WAIT_THRESHOLD_NS —
   *    прибавить разницу к *wait_time и увеличить *contention.
   *
   * Важно: при LOCK_TYPE_SPINLOCK нельзя вызывать усыпляющие функции
   * внутри критической секции (см. требования к безопасности).
   */
}

void sync_lock_release(struct sync_ctx *ctx) {
  /* TODO: освободить блокировку в зависимости от ctx->lock_type:
   *   LOCK_TYPE_SPINLOCK:  spin_unlock(&ctx->slock);
   *   LOCK_TYPE_MUTEX:     mutex_unlock(&ctx->mlock);
   *   LOCK_TYPE_SEMAPHORE: up(&ctx->sem);
   */
}
