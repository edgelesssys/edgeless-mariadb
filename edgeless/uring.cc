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
#include <stdexcept>

using namespace std;
using namespace edgeless;

uring::uring() : ring_()
{
  if (io_uring_queue_init(8, &ring_, 0) != 0)
    throw runtime_error("io_uring_queue_init failed");
  thread_= thread(&uring::consume, this);
}

void uring::submit(int fd, connection &con)
{
  auto &buf= con.buf_;
  buf.resize(1024);

  const lock_guard<mutex> lock(mutex_);
  const auto sqe= io_uring_get_sqe(&ring_);
  io_uring_prep_recv(sqe, fd, buf.data(), buf.size(), 0);
  sqe->user_data= reinterpret_cast<uintptr_t>(&con);
  io_uring_submit(&ring_);
}

void uring::consume()
{
  for (;;)
  {
    io_uring_cqe *cqe;
    int res;
    do
    {
      res= io_uring_wait_cqe(&ring_, &cqe);
    } while (res == -1 && errno == EINTR);

    if (res != 0)
      abort();

    auto &con= *reinterpret_cast<connection *>(cqe->user_data);
    con.notify(cqe->res);
    io_uring_cqe_seen(&ring_, cqe);
  }
}
