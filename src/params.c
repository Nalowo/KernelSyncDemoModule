// Интерфейс module_param_cb: num_threads, iterations, lock_type, run, result,
// stats, reset.

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include "kernel_sync_demo.h"

/* ---------------- num_threads ---------------- */

static int num_threads_set(const char *val, const struct kernel_param *kp) {
  unsigned int n;
  int ret = kstrtouint(val, 0, &n);

  if (ret)
    return ret;

  if (SD_MIN_THREADS > n || n > SD_MAX_THREADS)
    return SD_INVALID;

  if (g_ctx.is_test_active)
    return SD_BUSY;

  mutex_lock(&g_ctx.ctx_lock);
  if (g_ctx.is_test_active) {
    mutex_unlock(&g_ctx.ctx_lock);
    return SD_BUSY;
  }

  g_ctx.num_threads = n;
  mutex_unlock(&g_ctx.ctx_lock);
  return 0;
}

static const struct kernel_param_ops num_threads_ops = {
    .set = num_threads_set,
    .get = param_get_uint,
};

module_param_cb(num_threads, &num_threads_ops, &g_ctx.num_threads, 0644);
MODULE_PARM_DESC(num_threads, "Количество рабочих kthread (1..32)");

/* ---------------- iterations ---------------- */

static int iterations_set(const char *val, const struct kernel_param *kp) {
  unsigned int n;
  int ret = kstrtouint(val, 0, &n);

  if (ret)
    return ret;

  if (SD_MIN_ITERATIONS > n || SD_MAX_ITERATIONS < n)
    return SD_INVALID;

  if (g_ctx.is_test_active)
    return SD_BUSY;

  mutex_lock(&g_ctx.ctx_lock);
  if (g_ctx.is_test_active) {
    mutex_unlock(&g_ctx.ctx_lock);
    return SD_BUSY;
  }

  g_ctx.iterations = n;
  mutex_unlock(&g_ctx.ctx_lock);
  return 0;
}

static const struct kernel_param_ops iterations_ops = {
    .set = iterations_set,
    .get = param_get_uint,
};

module_param_cb(iterations, &iterations_ops, &g_ctx.iterations, 0644);
MODULE_PARM_DESC(iterations,
                 "Итераций инкремент+декремент на поток (1..1000000)");

/* ---------------- lock_type ---------------- */

static int lock_type_set(const char *val, const struct kernel_param *kp) {
  unsigned int n;
  int ret = kstrtouint(val, 0, &n);

  if (ret)
    return ret;

  if (g_ctx.is_test_active)
    return SD_BUSY;

  mutex_lock(&g_ctx.ctx_lock);

  if (g_ctx.is_test_active) {
    mutex_unlock(&g_ctx.ctx_lock);
    return SD_BUSY;
  }

  switch (n) {
  case LOCK_TYPE_MUTEX:
  case LOCK_TYPE_SPINLOCK:
  case LOCK_TYPE_SEMAPHORE:
    g_ctx.lock_type = n;
    break;
  default:
    mutex_unlock(&g_ctx.ctx_lock);
    return SD_INVALID;
  }
  mutex_unlock(&g_ctx.ctx_lock);
  return 0;
}

static const struct kernel_param_ops lock_type_ops = {
    .set = lock_type_set,
    .get = param_get_uint,
};

module_param_cb(lock_type, &lock_type_ops, &g_ctx.lock_type, 0644);
MODULE_PARM_DESC(lock_type, "Тип блокировки: 0=spinlock, 1=mutex, 2=semaphore");

/* ---------------- run (write-only) ---------------- */

static int run_set(const char *val, const struct kernel_param *kp) {
  unsigned int trigger;
  int ret = kstrtouint(val, 0, &trigger);

  if (ret)
    return ret;

  if (!trigger)
    return 0;

  if (g_ctx.is_test_active)
    return SD_BUSY;

  mutex_lock(&g_ctx.ctx_lock);

  if (g_ctx.is_test_active) {
    mutex_unlock(&g_ctx.ctx_lock);
    return SD_BUSY;
  }
  g_ctx.is_test_active = true;
  ret = sync_run_test(&g_ctx);
  g_ctx.is_test_active = false;
  mutex_unlock(&g_ctx.ctx_lock);
  return ret;
}

static const struct kernel_param_ops run_ops = {
    .set = run_set,
    .get = NULL,
};

module_param_cb(run, &run_ops, NULL, 0200);
MODULE_PARM_DESC(run,
                 "Записать 1, чтобы запустить тест с текущими параметрами");

/* ---------------- result (read-only) ---------------- */

static const char *lock_type_name(unsigned int lock_type) {
  switch (lock_type) {
  case LOCK_TYPE_SPINLOCK:
    return "spinlock";
  case LOCK_TYPE_MUTEX:
    return "mutex";
  case LOCK_TYPE_SEMAPHORE:
    return "semaphore";
  default:
    return "unknown";
  }
}

static const char *result_status_str(int result) {
  return result == 0 ? "ok" : "err";
}

static int result_get(char *buffer, const struct kernel_param *kp) {
  return scnprintf(buffer, PAGE_SIZE,
                   "counter=%lld threads=%u iterations=%u lock=%s %s\n",
                   g_ctx.shared_counter, g_ctx.num_threads, g_ctx.iterations,
                   lock_type_name(g_ctx.lock_type),
                   result_status_str(g_ctx.last_run_result));
}

static const struct kernel_param_ops result_ops = {
    .set = NULL,
    .get = result_get,
};

module_param_cb(result, &result_ops, NULL, 0444);
MODULE_PARM_DESC(result, "Итог последнего запуска теста");

/* ---------------- stats (read-only) ---------------- */

static int stats_get(char *buffer, const struct kernel_param *kp) {
  return scnprintf(buffer, PAGE_SIZE,
                   "contention=%u total_wait_ns=%lld avg_wait_ns=%lld\n",
                   g_ctx.contention_count, ktime_to_ns(g_ctx.total_wait_time),
                   (g_ctx.total_wait_time && g_ctx.contention_count)
                       ? g_ctx.total_wait_time / g_ctx.contention_count
                       : 0);
}

static const struct kernel_param_ops stats_ops = {
    .set = NULL,
    .get = stats_get,
};

module_param_cb(stats, &stats_ops, NULL, 0444);
MODULE_PARM_DESC(stats, "Статистика ожиданий последнего запуска");

/* ---------------- reset (write-only) ---------------- */

static int reset_set(const char *val, const struct kernel_param *kp) {
  unsigned int trigger;
  int ret = kstrtouint(val, 0, &trigger);

  if (ret)
    return ret;

  if (!trigger)
    return 0;

  if (g_ctx.is_test_active)
    return SD_BUSY;

  mutex_lock(&g_ctx.ctx_lock);

  if (g_ctx.is_test_active) {
    mutex_unlock(&g_ctx.ctx_lock);
    return SD_BUSY;
  }

  g_ctx.shared_counter = 0;
  g_ctx.total_wait_time = 0;
  g_ctx.contention_count = 0;
  g_ctx.last_run_result = 0;

  mutex_unlock(&g_ctx.ctx_lock);
  return 0;
}

static const struct kernel_param_ops reset_ops = {
    .set = reset_set,
    .get = NULL,
};

module_param_cb(reset, &reset_ops, NULL, 0200);
MODULE_PARM_DESC(
    reset, "Записать ненулевое значение, чтобы сбросить счётчик и статистику");
