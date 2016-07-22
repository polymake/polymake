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

static HV *secret_pkg;

static Perl_check_t def_ck_AASSIGN;

typedef struct method_info {
   OP* next_op;
   SV* filter;
   SV* fallback;
   I32 field_index;
   I32 filter_is_method;
   CV* accessor;
} method_info;

#ifndef NewOp
#define NewOp Newz
#endif

static
OP* pp_hide_orig_object(pTHX)
{
   OP *next=(PL_ppaddr[OP_ENTERSUB])(aTHX);
   AV *args=GvAV(PL_defgv);
   // imitate shift(@_) without cleaning out the 0-th slot
   ++AvARRAY(args);
   AvMAX(args)--;
   AvFILLp(args)--;
   PL_op->op_ppaddr=PL_ppaddr[OP_ENTERSUB];
   return next;
}

static
OP* pp_hide_orig_object_first(pTHX)
{
   PL_stack_sp+=2;
   return pp_hide_orig_object(aTHX);
}

static
SV* find_method(pTHX_ I32 index, method_info* info)
{
   dSP; dTOPss;
   SV *obj=SvRV(sv),
      *field=*av_fetch((AV*)obj, index, 1);
   SV *method_cv;
   for (;;) {
      if (SvROK(field)) {
         method_cv=SvRV(field);
         if (SvTYPE(method_cv)==SVt_PVCV)
            break;
         if (SvOBJECT(method_cv)) {
            sv=field;
            obj=method_cv;
            field=*av_fetch((AV*)obj, index, 1);
         } else {
            Perl_croak(aTHX_ "The method field contains a reference of a wrong type");
         }

      } else if (SvIOK(field)) {
         field=*av_fetch((AV*)obj, SvIVX(field), 1);

      } else if (SvPOK(field)) {
         if (SvCUR(field)) {
            GV *method_gv=gv_fetchmethod(SvSTASH(obj), SvPVX(field));
            method_cv= method_gv && isGV(method_gv)
                     ? (SV*)GvCV(method_gv)
                     : pm_perl_namespace_try_lookup(aTHX_ SvSTASH(obj), field, SVt_PVCV);
            if (method_cv) {
               sv_setsv(field, newRV(method_cv));
               break;
            } else {
               sv_setsv(field, &PL_sv_no);
            }
         }
         if (info) Perl_croak(aTHX_ "Undefined method called");
         return field;

      } else if (SvOK(field)) {
         Perl_croak(aTHX_ "The method field contains a value of a wrong type");

      } else if (info) {
         if ((method_cv=info->fallback)) {
            sv=TOPs; break;
         } else {
            Perl_croak(aTHX_ "Undefined method called");
         }
      } else {
         return field;
      }
   }
   if (info) {
      if (CvMETHOD((CV*)method_cv)) {
         SV **stack, **bottom, *orig=TOPs;
         int push_orig= sv!=orig && SvSTASH(method_cv)!=secret_pkg;
         EXTEND(SP,push_orig+1);
         for (stack=SP, bottom=PL_stack_base+TOPMARK+1; stack>bottom; --stack)
            stack[push_orig]=stack[-1];
         *stack=orig;
         if (push_orig) {
            *++stack=sv;
            info->next_op->op_next->op_ppaddr=&pp_hide_orig_object;
         }
         *(PL_stack_sp=SP+push_orig+1)=method_cv;
         return method_cv;

      } else {
         SETs(method_cv);
         return 0;
      }

   } else {
      return sv_2mortal(newRV(method_cv));
   }
}

static
OP* pp_access(pTHX)
{
   dSP; dTOPss;
   SV *obj, *method_name;
   HV* class;
   MAGIC* mg;
   if (!SvROK(sv) || (obj=SvRV(sv), !SvOBJECT(obj))) return Perl_pp_method_named(aTHX);

   class=SvSTASH(obj);
   method_name=cSVOP_sv;
   mg=SvMAGIC(method_name);
   do {
      if (class == (HV*)mg->mg_obj) {
         method_info* info=(method_info*)mg->mg_ptr;
         SV* field=*av_fetch((AV*)obj, info->field_index, 1);
         if (info->filter) {
            SV* rhs=SP[-1];     // rhs value
            SP[-1]=field;       // preserve it below the mark
            if (info->filter_is_method)
               XPUSHs(method_name);     // preserve ref(obj) on the stack
            else 
               SP[0]=method_name;
            XPUSHs(rhs);
            XPUSHs(info->filter);
            PUTBACK;
            return info->next_op;
         } else {
            SETs(field);        // replace ref(obj) on the stack top by the requested field
            (void)POPMARK;      // skip pp_entersub
            return info->next_op->op_next;
         }
      }
   } while ((mg=mg->mg_moremagic));

   return Perl_pp_method_named(aTHX);
}

