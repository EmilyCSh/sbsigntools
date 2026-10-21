/*
 * Copyright (C) Emily <info@emy.sh>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library under certain conditions as described in each individual source file,
 * and distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all
 * of the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the
 * file(s), but you are not obligated to do so. If you do not wish to do
 * so, delete this exception statement from your version. If you delete
 * this exception statement from all source files in the program, then
 * also delete it here.
 */

#ifndef SBSIGNTOOLS_UTIL_H
#define SBSIGNTOOLS_UTIL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define BUILD_ASSERT_OR_ZERO(condition) (sizeof(char[(condition) ? 1 : -1]) - 1)

uint32_t cpu_to_le32(uint32_t value);

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
