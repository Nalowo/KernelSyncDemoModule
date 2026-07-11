#ifndef KERNEL_SYNC_DEMO_H
#define KERNEL_SYNC_DEMO_H

#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/types.h>

/* Коды возвратаЫ */
#define SD_OK 0
#define SD_INVALID (-EINVAL)
#define SD_NOMEM (-ENOMEM)
#define SD_BUSY (-EBUSY)

/* Типы блокировок для lock_type */
#define LOCK_TYPE_SPINLOCK 0
#define LOCK_TYPE_MUTEX 1
#define LOCK_TYPE_SEMAPHORE 2

/* Порог для учёта contention (в наносекундах) */
#define SD_WAIT_THRESHOLD_NS 100

#define SD_MIN_THREADS 1
#define SD_MAX_THREADS 32
#define SD_MIN_ITERATIONS 1
#define SD_MAX_ITERATIONS 1000000

/* 2.1 Глобальный контекст модуля */
struct sync_ctx {
  unsigned int num_threads;
  unsigned int iterations;
  unsigned int lock_type;

  long long shared_counter; /* защищаемый счётчик */

  spinlock_t slock;
  struct mutex mlock;
  struct semaphore sem;

  /* Статистика */
  ktime_t total_wait_time;       /* суммарное время ожидания блокировки */
  unsigned int contention_count; /* число раз, когда поток ждал */

  /* Управление потоками */
  struct task_struct **threads;
  atomic_t threads_done;
  int last_run_result; /* 0 = OK, <0 = ошибка */

  /* Флаг "тест активен" и mutex, защищающий ctx от гонок между
   * параллельными echo в разные параметры. */
  bool is_test_active;
  struct mutex ctx_lock;
};

/* 2.2 Аргументы рабочего потока */
struct worker_args {
  struct sync_ctx *ctx;
  unsigned int thread_id;
  ktime_t wait_time; /* время ожидания конкретного потока */
};

/* Глобальный контекст модуля, определён в main.c */
extern struct sync_ctx g_ctx;

/* ---- sync.c: обёртки захвата/освобождения блокировок ---- */

/* Инициализировать примитив синхронизации, соответствующий ctx->lock_type */
void sync_locks_init(struct sync_ctx *ctx);

/* Захватить блокировку согласно ctx->lock_type.
 * Должна фиксировать t_before/t_after через ktime_get() и обновлять
 * *wait_time / считать contention при превышении SD_WAIT_THRESHOLD_NS.
 */
void sync_lock_acquire(struct sync_ctx *ctx, ktime_t *wait_time,
                       unsigned int *contention);

/* Освободить блокировку согласно ctx->lock_type */
void sync_lock_release(struct sync_ctx *ctx);

/* ---- worker.c: логика рабочих потоков ---- */

/* Точка входа kthread. arg — указатель на struct worker_args. */
int sync_worker_thread_fn(void *arg);

/* Запустить полный цикл теста: создать ctx->num_threads потоков,
 * дождаться их завершения, собрать статистику в ctx.
 * Возвращает SD_OK / SD_BUSY / SD_NOMEM.
 */
int sync_run_test(struct sync_ctx *ctx);

/* ---- params.c: интерфейс module_param_cb ---- */

/* Регистрация всех параметров модуля (num_threads, iterations, lock_type,
 * run, result, stats, reset) через module_param_cb выполняется на уровне
 * файла в params.c — отдельной функции регистрации не требуется.
 */

#endif /* KERNEL_SYNC_DEMO_H */
