// Логика рабочих потоков (kthread): цикл инкремент/декремент под блокировкой.

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include "kernel_sync_demo.h"

int sync_worker_thread_fn(void *arg)
{
	struct worker_args *wargs = arg;
	struct sync_ctx *ctx = wargs->ctx;
	unsigned int i;

	wargs->wait_time = ktime_set(0, 0);

	for (i = 0; i < ctx->iterations; i++) {
		/* TODO: захват блокировки для инкремента:
		 *   sync_lock_acquire(ctx, &wargs->wait_time, &ctx->contention_count);
		 *   ctx->shared_counter++;
		 *   sync_lock_release(ctx);
		 */

		/* TODO: отдельный захват/освобождение для декремента:
		 *   sync_lock_acquire(ctx, &wargs->wait_time, &ctx->contention_count);
		 *   ctx->shared_counter--;
		 *   sync_lock_release(ctx);
		 *
		 * Обязательное требование: блокировка захватывается и
		 * освобождается на каждой итерации — не держать её на
		 * весь цикл.
		 */
	}

	atomic_inc(&ctx->threads_done);

	return 0;
}

int sync_run_test(struct sync_ctx *ctx)
{
	/* TODO:
	 * 1. Проверить, что предыдущий тест не активен (иначе SD_BUSY).
	 * 2. sync_locks_init(ctx); сбросить shared_counter/total_wait_time/
	 *    contention_count.
	 * 3. Выделить ctx->threads = kmalloc(ctx->num_threads * sizeof(*), ...)
	 *    и массив struct worker_args под каждый поток.
	 * 4. Для каждого потока: kthread_create(sync_worker_thread_fn, &args[i], "sync_demo/%u", i)
	 *    — при ошибке освободить уже созданные потоки/память и вернуть SD_NOMEM.
	 * 5. wake_up_process() для каждого созданного потока.
	 * 6. Дождаться завершения всех потоков (kthread_stop() каждого —
	 *    т.к. функция потока сама возвращается, kthread_stop() тут
	 *    корректно дождётся завершения и заберёт код возврата).
	 * 7. Просуммировать wait_time из каждого worker_args в
	 *    ctx->total_wait_time.
	 * 8. kfree() временных массивов threads/args, обнулить ctx->threads.
	 * 9. Записать итог в ctx->last_run_result и вернуть его.
	 */

	return SD_OK;
}
