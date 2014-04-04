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

static HV *keyword_stash, *integer_stash, *float_stash, *UNIVERSAL_stash;

MODULE = Polymake::Overload             PACKAGE = Polymake::Overload

PROTOTYPES: DISABLE

void
can_signature(arg, signature, keywords)
   SV* arg;
   SV* signature;
   SV* keywords;
PPCODE:
{
   STRLEN method_name_len;
   char* method_name=SvPV(signature,method_name_len);
   I32 num_flags=0;
   HV *stash=
      SvROK(arg) && SvOBJECT(SvRV(arg))
      ? SvSTASH(SvRV(arg)) :
      SvTRUE(keywords) && (SvIsUV(arg) || (SvROK(arg) && SvTYPE(SvRV(arg)) == SVt_PVHV))
      ? keyword_stash :
      SvIOK(arg)
      ? integer_stash :
      SvNOK(arg)
      ? float_stash :
      (num_flags=looks_like_number(arg)) & IS_NUMBER_IN_UV
      ? integer_stash :
      num_flags & IS_NUMBER_NOT_INT
      ? float_stash
      : UNIVERSAL_stash;
   GV *glob=gv_fetchmeth(stash, method_name, method_name_len, 0);
   if (glob)
      PUSHs( sv_2mortal(newRV((SV*)GvCV(glob))) );
   else
      PUSHs( &PL_sv_undef );
}

void
is_dynamic_default(arg)
   SV *arg;
PPCODE:
{
   if (SvROK(arg) && (arg=SvRV(arg), SvTYPE(arg)==SVt_PVCV && CvSTASH(arg)==keyword_stash))
      PUSHs(&PL_sv_yes);
   else
      PUSHs(&PL_sv_no);
}

void
mark_dynamic_default(aref)
   SV* aref;
PPCODE:
{
   AV *array=(AV*)SvRV(aref);
   SV *val=AvARRAY(array)[AvFILLp(array)];
   CV *sub=(CV*)SvRV(val);
   if (!SvOBJECT(array)) {
      SvSTASH(array)=(HV*)SvREFCNT_inc_simple_NN(keyword_stash);
      SvOBJECT_on(array);
   }
   CvSTASH_set(sub, keyword_stash);
}

BOOT:
   keyword_stash=gv_stashpv("Polymake::Overload::keyword", TRUE);
   integer_stash=gv_stashpv("Polymake::Overload::integer", TRUE);
   float_stash=gv_stashpv("Polymake::Overload::float", TRUE);
   UNIVERSAL_stash=gv_stashpv("UNIVERSAL", FALSE);

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
