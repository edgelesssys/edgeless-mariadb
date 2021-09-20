/* Copyright (c) 2021, Edgeless Systems GmbH

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1335
   USA */

#include <sys/types.h>

extern "C" ssize_t ert_async_read(int fd, void *buf, size_t count);
extern "C" ssize_t ert_async_write(int fd, const void *buf, size_t count);
extern "C" int ert_async_read_wait(int fd, int timeout_ms);
