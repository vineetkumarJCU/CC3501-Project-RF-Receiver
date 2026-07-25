/* FreeRTOSConfig.h - application configuration for the official FreeRTOS RP2040 port. */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#include <stddef.h>

extern void freertos_assert_failed(const char *file, int line);

/* Hardware and SMP. Project values retained. */
#define configCPU_CLOCK_HZ                       (240UL * 1000UL * 1000UL)
#define configNUMBER_OF_CORES                    2
#define configRUN_MULTIPLE_PRIORITIES            0
#define configUSE_CORE_AFFINITY                  1
#define configTASK_DEFAULT_CORE_AFFINITY         tskNO_AFFINITY
#define configUSE_TASK_PREEMPTION_DISABLE        0
#define configUSE_PASSIVE_IDLE_HOOK              0
#define configTIMER_SERVICE_TASK_CORE_AFFINITY   tskNO_AFFINITY

/* Scheduler. */
#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configUSE_TICKLESS_IDLE                  0
#define configTICK_RATE_HZ                       1000
#define configMAX_PRIORITIES                     5
#define configMINIMAL_STACK_SIZE                 256
#define configMAX_TASK_NAME_LEN                  16
#define configTICK_TYPE_WIDTH_IN_BITS            TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                  1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES    1
#define configQUEUE_REGISTRY_SIZE                0
#define configENABLE_BACKWARD_COMPATIBILITY      1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS  0
#define configUSE_MINI_LIST_ITEM                 1
#define configSTACK_DEPTH_TYPE                   size_t
#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t

/* Software timers, queues and synchronization. */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             configMINIMAL_STACK_SIZE
#define configUSE_EVENT_GROUPS                   1
#define configUSE_STREAM_BUFFERS                 1
#define configUSE_TASK_NOTIFICATIONS             1
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_QUEUE_SETS                     0
#define configUSE_APPLICATION_TASK_TAG           0
#define configUSE_POSIX_ERRNO                    0

/* Memory allocation. Project uses heap_4 with a 64 KiB heap. */
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configSUPPORT_STATIC_ALLOCATION          0
#define configTOTAL_HEAP_SIZE                    (64U * 1024U)
#define configAPPLICATION_ALLOCATED_HEAP         0
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP 0
#define configHEAP_CLEAR_MEMORY_ON_FREE          0
#define configENABLE_HEAP_PROTECTOR              0

/* Hooks and diagnostics. */
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_MALLOC_FAILED_HOOK             1
#define configUSE_DAEMON_TASK_STARTUP_HOOK       0
#define configUSE_SB_COMPLETED_CALLBACK          0
#define configCHECK_FOR_STACK_OVERFLOW           2
#define configGENERATE_RUN_TIME_STATS            0
#define configUSE_TRACE_FACILITY                 0
#define configUSE_STATS_FORMATTING_FUNCTIONS     0
#define configUSE_NEWLIB_REENTRANT               0
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          1
#define configSUPPORT_PICO_SYNC_INTEROP          1
#define configSUPPORT_PICO_TIME_INTEROP          1

/* RP2040 port does not use Cortex-M interrupt priority configuration. */
#define configKERNEL_INTERRUPT_PRIORITY          0
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     0
#define configMAX_API_CALL_INTERRUPT_PRIORITY    0

/* API inclusion. Existing application API availability retained. */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_xTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTaskGetIdleTaskHandle           0
#define INCLUDE_eTaskGetState                    0
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xTaskAbortDelay                  0
#define INCLUDE_xTaskGetHandle                   0
#define INCLUDE_xTaskResumeFromISR               1

#define configASSERT(x) do { \
    if ((x) == 0) freertos_assert_failed(__FILE__, __LINE__); \
} while (0)

#endif /* FREERTOS_CONFIG_H */
