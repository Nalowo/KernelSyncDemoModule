// Интерфейс module_param_cb: num_threads, iterations, lock_type, run, result, stats, reset.

#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/atomic.h>

#include "kernel_sync_demo.h"

/* ---------------- num_threads ---------------- */

static int num_threads_set(const char *val, const struct kernel_param *kp)
{
	unsigned int n;
	int ret = kstrtouint(val, 0, &n);

	if (ret)
		return ret;

	/* TODO: проверить SD_MIN_THREADS <= n <= SD_MAX_THREADS,
	 * иначе вернуть SD_INVALID.
	 */

	*(unsigned int *)kp->arg = n;
	return 0;
}

static const struct kernel_param_ops num_threads_ops = {
	.set = num_threads_set,
	.get = param_get_uint,
};

module_param_cb(num_threads, &num_threads_ops, &g_ctx.num_threads, 0644);
MODULE_PARM_DESC(num_threads, "Количество рабочих kthread (1..32)");

/* ---------------- iterations ---------------- */

static int iterations_set(const char *val, const struct kernel_param *kp)
{
	unsigned int n;
	int ret = kstrtouint(val, 0, &n);

	if (ret)
		return ret;

	/* TODO: проверить SD_MIN_ITERATIONS <= n <= SD_MAX_ITERATIONS,
	 * иначе вернуть SD_INVALID.
	 */

	*(unsigned int *)kp->arg = n;
	return 0;
}

static const struct kernel_param_ops iterations_ops = {
	.set = iterations_set,
	.get = param_get_uint,
};

module_param_cb(iterations, &iterations_ops, &g_ctx.iterations, 0644);
MODULE_PARM_DESC(iterations, "Итераций инкремент+декремент на поток (1..1000000)");

/* ---------------- lock_type ---------------- */

static int lock_type_set(const char *val, const struct kernel_param *kp)
{
	unsigned int n;
	int ret = kstrtouint(val, 0, &n);

	if (ret)
		return ret;

	/* TODO: проверить n == LOCK_TYPE_SPINLOCK/MUTEX/SEMAPHORE (0/1/2),
	 * иначе вернуть SD_INVALID.
	 */

	*(unsigned int *)kp->arg = n;
	return 0;
}

static const struct kernel_param_ops lock_type_ops = {
	.set = lock_type_set,
	.get = param_get_uint,
};

module_param_cb(lock_type, &lock_type_ops, &g_ctx.lock_type, 0644);
MODULE_PARM_DESC(lock_type, "Тип блокировки: 0=spinlock, 1=mutex, 2=semaphore");

/* ---------------- run (write-only) ---------------- */

static int run_set(const char *val, const struct kernel_param *kp)
{
	unsigned int trigger;
	int ret = kstrtouint(val, 0, &trigger);

	if (ret)
		return ret;

	if (!trigger)
		return 0;

	/* TODO: вызвать sync_run_test(&g_ctx) и вернуть её результат
	 * (в т.ч. SD_BUSY, если предыдущий тест ещё выполняется).
	 */

	return 0;
}

static const struct kernel_param_ops run_ops = {
	.set = run_set,
	.get = NULL,
};

module_param_cb(run, &run_ops, NULL, 0200);
MODULE_PARM_DESC(run, "Записать 1, чтобы запустить тест с текущими параметрами");

/* ---------------- result (read-only) ---------------- */

static int result_get(char *buffer, const struct kernel_param *kp)
{
	/* TODO: сформировать строку вида
	 *   "counter=0 threads=4 iterations=1000 lock=spinlock ok"
	 * на основе g_ctx.shared_counter/num_threads/iterations/lock_type
	 * и g_ctx.last_run_result, используя scnprintf(buffer, PAGE_SIZE, ...).
	 */

	return scnprintf(buffer, PAGE_SIZE, "counter=%lld threads=%u iterations=%u lock=%u result=%d\n",
			  g_ctx.shared_counter, g_ctx.num_threads, g_ctx.iterations,
			  g_ctx.lock_type, g_ctx.last_run_result);
}

static const struct kernel_param_ops result_ops = {
	.set = NULL,
	.get = result_get,
};

module_param_cb(result, &result_ops, NULL, 0444);
MODULE_PARM_DESC(result, "Итог последнего запуска теста");

/* ---------------- stats (read-only) ---------------- */

static int stats_get(char *buffer, const struct kernel_param *kp)
{
	/* TODO: сформировать строку вида
	 *   "contention=42 total_wait_ns=18500 avg_wait_ns=440"
	 * avg_wait_ns = total_wait_ns / contention_count (беречься деления на 0).
	 */

	return scnprintf(buffer, PAGE_SIZE, "contention=%u total_wait_ns=%lld avg_wait_ns=0\n",
			  g_ctx.contention_count,
			  ktime_to_ns(g_ctx.total_wait_time));
}

static const struct kernel_param_ops stats_ops = {
	.set = NULL,
	.get = stats_get,
};

module_param_cb(stats, &stats_ops, NULL, 0444);
MODULE_PARM_DESC(stats, "Статистика ожиданий последнего запуска");

/* ---------------- reset (write-only) ---------------- */

static int reset_set(const char *val, const struct kernel_param *kp)
{
	unsigned int trigger;
	int ret = kstrtouint(val, 0, &trigger);

	if (ret)
		return ret;

	if (!trigger)
		return 0;

	/* TODO: проверить, что тест не активен (иначе SD_BUSY), затем
	 * обнулить g_ctx.shared_counter, total_wait_time, contention_count,
	 * last_run_result.
	 */

	return 0;
}

static const struct kernel_param_ops reset_ops = {
	.set = reset_set,
	.get = NULL,
};

module_param_cb(reset, &reset_ops, NULL, 0200);
MODULE_PARM_DESC(reset, "Записать ненулевое значение, чтобы сбросить счётчик и статистику");
