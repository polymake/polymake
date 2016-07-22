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
OP* pp_reveal_args(pTHX)
{
   dSP;
   I32 items=PTR2IV(*SP);
   *SP=SP[items+1];
   SP+=items;
   RETURN;
}

static
OP* pp_move_lhs_to_rhs(pTHX)
{
   dSP;
   // the value returned from put/put_multi is on the stack top:
   // insert it into the corresponding slot of the rhs list and replace it with undef here
   SP[PL_markstack_ptr[-1]-PL_markstack_ptr[0]]=TOPs;
   SETs(&PL_sv_undef);
   RETURN;
}

MODULE = Polymake::Core::Object                 PACKAGE = Polymake::Core::Object

PROTOTYPES: DISABLE

void
_prop_accessor(self, ...)
   SV *self;
PPCODE:
{
   AV *descr=(AV*)CvXSUBANY(cv).any_ptr;
   SV *prop=AvARRAY(descr)[0], *rhs;
   OP *o=PL_op->op_next;
   I32 hide_args = GIMME_V != G_ARRAY ? 1 : 0;
   I32 assign=0;

   if (o && o->op_type == OP_SASSIGN && !(o->op_private & OPpASSIGN_BACKWARDS)) {
      // setting a property: $this->PROP=value;
      EXTEND(SP, items+3+hide_args);
      rhs=*SP;
      PUSHMARK(SP);
      if (hide_args) {
        *(++SP)=NUM2PTR(SV*, items+2);
        SP[items]=prop;
        SP[items+1]=rhs;
        SP[items+2]=AvARRAY(descr)[2];
        SP[items+3]=self;
      } else {
        SP+=items;
        PUSHs(prop);
        PUSHs(rhs);
        PUSHs(AvARRAY(descr)[2]);
      }
      o->op_ppaddr=&Perl_pp_null;
#if PerlVersion >= 5160
      /* remove the LVALUE flag */
      PL_op->op_private &= ~OPpLVAL_INTRO;
#endif
      assign=OP_SASSIGN;

   } else if ((o=method_named_op(PL_op), o && (o->op_private & MethodIsCalledOnLeftSideOfArrayAssignment))) {
      // setting a property in a list assignment: (..., $this->PROP, ... )=(..., value, ...);
      if (hide_args) Perl_croak(aTHX_ "unexpected scalar context within list assignment");
      EXTEND(SP, items+3);
      // AASSIGN expects two marks: the topmost delimits the lvalues, the next below it - the rvalues
      rhs=SP[PL_markstack_ptr[-1]-PL_markstack_ptr[0]+1];
      PUSHMARK(SP);
      SP+=items;
      PUSHs(prop);
      PUSHs(rhs);
      PUSHs(AvARRAY(descr)[2]);
#if PerlVersion >= 5160
      /* remove the LVALUE flag */
      PL_op->op_private &= ~OPpLVAL_INTRO;
#endif
      assign=OP_AASSIGN;

   } else {
      // retrieving a property
      EXTEND(SP, items+2+hide_args);
      PUSHMARK(SP);
      if (hide_args) {
        *(++SP)=NUM2PTR(SV*, items+1);
        SP[items]=prop;
        SP[items+1]=AvARRAY(descr)[1];
        SP[items+2]=self;
      } else {
        SP+=items;
        PUSHs(prop);
        PUSHs(AvARRAY(descr)[1]);
      }
   }

   // We must repeat OP_ENTERSUB in order to execute the get or put method
   // Depending on context, an auxiliary operation can be added.
   if ((o=cUNOP->op_first)->op_type != OP_CUSTOM) {
      OP* reveal_op=newOP(OP_CUSTOM, 0);
      OP* last_new_op=reveal_op;
      OP* dummy_op=o;

      // we need a dummy operation ponting to the next op to be executed
      if (o->op_type == OP_NULL) {
         o->op_type = OP_CUSTOM;
      } else {
         dummy_op=newOP(OP_CUSTOM, 0);
      }

      reveal_op->op_ppaddr=&pp_reveal_args;
      dummy_op->op_next=reveal_op;

      if (assign) {
         OP* sub_op=newOP(OP_CUSTOM, 0);
         PL_op->op_private &= ~OPpLVAL_INTRO;
         sub_op->op_ppaddr=PL_op->op_ppaddr;
         sub_op->op_flags=PL_op->op_flags & ~OPf_KIDS;
         sub_op->op_private=PL_op->op_private;
         reveal_op->op_next=sub_op;
         if (assign==OP_SASSIGN) {
            // Now we've hidden the arguments for put/put_multi from the current OP_ENTERSUB
            // which would destroy all but the last one because of sacalar context.
            // They must be revealed before put/put_multi is called.
            sub_op->op_next=PL_op->op_next->op_next;  // skip OP_SASSIGN
            last_new_op=sub_op;
         } else {
            // Value returned from put/put_lvalue must be moved from the left to the right hand side of the list assignment
            // TODO: try to recognize list assignments in void context and skip this
            OP* move_op=newOP(OP_CUSTOM, 0);
            move_op->op_ppaddr=&pp_move_lhs_to_rhs;
            sub_op->op_next=move_op;
            move_op->op_next=PL_op->op_next;
            OpMORESIB_set(sub_op, move_op);
            last_new_op=move_op;
         }
         OpMORESIB_set(reveal_op, sub_op);
      } else {
         reveal_op->op_next=PL_op;
      }

      // include new OPs into the tree at places having further siblings (would have to deal with PERL_OP_PARENT otherwise...)
      if (dummy_op==o) {
         OpMORESIB_set(last_new_op, cUNOPo->op_first);
         cUNOPo->op_first=reveal_op;
      } else {
         OpMORESIB_set(last_new_op, o);
         OpMORESIB_set(dummy_op, reveal_op);
         cUNOP->op_first=dummy_op;
      }
      o=dummy_op;
   }
   PL_op= hide_args ? o : o->op_next;
}


