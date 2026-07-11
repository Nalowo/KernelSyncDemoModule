// init/exit модуля, инициализация глобального контекста.

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "kernel_sync_demo.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vlad");
MODULE_DESCRIPTION("Demo spinlock/mutex/semaphore");

/* Глобальный контекст модуля (объявлен как extern в kernel_sync_demo.h) */
struct sync_ctx g_ctx;

static int __init kernel_sync_demo_init(void) {
  g_ctx.num_threads = 4;
  g_ctx.iterations = 1000;
  g_ctx.lock_type = LOCK_TYPE_SPINLOCK;

  g_ctx.shared_counter = 0;
  g_ctx.total_wait_time = ktime_set(0, 0);
  g_ctx.contention_count = 0;

  g_ctx.threads = NULL;
  atomic_set(&g_ctx.threads_done, 0);
  g_ctx.last_run_result = SD_OK;

  g_ctx.is_test_active = false;
  mutex_init(&g_ctx.ctx_lock);

  sync_locks_init(&g_ctx);

  pr_info(
      "kernel_sync_demo: loaded (num_threads=%u iterations=%u lock_type=%u)\n",
      g_ctx.num_threads, g_ctx.iterations, g_ctx.lock_type);

  return 0;
}

static void __exit kernel_sync_demo_exit(void) {
  mutex_lock(&g_ctx.ctx_lock);

  if (g_ctx.threads != NULL) {
    kfree(g_ctx.threads);
    g_ctx.threads = NULL;
  }

  mutex_unlock(&g_ctx.ctx_lock);
  mutex_destroy(&g_ctx.ctx_lock);
  mutex_destroy(&g_ctx.mlock);

  pr_info("kernel_sync_demo: unloaded\n");
}

module_init(kernel_sync_demo_init);
module_exit(kernel_sync_demo_exit);
