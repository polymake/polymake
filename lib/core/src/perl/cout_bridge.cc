/* Copyright (c) 1997-2018
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

#include "polymake/perl/cout_bridge.h"

namespace pm { namespace perl { namespace glue {

ostreambuf_bridge::ostreambuf_bridge(pTHX_ GV* gv_arg)
{
   if (gv_arg && GvIO(gv_arg) && IoOFP(GvIO(gv_arg))) {
      pi=getTHX;
      gv=gv_arg;
      setp(buf, buf+sizeof(buf));
   } else {
      pi=NULL;
      gv=Nullgv;
   }
}

ostreambuf_bridge::int_type ostreambuf_bridge::overflow(int_type c)
{
   if (handover(false)) {
      if (!traits_type::eq(c, traits_type::eof())) {
         *pptr() = c;
         pbump(1);
      }
      return traits_type::not_eof(c);
   } else {
      return traits_type::eof();
   }
}

bool ostreambuf_bridge::handover(bool with_sync)
{
   dTHXa(pi);
   IO* io=GvIO(gv);
   if (io == NULL)
      throw std::runtime_error("internal error: STDOUT IO handle disappeared");
   PerlIO* fp=IoOFP(io);
   if (fp == NULL)
      throw std::runtime_error("internal error: STDOUT IO handle not opened for writing");
   int out_size=pptr() - pbase();
   if (out_size>0) {
      if (PerlIO_write(fp, buf, out_size) != out_size)
         throw std::runtime_error("internal error: buffered STDOUT not consumed completely");
      setp(buf, buf+sizeof(buf));
   }
   if (with_sync)
      return PerlIO_flush(fp) != EOF;
   return true;
}

int ostreambuf_bridge::sync()
{
   return handover(true) ? 0 : -1;
}

void connect_cout(pTHX)
{
   static ostreambuf_bridge cout_bridge_buf(aTHX_ gv_fetchpv("STDOUT", false, SVt_PVGV));
   cout.rdbuf(&cout_bridge_buf);
}

}

std::ostream cout(NULL);

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
