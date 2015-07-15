/* Copyright (c) 1997-2015
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

static
void unimport_function(pTHX_ SV *gv)
{
   CV* cv=GvCV(gv);
   if (cv != NULL) {
      SvREFCNT_dec(cv);
      GvCV_set(gv, Nullcv);
   }
   GvIMPORTED_CV_off(gv);
   GvASSUMECV_off(gv);
}

static
GV* do_can(pTHX_ SV *obj, SV *method)
{
   HV* stash=NULL;
   char* method_name=SvPVX(method);
   I32 method_name_len=SvCUR(method);

   if (SvGMAGICAL(obj)) mg_get(obj);

   if (SvROK(obj)) {
      obj = SvRV(obj);
      if (SvOBJECT(obj)) {
         stash = SvSTASH(obj);
      }
   } else if (SvPOKp(obj) && SvCUR(obj)) {
      stash = gv_stashsv(obj, 0);
   }

   return stash ? gv_fetchmeth(stash, method_name, method_name_len, 0) : Nullgv;
}

int pm_perl_canned_dup(pTHX_ MAGIC* mg, CLONE_PARAMS* param)
{
   return 0;
}

OP* pm_perl_select_method_helper_op(pTHX)
{
   PL_op->op_ppaddr=PL_ppaddr[OP_ENTERSUB];
   ++PL_stack_sp;
   return (PL_ppaddr[OP_ENTERSUB])(aTHX);
}

static
OP* pp_first(pTHX)
{
   dSP; dMARK;
   if (MARK<SP)
      SP=MARK+1;
   else
      XPUSHs(&PL_sv_undef);
   RETURN;
}

#if PerlVersion >= 5160
static
OP* safe_magic_lvalue_return_op(pTHX)
{
   if (cxstack[cxstack_ix].blk_gimme==G_SCALAR) {
      dSP;
      OP* next_op;
      SV* retval=TOPs;
      U32 retval_flags= SvTEMP(retval) && SvREFCNT(retval)==1 ? SvMAGICAL(retval) : 0;
      if (retval_flags) {
         SvMAGICAL_off(retval);
         next_op=Perl_pp_leavesub(aTHX);
         SvFLAGS(retval) |= retval_flags;
         return next_op;
      }
   }
   return Perl_pp_leavesub(aTHX);
}
#endif

static const MGVTBL array_flags_vtbl={ 0, 0, 0, 0, 0 };

MAGIC* pm_perl_array_flags_magic(pTHX_ SV* sv)
{
   return mg_findext(sv, PERL_MAGIC_ext, &array_flags_vtbl);
}

static inline
GV* retrieve_gv(pTHX_ OP* o, OP* const_op, SV** const_sv, PERL_CONTEXT* cx, PERL_CONTEXT* cx_bottom)
{
   GV* gv;
#ifdef USE_ITHREADS
   SV** saved_curpad=PL_curpad;
   PL_curpad=pm_perl_get_cx_curpad(aTHX_ cx, cx_bottom);
#endif
#if PerlVersion >= 5220
   if (o->op_type == OP_MULTIDEREF) {
      UNOP_AUX_item* items=cUNOP_AUXo->op_aux;
      gv=(GV*)UNOP_AUX_item_sv(++items);
      if (const_sv) *const_sv=UNOP_AUX_item_sv(++items);
   } else
#endif
   {
      gv=cGVOPo_gv;
      if (const_sv) *const_sv=cSVOPx_sv(const_op);
   }
#ifdef USE_ITHREADS
   PL_curpad=saved_curpad;
#endif
   return gv;
}

static
SV* compose_varname(pTHX_ OP* o, OP* const_op, SV** const_sv, const char prefix, PERL_CONTEXT* cx, PERL_CONTEXT* cx_bottom)
{
   GV* gv=retrieve_gv(aTHX_ o, const_op, const_sv, cx, cx_bottom);
   SV* varname=newSVpvf("%c%s::%.*s", prefix, HvNAME(GvSTASH(gv)), (int)GvNAMELEN(gv), GvNAME(gv));
   return sv_2mortal(varname);
}

SV* pm_perl_name_of_ret_var(pTHX)
{
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;
   while (cx >= cx_bottom) {
      if (CxTYPE(cx)==CXt_SUB && !SkipDebugFrame(cx,0)) {
         OP *o;
         if (cx->blk_gimme != G_SCALAR) break;
         o=cx->blk_sub.retop;
         if (o==NULL) break;
         while (o->op_type == OP_LEAVE) o=o->op_next;
         if (o->op_type != OP_LEAVESUB && o->op_type != OP_LEAVESUBLV) {
            if (o->op_type == OP_GVSV  &&  o->op_next->op_type == OP_SASSIGN) {
               GV *gv=retrieve_gv(aTHX_ o, 0, 0, cx, cx_bottom);
               return sv_2mortal(newSVpvn(GvNAME(gv),GvNAMELEN(gv)));
            }
            break;
         }
      }
      --cx;
   }
   return NULL;
}

static
OP* convert_eval_to_sub(pTHX)
{
   CV *cv=cxstack[cxstack_ix].blk_sub.cv;
   OP *me=PL_op, *start=me->op_next, *root=CvROOT(cv);
   root->op_type=OP_LEAVESUB;
   root->op_ppaddr=PL_ppaddr[OP_LEAVESUB];
   CvSTART(cv)=start;
   return start;
}

MODULE = Polymake                       PACKAGE = Polymake

I32
refcnt(x)
   SV *x;
PROTOTYPE: $
CODE:
{
   if (SvROK(x)) x=SvRV(x);
   RETVAL=SvREFCNT(x);
}
OUTPUT:
   RETVAL

void
refcmp(x,y,...)
   SV *x;
   SV *y;
PPCODE:
{
   if (SvRV(x)==SvRV(y)) XSRETURN_YES;
   XSRETURN_NO;
}

void
weak(ref)
   SV *ref;
PROTOTYPE: $
PPCODE:
{
   sv_rvweaken(ref);
}

void
is_weak(ref)
   SV *ref;
PROTOTYPE: $
CODE:
{
   if (SvWEAKREF(ref)) XSRETURN_YES;
   XSRETURN_NO;
}

void
readonly(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   write_protect_on(aTHX_ x);
   ++SP;
}

void
readonly_deep(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x)) {
      MAGIC *mg;
      x=SvRV(x);
      write_protect_on(aTHX_ x);
      if (SvMAGICAL(x) && (mg=pm_perl_get_cpp_magic(x))) {
         mg->mg_flags |= value_read_only;
      } else if (SvTYPE(x) == SVt_PVAV) {
         I32 l=av_len((AV*)x);
         if (l>=0) {
            SV **elem=AvARRAY(x), **last=elem+l;
            for (; elem<=last; ++elem)
               if (*elem) write_protect_on(aTHX_ *elem);
         }
      }
   } else {
      write_protect_on(aTHX_ x);
   }
   ++SP;
}

void
readwrite(x)
   SV* x;
PROTOTYPE: $
PPCODE:
{
   write_protect_off(aTHX_ x);
   ++SP;
}

void
is_readonly(x)
   SV* x;
PROTOTYPE: $
PPCODE:
{
   if (SvREADONLY(x))
      PUSHs(&PL_sv_yes);
   else
      PUSHs(&PL_sv_no);
}

void
is_lvalue(subref)
   SV *subref;
PROTOTYPE: $
PPCODE:
{
   CV *sub;
   if (!SvROK(subref) || (sub=(CV*)SvRV(subref), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   if (GIMME_V!=G_ARRAY) {
      if (CvFLAGS(sub) & CVf_LVALUE)
         PUSHs(&PL_sv_yes);
      else
         PUSHs(&PL_sv_no);
   } else if (CvFLAGS(sub) & CVf_LVALUE) {
      if (!CvISXSUB(sub) && CvROOT(sub)->op_type==OP_LEAVESUBLV)
         PUSHs(&PL_sv_no);      /* not faked */
      else
         PUSHs(&PL_sv_yes);
   }
}


