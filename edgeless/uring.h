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

#pragma once

#include <condition_variable>
#include <cstdint>
#include <liburing.h>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace edgeless
{
class connection final
{
  friend class uring;

public:
  int read(char *buf, int len);
  bool wait(std::chrono::milliseconds timeout) const;
  bool needs_submission();
  void notify(int res);

private:
  std::vector<uint8_t> buf_;
  int remaining_= 0;
  bool pending_= false;
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
};

class connection_container final
{
public:
  connection &operator[](int fd);
  connection &at(int fd);

private:
  std::map<int, connection> data_;
  std::mutex mutex_;
};

class uring final
{
public:
  uring();
  void submit(int fd, connection &con);

private:
  io_uring ring_;
  std::thread thread_;
  std::mutex mutex_;

  void consume();
};
} // namespace edgeless
