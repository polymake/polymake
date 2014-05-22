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
#include <stdio.h>

typedef OP* (*op_func)(pTHX);

static op_func saved_op_sassign, saved_op_aassign, saved_op_anonlist, saved_op_anonhash;
static SV *scalar_pkg, *array_pkg, *hash_pkg;

static
OP* custom_op_sassign(pTHX)
{
   dSP;
   SV *var=TOPs;
   OP *ret=(*saved_op_sassign)(aTHX);
   if (!(SvFLAGS(var) & (SVs_TEMP | SVs_PADMY | SVs_GMG | SVs_SMG | SVs_RMG)) && !SvTIED_mg(var, PERL_MAGIC_tiedscalar)) {
      SPAGAIN;
      PUSHMARK(PL_stack_sp);
      PUSHs(var);
      PUSHs(scalar_pkg);
      XPUSHs(var);
      PUTBACK;
      Perl_pp_tie(aTHX);
   }
   return ret;
}

static
OP* custom_op_aassign(pTHX)
{
   dSP;
   SV *var=TOPs;
   OP *ret=(*saved_op_aassign)(aTHX);
   if (!(SvFLAGS(var) & (SVs_TEMP | SVs_PADMY | SVs_GMG | SVs_SMG | SVs_RMG)) && !SvTIED_mg(var, PERL_MAGIC_tied)) {
      SPAGAIN;
      PUSHMARK(PL_stack_sp);
      PUSHs(var);
      PUSHs(SvTYPE(var)==SVt_PVAV ? array_pkg : hash_pkg);
      XPUSHs(sv_2mortal(newRV(var)));
      PUTBACK;
      Perl_pp_tie(aTHX);
   }
   return ret;
}

static inline
void tie_anon(pTHX_ SV *pkg)
{
   dSP;
   SV *var=TOPs, *varref=var;
   if (PL_op->op_flags & OPf_SPECIAL)
      var=SvRV(varref);
   else
      varref=sv_2mortal(newRV(var));
   PUSHMARK(PL_stack_sp);
   PUSHs(var);
   XPUSHs(pkg);
   XPUSHs(varref);
   PUTBACK;
   Perl_pp_tie(aTHX);
}

static
OP* custom_op_anonlist(pTHX)
{
   OP *ret=(*saved_op_anonlist)(aTHX);
   tie_anon(aTHX_ array_pkg);
   return ret;
}

static
OP* custom_op_anonhash(pTHX)
{
   OP *ret=(*saved_op_anonhash)(aTHX);
   tie_anon(aTHX_ hash_pkg);
   return ret;
}

MODULE = Polymake::Core::Customize              PACKAGE = Polymake::Core::Customize

PROTOTYPES: DISABLE

void
compile_start()
PPCODE:
{
   saved_op_sassign=PL_ppaddr[OP_SASSIGN];
   saved_op_aassign=PL_ppaddr[OP_AASSIGN];
   saved_op_anonlist=PL_ppaddr[OP_ANONLIST];
   saved_op_anonhash=PL_ppaddr[OP_ANONHASH];
   PL_ppaddr[OP_SASSIGN]=&custom_op_sassign;
   PL_ppaddr[OP_AASSIGN]=&custom_op_aassign;
   PL_ppaddr[OP_ANONLIST]=&custom_op_anonlist;
   PL_ppaddr[OP_ANONHASH]=&custom_op_anonhash;
}

void
compile_end()
PPCODE:
{
   PL_ppaddr[OP_SASSIGN]=saved_op_sassign;
   PL_ppaddr[OP_AASSIGN]=saved_op_aassign;
   PL_ppaddr[OP_ANONLIST]=saved_op_anonlist;
   PL_ppaddr[OP_ANONHASH]=saved_op_anonhash;
}

BOOT:
{
   static const char scalar_pkg_name[]="Polymake::Core::Customize::Scalar",
                     array_pkg_name[]="Polymake::Core::Customize::Array",
                     hash_pkg_name[]="Polymake::Core::Customize::Hash";

   scalar_pkg=newSVpvn_share(scalar_pkg_name, sizeof(scalar_pkg_name)-1, 0);
   array_pkg=newSVpvn_share(array_pkg_name, sizeof(array_pkg_name)-1, 0);
   hash_pkg=newSVpvn_share(hash_pkg_name, sizeof(hash_pkg_name)-1, 0);
}

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