void
declare_lvalue(subref,...)
   SV *subref;
PROTOTYPE: $
CODE:
{
   CV *sub;
   if (!SvROK(subref) || (sub=(CV*)SvRV(subref), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub [, TRUE_if_faked ]");
   CvFLAGS(sub) |= CVf_LVALUE | CVf_NODEBUG;
   if (!CvISXSUB(sub)) {
      OP *leave_op=CvROOT(sub);
      if (items==1 || !SvTRUE(ST(1))) {
         /* not faked */
         leave_op->op_type=OP_LEAVESUBLV;
         leave_op->op_ppaddr=PL_ppaddr[OP_LEAVESUBLV];
      }
#if PerlVersion >= 5160
      else {
         /* nowadays perl is fond of copying return values if they show any magic */ 
         leave_op->op_ppaddr=&safe_magic_lvalue_return_op;
      }
#endif
   }
}

void
declare_nodebug(subref,...)
   SV *subref;
PROTOTYPE: $
CODE:
{
   CV *sub;
   if (!SvROK(subref) || (sub=(CV*)SvRV(subref), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   CvFLAGS(sub) |= CVf_NODEBUG;
}

void
is_method(sub)
   SV *sub;
PROTOTYPE: $
PPCODE:
{
   if (!SvROK(sub)) {
      if (SvPOKp(sub)) XSRETURN_YES;    /* probably the method name */
   } else {
      sub=SvRV(sub);
      if (SvTYPE(sub) != SVt_PVCV)
         croak_xs_usage(cv, "\\&sub");
      if (CvMETHOD(sub)) XSRETURN_YES;
   }
}
XSRETURN_NO;

void
select_method(sub, ...)
   SV *sub;
PPCODE:
{
   int push=0, i;
   SV** stack;
   SV** bottom;
   if (SvROK(sub)) {
      sub=SvRV(sub);
      if (SvTYPE(sub) != SVt_PVCV)
         croak_xs_usage(cv, "\"method_name\" || \\&sub, Object, ...");
      if (CvMETHOD(sub)) {
         if (items==3 && SvIOK(ST(2)) && SvIVX(ST(2))==1) {
            push=1; goto push_obj;
         } else {
            HV *method_stash=GvSTASH(CvGV(sub));
            for (i=1; i<items; ++i) {
               SV *obj_ref=ST(i);
               if (SvSTASH(SvRV(obj_ref))==method_stash || sv_derived_from(obj_ref,HvNAME(method_stash))) {
                  push=i; goto push_obj;
               }
            }
         }
         Perl_croak(aTHX_ "no suitable object found");
      } else {
         goto ready;
      }
   } else if (SvPOKp(sub)) {
      for (i=1; i<items; ++i) {
         GV *method_gv=do_can(aTHX_ ST(i), sub);
         if (method_gv) {
            SV *cache_here=sub;
            sub=(SV*)GvCV(method_gv);
            if (sub) {
               if (!(SvFLAGS(cache_here) & (SVs_TEMP | SVf_FAKE | SVf_READONLY))) {
                  sv_setsv(cache_here, sv_2mortal(newRV(sub)));
               }
               if (CvMETHOD(sub)) {
                  push=i; goto push_obj;
               } else {
                  goto ready;
               }
            }
         }
      }
      Perl_croak(aTHX_ "method not found");
   } else {
      croak_xs_usage(cv, "\"method_name\" || \\&sub, Object, ...");
   }
 push_obj:
   for (stack=++SP, bottom=PL_stack_base+TOPMARK+1; stack>bottom; --stack)
      *stack=stack[-1];
   *stack=ST(push);
 ready:
   if (PL_op->op_next->op_type==OP_ENTERSUB) {
      PUSHs(sub);
      if (GIMME_V==G_SCALAR) {
         PL_op->op_flags ^= OPf_WANT_SCALAR^OPf_WANT_LIST;
         if (push) {
            --SP;
            PL_op->op_next->op_ppaddr=&pm_perl_select_method_helper_op;
         }
      }
   } else {
      PUSHs(sv_2mortal(newRV(sub)));
   }
}

void
set_prototype(sub, proto)
   SV *sub;
   SV *proto;
PROTOTYPE: $$
CODE:
{
   STRLEN l;
   char *p=SvPV(proto, l);
   if (!SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub, \"proto\"");
   sv_setpvn(sub, p, l);
}

void
is_unary(sub)
   SV *sub;
PROTOTYPE: $
PPCODE:
{
   if (!SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   if (SvPOK(sub)) {
      if (SvCUR(sub)==1 && *SvPVX(sub)=='$')
         XSRETURN_YES;
      else
         XSRETURN_NO;
   }
   XSRETURN_UNDEF;
}

void
is_string(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if ((SvFLAGS(x) & (SVf_IOK|SVf_NOK|SVf_POK|SVf_ROK|SVs_GMG|SVs_RMG)) == SVf_POK) XSRETURN_YES;
   XSRETURN_NO;
}

void
is_utf8string(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvUTF8(x)) XSRETURN_YES;
   XSRETURN_NO;
}

void
mark_as_utf8string(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   SvUTF8_on(x);
   ++SP;
}

void
downgradeUTF8(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   ++SP;
   SvGETMAGIC(x);
   if (SvUTF8(x)) {
      if ((SvFLAGS(x) & (SVs_TEMP|SVf_READONLY)) != SVs_TEMP) {
         x=sv_mortalcopy(x);
         SETs(x);
      }
      sv_utf8_downgrade(x,FALSE);
   }
}

void
is_integer(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvIOK(x)) XSRETURN_YES;
   XSRETURN_NO;
}

void
is_float(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvNOK(x)) XSRETURN_YES;
   XSRETURN_NO;
}

void
is_numeric(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if ((!SvPOK(x) || SvCUR(x)>0) && (SvIOK(x) | SvNOK(x))) XSRETURN_YES;
   XSRETURN_NO;
}

void
is_boolean(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
  if (x==&PL_sv_yes || x==&PL_sv_no ||
      (SvIOK(x) && (SvIVX(x)==1 || SvIVX(x)==0))) XSRETURN_YES;
  XSRETURN_NO;
}

void
true()
PROTOTYPE:
PPCODE:
   XSRETURN_YES;

void
false()
PROTOTYPE:
PPCODE:
   XSRETURN_NO;

void
extract_integer(str)
   SV* str;
PROTOTYPE: $
PPCODE:
{
   dTARGET;
   STRLEN l;
   char* start=SvPV(str, l);
   char* end=NULL;
   long val=strtol(start, &end, 10);
   for (; end < start+l; ++end)
      if (!isSPACE(*end))
         Perl_croak(aTHX_ "parse error: invalid integer value %.*s", (int)l, start);
   PUSHi(val);
}

void
extract_float(str)
   SV* str;
PROTOTYPE: $
PPCODE:
{
   dTARGET;
   STRLEN l;
   char* start=SvPV(str, l);
#ifdef my_atof2
   NV val=0;
   char* end=my_atof2(start, &val);
#else
   char* end=NULL;
   NV val=strtod(start, &end);
#endif
   for (; end < start+l; ++end)
      if (!isSPACE(*end))
         Perl_croak(aTHX_ "parse error: invalid floating-point value %.*s", (int)l, start);
   PUSHn(val);
}

void
is_object(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x) && SvOBJECT(SvRV(x)))
      XSRETURN_YES;
   XSRETURN_NO;
}

