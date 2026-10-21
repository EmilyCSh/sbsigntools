#ifndef SBSIGNTOOLS_UTIL_H
#define SBSIGNTOOLS_UTIL_H

#include <stdarg.h>
#include <stddef.h>

#define BUILD_ASSERT_OR_ZERO(condition) (sizeof(char[(condition) ? 1 : -1]) - 1)

void *mem_alloc_size(const void *parent, size_t size);
void *mem_alloc_zero(const void *parent, size_t count, size_t size);
void *mem_resize(const void *parent, void *ptr, size_t count, size_t size);
void *mem_reparent(const void *parent, const void *ptr);
void mem_free(const void *ptr);
void *mem_memdup(const void *parent, const void *data, size_t size);
char *mem_strdup(const void *parent, const char *str);
char *mem_strndup(const void *parent, const char *str, size_t len);
char *mem_vasprintf(const void *parent, const char *format, va_list args);
char *mem_asprintf(const void *parent, const char *format, ...);

#endif