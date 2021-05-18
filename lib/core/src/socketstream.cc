/* Copyright (c) 1997-2021
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

#include "polymake/socketstream.h"
#include "polymake/internal/CharBuffer.h"
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>
#include <limits.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <netdb.h>

namespace pm {

bool streambuf_ext::set_input_width(std::streamsize w)
{
   while (gptr() + w > egptr())
      if (this->underflow() == traits_type::eof())
         return false;
   input_limit = egptr();
   setg(eback(), gptr(), gptr() + w);
   return true;
}

void streambuf_ext::reset_input_width(bool slurfed)
{
   setg(eback(), slurfed ? egptr() : gptr(), input_limit);
   input_limit = nullptr;
}

void streambuf_ext::rewind(size_type w)
{
   char* const back_to = gptr() - w;
   if (back_to >= eback())
      setg(eback(), back_to, egptr());
   else
      throw std::runtime_error("streambuf_ext: attempt to rewind beyond the buffer boundary");
}

void socketbuf::init()
{
   char* out_buf = new char[BUFSIZ];
   try {
      char* in_buf = new char[bufsize = BUFSIZ];
      setg(in_buf, in_buf, in_buf);
   } catch (...) {
      delete[] out_buf;
      throw;
   }
   setp(out_buf, out_buf + BUFSIZ);
   input_limit = nullptr;
   my_poll.events = 0;
}

void socketbuf::connect(sockaddr_in& sa, int timeout, int retries)
{
   while (::connect(fd_, (sockaddr*)&sa, sizeof(sa))) {
      if (errno != ECONNREFUSED  &&  errno != ETIMEDOUT  &&  errno != EAGAIN)
         throw std::runtime_error(std::string("socketstream - connect failed: ") += strerror(errno));
      if (--retries < 0)
         throw connection_refused();
      if (timeout) sleep(timeout);
   }
}

socketbuf::socketbuf(in_addr_t addr, int port, int timeout, int retries)
   : fd_(socket(AF_INET, SOCK_STREAM, 0))
   , sfd_(-1)
{
   wfd_ = fd_;
   if (fd_ < 0)
      throw std::runtime_error(std::string("socketstream - socket failed: ") += strerror(errno));
   if (port <= 0 || port >= USHRT_MAX)
      throw std::runtime_error("invalid port number");
   sockaddr_in sa = { AF_INET };
   sa.sin_addr.s_addr = htonl(addr);
   sa.sin_port = htons(uint16_t(port));
   connect(sa, timeout, retries);
   init();
}

socketbuf::socketbuf(const char* hostname, const char* port, int timeout, int retries)
   : fd_(socket(AF_INET, SOCK_STREAM, 0))
   , sfd_(-1)
{
   wfd_ = fd_;
   if (fd_ < 0) throw std::runtime_error(std::string("socketstream - socket failed: ") += strerror(errno));
   static const addrinfo hints = { 0, PF_INET, SOCK_STREAM, IPPROTO_TCP, 0, nullptr, nullptr, nullptr };
   addrinfo *res, *res0;
   int error = getaddrinfo(hostname, port, &hints, &res0);
   if (error) {
      if (error == EAI_NONAME) {
         throw std::runtime_error("socketstream - unknown hostname");
      } else {
         std::ostringstream err_txt;
         err_txt << "socketstream - getaddrinfo failed: " << gai_strerror(error);
         throw std::runtime_error(err_txt.str());
      }
   }
   for (res = res0; res && res->ai_addrlen != sizeof(sockaddr_in); res = res->ai_next) ;
   if (!res)
      throw std::runtime_error("socketstream - no IPv4 address configured");
   sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(res->ai_addr);
   try {
      connect(*sa, timeout, retries);
   } catch (...) {
      close(fd_);
      freeaddrinfo(res0);
      throw;
   }
   freeaddrinfo(res0);
   init();
}

socketbuf::~socketbuf()
{
   sync();
   delete[] eback();  setg(nullptr, nullptr, nullptr);
   delete[] pbase();  setp(nullptr, nullptr);
   if (fd_ >= 0)
      close(fd_);
   if (sfd_ >= 0)
      close(sfd_);
   else if (wfd_ >= 0 && wfd_ != fd_)
      close(wfd_);
}

void socketbuf::set_congestible(bool s)
{
   if (s && fd_ == wfd_) {
      my_poll.fd = fd_;
      my_poll.events = POLLIN | POLLOUT | POLLERR | POLLHUP;
   } else {
      my_poll.events = 0;
   }
}

std::streamsize socketbuf::try_out(const char* start, std::streamsize size)
{
   if (my_poll.events) {
      for (;;) {
         if (poll(&my_poll, 1, -1) <= 0 ||
             (my_poll.revents & (POLLERR | POLLHUP)))
            return -1;
         if (my_poll.revents & POLLOUT)
            break;
         char* save_input_limit = input_limit;
         input_limit = nullptr;
         this->underflow();
         input_limit = save_input_limit;
      }
   }
   return write(wfd_, start, size);
}

int socketbuf::sync()
{
   char* start = pbase();
   std::streamsize out_size = pptr() - pbase();
   while (out_size > 0) {
      std::streamsize written = try_out(start, out_size);
      if (written < 0) return -1;
      out_size -= written;
      start += written;
   }
   setp(pbase(), epptr());
   return 0;
}

socketbuf::int_type socketbuf::overflow(int_type c)
{
   std::streamsize out_size = pptr() - pbase();
   if (out_size > 0) {
      std::streamsize written = try_out(pbase(), out_size);
      if (written <= 0) return traits_type::eof();
      if ((out_size -= written) > 0)
         std::memmove(pbase(), pbase() + written, out_size);
      static_cast<OutCharBuffer*>(static_cast<std::streambuf*>(this))->pbump(pbase() + out_size - pptr());
   }
   if (!traits_type::eq_int_type(c, traits_type::eof())) {
      *pptr() = traits_type::to_char_type(c);
      pbump(1);
   }
   return traits_type::not_eof(c);
}

socketbuf::int_type socketbuf::underflow()
{
   if (input_limit) return traits_type::eof();
   char* buf = eback();
   std::streamsize free = buf + bufsize - egptr(),
                pending = egptr() - gptr();
   if (!pending || free <= 2) {
      if (pending) {
         if (gptr() != buf) {
            std::memmove(buf, gptr(), pending);
         } else {
            char* new_buf = new char[(bufsize += pending)];
            std::memmove(new_buf, buf, pending);
            delete[] buf;
            buf = new_buf;
         }
      }
      setg(buf, buf, buf + pending);
      free = bufsize-pending;
   }
   std::streamsize gotten = read(fd_, egptr(), free);
   if (gotten <= 0) return traits_type::eof();
   setg(buf, gptr(), egptr() + gotten);
   return traits_type::to_int_type(*gptr());
}

std::streamsize socketbuf::showmanyc()
{
   char* buf = eback();
   if (buf + bufsize != egptr())
      setg(buf, buf, buf);

   fcntl(fd_, F_SETFL, O_NONBLOCK);
   std::streamsize gotten=read(fd_, buf, bufsize);
   int err = errno;
   fcntl(fd_, F_SETFL, 0);

   if (gotten >= 0) {
      setg(buf, buf, buf + gotten);
      return gotten;
   }
   return err == EAGAIN ? 0 : traits_type::eof();
}

socketbuf::int_type socketbuf::pbackfail(int_type c)
{
   if (traits_type::eq_int_type(c, traits_type::eof())) {
      if (gptr() > egptr()) {
         gbump(-1);
         return traits_type::to_int_type(*gptr());
      }
   } else {
      if (gptr() == eback()) {
         std::streamsize free = eback() + bufsize - egptr(),
                      pending = egptr() - gptr();
         if (free > 0) {
            if (pending > 0) {
               free = (free+1)/2;
               std::memmove(gptr()+free, gptr(), pending);
            }
            setg(eback(), gptr() + free, egptr() + free);
         } else {
            char* in_buf = new char[bufsize + bufsize/2];
            char* in_cur = in_buf + bufsize/4;
            std::memmove(in_cur, eback(), pending);
            delete[] eback();
            setg(in_buf, in_cur, in_cur + pending);
            bufsize += bufsize/2;
         }
      }
      gbump(-1);
      *gptr() = traits_type::to_char_type(c);
   }
   return c;
}

server_socketbuf::server_socketbuf(int arg, bool init_with_port)
{
   sfd_ = -1;
   if (init_with_port) {
      if ((wfd_ = fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
         throw std::runtime_error(std::string("server_socketbuf: socket failed: ") += strerror(errno));
      sockaddr_in sa={ AF_INET };
      sa.sin_addr.s_addr = INADDR_ANY;
      if (arg != 0) {
         if (arg <= 0 || arg >= USHRT_MAX)
            throw std::runtime_error("invalid port number");
         sa.sin_port = htons(uint16_t(arg));
         if (bind(fd_, (sockaddr*)&sa, sizeof(sa)))
            throw std::runtime_error(std::string("server_socketbuf: bind failed: ") += strerror(errno));
      } else {
         for (uint16_t port = 30000; port < USHRT_MAX; ++port) {
            sa.sin_port = htons(port);
            if (!bind(fd_, (sockaddr*)&sa, sizeof(sa))) {
               arg = port;
               break;
            }
            if (errno != EADDRINUSE)
               throw std::runtime_error(std::string("server_socketbuf: bind failed: ") += strerror(errno));
         }
         if (arg == 0)
            throw std::runtime_error(std::string("server_socketbuf: bind failed: all ports seem occupied"));
      }
   } else {
      wfd_ = fd_ = arg;
   }
   fcntl(fd_, F_SETFD, FD_CLOEXEC);
   if (listen(fd_, 1))
      throw std::runtime_error(std::string("server_socketbuf: listen failed: ") += strerror(errno));
}

server_socketbuf::server_socketbuf(const char* path)
{
   sfd_ = -1;
   if ((wfd_ = fd_ = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
      throw std::runtime_error(std::string("server_socketbuf: socket failed: ") += strerror(errno));
   sockaddr_un sa = { AF_UNIX };
   strncpy(sa.sun_path, path, sizeof(sa.sun_path)-1);
   sa.sun_path[sizeof(sa.sun_path) -1 ] = '\0';
   if (bind(fd_, (sockaddr*)&sa, sizeof(sa)))
      throw std::runtime_error(std::string("server_socketbuf: bind failed: ") += strerror(errno));
   fcntl(fd_, F_SETFD, FD_CLOEXEC);
   if (listen(fd_, 1))
      throw std::runtime_error(std::string("server_socketbuf: listen failed: ") += strerror(errno));
}

int socketstream::port() const
{
   socketbuf* buf=rdbuf();
   sockaddr_in sa;
   socklen_t l = sizeof(sa);
   if (getsockname(buf->fd_, (sockaddr*)&sa, &l))
      throw std::runtime_error(std::string("socketstream: getsockname failed: ") += strerror(errno));
   return ntohs(sa.sin_port);
}

void socketbuf::discard_out()
{
   setp(pbase(), epptr());
}

socketbuf* server_socketbuf::start(server_socketbuf* me)
{
   const int sfd = me->fd_;
   const int fd = accept(sfd, nullptr, nullptr);
   if (fd < 0)
      throw std::runtime_error(std::string("server_socketbuf: accept failed: ") += strerror(errno));
   fcntl(fd, F_SETFD, FD_CLOEXEC);
   socketbuf* buf = new (me) socketbuf(fd);
   buf->sfd_ = sfd;
   return buf;
}

int server_socketbuf::sync()
{
   return start(this)->sync();
}

server_socketbuf::int_type server_socketbuf::overflow(int_type c)
{
   return start(this)->overflow(c);
}

server_socketbuf::int_type server_socketbuf::underflow()
{
   return start(this)->underflow();
}

procstream::int_type procstream::skip(char c)
{
   return CharBuffer::ignore(std::iostream::rdbuf(), c);
}

} // end namespace std

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