void
inherit_class(obj,src)
   SV *obj;
   SV *src;
PPCODE:
{
   HV *stash;
   if (SvROK(src)) {
      src=SvRV(src);
      if (SvOBJECT(src))
         stash=SvSTASH(src);
      else
         goto DONE;
   } else {
      STRLEN l;
      const char *p=SvPV(src,l);
      if (!(stash=gv_stashpvn(p,l,FALSE)))
         Perl_croak(aTHX_ "unknown package %.*s",(int)l,p);
   }
   sv_bless(obj,stash);
 DONE:
   ++SP;        /* let obj appear at the stack top again */
}

void
symtable_of(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x) && (x=SvRV(x), SvOBJECT(x)))
      PUSHs(sv_2mortal(newRV((SV*)SvSTASH(x))));
   else
      PUSHs(&PL_sv_undef);
}

void
pkg_name(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x) && (x=SvRV(x), SvTYPE(x)==SVt_PVHV))
      PUSHs(sv_2mortal(newSVpv(HvNAME((HV*)x), 0)));
   else
      PUSHs(&PL_sv_undef);
}

void
get_pkg(pkg_name, ...)
   SV *pkg_name;
   PROTOTYPE: $;$
PPCODE:
{
   HV *stash=gv_stashsv(pkg_name, items==2 && SvTRUE(ST(1)));
   if (GIMME_V != G_VOID) {
      if (stash)
         PUSHs(sv_2mortal(newRV((SV*)stash)));
      else
         PUSHs(&PL_sv_undef);
   }
}

