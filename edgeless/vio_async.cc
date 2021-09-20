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

#include "ert.h"
#include <cassert>
#include <cerrno>
#include <openssl/ssl.h>

#include <poll.h>
#include <unistd.h>

using namespace std;

__attribute__((weak)) ssize_t ert_async_read(int fd, void *buf, size_t count)
{
  errno= ENOSYS;
  return -1;
}

__attribute__((weak)) ssize_t ert_async_write(int fd, const void *buf,
                                              size_t count)
{
  errno= ENOSYS;
  return -1;
}

__attribute__((weak)) int ert_async_read_wait(int fd, int timeout_ms)
{
  errno= ENOSYS;
  return -1;
}

static int bio_get_fd(BIO *b) noexcept
{
  assert(b);
  return reinterpret_cast<intptr_t>(BIO_get_data(b));
}

static int bio_read(BIO *b, char *buf, int len)
{
  const int result= ert_async_read(bio_get_fd(b), buf, len);
  // const int result= read(bio_get_fd(b), buf, len);

  if (result == -1 && errno == EAGAIN)
  {
    BIO_clear_retry_flags(b);
    BIO_set_retry_read(b);
  }

  return result;
}

static int bio_write(BIO *b, const char *buf, int len)
{
  return ert_async_write(bio_get_fd(b), buf, len);
  // auto fd= bio_get_fd(b);
  // int res;
  // do
  // {
  //   res= write(fd, buf, len);
  // } while (res == -1 && errno == EINTR);
  // return res;
}

static long bio_ctrl(BIO *b, int cmd, long larg, void *parg)
{
  switch (cmd)
  {
  case BIO_CTRL_FLUSH:
    // nothing to do since our bio doesn't buffer
    return 1;
  case BIO_C_GET_FD:
    const int fd= bio_get_fd(b);
    if (parg)
      *static_cast<int *>(parg)= fd;
    return fd;
  }

  return 0;
}

extern "C" int edgeless_ssl_use_async(SSL *ssl, int fd)
{
  assert(ssl);
  assert(fd > 0);

  static const auto biom= [] {
    const auto biom= BIO_meth_new(BIO_TYPE_DESCRIPTOR, "");
    assert(biom);
    BIO_meth_set_read(biom, bio_read);
    BIO_meth_set_write(biom, bio_write);
    BIO_meth_set_ctrl(biom, bio_ctrl);
    return biom;
  }();

  // SSL_set_wfd(ssl, fd);
  // SSL_set_rfd(ssl, fd);
  SSL_set_fd(ssl, fd);

  const auto bio= BIO_new(biom);
  if (!bio)
    return 1;
  BIO_set_data(bio, reinterpret_cast<void *>(fd));

  // SSL_set0_rbio(ssl, bio);
  // SSL_set0_wbio(ssl, bio);
  SSL_set_bio(ssl, bio, bio);
  return 0;
}

/**
  Wait for an I/O event on an SSL connection.

  @retval -1  Failure.
  @retval  0  The wait has timed out.
  @retval  1  The requested I/O event has occurred.
*/
extern "C" int edgeless_ssl_wait(SSL *ssl, int timeout)
{
  const auto b= SSL_get_rbio(ssl);
  if (!b)
    return -1;
  return ert_async_read_wait(bio_get_fd(b), timeout);

  pollfd pfd{};
  pfd.events= POLLIN;
  pfd.fd= bio_get_fd(b);
  return poll(&pfd, 1, timeout);
}
