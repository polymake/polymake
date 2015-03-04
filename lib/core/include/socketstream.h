/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

#ifndef POLYMAKE_SOCKETSTREAM_H
#define POLYMAKE_SOCKETSTREAM_H

#include <stdexcept>
#include <iostream>
#include <netinet/in.h>
#include <sys/poll.h>

#if defined(__GNUC__)

# define PM_SET_BUF_GET_CUR(buf,g,e) (buf)->_M_in_cur=(g), (buf)->_M_in_end=(e)
# define PM_SET_BUF_GET_END(buf,e) (buf)->_M_in_end=(e)
# define PM_SET_BUF_GET_END_OFF(buf,n) (buf)->_M_in_end=(buf)->_M_in_cur + (n)
# define PM_SET_BUF_GET_CUR_END(buf) (buf)->_M_in_cur=(buf)->_M_in_end;

#else

# define PM_SET_BUF_GET_CUR(buf,g,e) (buf)->setg((buf)->eback(), (g), (e))
# define PM_SET_BUF_GET_END(buf,e) (buf)->setg((buf)->eback(), (buf)->gptr(), e)
# define PM_SET_BUF_GET_END_OFF(buf,n) (buf)->setg((buf)->eback(), (buf)->gptr(), (buf)->gptr()+(n))
# define PM_SET_BUF_GET_CUR_END(buf) (buf)->gbump((buf)->egptr()-(buf)->gptr())

#endif

namespace pm {

class server_socketbuf;
class socketstream;

class streambuf_with_input_width : public std::streambuf {
protected:
   char *input_limit;
   streambuf_with_input_width() : input_limit(0) { }

public:
   streambuf_with_input_width(const std::string& src)
      : input_limit(0)
   {
      char *text=const_cast<char*>(src.c_str());
      setg(text, text, text+src.size());
   }

   bool set_input_width(int w);
   void reset_input_width(bool slurfed);

   void rewind(int w)
   {
      if (gptr()-w >= eback())
	 gbump(-w);
   }

   virtual int lines() { return 0; } // to be overloaded
};

class socketbuf : public streambuf_with_input_width {
public:
   socketbuf(int fd) : _fd(fd), _sfd(-1), _wfd(fd) { init(); }
   socketbuf(int rfd, int wfd) : _fd(rfd), _sfd(-1), _wfd(wfd) { init(); }
   socketbuf(in_addr_t addr, int port, int timeout, int retries);
   socketbuf(const char *hostname, const char *port, int timeout, int retries);
   ~socketbuf();

   void fill(pollfd& pfd) { pfd.fd=_fd; }

   void echo(int n)
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

   int try_out(const char* start, int size);
   int sync();
   int_type overflow(int_type c=traits_type::eof());
   int_type underflow();
   std::streamsize showmanyc();
   int_type pbackfail(int_type c=traits_type::eof());

   int _fd, _sfd, _wfd, bufsize;
   pollfd my_poll;

   friend class server_socketbuf;
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
   int_type overflow(int_type c=traits_type::eof());
   int_type underflow();
   std::streamsize showmanyc() { return 0; }
   int_type pbackfail(int_type) { return traits_type::eof(); }

private:
   static socketbuf* start(server_socketbuf*);
};

class istream_with_input_width : public std::istream {
public:
   istream_with_input_width(const std::string& src)
      : std::istream(new streambuf_with_input_width(src)) { }

   ~istream_with_input_width() { delete std::istream::rdbuf(); }

   struct set_w {
      int w;
      mutable std::istream *s;

      explicit set_w(int w_arg)
	 : w(w_arg), s(0) { }

      void apply(std::istream& s_arg) const
      {
	 if (static_cast<streambuf_with_input_width*>(s_arg.rdbuf())->set_input_width(w))
	    s=&s_arg;
	 else
	    s_arg.setstate(std::ios::failbit|std::ios::eofbit);
      }

      ~set_w()
      {
	 if (s) {
	    static_cast<streambuf_with_input_width*>(s->rdbuf())->reset_input_width(s->good());
	    s->clear(s->rdstate() & ~std::ios::eofbit);
	 }
      }
   };
};

inline istream_with_input_width::set_w setinputwidth(int w)
{
   return istream_with_input_width::set_w(w);
}

class procstream : public std::iostream {
protected:
   procstream(socketbuf *buf) : std::iostream(buf) { }
public:
   socketbuf* rdbuf() const { return static_cast<socketbuf*>(std::iostream::rdbuf()); }

   void close() { exceptions(goodbit); delete std::iostream::rdbuf(0); setstate(std::ios::eofbit); }
   ~procstream() { delete std::iostream::rdbuf(); }

   int_type skip(char c);
};

class socketstream : public procstream {
public:
   enum init_kind { init_with_port, init_with_fd };

   explicit socketstream(int arg=0, init_kind kind=init_with_port) :
      procstream(new server_socketbuf(arg, kind==init_with_port)) {}

   explicit socketstream(const char* path) :
      procstream(new server_socketbuf(path)) {}

   socketstream(const char* hostname, const char* port, int timeout=0, int retries=0) :
      procstream(new socketbuf(hostname,port,timeout,retries)) {}

   socketstream(in_addr_t addr, int port, int timeout=0, int retries=0) :
      procstream(new socketbuf(addr,port,timeout,retries)) {}

   int port() const;

   typedef socketbuf::connection_refused connection_refused;
};


inline
std::istream& operator>> (istream_with_input_width& in, const istream_with_input_width::set_w& sw)
{
   sw.apply(in);
   return in;
}

inline
std::iostream& operator>> (procstream& in, const istream_with_input_width::set_w& sw)
{
   sw.apply(in);
   return in;
}

}

namespace polymake {
using pm::socketstream;
}

#endif // POLYMAKE_SOCKETSTREAM_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