void
is_ARRAY(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x) && SvTYPE(SvRV(x)) == SVt_PVAV) XSRETURN_YES;
   XSRETURN_NO;
}

void
is_array(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x)) {
      SV *obj=SvRV(x);
      if (SvOBJECT(obj)) {
         if (SvAMAGIC(x) && gv_fetchmeth(SvSTASH(obj),"(@{}",4,0)) XSRETURN_YES;
         if (SvGMAGICAL(obj)) {
            if (mg_find(obj, PERL_MAGIC_tied)) XSRETURN_YES;
         } else {
            if (!gv_fetchmeth(SvSTASH(obj),".constructor",12,0)) XSRETURN_YES;
         }
      } else {
         if (SvTYPE(obj)==SVt_PVAV) XSRETURN_YES;
      }
   }
   XSRETURN_NO;
}

void
is_hash(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x) && SvTYPE(SvRV(x)) == SVt_PVHV) XSRETURN_YES;
   XSRETURN_NO;
}

void
is_code(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x) && SvTYPE(SvRV(x))== SVt_PVCV) XSRETURN_YES;
   XSRETURN_NO;
}

void
is_real_code(x)
   SV *x;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(x) && (x=SvRV(x), SvTYPE(x) == SVt_PVCV) && IsWellDefinedSub((CV*)x))
      return;   /* keep the CV reference on the stack */
   XSRETURN_NO;
}

void
defined_scalar(gv)
   SV* gv;
PROTOTYPE: $
PPCODE:
{
   SV* sv;
   if (SvTYPE(gv)==SVt_PVGV && (sv=GvSV(gv)) && SvOK(sv))
      XSRETURN_YES;
   else
      XSRETURN_NO;
}

void
declared_scalar(gv)
   SV* gv;
PROTOTYPE: $
PPCODE:
{
   if (SvTYPE(gv)==SVt_PVGV && GvIMPORTED_SV(gv))
      XSRETURN_YES;
   else
      XSRETURN_NO;
}

void
push_scalar(avref, sv)
   SV* avref;
   SV* sv;
PPCODE:
{
   AV* av;
   if (!SvROK(avref) || (av=(AV*)SvRV(avref), SvTYPE(av) != SVt_PVAV))
      croak_xs_usage(cv, "\\@array, scalar");
   av_push(av, SvREFCNT_inc_simple_NN(sv));
}

void
unimport_function(...)
CODE:
{
   SV* gv=ST(0);
   if (items==1) {
      unimport_function(aTHX_ gv);
   } else {
      int i=0;
      HV* stash= SvROK(gv) ? (HV*)(++i, SvRV(gv)) : CopSTASH(PL_curcop);
      for (; i<items; ++i) {
         STRLEN l;
         const char* n=SvPV(ST(i),l);
         SV** gvp=hv_fetch(stash, n, l, FALSE);
         if (gvp != NULL)
            unimport_function(aTHX_ *gvp);
         else
            Perl_croak(aTHX_ "unknown function %s::%.*s\n", HvNAME(stash),(int)l,n);
      }
   }
}

void
forget_function(ref)
   SV* ref;
PROTOTYPE: $
CODE:
{
   CV* sub=NULL;
   GV* glob=NULL;
   if (SvROK(ref)) {
      sub=(CV*)SvRV(ref);
      if (SvTYPE(sub) == SVt_PVCV)
         glob=CvGV(sub);
      else
         croak_xs_usage(cv, "\\&sub || *glob");
   } else if (SvTYPE(ref) == SVt_PVGV) {
      if ((sub=GvCV((GV*)ref)))
         glob=(GV*)ref;
      else
         XSRETURN_EMPTY;
   }
   SvREFCNT_dec(sub);
   GvCV_set(glob, Nullcv);
   GvIMPORTED_CV_off(glob);
   GvASSUMECV_off(glob);
}

void
method_name(sub)
   SV *sub;
