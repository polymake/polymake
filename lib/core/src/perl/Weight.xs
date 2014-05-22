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

MODULE = Polymake::Core::Rule::Weight           PACKAGE = Polymake::Core::Rule::Weight

PROTOTYPES: DISABLE

void
add_atom(wt,major,minor)
   SV *wt=SvRV(ST(0));
   I32 major;
   I32 minor;
CODE:
   {
      I32 l=SvCUR(wt)/sizeof(I32);
      ((I32*)SvPVX(wt))[l-1-major] += minor;
   }

void
set_atom(wt,major,minor)
   SV *wt=SvRV(ST(0));
   I32 major;
   I32 minor;
CODE:
   {
      I32 l=SvCUR(wt)/sizeof(I32);
      ((I32*)SvPVX(wt))[l-1-major] = minor;
   }

void
sum(wt1,wt2)
   SV *wt1=SvRV(ST(0));
   SV *wt2=SvRV(ST(1));
CODE:
   {
      I32 *wtp1=(I32*)SvPVX(wt1),
          *wtp2=(I32*)SvPVX(wt2);
      I32 l=SvCUR(wt1)/sizeof(I32);
      for ( ; l>0; l--)
         (*wtp1++) += (*wtp2++);
   }

I32
compare(wt1,wt2,reverse)
   SV *wt1 = SvRV(ST(0));
   SV *wt2 = SvRV(ST(1));
   I32 reverse;
CODE:
   RETVAL=0;
   {
      I32* wtp1=(I32*)SvPVX(wt1);
      I32* wtp2=(I32*)SvPVX(wt2);
      I32 l=SvCUR(wt1)/sizeof(I32);
      while (--l>=0 && !(RETVAL = (*wtp1++) - (*wtp2++))) ;
      if (reverse) RETVAL=-RETVAL;
   }
OUTPUT:
   RETVAL

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
