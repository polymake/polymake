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

int Scope_local_marker_index;

static
void localize_marker(pTHX_ void* p)
{
   if (PL_in_eval & ~(EVAL_INREQUIRE))  /* functionality of $^S less the lex test */
      Perl_croak(aTHX_ "Scope::end missing");
}

typedef struct local_var_ptrs {
   SV* var;
   void* orig_any;
   U32 orig_flags;
   char* orig_pv;       // as a representative for SV_HEAD_UNION
   SV* temp_owner;
} local_var_ptrs;

static
local_var_ptrs* do_local_var(SV* var, SV* value)
{
   local_var_ptrs* ptrs;
   New(0, ptrs, 1, local_var_ptrs);
   ptrs->var=var;
   ptrs->orig_any=SvANY(var);
   ptrs->orig_flags=SvFLAGS(var) & ~SVs_TEMP;
   ptrs->orig_pv=var->sv_u.svu_pv;
   var->sv_u.svu_pv=value->sv_u.svu_pv;
   SvANY(var)=SvANY(value);
   SvFLAGS(var)=SvFLAGS(value) & ~SVs_TEMP;
   ptrs->temp_owner=value;
   SvREFCNT_inc_simple_void_NN(var);
   SvREFCNT_inc_simple_void_NN(value);
   return ptrs;
}

static
void undo_local_var(pTHX_ void* p)
{
   local_var_ptrs* ptrs=(local_var_ptrs*)p;
   SV* var=ptrs->var;
   SvANY(var)=ptrs->orig_any;
   SvFLAGS(ptrs->temp_owner)=SvFLAGS(var);
   ptrs->temp_owner->sv_u.svu_pv=var->sv_u.svu_pv;
   var->sv_u.svu_pv=ptrs->orig_pv;
   SvFLAGS(var)=ptrs->orig_flags;
   SvREFCNT_dec(var);
   SvREFCNT_dec(ptrs->temp_owner);
   Safefree(p);
}

typedef struct local_scalar_ptrs {
   SV* var;
   SV orig;
} local_scalar_ptrs;

static
local_scalar_ptrs* do_local_scalar(pTHX_ SV* var, SV* value, I32 tmp_refcnt)
{
   local_scalar_ptrs* ptrs;
   New(0, ptrs, 1, local_scalar_ptrs);
   ptrs->var=var;
   ptrs->orig.sv_any=var->sv_any;
   ptrs->orig.sv_refcnt=var->sv_refcnt-tmp_refcnt;
   ptrs->orig.sv_flags=var->sv_flags;
   ptrs->orig.sv_u.svu_pv=var->sv_u.svu_pv;
   var->sv_any=NULL;
   var->sv_flags=0;
   var->sv_refcnt=1+tmp_refcnt;
   sv_setsv(var,value);
   return ptrs;
}

static
void undo_local_scalar(pTHX_ void* p)
{
   local_scalar_ptrs* ptrs=(local_scalar_ptrs*)p;
   SV* var=ptrs->var;
   if (SvREFCNT(var)>1) {
      SvREFCNT_dec(var);
   } else {
      SvREFCNT(var)=0;  sv_clear(var);
   }
   var->sv_any=ptrs->orig.sv_any;
   var->sv_refcnt=ptrs->orig.sv_refcnt;
   var->sv_flags=ptrs->orig.sv_flags;
   var->sv_u.svu_pv=ptrs->orig.sv_u.svu_pv;
   Safefree(p);
}

void
pm_perl_localize_scalar(pTHX_ SV* var)
{
   save_destructor_x(&undo_local_scalar, do_local_scalar(aTHX_ var, sv_mortalcopy(var), 0));
}

void
pm_perl_localize_array(pTHX_ GV* ar_gv, SV* ar_ref)
{
   Perl_assert(SvTYPE(ar_gv)==SVt_PVGV && SvROK(ar_ref) && SvTYPE(SvRV(ar_ref))==SVt_PVAV);
   save_destructor_x(&undo_local_var, do_local_var((SV*)GvAV(ar_gv), SvRV(ar_ref)));
}

