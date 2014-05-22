/* Copyright (c) 1997-2014
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

#ifndef POLYMAKE_PIPESTREAM_H
#define POLYMAKE_PIPESTREAM_H

#include "polymake/socketstream.h"
#include <sys/types.h>

namespace pm {

class background_process {
public:
   background_process(const char* progname, const char* const command[])
   {
      start(progname, command, 0, 0, 0);
   }

   background_process(const char* const command[])
   {
      start(command[0], command, 0, 0, 0);
   }

   background_process(const char* progname, const char* const command[], std::istream *Stdin, std::ostream *Stdout, std::ostream *Stderr)
   {
      start(progname, command, Stdin, Stdout, Stderr);
   }

   background_process(const char* const command[], std::istream *Stdin, std::ostream *Stdout, std::ostream *Stderr)
   {
      start(command[0], command, Stdin, Stdout, Stderr);
   }

   ~background_process();
protected:
   pid_t _pid;
   void start(const char* progname, const char* const command[], std::istream *Stdin, std::ostream *Stdout, std::ostream *Stderr);
};

class pipestream_base {
protected:
   pipestream_base() : _pid(0) { }
   ~pipestream_base();

   pid_t _pid;

   socketbuf* start(const char* progname, const char* const command[]);
   socketbuf* open2(const char* in_file, const char* out_file);
};

class pipestream
   : protected pipestream_base, public procstream {
public:
   pipestream(const char* progname, const char* const command[])
      : procstream(start(progname, command)) { }

   pipestream(const char* const command[])
      : procstream(start(command[0], command)) { }

   pipestream(int fd_arg)
      : procstream(new socketbuf(fd_arg)) { }

   pipestream(const char* in_file, const char* out_file)
      : procstream(open2(in_file,out_file)) { }
};

}

namespace polymake {
using pm::pipestream;
}

#endif // POLYMAKE_PIPESTREAM_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
