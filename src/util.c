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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

uint32_t cpu_to_le32(uint32_t value)
{
	unsigned char bytes[4] = {
		value & 0xff,
		(value >> 8) & 0xff,
		(value >> 16) & 0xff,
		(value >> 24) & 0xff
	};
    
	uint32_t result;

	memcpy(&result, bytes, sizeof(result));
	return result;
}

union mem_header {
	struct {
		union mem_header *parent;
		union mem_header *child;
		union mem_header *prev;
		union mem_header *next;
	} links;
	long double alignment;
	void *pointer_alignment;
};

static union mem_header *mem_header(const void *ptr)
{
	return (union mem_header *)ptr - 1;
}

static void mem_unlink(union mem_header *header)
{
	if (header->links.prev) {
		header->links.prev->links.next = header->links.next;
	} else if (header->links.parent) {
		header->links.parent->links.child = header->links.next;
	}

	if (header->links.next) {
		header->links.next->links.prev = header->links.prev;
	}
}

static void mem_link(union mem_header *header, const void *parent)
{
	header->links.parent = parent ? mem_header(parent) : NULL;
	header->links.prev = NULL;
	header->links.next = parent ? header->links.parent->links.child : NULL;
	if (header->links.next) {
		header->links.next->links.prev = header;
	}

	if (parent) {
		header->links.parent->links.child = header;
	}
}

void *mem_alloc_size(const void *parent, size_t size)
{
	union mem_header *header;

	if (size > SIZE_MAX - sizeof(*header)) {
		errno = ENOMEM;
		return NULL;
	}

	header = malloc(sizeof(*header) + size);
    
	if (!header) {
		return NULL;
	}

	header->links.child = NULL;
	mem_link(header, parent);
	return header + 1;
}

void *mem_alloc_zero(const void *parent, size_t count, size_t size)
{
	void *ptr = mem_resize(parent, NULL, count, size);

	if (ptr) {
		memset(ptr, 0, count * size);
	}

	return ptr;
}

void *mem_resize(const void *parent, void *ptr, size_t count, size_t size)
{
	union mem_header *header, *child;

	if (size && count > (SIZE_MAX - sizeof(*header)) / size) {
		errno = ENOMEM;
		return NULL;
	}

	size *= count;

	if (!ptr) {
		return mem_alloc_size(parent, size);
	}

	header = realloc(mem_header(ptr), sizeof(*header) + size);

	if (!header) {
		return NULL;
	}
    
	if (header->links.prev) {
		header->links.prev->links.next = header;
	} else if (header->links.parent) {
		header->links.parent->links.child = header;
	}

	if (header->links.next) {
		header->links.next->links.prev = header;
	}

	for (child = header->links.child; child; child = child->links.next) {
		child->links.parent = header;
	}

	return header + 1;
}

void *mem_reparent(const void *parent, const void *ptr)
{
	union mem_header *header;

	if (!ptr) {
		return NULL;
	}

	header = mem_header(ptr);
	mem_unlink(header);
	mem_link(header, parent);
	return header + 1;
}

void mem_free(const void *ptr)
{
	union mem_header *header;

	if (!ptr) {
		return;
	}

	header = mem_header(ptr);
	mem_unlink(header);

	while (header->links.child) {
		mem_free(header->links.child + 1);
	}

	free(header);
}

void *mem_memdup(const void *parent, const void *data, size_t size)
{
	void *ptr = mem_alloc_size(parent, size);

	if (ptr && size) {
		memcpy(ptr, data, size);
	}

	return ptr;
}

char *mem_strndup(const void *parent, const char *str, size_t len)
{
	char *copy;
	size_t length = 0;

	while (length < len && str[length] != '\0') {
		length++;
	}
    
	len = length;
	copy = mem_alloc_size(parent, len + 1);

	if (copy) {
		memcpy(copy, str, len);
		copy[len] = '\0';
	}

	return copy;
}

char *mem_strdup(const void *parent, const char *str)
{
	return mem_strndup(parent, str, strlen(str));
}

char *mem_vasprintf(const void *parent, const char *format, va_list args)
{
	va_list copy;
	char *str;
	int len;

	va_copy(copy, args);
	len = vsnprintf(NULL, 0, format, copy);
	va_end(copy);

	if (len < 0) {
		return NULL;
	}

	str = mem_alloc_size(parent, (size_t)len + 1);
	if (str) {
		va_copy(copy, args);

		if (vsnprintf(str, (size_t)len + 1, format, copy) != len) {
			mem_free(str);
			str = NULL;
		}

		va_end(copy);
	}
    
	return str;
}

char *mem_asprintf(const void *parent, const char *format, ...)
{
	va_list args;
	char *str;

	va_start(args, format);
	str = mem_vasprintf(parent, format, args);
	va_end(args);
	return str;
}