static
local_scalar_ptrs* do_local_ref(pTHX_ SV** varp, SV* value)
{
   local_scalar_ptrs* ptrs;
   New(0, ptrs, 1, local_scalar_ptrs);
   ptrs->var=*varp;
   ptrs->orig.sv_any=(SV*)varp;
   *varp=SvREFCNT_inc_simple_NN(value);
   return ptrs;
}

static
void undo_local_ref(pTHX_ void* p)
{
   local_scalar_ptrs* ptrs=(local_scalar_ptrs*)p;
   SV** varp=(SV**)ptrs->orig.sv_any;
   SvREFCNT_dec(*varp);
   *varp=ptrs->var;
   Safefree(p);
}

typedef struct local_incr_ptrs {
   SV* var;
   I32 incr;
} local_incr_ptrs; 

static
local_incr_ptrs* do_local_incr(pTHX_ SV* var, I32 incr)
{
   local_incr_ptrs* ptrs;
   New(0, ptrs, 1, local_incr_ptrs);
   ptrs->var=var;
   ptrs->incr=incr;
   if (SvIOK(var) || SvPOK(var))
      sv_setiv(var, SvIV(var)+incr);
   else if (SvNOK(var))
      sv_setnv(var, SvNVX(var)+incr);
   else
      sv_setiv(var,incr);
   return ptrs;
}

static
void undo_local_incr(pTHX_ void* p)
{
   local_incr_ptrs* ptrs=(local_incr_ptrs*)p;
   SV* var=ptrs->var;
   if (SvIOK(var))
      sv_setiv(var, SvIVX(var)-ptrs->incr);
   else if (SvNOK(ptrs->var))
      sv_setnv(var, SvNVX(var)-ptrs->incr);
   else
      Safefree(p), Perl_croak(aTHX_ "undoing local increment: variable is no more numerical");
   Safefree(p);
}

static
local_incr_ptrs* do_local_push(pTHX_ SV* av, SV** src, int n, int side)
{
   local_incr_ptrs* ptrs;
   SV **dst, **src_end;
   New(0, ptrs, 1, local_incr_ptrs);
   ptrs->var=av;
   ptrs->incr=side*n;
   av_extend((AV*)av, AvFILLp(av)+n);
   if (side<0) {
      dst=AvARRAY(av);
      Move(dst, dst+n, AvFILLp(av)+1, SV*);
   } else {
      dst=AvARRAY(av)+AvFILLp(av)+1;
   }
   for (src_end=src+n; src<src_end; ++src, ++dst) {
      SV* d=*src;
      if (SvREADONLY(d) || !SvTEMP(d))
         *dst=newSVsv(d);
      else
         *dst=SvREFCNT_inc_simple_NN(d);
   }
   AvFILLp(av)+=n;
   return ptrs;
}

static
void undo_local_push(pTHX_ void* p)
{
   local_incr_ptrs* ptrs=(local_incr_ptrs*)p;
   SV* av=ptrs->var;
   I32 n=ptrs->incr;
   SV **e, **eend;
   if (n>0) {
      for (e=AvARRAY(av)+AvFILLp(av), eend=e-n; e>eend; --e) {
         SvREFCNT_dec(*e);
         *e=&PL_sv_undef;
      }
      AvFILLp(av)-=n;
   } else {
      for (eend=AvARRAY(av)-1, e=eend-n; e>eend; --e)
         SvREFCNT_dec(*e);
      AvFILLp(av)+=n;
      ++eend;
      Move(eend-n, eend, AvFILLp(av)+1, SV*);
      for (e=eend+AvFILLp(av)+1, eend=e-n; e<eend; ++e)
         *e=&PL_sv_undef;
   }
   Safefree(p);
}

typedef struct local_pop_ptrs {
   AV* av;
   SV* val;
} local_pop_ptrs;