// better to repeat some code than to put extra tests in the heavily used pp_access
static
OP* pp_method_access(pTHX)
{
   dSP; dTOPss;
   SV *obj, *method_name;
   HV* class;
   MAGIC* mg;
   if (!SvROK(sv) || (obj=SvRV(sv), !SvOBJECT(obj))) return Perl_pp_method_named(aTHX);

   class=SvSTASH(obj);
   method_name=cSVOP_sv;
   mg=SvMAGIC(method_name);
   do {
      if (class == (HV*)mg->mg_obj) {
         method_info* info=(method_info*)mg->mg_ptr;
         SV* method=find_method(aTHX_ info->field_index, 0);
         SETs(method);
         (void)POPMARK;
         return info->next_op->op_next;
      }
   } while ((mg=mg->mg_moremagic));

   return Perl_pp_method_named(aTHX);
}

static
OP* pp_method_defined(pTHX)
{
   dSP; dTOPss;
   SV *obj, *method_name;
   HV* class;
   MAGIC* mg;
   if (!SvROK(sv) || (obj=SvRV(sv), !SvOBJECT(obj))) return Perl_pp_method_named(aTHX);

   class=SvSTASH(obj);
   method_name=cSVOP_sv;
   mg=SvMAGIC(method_name);
   do {
      if (class == (HV*)mg->mg_obj) {
         method_info* info=(method_info*)mg->mg_ptr;
         I32 is_assignment= info->next_op->op_next->op_type == OP_DORASSIGN;
         SV* field=*av_fetch((AV*)obj, info->field_index, is_assignment);
         SETs(field);        // replace ref(obj) on the stack top by the requested field
         if (SvROK(field) ? SvTYPE(SvRV(field)) != SVt_PVCV : SvIOK(field)) {
            // if it's a reference to another object to follow, pretend it's undefined
            if (is_assignment)
               SvOK_off(field);
            else
               SETs(&PL_sv_undef);
         }
         (void)POPMARK;      // skip pp_entersub
         return info->next_op->op_next;
      }
   } while ((mg=mg->mg_moremagic));

   return Perl_pp_method_named(aTHX);
}

static
OP* pp_method_call(pTHX)
{
   dSP; dTOPss;
   SV *obj, *method_name;
   HV *class;
   MAGIC *mg;
   if (!SvROK(sv) || (obj=SvRV(sv), !SvOBJECT(obj))) return Perl_pp_method_named(aTHX);

   class=SvSTASH(obj);
   method_name=cSVOP_sv;
   mg=SvMAGIC(method_name);
   do {
      if (class == (HV*)mg->mg_obj) {
         method_info *info=(method_info*)mg->mg_ptr;
         (void)POPMARK;
         (void)find_method(aTHX_ info->field_index, info);
         return info->next_op->op_next;
      }
   } while ((mg=mg->mg_moremagic));

   return Perl_pp_method_named(aTHX);
}


static
OP* intercept_ck_aassign(pTHX_ OP* o)
{
   OP* lhs;
   o=def_ck_AASSIGN(aTHX_ o);
   lhs=OpSIBLING(cUNOPo->op_first);
   if (lhs->op_type == OP_NULL) lhs=cUNOPx(lhs)->op_first;
   while (lhs) {
      if (lhs->op_type == OP_ENTERSUB) {
         OP* meth_op=method_named_op(lhs);
         if (meth_op) meth_op->op_private |= MethodIsCalledOnLeftSideOfArrayAssignment;
      }
      lhs=OpSIBLING(lhs);
   }
   return o;
}

static
void catch_ptrs(pTHX_ SV *dummy)
{
   PL_check[OP_AASSIGN]=&intercept_ck_aassign;
}

static
void reset_ptrs(pTHX_ SV *dummy)
{
   PL_check[OP_AASSIGN]=def_ck_AASSIGN;
}

MODULE = Polymake::Struct               PACKAGE = Polymake::Struct

PROTOTYPES: DISABLE

void
access_field(obj_ref, ...)
   SV *obj_ref=ST(0);