PROTOTYPE: $
PPCODE:
{
   dTARGET;
   GV* subgv;
   if (!SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   subgv=CvGV(sub);
   sv_setpvn(TARG, GvNAME(subgv), GvNAMELEN(subgv));
   PUSHs(TARG);
}

void
sub_pkg(sub)
   SV *sub;
PROTOTYPE: $
PPCODE:
{
   if (SvROK(sub)) {
      dTARGET;
      sub=SvRV(sub);
      if (SvTYPE(sub) != SVt_PVCV)
         croak_xs_usage(cv, "\\&sub");
      sv_setpv(TARG, HvNAME(CvSTASH(sub)));
      PUSHs(TARG);
   } else {
      PUSHs(&PL_sv_undef);
   }
}

void
sub_file(sub)
   SV *sub;
PROTOTYPE: $
PPCODE:
{
   if (!SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   if (CvSTART(sub)) {
      dTARGET;
      sv_setpv(TARG, CopFILE((COP*)CvSTART(sub)));
      PUSHs(TARG);
   } else {
      PUSHs(&PL_sv_undef);
   }
}

void
set_sub_file(sub,filename)
   SV *sub;
   SV *filename;
PROTOTYPE: $$
PPCODE:
{
   COP *c;
   const char *n;
   STRLEN l;
   if (!SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub, \"filename\"");
   n=SvPV(filename,l);
   c=(COP*)CvSTART(sub);
   while (c) {
      if (c->op_type==OP_NEXTSTATE || c->op_type==OP_DBSTATE) {
         CopFILE_free(c);
         CopFILE_setn(c,n,l);
      }
      c=(COP*)c->op_sibling;
   }
}

void
sub_firstline(sub)
   SV *sub;
PROTOTYPE: $
PPCODE:
{
   if (!SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   if (CvSTART(sub)) {
      dTARGET;
      PUSHi(CopLINE((COP*)CvSTART(sub)));
   } else {
      PUSHs(&PL_sv_undef);
   }
}

void
method_owner(sub)
   SV *sub;
PROTOTYPE: $
PPCODE:
{
   dTARGET;
   if (!SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   sv_setpv(TARG, HvNAME(GvSTASH(CvGV(sub))));
   PUSHs(TARG);
}

void
define_function(pkg, name_sv, sub, ...)
   SV *pkg;
   SV *name_sv;
   SV *sub;
   I32 flags= items>3 ? SvIV(ST(3)) : FALSE;
PPCODE:
if (!SvROK(sub) ||
    (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV) ||
    SvROK(name_sv) ||
    (!SvPOK(pkg) && (!SvROK(pkg) || SvTYPE(SvRV(pkg))!=SVt_PVHV))) {
   croak_xs_usage(cv, "\"pkg\" || \\%%stash, \"name\", \\&sub [, TRUE ]");
} else {
   HV *pkg_stash=SvROK(pkg) ? (HV*)SvRV(pkg) : gv_stashsv(pkg, flags & GV_ADD);
   GV *glob;
   CV *was_here;
   STRLEN namelen;
   const char *name=SvPV(name_sv, namelen);
   if (!pkg_stash)
      Perl_croak(aTHX_ "unknown package %.*s", (int)SvCUR(pkg), SvPVX(pkg));
   glob=(GV*)*hv_fetch(pkg_stash, name, namelen, TRUE);
   if (SvTYPE(glob) != SVt_PVGV)
      gv_init(glob, pkg_stash, name, namelen, GV_ADDMULTI);

   if ((flags & 2) && (was_here=GvCV(glob)) && IsWellDefinedSub(was_here)) {
      if (GIMME_V != G_VOID)
         PUSHs(sv_2mortal(newRV((SV*)was_here)));

   } else {
      sv_setsv((SV*)glob, ST(2));
      if (CvANON(sub)) {
         CvANON_off(sub);
         CvGV_set((CV*)sub, glob);
         if (!CvISXSUB(sub)) {
            SV *file=CopFILESV((COP*)CvSTART(sub));
            if (file && (!SvOK(file) || !SvPVX(file) || !strncmp(SvPVX(file), "(eval ", 6)))
               sv_setpvf(file, "(%s::%.*s)", HvNAME(pkg_stash), (int)namelen, name);
         }
      }
      PUSHs(ST(2));
      if (CvMETHOD(sub)) {
         PUTBACK;
         Perl_mro_method_changed_in(aTHX_ pkg_stash);
      }
   }
}


void
set_sub_name(sub, name_sv)
   SV *sub;
   SV *name_sv;
PPCODE:
if (SvROK(name_sv) || !SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub) != SVt_PVCV)) {
   croak_xs_usage(cv, "\\&sub, \"name\"");
} else {
   GV *glob;
   STRLEN namelen;
   const char *name=SvPV(name_sv,namelen);
   glob=(GV*)*hv_fetch(GvSTASH(CvGV(sub)), name, namelen, TRUE);
   if (SvTYPE(glob) != SVt_PVGV)
      gv_init(glob, GvSTASH(CvGV(sub)), name, namelen, GV_ADDMULTI);
   CvANON_off(sub);
   CvGV_set((CV*)sub, glob);
}


void
can(obj,method,...)
   SV *obj;
   SV *method;
PPCODE:
{
   GV *glob=do_can(aTHX_ obj, method);
   if (glob)
      PUSHs( sv_2mortal(newRV((SV*)GvCV(glob))) );
   else
      PUSHs( &PL_sv_undef );
}


void
set_method(sub)
   SV *sub;
PROTOTYPE: $
PPCODE:
{
   CvMETHOD_on(SvRV(sub));
   ++SP;
}

void
ones(bitset)
   SV *bitset;
PROTOTYPE: $
PPCODE:
{
   I32 gimme=GIMME_V;
   if (SvOK(bitset)) {
      I32 l=SvCUR(bitset)<<3, i;
      const unsigned char *s=(unsigned char*)SvPVX(bitset);
      unsigned int bit=1;
      EXTEND(SP,l);
      for (i=0; i<l; ++i) {
         if ((*s) & bit) {
            PUSHs(sv_2mortal(newSViv(i)));
            if (gimme==G_SCALAR) break;
         }
         if ((bit<<=1)==(1<<8)) {
            ++s;  bit=1;
         }
      }
   }
}

void
first(...)
PPCODE:
{
   OP *o=cUNOP->op_first, *kid;
   if (!o->op_sibling)
      o=cUNOPo->op_first;
   while ((kid=o->op_sibling)) o=kid;
   if (o->op_type==OP_NULL)
      o=cUNOPo->op_first;
   o->op_next=PL_op->op_next;
   o->op_ppaddr=&pp_first;
   if (items)
      ++SP;
   else
      XPUSHs(&PL_sv_undef);
}

void
swap_arrays(avref1, avref2)
   SV *avref1;
   SV *avref2;
PPCODE:
{
   AV *av1, *av2;
   XPVAV *any1;
   U32 flags1;
   SV **array1;
   if (!SvROK(avref1) || !SvROK(avref2))
      croak_xs_usage(cv, "\\@array1, \\@array2");
   av1=(AV*)SvRV(avref1);
   av2=(AV*)SvRV(avref2);
   if (SvTYPE(av1) != SVt_PVAV || SvTYPE(av2) != SVt_PVAV)
      croak_xs_usage(cv, "\\@array1, \\@array2");
   any1=SvANY(av1);
   flags1=SvFLAGS(av1);
   SvANY(av1)=SvANY(av2);
   SvFLAGS(av1)=SvFLAGS(av2);
   SvANY(av2)=any1;
   SvFLAGS(av2)=flags1;
   array1=AvARRAY(av1);
   AvARRAY(av1)=AvARRAY(av2);
   AvARRAY(av2)=array1;
}

void
swap_array_elems(avref,ix1,ix2)
   SV *avref;
   IV ix1;
   IV ix2;
PPCODE:
{
   AV *av;
   SV *tmp;
   I32 s;
   if (!SvROK(avref) || (av=(AV*)SvRV(avref), SvTYPE(av) != SVt_PVAV))
      croak_xs_usage(cv, "\\@array, index1, index2");
   s=AvFILL(av)+1;
   if (ix1<0) ix1+=s;
   if (ix2<0) ix2+=s;
   if (ix1==ix2 || ix1<0 || ix2<0 || ix1>=s || ix2>=s)
      croak("swap_array_elems: invalid indices");
   tmp=AvARRAY(av)[ix1];
   AvARRAY(av)[ix1]=AvARRAY(av)[ix2];
   AvARRAY(av)[ix2]=tmp;
}

void
disable_debugging()
PPCODE:
{
   PL_runops=PL_runops_std;
}

void
enable_debugging()
PPCODE:
{
   PL_runops=PL_runops_dbg;
}

void
stop_here_gdb(...)
PPCODE:
{
   if (items>0) {
      SV *x=ST(0);
      assert(SvANY(x));
      PERL_UNUSED_VAR(x);
      ++SP;
   }
}

MODULE = Polymake                       PACKAGE = Polymake::Core

void
name_of_arg_var(arg_no)
   I32 arg_no;
PPCODE:
{
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;
   PUSHs(&PL_sv_undef); /* default answer */

   while (cx >= cx_bottom) {
      if (CxTYPE(cx)==CXt_SUB && !SkipDebugFrame(cx,0)) {
         OP *o=cx->blk_oldcop->op_next;
         if (o->op_type == OP_PUSHMARK) {
            do {
               o=o->op_sibling;
               if (!o) goto Leave;
            } while (--arg_no>=0);
            if (o->op_type == OP_NULL) o=cUNOPo->op_first;
            if (o->op_type == OP_GVSV) {
               GV *gv=retrieve_gv(aTHX_ o, 0, 0, cx, cx_bottom);
               SETs(sv_2mortal(newSVpvn(GvNAME(gv),GvNAMELEN(gv))));
            }
         }
Leave:   break;
      }
      --cx;
   }
}

void
name_of_ret_var()
PPCODE:
{
   SV *var_sv=pm_perl_name_of_ret_var(aTHX);
   if (var_sv)
      XPUSHs(var_sv);
   else
      XPUSHs(&PL_sv_undef);
}

void
name_of_custom_var(expect_assignment)
   I32 expect_assignment;
PPCODE:
{
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;

   /* to keep things easier, only recognize assignments to whole arrays/hashes and hash element with literally given keys */

   while (cx >= cx_bottom) {
      if (CxTYPE(cx)==CXt_SUB && !SkipDebugFrame(cx,0)) {
         OP *o=cx->blk_oldcop->op_next;
         if (o->op_type == OP_PUSHMARK) {
            o=o->op_sibling;
            if (o) {
               int allow_scalar=FALSE, allow_list=FALSE;
               if (expect_assignment) {
                  switch (o->op_type) {
                  case OP_SASSIGN:
                     o=cBINOPo->op_last;                        /* descend to the last SASSIGN operand = lhs term */
                     allow_scalar=TRUE;
                     break;
                  case OP_AASSIGN:
                     o=cLISTOPo->op_last;                       /* descend to the last AASSIGN operand = lhs list */
                     o=cUNOPo->op_first->op_sibling;            /* pushmark, then lhs item: must be alone */
                     allow_list= o && !o->op_sibling;
                     break;
                  }
               } else {
                  allow_scalar=allow_list=TRUE;
               }
               switch (o->op_type) {
               case OP_NULL:
                  if (allow_scalar) {
                     o=cUNOPo->op_first;
                     switch (o->op_type) {
                     case OP_GVSV:
                        XPUSHs(compose_varname(aTHX_ o, 0, 0, '$', cx, cx_bottom));
                        break;
#if PerlVersion >= 5220
                     case OP_MULTIDEREF:
                        {
                           UNOP_AUX_item* items = cUNOP_AUXo->op_aux;
                           UV actions = items->uv;
                           if (actions == (MDEREF_HV_gvhv_helem | MDEREF_INDEX_const | MDEREF_FLAG_last)) {
                              SV *key_sv;
                              XPUSHs(compose_varname(aTHX_ o, 0, &key_sv, '%', cx, cx_bottom));
                              XPUSHs(key_sv);
                           }
                        }
                        break;
#endif
                     }
                  }
                  break;
               case OP_HELEM:
                  if (allow_scalar) {
                     OP *key_op;
                     o=cUNOPo->op_first;                        /* hash element: hash reference comes first */
                     if (o->op_type == OP_RV2HV) {
                        key_op=o->op_sibling;
                        if (key_op && key_op->op_type == OP_CONST) {
                           SV *key_sv;
                           XPUSHs(compose_varname(aTHX_ cUNOPo->op_first, key_op, &key_sv, '%', cx, cx_bottom));
                           XPUSHs(key_sv);
                        }
                     }
                  }
                  break;
               case OP_RV2AV:
                  if (allow_list)
                     XPUSHs(compose_varname(aTHX_ cUNOPo->op_first, 0, 0, '@', cx, cx_bottom));
                  break;
               case OP_RV2HV:
                  if (allow_list)
                     XPUSHs(compose_varname(aTHX_ cUNOPo->op_first, 0, 0, '%', cx, cx_bottom));
                  break;
               }
            }
         }
         break;
      }
      --cx;
   }
}

void
passed_to(sub)
   SV *sub;
PPCODE:
{
   PERL_CONTEXT *cx_bottom=cxstack, *cx_top=cx_bottom+cxstack_ix, *cx=cx_top;
   OP *o;
   while (cx >= cx_bottom) {
      if (CxTYPE(cx)==CXt_SUB && !SkipDebugFrame(cx,0)) {
         o=cx->blk_sub.retop;
         while (o->op_type == OP_LEAVE) o=o->op_next;
         if (o->op_type != OP_LEAVESUB && o->op_type != OP_LEAVESUBLV) {
            if (o->op_type == OP_GV && o->op_next->op_type == OP_ENTERSUB) {
               CV *cv;
#ifdef USE_ITHREADS
               SV **saved_curpad=PL_curpad;
               PL_curpad=pm_perl_get_cx_curpad(aTHX_ cx, cx_bottom);
#endif
               cv=GvCV(cGVOPo_gv);
#ifdef USE_ITHREADS
               PL_curpad=saved_curpad;
#endif
               if (cv == (CV*)SvRV(sub)) XSRETURN_YES;
            }
            break;
         }
      }
      --cx;
   }
   XSRETURN_NO;
}

void
get_array_flags(avref)
   SV *avref;
PPCODE:
{
   SV* av;
   if (SvROK(avref) && (av=SvRV(avref), SvTYPE(av)==SVt_PVAV)) {
      MAGIC* mg=pm_perl_array_flags_magic(aTHX_ av);
      if (mg) {
         dTARGET;
         PUSHi(mg->mg_len);
      } else {
         PUSHs(&PL_sv_undef);
      }
   } else {
      croak_xs_usage(cv, "\\@array");
   }
}

void
get_array_annex(avref)
   SV *avref;
PPCODE:
{
   SV* av;
   if (SvROK(avref) && (av=SvRV(avref), SvTYPE(av)==SVt_PVAV)) {
      MAGIC* mg=pm_perl_array_flags_magic(aTHX_ av);
      if (mg && mg->mg_obj) {
         PUSHs(mg->mg_obj);
      } else {
         PUSHs(&PL_sv_undef);
      }
   } else {
      croak_xs_usage(cv, "\\@array");
   }
}

void
set_array_flags(avref, flags, ...)
   SV *avref;
   I32 flags;
PPCODE:
{
   SV* av;
   if (items <= 3 && SvROK(avref) && (av=SvRV(avref), SvTYPE(av)==SVt_PVAV)) {
      MAGIC* mg=pm_perl_array_flags_magic(aTHX_ av);
      if (!mg)
         mg=sv_magicext(av, Nullsv, PERL_MAGIC_ext, &array_flags_vtbl, Nullch, 0);
      mg->mg_len=flags;
      if (items==3)
         mg->mg_obj=ST(2);
   } else {
      croak_xs_usage(cv, "\\@array, flags [, annex]");
   }
}

void
delete_array_flags(avref)
   SV *avref;
PPCODE:
{
   SV* av;
   if (SvROK(avref) && (av=SvRV(avref), SvTYPE(av)==SVt_PVAV)) {
      MAGIC* prev_mg=NULL;
      MAGIC* mg;
      for (mg=SvMAGIC(av); mg != NULL; prev_mg=mg, mg=mg->mg_moremagic) {
         if (mg->mg_virtual == &array_flags_vtbl) {
            if (prev_mg != NULL) {
               prev_mg->mg_moremagic=mg->mg_moremagic;
            } else {
               SvMAGIC_set(av, mg->mg_moremagic);
            }
            Safefree(mg);
            mg_magical(av);
            break;
         }
      }
   } else {
      croak_xs_usage(cv, "\\@array");
   }
}

void
compiling_in(...)
PPCODE:
if (items==0) {
   XPUSHs(sv_2mortal(newSVpv(HvNAME(PL_curstash), 0)));
} else {
   SV *pkg=ST(0);
   HV *stash=SvROK(pkg) ? (HV*)SvRV(pkg) : gv_stashsv(pkg, FALSE);
   PUSHs(PL_curstash == stash ? &PL_sv_yes : &PL_sv_no);
}

void
compiling_in_sub()
PPCODE:
{
   CV *cv=PL_compcv;
   if (cv && SvTYPE(cv)==SVt_PVCV && (!CvUNIQUE(cv) || SvFAKE(cv)))
      XPUSHs(&PL_sv_yes);
   else
      XPUSHs(&PL_sv_no);
}

void
defuse_environ_bug()
PPCODE:
{
#if !defined(__APPLE__)
   PL_origenviron=environ;
#endif
}

void
rescue_static_code(for_script)
   I32 for_script;
PPCODE:
{
   /* We must convert a "one-shot" sub made for eval to a real persistent sub:
      1. In script mode, short-circuit this operation, making the first real op in the script
         the start one for all future calls.
         In rulefile mode, rewind back to the first real op of the rule sub.
      2. Prepare the special start operation converting the root to LEAVESUB,
         since all subsequent calls will be made via ENTERSUB.
         This op will reside in an unused NULL enclosing this XSUB's call.
      3. Store the root operation (LEAVEEVAL) and increase its refcount,
         otherwise get destroyed in pp_require
      4. Provide CvDEPTH be decreased on exit, since LEAVEEVAL doesn't care about it.
   */
   OP *start=PL_op, *tmp_start=cUNOPx(start)->op_first, *root=PL_eval_root;
   PERL_CONTEXT *cx=cxstack+cxstack_ix;
   CV *script_cv;
   /* 1. */
   if (for_script) {
      script_cv=cx->blk_eval.cv;
      while (start->op_type != OP_NEXTSTATE && start->op_type != OP_DBSTATE && start->op_type != OP_LEAVEEVAL) {
         start=start->op_next;
      }
   } else if (CxTYPE(cx) == CXt_EVAL && (script_cv=cx->blk_eval.cv, CvUNIQUE(script_cv))) {
      start=((LISTOP*)((UNOP*)root)->op_first)->op_first;
   } else {
      /* repeated call */
      XSRETURN_EMPTY;
   }
   /* 2. */
   CvSTART(script_cv)=tmp_start;
   CvANON_on(script_cv);
   CvGV_set(script_cv, (GV*)&PL_sv_undef);
   tmp_start->op_next=start;
   tmp_start->op_ppaddr=&convert_eval_to_sub;
   /* 3. */
   CvEVAL_off(script_cv);
   OP_REFCNT_LOCK;
   OpREFCNT_inc(root);
   OP_REFCNT_UNLOCK;
   CvROOT(script_cv)=root;
   PUSHs(sv_2mortal(newRV((SV*)script_cv)));
   /* 4. */
   LEAVE;
   CvDEPTH(script_cv)=0;
   SAVELONG(CvDEPTH(script_cv));
   CvDEPTH(script_cv)=1;
   ENTER;
}

BOOT:
if (PL_DBgv) {
   CvNODEBUG_on(get_cv("Polymake::select_method", FALSE));
   CvNODEBUG_on(get_cv("Polymake::disable_debugging", FALSE));
   CvNODEBUG_on(get_cv("Polymake::enable_debugging", FALSE));
   CvNODEBUG_on(get_cv("Polymake::weak", FALSE));
   CvNODEBUG_on(get_cv("Polymake::Core::name_of_arg_var", FALSE));
   CvNODEBUG_on(get_cv("Polymake::Core::name_of_ret_var", FALSE));
   CvNODEBUG_on(get_cv("Polymake::Core::name_of_custom_var", FALSE));
   CvNODEBUG_on(get_cv("Polymake::Core::passed_to", FALSE));
   CvNODEBUG_on(get_cv("Polymake::Core::rescue_static_code", FALSE));
}
CvFLAGS(get_cv("Polymake::readonly", FALSE)) |= CVf_NODEBUG | CVf_LVALUE;
CvFLAGS(get_cv("Polymake::readonly_deep", FALSE)) |= CVf_NODEBUG | CVf_LVALUE;
CvFLAGS(get_cv("Polymake::readwrite", FALSE)) |= CVf_NODEBUG | CVf_LVALUE;
CvFLAGS(get_cv("Polymake::stop_here_gdb", FALSE)) |= CVf_NODEBUG | CVf_LVALUE;

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