void
_get_alternatives(...)
PPCODE:
{
   SV* descend_path= items==1 ? ST(0) : Nullsv;
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;
   while (cx >= cx_bottom) {
      if (CxTYPE(cx)==CXt_SUB && !SkipDebugFrame(cx,0)) {
         OP* o=cx->blk_sub.retop;
         if (o == NULL) break;         // called from call_sv due to some magic: assume no alternatives

         if (!(o->op_type==OP_LEAVESUB ||      // not the last operation in a sub (forwarding from get_multi to get)
               o->op_type==OP_LEAVESUBLV ||
               (o->op_type==OP_LEAVE &&        // in debug mode spurious intermediate operations may appear
                (o->op_next->op_type==OP_LEAVESUB ||
                 o->op_next->op_type==OP_LEAVESUBLV)))) {
            I32 skip=FALSE, push= GIMME_V == G_ARRAY;
#ifdef USE_ITHREADS
            SV** saved_curpad=NULL;
#endif
            OP* nop=o;
            OP* gvop;
            AV* path_av=NULL;
            if (descend_path) {
               while (nop->op_type == OP_METHOD_NAMED && nop->op_next->op_type == OP_ENTERSUB) {
                  if (path_av == NULL) {
                     path_av=newAV();
                     AvREAL_off(path_av);
                     sv_upgrade(descend_path, SVt_RV);
                     SvRV_set(descend_path, (SV*)path_av);
                     SvROK_on(descend_path);
                  }
#ifdef USE_ITHREADS
                  if (!saved_curpad) {
                     saved_curpad=PL_curpad;
                     PL_curpad=pm_perl_get_cx_curpad(aTHX_ cx, cx_bottom);
                  }
#endif
                  av_push(path_av, cSVOPx_sv(nop));
                  nop=nop->op_next->op_next;
               }
            }
            while ((nop->op_type == OP_CONST && (gvop=NULL, nop=nop->op_next)->op_type == OP_BIT_OR) ||
                   (nop->op_type == OP_PUSHMARK && (nop=nop->op_next)->op_type == OP_GV &&
                    (gvop=nop, nop=nop->op_next)->op_type == OP_ENTERSUB && (nop=nop->op_next)->op_type == OP_BIT_OR)) {
               if (push) {
#ifdef USE_ITHREADS
                  if (!saved_curpad) {
                     saved_curpad=PL_curpad;
                     PL_curpad=pm_perl_get_cx_curpad(aTHX_ cx, cx_bottom);
                  }
#endif
                  if (gvop) {
                     GV* cgv=cGVOPx_gv(gvop);
                     XPUSHs(sv_2mortal(newSVpvn(GvNAME(cgv),GvNAMELEN(cgv))));
                  } else {
                     XPUSHs(cSVOPo_sv);
                  }
               }
               skip=TRUE;
               o=nop=nop->op_next;
            }
            if (skip && path_av == NULL) cx->blk_sub.retop=o;
#ifdef USE_ITHREADS
            if (saved_curpad) PL_curpad=saved_curpad;
#endif
            break;
         }
      }
      --cx;
   }
}

void
_expect_array_access()
PPCODE:
{
   PERL_CONTEXT *cx_bottom=cxstack, *cx_top=cx_bottom+cxstack_ix, *cx=cx_top;
   OP *o;
   while (cx >= cx_bottom) {
      if (CxTYPE(cx)==CXt_SUB) {
         if (!SkipDebugFrame(cx,0)) {
            o=cx->blk_sub.retop;
            for (;;) {
               if (!o) {
                  if (cx->blk_gimme==G_ARRAY) XSRETURN_YES;
                  else XSRETURN_NO;
               }
               if (o->op_type == OP_LEAVE) o=o->op_next;
               else break;
            }
            if (o->op_type != OP_LEAVESUB && o->op_type != OP_LEAVESUBLV) {
               if (o->op_type == OP_RV2AV) XSRETURN_YES;
#if PerlVersion >= 5220
               if (o->op_type == OP_MULTIDEREF) XSRETURN_YES;
#endif
               break;
            }
         }
      }
      --cx;
   }
   XSRETURN_NO;
}

MODULE = Polymake::Core::Object                 PACKAGE = Polymake::Core::ObjectType

void
create_prop_accessor(descr, pkg)
   SV* descr;
   SV* pkg;
PPCODE:
{
   SV* sub=newSV(0);
   HV* stash=NULL;
   sv_upgrade(sub, SVt_PVCV);
   CvXSUB(sub)=&XS_Polymake__Core__Object__prop_accessor;
   CvFLAGS(sub)=CvFLAGS(cv) | CVf_ANON | CVf_LVALUE | CVf_METHOD | CVf_NODEBUG;
   if (SvPOK(pkg))
      stash=gv_stashpv(SvPVX(pkg), TRUE);
   else if (SvROK(pkg))
      stash=(HV*)SvRV(pkg);
   CvSTASH_set((CV*)sub, stash);
   CvXSUBANY(sub).any_ptr=SvREFCNT_inc_NN(SvRV(descr));
   PUSHs(sv_2mortal(newRV_noinc(sub)));
}

BOOT:
if (PL_DBgv) {
   CvNODEBUG_on(get_cv("Polymake::Core::Object::_prop_accessor", FALSE));
   CvNODEBUG_on(get_cv("Polymake::Core::Object::_get_alternatives", FALSE));
   CvNODEBUG_on(get_cv("Polymake::Core::Object::_expect_array_access", FALSE));
}

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