PPCODE:
{
   I32 index=CvDEPTH(cv);
   OP* o=method_named_op(PL_op);
   SV* obj;
   if (SvROK(obj_ref))
      obj=SvRV(obj_ref);
   else
      Perl_croak(aTHX_ "field access for %.*s called as static method", (int)SvCUR(obj_ref), SvPVX(obj_ref));

   if (o != NULL) {
      OP* next_op=PL_op->op_next;
      SV* filter=NULL;
      SV* method_name=cSVOPo_sv;
      HV* class=SvSTASH(obj);
      MAGIC* mg=NULL;

      if (SvTYPE(method_name) == SVt_PVMG) {
         // maybe the first object of some derived class?
         mg=SvMAGIC(method_name);
         do {
            if (((method_info*)mg->mg_ptr)->accessor == cv) break;
         } while ((mg=mg->mg_moremagic) != NULL);
      }

      if (mg == NULL) {
         method_info info;
         if (next_op->op_type == OP_SASSIGN && !(next_op->op_private & OPpASSIGN_BACKWARDS)) {
            filter=GvSV(CvGV(cv));
            if (filter && (SvROK(filter) || (SvPOK(filter) && SvCUR(filter)))) {
               OP *sub_op;
               NewOp(0, sub_op, 1, OP);
               Copy(PL_op, sub_op, 1, OP);
               sub_op->op_private &= ~OPpLVAL_INTRO;
               sub_op->op_next=next_op;
               next_op->op_private ^= OPpASSIGN_BACKWARDS;
               next_op=sub_op;
               if (SvROK(filter)) {
                  filter=SvRV(filter);
               } else {
                  GV *method_gv=gv_fetchmethod(SvSTASH(obj), SvPVX(filter));
                  CV *filter_cv= method_gv && isGV(method_gv)
                               ? GvCV(method_gv)
                               : (CV*)pm_perl_namespace_try_lookup(aTHX_ SvSTASH(obj), filter, SVt_PVCV);
                  if (!filter_cv) Perl_croak(aTHX_ "access filter method %.*s not found", (int)SvCUR(filter), SvPVX(filter));
                  filter=(SV*)filter_cv;
               }
            } else {
               next_op=PL_op;
               filter=NULL;
            }
         } else {
            next_op=PL_op;
         }

         info.field_index=index;
         info.filter=filter;
         info.filter_is_method=filter && CvMETHOD((CV*)filter);
         info.next_op=next_op;
         info.fallback=NULL;
         info.accessor=cv;

         if (SvTYPE(method_name) < SVt_PVMG) {
            // first use of this operation
            U32 flags=SvFLAGS(method_name) & (SVf_FAKE | SVf_READONLY);
            SvFLAGS(method_name) &= ~(SVf_FAKE | SVf_READONLY);
            sv_magicext(method_name, (SV*)class, PERL_MAGIC_ext, 0, (char*)&info, sizeof(info));
            SvFLAGS(method_name) |= flags;
            o->op_ppaddr=&pp_access;
         } else {
            sv_magicext(method_name, (SV*)class, PERL_MAGIC_ext, 0, (char*)&info, sizeof(info));
         }

      } else {
         // first object of some derived class
         sv_magicext(method_name, (SV*)class, PERL_MAGIC_ext, 0, mg->mg_ptr, 0);
         filter=((method_info*)mg->mg_ptr)->filter;
      }

      if (filter) {
         OP* prev=OpSIBLING(cUNOP->op_first);
         while (prev->op_next != o) prev=prev->op_next;
         PL_op=prev;
         PUSHMARK(SP);  // restore the mark
         return;        // avoid PUTBACK
      }
   }
   PUSHs(*av_fetch((AV*)obj, index, 1));
}


void
method_call(obj)
   SV *obj=SvRV(ST(0));
