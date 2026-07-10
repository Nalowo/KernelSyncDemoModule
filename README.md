# kernel_sync_demo

Модуль ядра Linux, демонстрирующий работу трёх примитивов синхронизации —
`spinlock_t`, `struct mutex` и `struct semaphore` — на общем разделяемом
счётчике (`shared_counter`), к которому параллельно обращаются несколько
`kthread`. Тип блокировки выбирается на лету, модуль собирает статистику
времени ожидания (contention).

## Требования

- Заголовки текущего ядра (`/lib/modules/$(uname -r)/build`) или указанный
  через `KDIR` путь к дереву исходников ядра.
- `make`, `gcc`, права root для `insmod`/`rmmod`.

## Сборка

```bash
make
# при необходимости указать другое дерево ядра:
make KDIR=/path/to/kernel/build
```

Результат — `kernel_sync_demo.ko` в корне проекта.

Очистка артефактов сборки:

```bash
make clean
```

## Структура проекта

```
kernel_sync_demo_module/
├── Makefile              # верхнеуровневый make, вызывает сборку через дерево ядра
├── Kbuild                # obj-m и список объектных файлов для kbuild
└── src/
    ├── kernel_sync_demo.h  # struct sync_ctx, struct worker_args, коды SD_*, прототипы
    ├── main.c               # module_init/module_exit, глобальный контекст g_ctx
    ├── params.c             # интерфейс module_param_cb (num_threads, iterations,
    │                        #   lock_type, run, result, stats, reset)
    ├── worker.c             # логика рабочих потоков (kthread)
    └── sync.c               # обёртки захвата/освобождения блокировок
```

## Параметры модуля

| Параметр | Тип | По умолчанию | Ограничения |
|---|---|---|---|
| `num_threads` | uint | 4 | 1–32 |
| `iterations` | uint | 1000 | 1–1 000 000 |
| `lock_type` | uint | 0 | 0 = spinlock, 1 = mutex, 2 = semaphore |

Параметры можно задать при загрузке или менять на лету через
`/sys/module/kernel_sync_demo/parameters/`.

## Интерфейс `/sys/module/kernel_sync_demo/parameters/`

| Файл | Доступ | Назначение |
|---|---|---|
| `run` | write | запустить тест с текущими параметрами (`-EBUSY`, если предыдущий ещё выполняется) |
| `result` | read | итог последнего запуска: `counter=0 threads=4 iterations=1000 lock=spinlock ok` |
| `lock_type` | write | сменить тип блокировки (0/1/2) |
| `stats` | read | статистика ожиданий: `contention=42 total_wait_ns=18500 avg_wait_ns=440` |
| `reset` | write | сбросить счётчик и статистику (тест должен быть неактивен) |

## Пример использования

```bash
# Загрузка модуля
sudo insmod kernel_sync_demo.ko num_threads=8 iterations=5000 lock_type=0

# Запуск теста со spinlock
echo 1 > /sys/module/kernel_sync_demo/parameters/run

# Результат
cat /sys/module/kernel_sync_demo/parameters/result
# counter=0 threads=8 iterations=5000 lock=spinlock ok

# Статистика
cat /sys/module/kernel_sync_demo/parameters/stats
# contention=317 total_wait_ns=142000 avg_wait_ns=447

# Переключиться на mutex и запустить снова
echo 1 > /sys/module/kernel_sync_demo/parameters/lock_type
echo 1 > /sys/module/kernel_sync_demo/parameters/run

# Сброс состояния
echo 1 > /sys/module/kernel_sync_demo/parameters/reset

# Выгрузка
sudo rmmod kernel_sync_demo
dmesg | tail
```