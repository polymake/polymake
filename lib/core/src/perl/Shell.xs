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

#include "polymake/perl/Ext.h"

#if PerlVersion < 5120
# define LineNumCorr +1
#else
# define LineNumCorr
#endif

MODULE = Polymake::Core::Shell          PACKAGE = Polymake::Core::Shell

PROTOTYPES: DISABLE

void
line_continued()
PPCODE:
{
   if (PL_parser->lex_brackets==0 && PL_parser->lex_state==LEX_NORMAL && PL_parser->expect==XSTATE)
      XPUSHs(&PL_sv_undef);
   else
      XPUSHs(sv_2mortal(newSViv(CopLINE(&PL_compiling) LineNumCorr)));
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
