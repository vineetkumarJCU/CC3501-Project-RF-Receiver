#ifndef FREERTOS_HOOKS_H
#define FREERTOS_HOOKS_H

void freertos_fatal_error(const char *message);
void freertos_assert_failed(const char *file, int line);

#endif /* FREERTOS_HOOKS_H */
