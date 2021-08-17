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

#include "uring.h"
#include <cassert>
#include <cstring>

using namespace std;
using namespace edgeless;

int connection::read(char *buf, int len)
{
  const lock_guard<mutex> lock(mutex_);

  if (remaining_ < 0)
  {
    errno= remaining_;
    return -1;
  }

  if (remaining_ == 0)
  {
    errno= EAGAIN;
    return -1;
  }

  if (remaining_ < len)
    len= remaining_;
  memcpy(buf, &buf_[buf_.size() - remaining_], len);
  remaining_-= len;

  return len;
}

bool connection::wait(std::chrono::milliseconds timeout) const
{
  unique_lock<mutex> lock(mutex_);
  return cv_.wait_for(lock, timeout, [this] { return !pending_; });
}

bool connection::needs_submission()
{
  const lock_guard<mutex> lock(mutex_);
  if (pending_ || remaining_ != 0)
    return false;
  pending_= true;
  return true;
}

void connection::notify(int res)
{
  const lock_guard<mutex> lock(mutex_);
  assert(pending_);
  pending_= false;
  remaining_= res;
  if (res > 0)
    buf_.resize(res);
  cv_.notify_all();
}

connection &connection_container::operator[](int fd)
{
  const lock_guard<mutex> lock(mutex_);
  return data_[fd];
}

connection &connection_container::at(int fd)
{
  const lock_guard<mutex> lock(mutex_);
  return data_.at(fd);
}
