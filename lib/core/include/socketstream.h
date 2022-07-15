/* Copyright (c) 1997-2022
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#pragma once

#include <stdexcept>
#include <iostream>
#include <netinet/in.h>
#include <sys/poll.h>
#include "polymake/internal/streambuf_ext.h"

namespace pm {

class server_socketbuf;
class procstream;
class socketstream;

class socketbuf : public streambuf_ext {
public:
   socketbuf(int fd) : fd_(fd), sfd_(-1), wfd_(fd) { init(); }
   socketbuf(int rfd, int wfd) : fd_(rfd), sfd_(-1), wfd_(wfd) { init(); }
   socketbuf(in_addr_t addr, int port, int timeout, int retries);
   socketbuf(const char *hostname, const char *port, int timeout, int retries);
   ~socketbuf();

   void fill(pollfd& pfd) { pfd.fd = fd_; }

   void echo(size_type n)
   {
      sputn(gptr(), n);
      gbump(n);
   }

   void set_congestible(bool s);

   class connection_refused : public std::runtime_error {
   public:
      connection_refused() : std::runtime_error("connection refused") {}
   };

protected:
   socketbuf() { }
   void init();
   void connect(sockaddr_in& sa, int timeout, int retries);

   std::streamsize try_out(const char* start, size_type size);
   int sync();
   int_type overflow(int_type c = traits_type::eof());
   int_type underflow();
   std::streamsize showmanyc();
   int_type pbackfail(int_type c = traits_type::eof());

   void discard_out();

   int fd_, sfd_, wfd_;
   size_type bufsize;
   pollfd my_poll;

   friend class server_socketbuf;
   friend class procstream;
   friend class socketstream;
};

class server_socketbuf : public socketbuf {
public:
   // if init_with_port: create a server socket listening on the given port, or on a random free port if arg==0
   // if !init_with_port: arg is file descriptor of an already created server socket
   server_socketbuf(int arg, bool init_with_port);

   // create a UNIX-domain socket at the given path in the filesystem and listen on it
   explicit server_socketbuf(const char* path);

protected:
   int sync();
   int_type overflow(int_type c = traits_type::eof());
   int_type underflow();
   std::streamsize showmanyc() { return 0; }
   int_type pbackfail(int_type) { return traits_type::eof(); }

private:
   static socketbuf* start(server_socketbuf*);
};

class procstream : public std::iostream {
protected:
   procstream(socketbuf *buf) : std::iostream(buf) { }
public:
   socketbuf* rdbuf() const { return static_cast<socketbuf*>(std::iostream::rdbuf()); }

   void close()
   {
      exceptions(goodbit);
      delete std::iostream::rdbuf(nullptr);
      setstate(std::ios::eofbit);
   }

   ~procstream() { delete std::iostream::rdbuf(); }

   int_type skip(char c);

   void discard_out()
   {
      rdbuf()->discard_out();
   }
};

class socketstream : public procstream {
public:
   enum init_kind { init_with_port, init_with_fd };

   explicit socketstream(int arg = 0, init_kind kind = init_with_port)
      : procstream(new server_socketbuf(arg, kind == init_with_port)) {}

   explicit socketstream(const char* path)
      : procstream(new server_socketbuf(path)) {}

   socketstream(const char* hostname, const char* port, int timeout = 0, int retries = 0)
      : procstream(new socketbuf(hostname,port,timeout,retries)) {}

   socketstream(in_addr_t addr, int port, int timeout = 0, int retries = 0)
      : procstream(new socketbuf(addr,port,timeout,retries)) {}

   int port() const;

   typedef socketbuf::connection_refused connection_refused;
};

}

namespace polymake {
using pm::socketstream;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