static
local_pop_ptrs* do_local_pop(pTHX_ AV* av)
{
   local_pop_ptrs* ptrs;
   if (AvFILLp(av)<0)
      Perl_croak(aTHX_ "local_pop on an empty array");
   SvREFCNT_inc_simple_void_NN(av);
   New(0, ptrs, 1, local_pop_ptrs);
   ptrs->av=av;
   ptrs->val=av_pop(av);
   return ptrs;
}

static
void undo_local_pop(pTHX_ void* p)
{
   local_pop_ptrs* ptrs=(local_pop_ptrs*)p;
   AV* av=ptrs->av;
   av_push(av,ptrs->val);
   SvREFCNT_dec(av);
   Safefree(p);
}

static
local_pop_ptrs* do_local_shift(pTHX_ AV* av)
{
   local_pop_ptrs* ptrs;
   if (AvFILLp(av)<0)
      Perl_croak(aTHX_ "local_shift on an empty array");
   SvREFCNT_inc_simple_void_NN(av);
   New(0, ptrs, 1, local_pop_ptrs);
   ptrs->av=av;
   ptrs->val=av_shift(av);
   return ptrs;
}

static
void undo_local_shift(pTHX_ void* p)
{
   local_pop_ptrs* ptrs=(local_pop_ptrs*)p;
   AV* av=ptrs->av;
   av_unshift(av,1);
   AvARRAY(av)[0]=ptrs->val;
   SvREFCNT_dec(av);
   Safefree(p);
}

static
local_incr_ptrs* do_local_shorten(pTHX_ AV* av, int n)
{
   local_incr_ptrs* ptrs;
   I32 store_incr;
   if (n>=0) {
      if (AvFILLp(av)<n)
         Perl_croak(aTHX_ "local_shorten: array has less than %d elements", n);
      store_incr=AvFILLp(av)-n;
   } else {
      if (AvFILLp(av)<-n)
         Perl_croak(aTHX_ "local_shorten: array has less than %d elements", -n);
      AvARRAY(av)-=n;
      store_incr=n;
      n+=AvFILLp(av);
   }
   SvREFCNT_inc_simple_void_NN(av);
   New(0, ptrs, 1, local_incr_ptrs);
   ptrs->var=(SV*)av;
   ptrs->incr=store_incr;
   AvFILLp(av)=n;
   return ptrs;
}

static
void undo_local_shorten(pTHX_ void* p)
{
   local_incr_ptrs* ptrs=(local_incr_ptrs*)p;
   AV* av=(AV*)ptrs->var;
   if (ptrs->incr>=0) {
      AvFILLp(av)+=ptrs->incr;
   } else {
      AvARRAY(av)+=ptrs->incr;
      AvFILLp(av)-=ptrs->incr;
   }
   SvREFCNT_dec(av);
   Safefree(p);
}

typedef struct local_swap_ptrs {
   AV* var;
   I32 ix1, ix2;
} local_swap_ptrs;

static
local_swap_ptrs* do_local_swap(pTHX_ SV* var, I32 ix1, I32 ix2)
{
   local_swap_ptrs* ptrs;
   SV* tmp;
   AV* av=(AV*)SvREFCNT_inc_NN(SvRV(var));
   if (ix1<0) ix1+=AvFILL(av)+1;
   if (ix2<0) ix2+=AvFILL(av)+1;
   if (ix1>AvFILL(av) || ix2>AvFILL(av)) Perl_croak(aTHX_ "local_swap: indices out of range");
   New(0, ptrs, 1, local_swap_ptrs);
   ptrs->var=av;
   ptrs->ix1=ix1;
   ptrs->ix2=ix2;
   tmp=AvARRAY(av)[ix1];
   AvARRAY(av)[ix1]=AvARRAY(av)[ix2];
   AvARRAY(av)[ix2]=tmp;
   return ptrs;
}

static
void undo_local_swap(pTHX_ void* p)
{
   local_swap_ptrs* ptrs=(local_swap_ptrs*)p;
   AV* av=ptrs->var;
   SV* tmp=AvARRAY(av)[ptrs->ix1];
   AvARRAY(av)[ptrs->ix1]=AvARRAY(av)[ptrs->ix2];
   AvARRAY(av)[ptrs->ix2]=tmp;
   SvREFCNT_dec(av);
   Safefree(p);
}

