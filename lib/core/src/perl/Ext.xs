/* Copyright (c) 1997-2016
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
#include <stdio.h>

I32 pm_perl_skip_debug_cx=FALSE;

SV** pm_perl_get_cx_curpad(pTHX_ PERL_CONTEXT* cx, PERL_CONTEXT* cx_bottom)
{
   CV* cv;
   I32 d;
   while (--cx>=cx_bottom) {
      switch (CxTYPE(cx)) {
      case CXt_SUB:
         cv=cx->blk_sub.cv;
         if (!SkipDebugSub(cv)) {
            d=cx->blk_sub.olddepth;
            goto FOUND;
         }
         break;
      case CXt_EVAL:
         if (!CxTRYBLOCK(cx)) {
            cv=cx->blk_eval.cv;
            d=0;
            goto FOUND;
         }
         break;
      }
   }
   cv=PL_main_cv;
   d=0;
 FOUND:
   return PadARRAY((PadlistARRAY(CvPADLIST(cv)))[d+1]);
}

CV* pm_perl_get_cur_cv(pTHX)
{
   PERL_CONTEXT* cx_bottom=cxstack;
   PERL_CONTEXT* cx=cx_bottom+cxstack_ix;
   while (cx >= cx_bottom) {
      switch (CxTYPE(cx)) {
      case CXt_SUB: {
         CV *cv=cx->blk_sub.cv;
         if (!SkipDebugSub(cv)) return cv;
         break;
      }
      case CXt_EVAL: {
         CV *cv=cx->blk_eval.cv;
         if (cv && !CxTRYBLOCK(cx)) return cv;
         break;
      }
      }
      --cx;
   }
   return PL_main_cv;
}

/* for debugging */
const char* pm_perl_get_stash_name(pTHX_ SV *x)
{
   if (SvROK(x)) x=SvRV(x);
   switch (SvTYPE(x)) {
   case SVt_PVGV:
      return HvNAME(GvSTASH(x));
   case SVt_PVCV:
      return HvNAME(CvSTASH(x));
   case SVt_PVHV:
      if (!SvOBJECT(x)) return HvNAME(x);
   default:
      if (SvOBJECT(x)) return HvNAME(SvSTASH(x));
   }
   return "*** neither an object/stash/glob/code ***";
}

const char* pm_perl_get_gv_name(pTHX_ GV *x)
{
   if (SvTYPE(x)==SVt_PVGV) return GvNAME(x);
   return "*** not a glob ***";
}

MODULE = Polymake::Ext          PACKAGE = Polymake::Ext

PROTOTYPES: DISABLE

void
dump_sub(gv)
   SV *gv;
CODE:
{
#ifdef DEBUGGING
   Perl_dump_sub(aTHX_ (GV*)gv);
#else
   Perl_croak(aTHX_ "this perl is compiled without DEBUGGING");
   PERL_UNUSED_VAR(gv);
#endif
}

BOOT:
if (PL_DBgv)
   pm_perl_skip_debug_cx=TRUE;

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
