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

#include "polymake/pipestream.h"
#include <stdexcept>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstdlib>

namespace pm {
namespace {

class FileBuffer : public std::filebuf {
private:
   // never create
   FileBuffer();
   ~FileBuffer();
public:
   static int get_fd(std::filebuf *_buf)
   {
      FileBuffer *buf=static_cast<FileBuffer*>(_buf);
#if defined(__GNUC__)
      return buf->_M_file.fd();
#elif defined(__INTEL_COMPILER)
      return buf->fd();
#else
# error "don't know how to query the file descriptor"
#endif
   }
};

int get_fd(std::istream *in)
{
   std::ifstream *fs=dynamic_cast<std::ifstream*>(in);
   if (fs)
      return FileBuffer::get_fd(fs->rdbuf());
   procstream *ps=dynamic_cast<procstream*>(in);
   if (ps) {
      pollfd dummy_poll;
      ps->rdbuf()->fill(dummy_poll);
      return dummy_poll.fd;
   }
   return -1;
}

int get_fd(std::ostream *out)
{
   std::ofstream *fs=dynamic_cast<std::ofstream*>(out);
   if (fs) {
      int fd=FileBuffer::get_fd(fs->rdbuf());
      // assume both processes writing to the file in parallel
      fcntl(fd, F_SETFL, O_APPEND);
      return fd;
   }
   procstream *ps=dynamic_cast<procstream*>(out);
   if (ps) {
      pollfd dummy_poll;
      ps->rdbuf()->fill(dummy_poll);
      return dummy_poll.fd;
   }
   return -1;
}

void redirect(int from_fd, int to_fd)
{
   if (from_fd<0) {
      std::cerr << "fd(" << to_fd << ") redirect failed: source stream not bound to any file" << std::endl;
      std::exit(1);
   }
   if (dup2(from_fd, to_fd)<0) {
      int err=errno;
      std::cerr << "dup2(" << from_fd << ',' << to_fd << ") failed: errno=" << err << std::endl;
      std::exit(1);
   }
}
}

void background_process::start(const char* progname, const char* const command[],
			       std::istream *Stdin, std::ostream *Stdout, std::ostream *Stderr)
{
   _pid=fork();
   if (_pid<0)
      throw std::runtime_error("background_process: fork() failed");
   if (!_pid) {
      // child branch
      if (Stdin)  redirect(get_fd(Stdin),0);
      if (Stdout) redirect(get_fd(Stdout),1);
      if (Stderr) redirect(get_fd(Stderr),2);
      execvp(progname, const_cast<char* const*>(command));
      std::cerr << "background_process: exec(" << progname << ") failed" << std::endl;
      std::exit(1);
   }
}

background_process::~background_process()
{
   if (_pid>0) waitpid(_pid,0,0);
}

socketbuf* pipestream_base::open2(const char* in_file, const char* out_file)
{
   int fd_in=open(in_file, O_RDONLY);
   if (fd_in<0)
      throw std::runtime_error("pipestream: open(INFILE) failed");
   int fd_out=open(out_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
   if (fd_out<0) {
      close(fd_in);
      throw std::runtime_error("pipestream: open(OUTFILE) failed");
   }
   return new socketbuf(fd_in,fd_out);
}

socketbuf* pipestream_base::start(const char* progname, const char* const command[])
{
   int skp[2];
   if (socketpair(AF_UNIX,SOCK_STREAM,0,skp))
      throw std::runtime_error("pipestream: socketpair() failed");
   _pid=fork();
   if (_pid<0)
      throw std::runtime_error("pipestream: fork() failed");
   if (!_pid) {
      // child branch
      close(skp[0]);

      if (dup2(skp[1],0)<0 || dup2(skp[1],1)<0) {
	 std::cerr << "pipestream: dup() failed" << std::endl;
         std::exit(1);
      }
      close(skp[1]);
      execvp(progname, const_cast<char* const*>(command));
      std::cerr << "pipestream: exec(" << progname << ") failed" << std::endl;
      std::exit(1);
   }

   // parent branch goes on here
   close(skp[1]);
   return new socketbuf(skp[0]);      
}

pipestream_base::~pipestream_base()
{
   if (_pid>0) waitpid(_pid,0,0);
}

} // end namespace pm

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
