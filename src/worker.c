// Логика рабочих потоков (kthread): цикл инкремент/декремент под блокировкой.

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/slab.h>

#include "kernel_sync_demo.h"

int sync_worker_thread_fn(void *arg) {
  struct worker_args *wargs = arg;
  struct sync_ctx *ctx = wargs->ctx;
  unsigned int i;

  wargs->wait_time = ktime_set(0, 0);

  for (i = 0; i < ctx->iterations; i++) {
    sync_lock_acquire(ctx, &wargs->wait_time, &ctx->contention_count);
    ctx->shared_counter++;
    sync_lock_release(ctx);

    sync_lock_acquire(ctx, &wargs->wait_time, &ctx->contention_count);
    ctx->shared_counter--;
    sync_lock_release(ctx);
  }

  atomic_inc(&ctx->threads_done);

  return 0;
}

int sync_run_test(struct sync_ctx *ctx) {
  ctx->threads =
      kmalloc_array(ctx->num_threads, sizeof(*ctx->threads), GFP_KERNEL);
  struct worker_args *worker_array =
      kmalloc_array(ctx->num_threads, sizeof(struct worker_args), GFP_KERNEL);
  if (ctx->threads == NULL || worker_array == NULL) {
    if (ctx->threads != NULL) {
      kfree(ctx->threads);
      ctx->threads = NULL;
    }
    if (worker_array != NULL)
      kfree(worker_array);
    ctx->last_run_result = SD_NOMEM;
    return SD_NOMEM;
  }

  ctx->shared_counter = 0;
  ctx->total_wait_time = ktime_set(0, 0);
  ctx->contention_count = 0;
  atomic_set(&ctx->threads_done, 0);

  sync_locks_init(ctx);

  for (unsigned int i = 0; i < ctx->num_threads; ++i) {
    struct worker_args *args = &(worker_array[i]);
    args->ctx = ctx;
    args->thread_id = i;
    args->wait_time = 0;

    ctx->threads[i] =
        kthread_create(sync_worker_thread_fn, args, "sync_demo/%u", i);
    if (IS_ERR(ctx->threads[i])) {
      for (int j = 0; j < i; ++j) {
        if (!IS_ERR(ctx->threads[j])) {
          kthread_stop(ctx->threads[j]);
          put_task_struct(ctx->threads[j]);
        }
      }
      kfree(worker_array);
      kfree(ctx->threads);
      ctx->threads = NULL;
      ctx->last_run_result = SD_NOMEM;
      return SD_NOMEM;
    }
    get_task_struct(ctx->threads[i]);
  }

  for (unsigned int i = 0; i < ctx->num_threads; ++i) {
    wake_up_process(ctx->threads[i]);
  }

  for (unsigned int i = 0; i < ctx->num_threads; ++i) {
    kthread_stop(ctx->threads[i]);
    ctx->total_wait_time = ktime_add(ctx->total_wait_time, worker_array[i].wait_time);
    put_task_struct(ctx->threads[i]);
  }

  kfree(worker_array);
  kfree(ctx->threads);
  ctx->threads = NULL;
  ctx->last_run_result = SD_OK;
  return SD_OK;
}