typedef struct local_bless_ptrs {
   SV* var;
   HV* stash;
   I32 mg_flags;
} local_bless_ptrs;

static
local_bless_ptrs* do_local_bless(pTHX_ SV* ref, SV* pkg)
{
   local_bless_ptrs* ptrs;
   SV* var;
   HV* stash;
   if (!SvROK(ref) || (var=SvRV(ref), !SvOBJECT(var)))
      Perl_croak(aTHX_ "usage: local_bless(\\object, \"pkg\" || \\%%stash");
   if (SvPOK(pkg)) {
      if (!(stash=gv_stashsv(pkg, FALSE)))
        Perl_croak(aTHX_ "unknown package %.*s", (int)SvCUR(pkg), SvPVX(pkg));
   } else if (!SvROK(pkg) || (stash=(HV*)SvRV(pkg), SvTYPE(stash)!=SVt_PVHV)) {
      Perl_croak(aTHX_ "usage: local_bless(\\object, \"pkg\" || \\%%stash)");
   }
   New(0, ptrs, 1, local_bless_ptrs);
   ptrs->var=SvREFCNT_inc_simple_NN(var);
   ptrs->stash=(HV*)SvREFCNT_inc(SvSTASH(var));
   ptrs->mg_flags=SvFLAGS(var) & (SVs_GMG|SVs_SMG|SVs_RMG|SVf_AMAGIC);
   sv_bless(ref, stash);
   return ptrs;
}

static
void undo_local_bless(pTHX_ void* p)
{
   local_bless_ptrs* ptrs=(local_bless_ptrs*)p;
   SV* var=ptrs->var;
   HV* tmp_stash=SvSTASH(var);
   SvSTASH_set(var, ptrs->stash);
   SvFLAGS(var) &= ~(SVs_GMG|SVs_SMG|SVs_RMG|SVf_AMAGIC);
   SvFLAGS(var) |= ptrs->mg_flags;
   SvREFCNT_dec(var);
   SvREFCNT_dec(tmp_stash);
   Safefree(p);
}

MODULE = Polymake::Scope                PACKAGE = Polymake::Scope

void
begin_locals(scope)
   SV *scope;
CODE:
LEAVE;
{
   save_destructor_x(&localize_marker, SvRV(scope));
}
ENTER;

void
end_locals(scope_ref)
   SV *scope_ref;
CODE:
LEAVE;
{
   ANY* mainstack=PL_savestack;
   I32 frame_top=PL_savestack_ix, frame_bottom=PL_scopestack[PL_scopestack_ix-1], f, to_save;
   AV* scope=(AV*)SvRV(scope_ref);

   /* find the marker or resign */
   for (f=frame_top-3; f >= frame_bottom; --f)
      if (mainstack[f].any_ptr == (void*)&localize_marker
          && f+2 < frame_top
          && mainstack[f+2].any_i32 == SAVEt_DESTRUCTOR_X) {

         if (scope != (AV*)mainstack[f+1].any_ptr) break;

         to_save=frame_top-(f+3);
         if (to_save > 0) {
            SV* marker=AvARRAY(scope)[Scope_local_marker_index];
            sv_catpvn(marker, (char*)&(mainstack[f+3]), to_save*sizeof(ANY));
            PL_savestack_ix=f;  /* pop the marker and the saved locals quickly */
         }
         scope=Nullav;
         break;
      }
   if (scope != Nullav) Perl_croak(aTHX_ "Scope: begin-end mismatch");
}
ENTER;

void
unwind(marker)
   SV *marker;
CODE:
{
   I32 saved=SvCUR(marker)/sizeof(ANY);
   if (saved) {
      SSGROW(saved);
      Copy(SvPVX(marker), &(PL_savestack[PL_savestack_ix]), saved, ANY);
      PL_savestack_ix+=saved;
   }
   /* LEAVE in pp_entersub will execute all actions */
}

MODULE = Polymake::Scope                PACKAGE = Polymake