PPCODE:
{
   method_info info, *infop=&info;
   I32 index=CvDEPTH(cv);
   OP *o=method_named_op(PL_op),
      *next_op=PL_op->op_next;
   SV *fallback=GvSV(CvGV(cv));
   if (fallback) {
      if (SvROK(fallback)) fallback=SvRV(fallback);
      if (SvTYPE(fallback)!=SVt_PVCV) fallback=NULL;
   }

   if (o) {
      SV* method_name=cSVOPo_sv;
      HV* class=SvSTASH(obj);
      MAGIC* mg=NULL;

      if (SvTYPE(method_name) == SVt_PVMG) {
         // maybe the first object of some derived class?
         mg=SvMAGIC(method_name);
         do {
            if (((method_info*)mg->mg_ptr)->accessor == cv) break;
         } while ((mg=mg->mg_moremagic) != NULL);
      }

      if (mg == NULL) {
         info.field_index=index;
         info.filter=NULL;
         info.next_op=PL_op;
         info.fallback=fallback;
         info.accessor=cv;

         if (SvTYPE(method_name) < SVt_PVMG) {
            // first use of this operation
            U32 flags=SvFLAGS(method_name) & (SVf_FAKE | SVf_READONLY);
            SvFLAGS(method_name) &= ~(SVf_FAKE | SVf_READONLY);
            sv_magicext(method_name, (SV*)class, PERL_MAGIC_ext, 0, (char*)&info, sizeof(info));
            SvFLAGS(method_name) |= flags;
            switch (next_op->op_type) {
            case OP_SASSIGN:
            case OP_UNDEF:
               o->op_ppaddr=&pp_access;
               break;
            case OP_DEFINED:
            case OP_DOR:
            case OP_DORASSIGN:
               o->op_ppaddr=&pp_method_defined;
               break;
            case OP_ENTERSUB:
               o->op_ppaddr=&pp_method_call;
               break;
            default:
               o->op_ppaddr= (o->op_private & MethodIsCalledOnLeftSideOfArrayAssignment) ? &pp_access : &pp_method_access;
               break;
            }
         } else {
            sv_magicext(method_name, (SV*)class, PERL_MAGIC_ext, 0, (char*)&info, sizeof(info));
         }

      } else {
         // first object of some derived class
         sv_magicext(method_name, (SV*)class, PERL_MAGIC_ext, 0, mg->mg_ptr, 0);
         infop=(method_info*)mg->mg_ptr;
      }
   }
   switch (next_op->op_type) {
   default:
      if (!(o && o->op_ppaddr == &pp_access)) {
         PUSHs(find_method(aTHX_ index, 0));
         break;
      }
      // FALLTHRU
   case OP_SASSIGN:
   case OP_UNDEF:
      PUSHs(*av_fetch((AV*)obj, index, 1));
      break;
   case OP_DEFINED:
   case OP_DOR:
   case OP_DORASSIGN:
      PUSHs(*av_fetch((AV*)obj, index, next_op->op_type == OP_DORASSIGN));
      // if it's an index to another field to follow, pretend it's undefined
      if (SvROK(TOPs) ? SvTYPE(SvRV(TOPs)) != SVt_PVCV : SvIOK(TOPs)) {
         if (next_op->op_type == OP_DORASSIGN)
            SvOK_off(TOPs);
         else
            SETs(&PL_sv_undef);
      }
      break;
   case OP_ENTERSUB:
      if (!o) {
         info.fallback=fallback;
         info.next_op=PL_op;
      }
      if (find_method(aTHX_ index, infop)) {
         if (next_op->op_ppaddr==&pp_hide_orig_object)
            next_op->op_ppaddr=&pp_hide_orig_object_first;
         else
            next_op->op_ppaddr=&pm_perl_select_method_helper_op;
      }
      ++SP;
      /* TRICK: even if find_method pushed two or more items on the stack (object, hidden object, method), this XSUB may push only one
         (due to scalar context imposed on this op).  Thus we pretend here to push just one item, and the helper
         op unveils the rest. */
   }
}


I32
get_field_index(sub)
   SV *sub;
CODE:
   RETVAL=SvROK(sub) && (sub=SvRV(sub), CvSTASH((CV*)sub)==secret_pkg) ? CvDEPTH(sub) : -1;
OUTPUT:
   RETVAL


void
get_field_filter(sub)
   SV *sub;
PPCODE:
{
   SV *filter=&PL_sv_undef;
   if (SvROK(sub) && (sub=SvRV(sub), CvSTASH((CV*)sub)==secret_pkg)) {
      GV *field_gv=CvGV(sub);
      filter=GvSV(field_gv);
      if (filter && SvROK(filter) && SvTYPE(SvRV(filter))==SVt_PVCV)
         filter=sv_mortalcopy(filter);
      else if (filter && SvPOK(filter) && SvCUR(filter)) {
         GV *method_gv=gv_fetchmethod(GvSTASH(field_gv), SvPVX(filter));
         if (method_gv && isGV(method_gv))
            filter=sv_2mortal(newRV((SV*)GvCV(method_gv)));
         else
            filter=&PL_sv_undef;
      } else {
         filter=&PL_sv_undef;
      }
   }
   PUSHs(filter);
}


void
create_accessor(index, xsubr)
   I32 index;
   SV *xsubr;
