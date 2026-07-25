#include "freertos_hooks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

void freertos_fatal_error(const char *message)
{
    printf("FreeRTOS fatal error: %s\n", message);
    taskDISABLE_INTERRUPTS();
    for (;;) {
        __asm volatile ("nop");
    }
}

void freertos_assert_failed(const char *file, int line)
{
    printf("FreeRTOS assertion failed: %s:%d\n", file, line);
    taskDISABLE_INTERRUPTS();
    for (;;) {
        __asm volatile ("nop");
    }
}

void vApplicationMallocFailedHook(void)
{
    freertos_fatal_error("malloc failed");
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    freertos_fatal_error(task_name != NULL ? task_name : "stack overflow");
}