void
local_scalar(var, value)
   SV *var;
   SV *value;
PROTOTYPE: $$
CODE:
{
   if ( (isGV(var) ? !(var=GvSV(var))
                   : SvTYPE(var) >= SVt_PVAV) ||
        SvTYPE(value) >= SVt_PVAV )
      croak_xs_usage(cv, "*glob || $var, value");
   LEAVE;
   save_destructor_x(&undo_local_scalar, do_local_scalar(aTHX_ var, value, 0));
   ENTER;
}

void
local_save_scalar(var)
   SV *var;
PROTOTYPE: $
PPCODE:
{
   if ( isGV(var) ? !(var=GvSV(var))
                  : SvTYPE(var) >= SVt_PVAV )
      croak_xs_usage(cv, "*glob || $var");
   LEAVE;
   save_destructor_x(&undo_local_scalar, do_local_scalar(aTHX_ var, sv_mortalcopy(var), 0));
   ENTER;
}

void
local_array(var, value)
   SV *var;
   SV *value;
PROTOTYPE: $$
PPCODE:
{
   if (isGV(var) ? !(var=(SV*)GvAV(var))
                 : !SvROK(var) || (var=SvRV(var), SvTYPE(var) != SVt_PVAV))
      croak_xs_usage(cv, "*glob || \\@array, \\@array");
   if (!SvROK(value) || (value=SvRV(value), SvTYPE(value) != SVt_PVAV))
      croak_xs_usage(cv, "*glob || \\@array, \\@array");
   LEAVE;
   save_destructor_x(&undo_local_var, do_local_var(var, value));
   ENTER;
}

void
local_hash(var, value)
   SV *var;
   SV *value;
PROTOTYPE: $$
PPCODE:
{
   if (isGV(var) ? !(var=(SV*)GvHV(var))
                 : !SvROK(var) || (var=SvRV(var), SvTYPE(var) != SVt_PVHV))
      croak_xs_usage(cv, "*glob || \\%%hash, \\%%hash");
   if (!SvROK(value) || (value=SvRV(value), SvTYPE(value) != SVt_PVHV))
      croak_xs_usage(cv, "*glob || \\%%hash, \\%%hash");
   LEAVE;
   save_destructor_x(&undo_local_var, do_local_var(var, value));
   ENTER;
}

void
local_sub(var, value)
   SV *var;
   SV *value;
PROTOTYPE: $$
PPCODE:
{
   if (isGV(var) ? !(var=(SV*)GvCV(var))
                 : !SvROK(var) || (var=SvRV(var), SvTYPE(var) != SVt_PVCV))
      croak_xs_usage(cv, "*glob || \\&sub, \\&sub");
   if (!SvROK(value) || (value=SvRV(value), SvTYPE(value) != SVt_PVCV))
      croak_xs_usage(cv, "*glob || \\&sub, \\&sub");
   LEAVE;
   save_destructor_x(&undo_local_var, do_local_var(var, value));
   ENTER;
}

void
local_refs(...)
PPCODE:
{
   I32 i, tmp_refcnt=0;
   if (items%2) Perl_croak(aTHX_ "local_refs: odd argument list");
   LEAVE;
   for (i=0; i<items; i+=2) {
      SV *var=ST(i), *value=ST(i+1);
      if (SvROK(var)) {
         GV *glob=(GV*)SvRV(var);
         if (SvTYPE(glob)==SVt_PVGV) {
            if (SvROK(value)) {
               value=SvRV(value);
               switch (SvTYPE(value)) {
               case SVt_PVAV:
                  save_destructor_x(&undo_local_ref, do_local_ref(aTHX_ (SV**)&(GvGP(glob)->gp_av), value));
                  break;
               case SVt_PVHV:
                  save_destructor_x(&undo_local_ref, do_local_ref(aTHX_ (SV**)&(GvGP(glob)->gp_hv), value));
                  break;
               case SVt_PVCV:
                  save_destructor_x(&undo_local_ref, do_local_ref(aTHX_ (SV**)&(GvGP(glob)->gp_cv), value));
                  break;
               default:
                  ENTER;
                  Perl_croak(aTHX_ "local_refs: only array, hash, or code references allowed");
               }
               continue;
            }
            var=GvSV(glob);
         } else if (SvTEMP(var)) {
            var=(SV*)glob;
            tmp_refcnt=1;
         }
      } else if (SvTEMP(var)) {
         ENTER;
         Perl_croak(aTHX_ "local_refs: temporary target");
      }
      save_destructor_x(&undo_local_scalar, do_local_scalar(aTHX_ var, value, tmp_refcnt));
   }
   ENTER;
}