PPCODE:
{
   SV *sub=newSV(0);
   CV *xsub=(CV*)SvRV(xsubr);
   sv_upgrade(sub, SVt_PVCV);
   CvDEPTH(sub)=index;
   CvXSUB(sub)=CvXSUB(xsub);
   CvFLAGS(sub)=CvFLAGS(cv) | CVf_ANON | CVf_LVALUE | CVf_METHOD | CVf_NODEBUG; /* the standard flags should be the same by all XSUBs */
   CvSTASH_set((CV*)sub, CvSTASH(xsub));
   PUSHs(sv_2mortal(newRV_noinc(sub)));
}


void
make_body(...)
PPCODE:
{
   AV *av=newAV();
   HV *stash;
   SV **ary, **src=SP+1, **src_end=SP+items, *pkg_from=*src_end, *rv;
   New(0, ary, items-1, SV*);
   AvALLOC(av) = ary;
   AvARRAY(av) = ary;
   AvFILLp(av) = items-2;
   AvMAX(av) = items-2;
   for (; src < src_end; ++src, ++ary) {
      SV *sv=*src;
      if ((SvFLAGS(sv) & (SVs_TEMP|SVs_GMG|SVs_SMG))==SVs_TEMP) {
         SvTEMP_off(sv);
         SvREFCNT_inc_simple_void_NN(sv);
         *ary = sv;
      } else {
         SV *copy_sv=NEWSV(0,0);
         sv_setsv(copy_sv, sv);
         *ary = copy_sv;
      }
   }
   rv=newRV_noinc((SV*)av);
   if (SvROK(pkg_from)) {
      pkg_from=SvRV(pkg_from);
      if (SvOBJECT(pkg_from))
         stash=SvSTASH(pkg_from);
      else
         Perl_croak(aTHX_ "anonymous reference given instead of class name");
   } else {
      STRLEN l;
      const char *p=SvPV(pkg_from,l);
      stash=gv_stashpvn(p,l,TRUE);
   }
   sv_bless(rv,stash);
   PUSHs(sv_2mortal(rv));
}

void
make_alias(body, index)
  SV *body;
  I32 index;
PROTOTYPE: $$
PPCODE:
{
   SV **dst=AvARRAY(SvRV(body))+index;
   GV *gv=gv_fetchpv(SvPV_nolen(*dst), TRUE, SVt_PV);
   SvREFCNT_dec(*dst);
   *dst=SvREFCNT_inc(GvSV(gv));
}

void
original_object()
PPCODE:
{
   XPUSHs(AvALLOC(GvAV(PL_defgv))[0]);
}

void
pass_original_object(subr)
   SV *subr;
PPCODE:
{
   if (!SvROK(subr) || (subr=SvRV(subr), SvTYPE(subr)!=SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   SvSTASH(subr)=secret_pkg;
   SvREFCNT_inc_simple_void_NN(secret_pkg);
   ++SP;
}

void
mark_as_default(sv)
   SV *sv;
PPCODE:
{
   if (!SvTEMP(sv))
      sv=sv_mortalcopy(sv);
   PUSHs(sv);
   sv_magicext(sv, Nullsv, PERL_MAGIC_ext, 0, (const char*)&secret_pkg, 0);
}

void
is_default(sv)
   SV *sv;
PPCODE:
{
   if (SvTYPE(sv) == SVt_PVMG) {
      MAGIC *mg=SvMAGIC(sv);
      if (mg && mg->mg_type==PERL_MAGIC_ext && mg->mg_ptr==(const char*)&secret_pkg)
         XSRETURN_YES;
   }
   XSRETURN_NO;
}

BOOT:
{
   secret_pkg=gv_stashpv("Polymake::Struct::.secret", TRUE);
   CvSTASH_set(get_cv("Polymake::Struct::method_call", FALSE), secret_pkg);
   CvSTASH_set(get_cv("Polymake::Struct::access_field", FALSE), secret_pkg);
   if (PL_DBgv) {
      CvNODEBUG_on(get_cv("Polymake::Struct::make_body", FALSE));
      CvNODEBUG_on(get_cv("Polymake::Struct::original_object", FALSE));
      CvNODEBUG_on(get_cv("Polymake::Struct::pass_original_object", FALSE));
      CvNODEBUG_on(get_cv("Polymake::Struct::mark_as_default", FALSE));
   }
   def_ck_AASSIGN=PL_check[OP_AASSIGN];
   pm_perl_namespace_register_plugin(aTHX_ catch_ptrs, reset_ptrs, &PL_sv_undef);
}

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
