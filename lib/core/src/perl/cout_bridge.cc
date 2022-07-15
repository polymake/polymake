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

#include "polymake/perl/cout_bridge.h"
#include "polymake/internal/PlainParser.h"

namespace polymake { namespace perl {

std::ostream cout{nullptr};

} }

namespace pm { namespace perl { namespace glue {

ostreambuf_bridge::ostreambuf_bridge(pTHX_ GV* gv_arg)
{
   if (gv_arg && GvIO(gv_arg) && IoOFP(GvIO(gv_arg))) {
      gv = gv_arg;
      setp(buf, buf+sizeof(buf));
   } else {
      gv = nullptr;
   }
}

ostreambuf_bridge::int_type ostreambuf_bridge::overflow(int_type c)
{
   if (handover(false)) {
      if (!traits_type::eq_int_type(c, traits_type::eof())) {
         *pptr() = traits_type::to_char_type(c);
         pbump(1);
      }
      return traits_type::not_eof(c);
   } else {
      return traits_type::eof();
   }
}

bool ostreambuf_bridge::handover(bool with_sync)
{
   dTHX;
   IO* io = GvIO(gv);
   if (!io)
      throw std::runtime_error("internal error: STDOUT IO handle disappeared");
   PerlIO* fp=IoOFP(io);
   if (!fp)
      throw std::runtime_error("internal error: STDOUT IO handle not opened for writing");
   const std::streamsize out_size = pptr() - pbase();
   if (out_size > 0) {
      if (PerlIO_write(fp, buf, out_size) != out_size)
         throw std::runtime_error("internal error: buffered STDOUT not consumed completely");
      setp(buf, buf + sizeof(buf));
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
   static ostreambuf_bridge cout_bridge_buf(aTHX_ get_named_variable(aTHX_ "STDOUT", SVt_PVGV));
   polymake::perl::cout.rdbuf(&cout_bridge_buf);
   pm::cout.set_ostream(polymake::perl::cout);
}

} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