void
local_incr(var, ...)
   SV* var;
PROTOTYPE: $;$
PPCODE:
{
   SV* incr= items==2 ? ST(1) : NULL;
   if ( items>2 ||
        (isGV(var) ? !(var=GvSV(var))
                   : SvTYPE(var) >= SVt_PVAV) ||
        (incr && SvTYPE(incr) >= SVt_PVAV) )
      croak_xs_usage(cv, "*glob || $var [, incr]");
   LEAVE;
   save_destructor_x(&undo_local_incr, do_local_incr(aTHX_ var, incr ? SvIV(incr) : 1));
   ENTER;
}

void
local_push(avref, ...)
   SV* avref;
PROTOTYPE: $@
PPCODE:
{
   SV* av=NULL;
   if (isGV(avref)
       ? (av=(SV*)GvAV(avref)) == NULL
       : !SvROK(avref) || (av=SvRV(avref), SvTYPE(av) != SVt_PVAV || SvGMAGICAL(av)))
      croak_xs_usage(cv, "*glob || \\@array, data ...");
   if (items>=2) {
      LEAVE;
      save_destructor_x(&undo_local_push, do_local_push(aTHX_ av, &ST(1), items-1, 1));
      ENTER;
   }
}

void
local_unshift(avref, ...)
   SV *avref;
PROTOTYPE: $@
PPCODE:
{
   SV* av=NULL;
   if (isGV(avref)
       ? (av=(SV*)GvAV(avref)) == NULL
       : !SvROK(avref) || (av=SvRV(avref), SvTYPE(av) != SVt_PVAV || SvGMAGICAL(av)))
      croak_xs_usage(cv, "*glob || \\@array, data ...");
   if (items>=2) {
      LEAVE;
      save_destructor_x(&undo_local_push, do_local_push(aTHX_ av, &ST(1), items-1, -1));
      ENTER;
   }
}

void
local_pop(avref)
   SV* avref;
PROTOTYPE: $
PPCODE:
{
   AV* av=NULL;
   SV* ret=NULL;
   if (isGV(avref)
       ? (av=GvAV(avref)) == NULL
       : !SvROK(avref) || (av=(AV*)SvRV(avref), SvTYPE(av) != SVt_PVAV || SvGMAGICAL(av)))
      croak_xs_usage(cv, "*glob || \\@array");
   if (GIMME_V != G_VOID && AvFILLp(av)>=0) {
      ret=AvARRAY(av)[AvFILLp(av)];
   }
   LEAVE;
   save_destructor_x(&undo_local_pop, do_local_pop(aTHX_ av));
   ENTER;
   if (ret) PUSHs(sv_mortalcopy(ret));
}

void
local_shift(avref)
   SV* avref;
PROTOTYPE: $
PPCODE:
{
   AV* av=NULL;
   SV* ret=NULL;
   if (isGV(avref)
       ? (av=GvAV(avref)) == NULL
       : !SvROK(avref) || (av=(AV*)SvRV(avref), SvTYPE(av) != SVt_PVAV || SvGMAGICAL(av)))
      croak_xs_usage(cv, "*glob || \\@array");
   if (GIMME_V != G_VOID && AvFILLp(av)>=0) {
      ret=AvARRAY(av)[0];
   }
   LEAVE;
   save_destructor_x(&undo_local_shift, do_local_shift(aTHX_ av));
   ENTER;
   if (ret) PUSHs(sv_mortalcopy(ret));
}

