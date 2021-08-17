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
#include <cerrno>
#include <openssl/ssl.h>

using namespace std;
using namespace edgeless;

static uring ring;
static connection_container connections;

static int bio_get_fd(BIO *b) noexcept
{
  assert(b);
  return reinterpret_cast<intptr_t>(BIO_get_data(b));
}

static int bio_read(BIO *b, char *buf, int len)
{
  const int fd= bio_get_fd(b);
  connection &con= connections[fd];

  const int ret= con.read(buf, len);
  const int err= errno;

  if (con.needs_submission())
    ring.submit(fd, con);

  if (ret == -1 && err == EAGAIN)
  {
    BIO_clear_retry_flags(b);
    BIO_set_retry_read(b);
  }

  errno= err;
  return ret;
}

static long bio_ctrl(BIO *b, int cmd, long larg, void *parg)
{
  switch (cmd)
  {
  case BIO_C_GET_FD:
    const int fd= bio_get_fd(b);
    if (parg)
      *static_cast<int *>(parg)= fd;
    return fd;
  }

  return 0;
}

extern "C" int edgeless_ssl_use_uring(SSL *ssl, int fd)
{
  assert(ssl);
  assert(fd > 0);

  static const auto biom= [] {
    const auto biom= BIO_meth_new(BIO_TYPE_DESCRIPTOR, "");
    assert(biom);
    BIO_meth_set_read(biom, bio_read);
    BIO_meth_set_ctrl(biom, bio_ctrl);
    return biom;
  }();

  SSL_set_wfd(ssl, fd);

  const auto bio= BIO_new(biom);
  if (!bio)
    return 1;
  BIO_set_data(bio, reinterpret_cast<void *>(fd));

  SSL_set0_rbio(ssl, bio);
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
  const int fd= bio_get_fd(b);

  try
  {
    return connections.at(fd).wait(chrono::milliseconds(timeout)) ? 1 : 0;
  }
  catch (...)
  {
    return -1;
  }
}
