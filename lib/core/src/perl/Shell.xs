/* Copyright (c) 1997-2017
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

#include "polymake/perl/Ext.h"

MODULE = Polymake::Core::Shell          PACKAGE = Polymake::Core::Shell

PROTOTYPES: DISABLE

void
line_continued()
PPCODE:
{
   if (PL_parser->lex_brackets==0 && PL_parser->lex_state==LEX_NORMAL && PL_parser->expect==XSTATE)
      XPUSHs(&PL_sv_undef);
   else
      XPUSHs(sv_2mortal(newSViv(CopLINE(&PL_compiling))));
}

void
enforce_scalar_context()
PPCODE:
{
   // visit all top-level ENTERSUB OPs and make them believe they are running in scalar context
   // this is necessary for running test code snippets which may contain VISUAL calls
   // current OP is supposed to be an ENTERSUB followed by a NEXTSTATE
   OP* o;
   for (o=PL_op->op_next; OpHAS_SIBLING(o); o=OpSIBLING(o)) {
      OP* sub_op= o->op_type == OP_NULL ? cUNOPo->op_first : o;
      if (sub_op->op_type == OP_ENTERSUB &&
          (sub_op->op_flags & OPf_WANT) == OPf_WANT_VOID) {
         sub_op->op_flags ^= OPf_WANT_SCALAR | OPf_WANT_VOID;
      }
   }
}

BOOT:
if (PL_DBgv) {
   CvNODEBUG_on(get_cv("Polymake::Core::Shell::line_continued", FALSE));
}

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