void
local_shorten(avref, n)
   SV* avref;
   I32 n;
PROTOTYPE: $$$
PPCODE:
{
   AV* av=NULL;
   if (isGV(avref)
       ? (av=GvAV(avref)) == NULL
       : !SvROK(avref) || (av=(AV*)SvRV(avref), SvTYPE(av) != SVt_PVAV || SvGMAGICAL(av)))
      croak_xs_usage(cv, "*glob || \\@array, last_elem");
   LEAVE;
   save_destructor_x(&undo_local_shorten, do_local_shorten(aTHX_ av, n));
   ENTER;
}

void
local_swap(avref, ix1, ix2)
   SV *avref;
   I32 ix1;
   I32 ix2;
PROTOTYPE: $$$
PPCODE:
{
   LEAVE;
   save_destructor_x(&undo_local_swap, do_local_swap(aTHX_ avref, ix1, ix2));
   ENTER;
}

void
local_bless(ref, pkg)
   SV *ref;
   SV *pkg;
PROTOTYPE: $$
PPCODE:
{
   LEAVE;
   save_destructor_x(&undo_local_bless, do_local_bless(aTHX_ ref, pkg));
   ENTER;
}

void
propagate_match()
PPCODE:
{
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;
   while (cx >= cx_bottom) {
      if (CxTYPE(cx)==CXt_SUB) {
         cx->blk_oldpm=PL_curpm;
         if (!SkipDebugFrame(cx,0)) break;
      }
      --cx;
   }
}

void
caller_object(pkg, ...)
   SV* pkg;
PPCODE:
{
   AV* args;
   int descend_to_method=TRUE;
   PERL_CONTEXT* cx_bottom;
   PERL_CONTEXT* cx;
   PERL_UNUSED_ARG(pkg);
   for (cx_bottom=cxstack, cx=cx_bottom+cxstack_ix; cx>=cx_bottom; --cx) {
      if (CxTYPE(cx)==CXt_SUB) {
         CV *cv=cx->blk_sub.cv;
         if (descend_to_method) {
            /* there are no methods in DB:: */
            if (!CvMETHOD(cv)) continue;
            if (pm_perl_skip_debug_cx) {
               /* the real argument list is stored in the block corresponding to DB::sub, not here */
               descend_to_method=FALSE;
               continue;
            } else if (!CxHASARGS(cx)) continue;
         } else if (SkipDebugSub(cv)) {
            if (!CxHASARGS(cx)) {
               descend_to_method=TRUE; continue;
            }
         } else continue;

         args=cx->blk_sub.argarray;
         if (AvFILLp(args)>=0 || AvALLOC(args)<AvARRAY(args)) {
            SV *obj=*AvALLOC(args);     // the first arg may be shifted
            if ((SvROK(obj) && SvOBJECT(SvRV(obj))) || SvPOK(obj)) {
               int i;
               for (i=0; i<items; ++i)
                  if (sv_derived_from(obj, SvPVX(ST(i)))) {
                     PUSHs(sv_mortalcopy(obj));
                     if (GIMME_V==G_ARRAY) mXPUSHi(i);
                     break;
                  }
            }
         }
         break;
      }
   }
}


BOOT:
{
   Scope_local_marker_index=CvDEPTH(get_cv("Polymake::Scope::local_marker", FALSE));
   if (PL_DBgv) {
      CvNODEBUG_on(get_cv("Polymake::Scope::begin_locals", FALSE));
      CvNODEBUG_on(get_cv("Polymake::Scope::end_locals", FALSE));
      CvNODEBUG_on(get_cv("Polymake::Scope::unwind", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_scalar", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_save_scalar", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_array", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_hash", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_sub", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_refs", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_incr", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_push", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_unshift", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_pop", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_shift", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_shorten", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_swap", FALSE));
      CvNODEBUG_on(get_cv("Polymake::local_bless", FALSE));
      CvNODEBUG_on(get_cv("Polymake::propagate_match", FALSE));
      CvNODEBUG_on(get_cv("Polymake::caller_object", FALSE));
   }
}

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
