/* Copyright (c) 1997-2021
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

namespace pm { namespace perl { namespace glue {

namespace {

Perl_ppaddr_t def_pp_LEAVE, def_pp_OPEN;

template <typename LocalHandler>
struct local_wrapper {
   static void undo(pTHX_ void* p)
   {
      // make a local copy because the save stack can be reallocated during execution of undo action
      const LocalHandler handler(*reinterpret_cast<LocalHandler*>(PL_savestack + (PL_savestack_ix - PTR2IV(p))));
      handler.undo(aTHX);
   }

   static void* alloc(pTHX)
   {
      const I32 save_ix = PL_savestack_ix;
      (void)SSNEWt(1, LocalHandler);
      save_destructor_x(&undo, NUM2PTR(void*, PL_savestack_ix - save_ix));
      return &(PL_savestack[save_ix]);
   }
};

template <typename LocalHandler, typename... Args>
void local_do(pTHX_ Args&&... args)
{
   new(local_wrapper<LocalHandler>::alloc(aTHX)) LocalHandler(aTHX_ args...);
}

// --------------------

struct local_ref_handler {
   SV* var;
   void* orig_any;
   U32 orig_flags;
   char* orig_pv;       // as a representative for SV_HEAD_UNION
   SV* temp_owner;

   local_ref_handler(pTHX_ SV* var_, SV* value)
      : var(var_)
      , orig_any(SvANY(var_))
      , orig_flags(SvFLAGS(var_) & ~SVs_TEMP)
      , orig_pv(var_->sv_u.svu_pv)
      , temp_owner(value)
   {
      var->sv_u.svu_pv = value->sv_u.svu_pv;
      SvANY(var) = SvANY(value);
      SvFLAGS(var) = SvFLAGS(value) & ~SVs_TEMP;
      SvREFCNT_inc_simple_void_NN(var);
      SvREFCNT_inc_simple_void_NN(value);
   }

   void undo(pTHX) const
   {
      SvANY(var) = orig_any;
      SvFLAGS(temp_owner) = SvFLAGS(var);
      temp_owner->sv_u.svu_pv = var->sv_u.svu_pv;
      var->sv_u.svu_pv = orig_pv;
      SvFLAGS(var) = orig_flags;
      SvREFCNT_dec(var);
      SvREFCNT_dec(temp_owner);
   }
};

}

OP* parse_expression_in_parens(pTHX)
{
   lex_read_space(0);
   if (PL_parser->bufptr == PL_parser->bufend || *PL_parser->bufptr != '(')
      return nullptr;
   lex_read_to(PL_parser->bufptr+1);
   OP* o = parse_termexpr(0);
   if (!o) return nullptr;
   lex_read_space(0);
   if (PL_parser->bufptr == PL_parser->bufend || *PL_parser->bufptr != ')') {
      op_free(o);
      return nullptr;
   }
   lex_read_to(PL_parser->bufptr+1);
   return o;
}

}
namespace ops {

using namespace pm::perl::glue;

OP* local_ref(pTHX)
{
   dSP;
   SV* left = POPs;
   SV* right = GIMME_V != G_VOID ? TOPs : POPs;
   if (!SvROK(right))
      DIE(aTHX_ "local ref value must be a reference");
   SV* value = SvRV(right);
   SV* var;
   switch (SvTYPE(value)) {
   case SVt_PVAV:
      if (SvTYPE(left) == SVt_PVGV) {
         var = (SV*)GvAV(left);
         if (!var || !GvIMPORTED_AV(left))
            DIE(aTHX_ "local ref target array not declared");
         break;
      } else if (SvROK(left)) {
         var = SvRV(left);
         if (SvTYPE(var) == SVt_PVAV) break;
      }
      DIE(aTHX_ "local ref illegal/incompatible arguments: array references expected");
   case SVt_PVHV:
      if (SvTYPE(left) == SVt_PVGV) {
         var = (SV*)GvHV(left);
         if (!var || !GvIMPORTED_HV(left))
            DIE(aTHX_ "local ref target hash not declared");
         break;
      } else if (SvROK(left)) {
         var = SvRV(left);
         if (SvTYPE(var) == SVt_PVHV) break;
      }
      DIE(aTHX_ "local ref illegal/incompatible arguments: hash reference expected");
   case SVt_PVCV:
      if (SvTYPE(left) == SVt_PVGV) {
         var = (SV*)GvCV(left);
         if (!var)
            DIE(aTHX_ "local ref target sub not defined");
         break;
      } else if (SvROK(left)) {
         var = SvRV(left);
         if (SvTYPE(var) == SVt_PVCV) break;
      }
      DIE(aTHX_ "local ref illegal/incompatible arguments: code reference expected");
   default:
      DIE(aTHX_ "local ref unsupported value type: must be an array, hash or code reference");
   }
   local_do<local_ref_handler>(aTHX_ var, value);
   RETURN;
}

}
namespace glue { namespace {

int parse_local_ref(pTHX_ OP** op_ptr)
{
   op_keeper<OP> o(aTHX_ parse_termexpr(0));
   if (!o || o->op_type != OP_SASSIGN)
      return KEYWORD_PLUGIN_DECLINE;
   o->op_ppaddr = ops::local_ref;
   *op_ptr = o.release();
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

struct local_scalar_handler {
   SV* var;
   SV orig;

   local_scalar_handler(pTHX_ SV* var_, SV* value)
      : var(var_)
   {
      orig.sv_any = var->sv_any;
      orig.sv_refcnt = var->sv_refcnt;
      orig.sv_flags = var->sv_flags;
      orig.sv_u.svu_pv = var->sv_u.svu_pv;
      var->sv_any = nullptr;
      var->sv_flags = 0;
      var->sv_refcnt = 1;
      sv_setsv(var, value);
   }

   void undo(pTHX) const
   {
      if (SvREFCNT(var) > 1) {
         SvREFCNT_dec(var);
      } else {
         SvREFCNT(var) = 0;
         sv_clear(var);
      }
      var->sv_any = orig.sv_any;
      var->sv_refcnt = orig.sv_refcnt;
      var->sv_flags = orig.sv_flags;
      var->sv_u.svu_pv = orig.sv_u.svu_pv;
   }
};

OP* local_scalar_op(pTHX)
{
   dSP;
   SV* left = POPs;
   SV* right = GIMME_V != G_VOID ? TOPs : POPs;
   local_do<local_scalar_handler>(aTHX_ left, right);
   RETURN;
}

// --------------------

} }
namespace ops {

void
localize_scalar(pTHX_ SV* var, SV* value)
{
   local_do<local_scalar_handler>(aTHX_ var, value);
}

void
localize_scalar(pTHX_ SV* var)
{
   localize_scalar(aTHX_ var, sv_mortalcopy(var));
}

}
namespace glue { namespace {

OP* local_save_scalar_op(pTHX)
{
   dSP;
   SV* var = GIMME_V != G_VOID ? TOPs : POPs;
   ops::localize_scalar(aTHX_ var);
   RETURN;
}

// --------------------

struct local_incr_handler {
   SV* var;
   IV incr;

   local_incr_handler(pTHX_ SV* var_, IV incr_)
      : var(var_)
      , incr(incr_) {}

   void undo(pTHX) const
   {
      if (SvIOK(var))
         sv_setiv(var, SvIVX(var) - incr);
      else if (SvNOK(var))
         sv_setnv(var, SvNVX(var) - NV(incr));
      else
         Perl_croak(aTHX_ "undoing local increment: variable is no more numerical");
   }
}; 

OP* local_incr_op(pTHX)
{
   dSP;
   SV* var = GIMME_V != G_VOID ? TOPs : POPs;
   local_do<local_incr_handler>(aTHX_ var, 1 - PL_op->op_private);
   RETURN;
}

int parse_local_scalar(pTHX_ OP** op_ptr)
{
   op_keeper<OP> o(aTHX_ parse_termexpr(0));
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   if (o->op_type == OP_SASSIGN) {
      OP* left = ((BINOP*)o.operator->())->op_last;
      if (left->op_type != OP_PADSV && left->op_type != OP_ENTERSUB && left->op_type != OP_RV2SV) {
         report_parse_error("local scalar applicable to lexical variables, scalars delivered by dereferencing or returned from subs");
         return KEYWORD_PLUGIN_DECLINE;
      }
      o->op_ppaddr = local_scalar_op;
   } else {
      OP* var = o.release();
      switch (var->op_type) {
      case OP_PREINC:
      case OP_I_PREINC:
         o = PmNewCustomOP(UNOP, 0, var);
         o->op_ppaddr = local_incr_op;
         o->op_private = 0;
         break;
      case OP_PREDEC:
      case OP_I_PREDEC:
         o = PmNewCustomOP(UNOP, 0, var);
         o->op_ppaddr = local_incr_op;
         o->op_private = 2;
         break;
      case OP_POSTINC:
      case OP_I_POSTINC:
         report_parse_error("local scalar not compatible with post-increment");
         return KEYWORD_PLUGIN_DECLINE;
      case OP_POSTDEC:
      case OP_I_POSTDEC:
         report_parse_error("local scalar not compatible with post-decrement");
         return KEYWORD_PLUGIN_DECLINE;
      default:
         o = PmNewCustomOP(UNOP, 0, op_lvalue(var, var->op_type));
         o->op_ppaddr = local_save_scalar_op;
         break;
      }
   }
   *op_ptr = o.release();
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

struct local_push_unshift_handler {
   AV* av;
   IV n;

   local_push_unshift_handler(AV* av_, IV n_)
      : av(av_)
      , n(n_) {}

   void insert_elems(pTHX_ SV* const * src, SV** dst)
   {
      for (SV* const * const src_end = src + n; src < src_end; ++src, ++dst) {
         SV* d = *src;
         if (SvREADONLY(d) || !SvTEMP(d))
            *dst = newSVsv(d);
         else
            *dst = SvREFCNT_inc_simple_NN(d);
      }
      AvFILLp(av) += n;
   }
};

struct local_push_handler : local_push_unshift_handler {

   local_push_handler(pTHX_ AV* av_, SV* const * src, IV n_)
      : local_push_unshift_handler(av_, n_)
   {
      av_extend(av, AvFILLp(av) + n);
      insert_elems(aTHX_ src, AvARRAY(av)+AvFILLp(av)+1);
   }

   void undo(pTHX) const
   {
      for (SV **e = AvARRAY(av) + AvFILLp(av), **stop = e-n; e > stop; --e) {
         SvREFCNT_dec(*e);
         *e = PmEmptyArraySlot;
      }
      AvFILLp(av) -= n;
   }
};

struct local_unshift_handler : local_push_unshift_handler {

   local_unshift_handler(pTHX_ AV* av_, SV* const * src, IV n_)
      : local_push_unshift_handler(av_, n_)
   {
      av_extend(av, AvFILLp(av)+n);
      SV** dst = AvARRAY(av);
      Move(dst, dst + n_, AvFILLp(av)+1, SV*);
      insert_elems(aTHX_ src, dst);
   }

   void undo(pTHX) const
   {
      SV **e, **stop;
      for (stop = AvARRAY(av)-1, e = stop + n; e > stop; --e)
         SvREFCNT_dec(*e);
      AvFILLp(av) -= n;
      ++stop;
      Move(stop + n, stop, AvFILLp(av)+1, SV*);
      for (e = stop + AvFILLp(av)+1, stop = e+n; e < stop; ++e)
         *e = PmEmptyArraySlot;
   }
};

template <bool is_unshift>
OP* local_push_unshift_op(pTHX)
{
   dSP;  dMARK;  dORIGMARK;
   AV* av = (AV*)*++MARK;
   IV n = SP - MARK;
   if (n > 0)
      local_do<std::conditional_t<is_unshift, local_unshift_handler, local_push_handler>>(aTHX_ av, MARK+1, n);
   SP = ORIGMARK;
   RETURN;
}

template <bool is_unshift>
int parse_local_push_unshift(pTHX_ OP** op_ptr)
{
   OP* o = parse_termexpr(0);
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   o->op_ppaddr = local_push_unshift_op<is_unshift>;
   *op_ptr = o;
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

struct local_pop_handler {
   AV* av;
   SV* val;

   local_pop_handler(pTHX_ AV* av_)
      : av((AV*)SvREFCNT_inc_simple_NN(av_))
   {
      val = av_pop(av);
   }

   void undo(pTHX) const
   {
      auto localizing = PL_localizing;
      PL_localizing = 2;
      av_push(av, val);
      PL_localizing = localizing;
      SvREFCNT_dec(av);
   }
};

OP* local_pop_op(pTHX)
{
   dSP;
   AV* av = (AV*)POPs;
   SV* ret = nullptr;
   if (AvFILLp(av) >= 0) {
      ret = AvARRAY(av)[AvFILLp(av)];
      local_do<local_pop_handler>(aTHX_ av);
   }
   if (GIMME_V != G_VOID) {
      if (!ret) ret = &PL_sv_undef;
      PUSHs(ret);
   }
   RETURN;
}

int parse_local_pop(pTHX_ OP** op_ptr)
{
   OP* o = parse_termexpr(0);
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   o->op_ppaddr = local_pop_op;
   o->op_private |= OPpLVAL_INTRO;
   *op_ptr = o;
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

struct local_shift_handler {
   AV* av;
   SV* val;

   local_shift_handler(pTHX_ AV* av_)
      : av(av_)
   {
      SvREFCNT_inc_simple_void_NN(av);
      val = av_shift(av);
   }

   void undo(pTHX) const
   {
      auto localizing = PL_localizing;
      PL_localizing = 2;
      av_unshift(av, 1);
      PL_localizing = localizing;
      AvARRAY(av)[0] = val;
      SvREFCNT_dec(av);
   }
};

OP* local_shift_op(pTHX)
{
   dSP;
   AV* av = (AV*)POPs;
   SV* ret = nullptr;
   if (AvFILLp(av) >= 0) {
      ret = AvARRAY(av)[0];
      local_do<local_shift_handler>(aTHX_ av);
   }
   if (GIMME_V != G_VOID) {
      if (!ret) ret = &PL_sv_undef;
      PUSHs(ret);
   }
   RETURN;
}

int parse_local_shift(pTHX_ OP** op_ptr)
{
   OP* o = parse_termexpr(0);
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   o->op_ppaddr = local_shift_op;
   o->op_private |= OPpLVAL_INTRO;
   *op_ptr = o;
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

struct local_splice_handler {
   AV* av;
   IV stretch;

   local_splice_handler(pTHX_ AV* av_, IV first, IV size)
      : av(av_)
   {
      SvREFCNT_inc_simple_void_NN(av);
      AvFILLp(av) -= size;
      if (first == 0) {
         AvARRAY(av) += size;
         stretch = size;
      } else {
         stretch = -size;
      }
   }

   void undo(pTHX) const
   {
      if (stretch > 0) {
         AvARRAY(av) -= stretch;
         AvFILLp(av) += stretch;
      } else {
         AvFILLp(av) -= stretch;
      }
      SvREFCNT_dec(av);
   }
};

OP* local_splice_op(pTHX)
{
   dSP;  dMARK;  dORIGMARK;
   AV* av = (AV*)*++MARK;
   if (MARK+2 < SP)
      DIE(aTHX_ "unsupported local splice with insertion");
   const IV len = AvFILLp(av)+1;
   IV first, size;
   if (MARK < SP) {
      ++MARK;
      first = SvIV(*MARK);
      if (first < 0) {
         first += len;
         if (first < 0)
            DIE(aTHX_ "local splice start index too low");
      } else if (first > len) {
         first = len;
      }
      if (MARK < SP) {
         if (first != 0)
            DIE(aTHX_ "unsupported local splice in the middle");
         ++MARK;
         size = SvIV(*MARK);
         if (size < 0) {
            size += len;
            if (size < 0)
               DIE(aTHX_ "local splice size too low");
         } else if (size > len) {
            DIE(aTHX_ "local splice size too high");
         }
      } else {
         size = len - first;
      }
   } else {
      first = 0;
      size = len;
   }

   SP = ORIGMARK;
   if (size != 0) {
      if (GIMME_V == G_ARRAY) {
         EXTEND(SP, size);
         Copy(AvARRAY(av) + first, SP+1, size, SV*);
         SP += size;
      }
      local_do<local_splice_handler>(aTHX_ av, first, size);
   }
   RETURN;
}

int parse_local_splice(pTHX_ OP** op_ptr)
{
   OP* o = parse_termexpr(0);
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   o->op_ppaddr = local_splice_op;
   *op_ptr = o;
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

struct local_swap_handler {
   AV* av;
   IV ix1, ix2;

   local_swap_handler(pTHX_ AV* av_, IV ix1_, IV ix2_)
      : av(av_)
      , ix1(ix1_)
      , ix2(ix2_)
   {
      SvREFCNT_inc_simple_void_NN(av);
      std::swap(AvARRAY(av)[ix1], AvARRAY(av)[ix2]);
   }

   void undo(pTHX) const
   {
      std::swap(AvARRAY(av)[ix1], AvARRAY(av)[ix2]);
      SvREFCNT_dec(av);
   }
};

OP* local_swap_op(pTHX)
{
   dSP;
   IV ix2 = POPi;
   IV ix1 = POPi;
   AV* av = (AV*)POPs;
   if (ix1 < 0) ix1 += AvFILL(av)+1;
   if (ix2 < 0) ix2 += AvFILL(av)+1;
   if (ix1 > AvFILL(av) || ix2 > AvFILL(av)) DIE(aTHX_ "local swap: index out of range");
   local_do<local_swap_handler>(aTHX_ av, ix1, ix2);
   RETURN;
}

int parse_local_swap(pTHX_ OP** op_ptr)
{
   op_keeper<OP> o(aTHX_ parse_listexpr(0));
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   o->op_ppaddr = local_swap_op;
   o->op_type = OP_CUSTOM;
   LISTOP* lo = (LISTOP*)o.operator->();
   OP* pushmark = lo->op_first;
   if (pushmark->op_type != OP_PUSHMARK || !OpHAS_SIBLING(pushmark))
      return KEYWORD_PLUGIN_DECLINE;
   OP* avop = OpSIBLING(pushmark);
   if (avop->op_type != OP_RV2AV && avop->op_type != OP_PADAV || !OpHAS_SIBLING(avop)) {
      report_parse_error("expected: local swap @array, index1, index2");
      return KEYWORD_PLUGIN_DECLINE;
   }
   OP* ix1op = OpSIBLING(avop);
   if (!OpHAS_SIBLING(ix1op)) {
      report_parse_error("expected: local swap @array, index1, index2");
      return KEYWORD_PLUGIN_DECLINE;
   }
   OP* ix2op = OpSIBLING(ix1op);
   if (OpHAS_SIBLING(ix2op)) {
      report_parse_error("expected: local swap @array, index1, index2");
      return KEYWORD_PLUGIN_DECLINE;
   }
   
   lo->op_first = doref(avop, OP_NULL, TRUE);
   op_free(pushmark);
   *op_ptr = o.release();
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

struct local_bless_handler {
   SV* var;
   HV* old_stash;
   I32 mg_flags;

   local_bless_handler(pTHX_ SV* ref, HV* stash)
      : var(SvRV(ref))
   {
      SvREFCNT_inc_simple_void_NN(var);
      old_stash = (HV*)SvREFCNT_inc_NN(SvSTASH(var));
      mg_flags = SvFLAGS(var) & (SVs_GMG | SVs_SMG | SVs_RMG | SVf_AMAGIC);
      sv_bless(ref, stash);
   }

   void undo(pTHX) const
   {
      HV* stash = SvSTASH(var);
      SvSTASH_set(var, old_stash);
      SvFLAGS(var) &= ~(SVs_GMG | SVs_SMG | SVs_RMG | SVf_AMAGIC);
      SvFLAGS(var) |= mg_flags;
      SvREFCNT_dec(var);
      SvREFCNT_dec(stash);
   }
};

OP* local_bless_op(pTHX)
{
   dSP;
   HV* stash = MAXARG == 1 ? CopSTASH(PL_curcop) : gv_stashsv(POPs, GV_NOADD_NOINIT);
   SV* sv = TOPs;
   if (!SvROK(sv) || !SvOBJECT(SvRV(sv)))
      DIE(aTHX_ "local bless applied to a non-object");
   local_do<local_bless_handler>(aTHX_ sv, stash);
   RETURN;
}

int parse_local_bless(pTHX_ OP** op_ptr)
{
   OP* o = parse_termexpr(0);
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   o->op_ppaddr = local_bless_op;
   *op_ptr = o;
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

#if PerlVersion >= 5240
# define PmCxSaveStackIndex(cx) (cx->blk_oldsaveix)
#else
# define PmCxSaveStackIndex(cx) (PL_scopestack[cx->blk_oldscopesp-1])
#endif

bool save_localizations(pTHX_ I32& start, I32& end)
{
   PERL_CONTEXT* cx = cxstack + cxstack_ix;
   assert(CxTYPE(cx) == CXt_BLOCK);

   const I32 save_start = PmCxSaveStackIndex(cx);
   const I32 save_end = PL_savestack_ix;
   I32 last_kept = save_end;
   I32 tmp_save_end = -1;

   while (PL_savestack_ix > save_start) {
      const UV save_code = PL_savestack[PL_savestack_ix-1].any_uv & SAVE_MASK;
      int num_words = 0;
      // all kinds of save operations generated for localizing values
      switch (save_code) {
      case SAVEt_GP:
      case SAVEt_GVSV:
      case SAVEt_GENERIC_SVREF:
      case SAVEt_SV:
      case SAVEt_AV:
      case SAVEt_HV:
      case SAVEt_ADELETE:
         num_words = 3;
         break;
      case SAVEt_AELEM:
      case SAVEt_HELEM:
      case SAVEt_DELETE:
#if PerlVersion >= 5180
      case SAVEt_GVSLOT:
#endif
         num_words = 4;
         break;
      case SAVEt_DESTRUCTOR_X:
         num_words = 3;
         if (PL_savestack_ix-4 > save_start) {
            const UV next_item = PL_savestack[PL_savestack_ix-4].any_uv;
            if ((next_item & SAVE_MASK) == SAVEt_ALLOC)
               num_words += 1 + int(next_item >> SAVE_TIGHT_SHIFT);
         }
         break;
      }
      if (num_words == 0) {
         // restoring action not related to localization: execute immediately
         // before that, rescue other actions onto mortals stack, otherwise they could be overwritten if this action triggers other localizations
         if (tmp_save_end < 0 && last_kept != save_end) {
            EXTEND_MORTAL(save_end - save_start);
            PL_tmps_ix += save_end - save_start;
            tmp_save_end = I32(PL_tmps_ix);
            const I32 tmp_save_start = tmp_save_end - (save_end - last_kept);
            Copy(&(PL_savestack[last_kept]), &(PL_tmps_stack[tmp_save_start]), save_end - last_kept, ANY);
            last_kept = tmp_save_start;
         }
         leave_scope(PL_savestack_ix-1);
      } else {
         PL_savestack_ix -= num_words;
         last_kept -= num_words;
         if (tmp_save_end >= 0) // collecting the items on mortals stack
            Copy(&(PL_savestack[PL_savestack_ix]), &(PL_tmps_stack[last_kept]), num_words, ANY);
      }
   }
   assert(PL_savestack_ix == save_start);
   start = last_kept;
   if (tmp_save_end >= 0) {
      end = tmp_save_end;
      PL_tmps_ix -= save_end - save_start;
      return true;
   } else {
      end = save_end;
      return false;
   }
}

struct local_magic_t : MAGIC {
   ANY locals[1];
};

int undo_saved_locals(pTHX_ SV* sv, MAGIC* mg)
{
   const local_magic_t* lmg = static_cast<local_magic_t*>(mg);
   const I32 save_start = PL_savestack_ix;
   const I32 num_saved = I32(lmg->mg_len);
   SSGROW(num_saved);
   Copy(&(lmg->locals[0]), &(PL_savestack[save_start]), num_saved, ANY);
   PL_savestack_ix += num_saved;
   leave_scope(save_start);
   return 0;
}

const MGVTBL local_magic_vtbl = { 0, 0, 0, 0, &undo_saved_locals };

OP* leave_local_block_op(pTHX)
{
   dSP;
   dPOPss;
   I32 save_start, save_end;
   const bool moved_to_mortals = save_localizations(aTHX_ save_start, save_end);
   const I32 num_saved = save_end - save_start;
   if (num_saved > 0) {
      if (SvTYPE(sv) == SVt_NULL) {
         (void)SvUPGRADE(sv, SVt_PVMG);
      } else if (SvOK(sv) || SvTYPE(sv) != SVt_PVMG) {
         DIE(aTHX_ "local with: wrong storage value");
      }
      const size_t mgsz = sizeof(local_magic_t)+(num_saved-1)*sizeof(ANY);
      char* mg_raw;
      Newxz(mg_raw, mgsz, char);
      local_magic_t* lmg = reinterpret_cast<local_magic_t*>(mg_raw);
      lmg->mg_type = PERL_MAGIC_ext;
      lmg->mg_virtual = const_cast<MGVTBL*>(&local_magic_vtbl);
      lmg->mg_len = num_saved;
      lmg->mg_moremagic = SvMAGIC(sv);
      SvMAGIC_set(sv, lmg);
      SvRMAGICAL_on(sv);
      if (moved_to_mortals)
         Copy(&(PL_tmps_stack[save_start]), &(lmg->locals[0]), num_saved, ANY);
      else
         Copy(&(PL_savestack[save_start]), &(lmg->locals[0]), num_saved, ANY);
   }
   PUTBACK;
   return def_pp_LEAVE(aTHX);
}

int parse_local_block(pTHX_ OP** op_ptr)
{
   op_keeper<OP> scope(aTHX_ parse_expression_in_parens(aTHX));
   if (!scope) {
      report_parse_error("expected: local with(EXPR) { BLOCK }");
      return KEYWORD_PLUGIN_DECLINE;
   }
   lex_read_space(0);
   if (PL_parser->bufptr == PL_parser->bufend || *PL_parser->bufptr != '{') {
      report_parse_error("expected: local with(EXPR) { BLOCK }");
      return KEYWORD_PLUGIN_DECLINE;
   }
   op_keeper<OP> b(aTHX_ parse_block(0));
   if (!b || b->op_type != OP_LINESEQ)
      return KEYWORD_PLUGIN_DECLINE;
   OP* o = op_prepend_elem(OP_LINESEQ, newOP(OP_ENTER, 0), b.release());
   o = op_append_elem(OP_LINESEQ, o, op_lvalue(scope.release(), OP_SASSIGN));
   o->op_ppaddr = leave_local_block_op;
   o->op_type = OP_CUSTOM;
   *op_ptr = o;
   return KEYWORD_PLUGIN_STMT;
}

// --------------------

OP* leave_local_if_op(pTHX)
{
   I32 save_start, save_end;
   const bool moved_to_mortals = save_localizations(aTHX_ save_start, save_end);
   OP* ret = def_pp_LEAVE(aTHX);
   const I32 num_words = save_end - save_start;
   if (num_words != 0) {
      if (moved_to_mortals)
         Copy(&(PL_tmps_stack[save_start]), &(PL_savestack[PL_savestack_ix]), num_words, ANY);
      PL_savestack_ix += num_words;
   }
   return ret;
}

int parse_local_if(pTHX_ OP** op_ptr)
{
   PL_ppaddr[OP_LEAVE] = leave_local_if_op;
   OP* o = parse_barestmt(0);
   PL_ppaddr[OP_LEAVE] = def_pp_LEAVE;
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   *op_ptr = o;
   return KEYWORD_PLUGIN_STMT;
}

// --------------------

OP* local_caller_op(pTHX)
{
   dSP;
   dPOPss;
   OP* op_next_state = (OP*)PL_curcop;
   while ((op_next_state = OpSIBLING(op_next_state)) && op_next_state->op_type != OP_NEXTSTATE && op_next_state->op_type != OP_DBSTATE) ;
   if (op_next_state) {
      HV* src_stash = nullptr;
      if (SvPOK(sv)) {
         src_stash = gv_stashsv(sv, GV_ADD);
         if (GIMME_V != G_VOID) PUSHs(sv_2mortal(newRV((SV*)src_stash)));
      } else if (!SvROK(sv) || (src_stash = (HV*)SvRV(sv), SvTYPE(src_stash) != SVt_PVHV)) {
         DIE(aTHX_ "invalid package specified in local caller");
      } else if (GIMME_V != G_VOID) {
         ++SP;
      }
#if PerlVersion >= 5180 || !defined(USE_ITHREADS)
      HV** stashp = &CopSTASH((COP*)op_next_state);
      save_hptr(stashp);
      *stashp = src_stash;
#else
      char** stashnamep = &CopSTASHPV((COP*)op_next_state);
      save_pptr(stashnamep);
      *stashnamep = HvNAME(src_stash);
      I32* stashlenp = &CopSTASH_len((COP*)op_next_state);
      save_I32(stashlenp);
      *stashlenp = HvNAMELEN(src_stash);
#endif
   }
   RETURN;
}

int parse_local_caller(pTHX_ OP** op_ptr)
{
   OP* expr = parse_termexpr(0);
   if (!expr) return KEYWORD_PLUGIN_DECLINE;
   OP* o = PmNewCustomOP(UNOP, 0, Perl_scalar(aTHX_ expr));
   o->op_ppaddr = local_caller_op;
   *op_ptr = o;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

struct local_saveio_handler {
   GV* gv;
   GV* saved;

   local_saveio_handler(pTHX_ GV* gv_)
      : gv((GV*)SvREFCNT_inc_simple_NN(gv_))
   {
      if (GvIOp(gv)) {
         saved = (GV*)newSV(0);
         gv_init(saved, nullptr, "__ANONIO__", 10, 0);
         if (do_openn(saved, ">&=", 3, FALSE, 0, 0, nullptr, (SV**)&gv, 1)) {
            do_close(gv, FALSE);
         } else {
            SvREFCNT_dec(saved);
            saved = nullptr;
         }
      } else {
         saved = nullptr;
      }
   }

   void undo (pTHX) const
   {
      if (GvIOp(gv)) do_close(gv, FALSE);
      if (saved) {
         (void)do_openn(gv, ">&=", 3, FALSE, 0, 0, nullptr, (SV**)&saved, 1);
         SvREFCNT_dec(saved);
      }
      SvREFCNT_dec(gv);
   }
};

OP* local_close_op(pTHX)
{
   dSP;
   dPOPss;
   if (SvTYPE(sv) != SVt_PVGV)
      DIE(aTHX_ "not an IO handle in local close");
   local_do<local_saveio_handler>(aTHX_ (GV*)sv);
   RETURN;
}

OP* local_open_op(pTHX)
{
   SV* sv = PL_stack_base[TOPMARK+1];
   if (SvTYPE(sv) != SVt_PVGV)
      DIE(aTHX_ "not an IO handle in local open");
   local_do<local_saveio_handler>(aTHX_ (GV*)sv);
   return def_pp_OPEN(aTHX);
}

int parse_local_open_close(pTHX_ OP** op_ptr, OP* (*ppaddr)(pTHX))
{
   OP* o = parse_termexpr(0);
   if (!o) return KEYWORD_PLUGIN_DECLINE;
   o->op_ppaddr = ppaddr;
   *op_ptr = o;
   PL_hints |= HINT_BLOCK_SCOPE;
   return KEYWORD_PLUGIN_EXPR;
}

// --------------------

bool following_keyword(pTHX_ const AnyString& kw, bool skip_it = false)
{
   if (PL_parser->bufptr + kw.len < PL_parser->bufend
       && !strncmp(PL_parser->bufptr, kw.ptr, kw.len)
       && !isALNUM(PL_parser->bufptr[kw.len])) {
      if (skip_it)
         lex_read_to(PL_parser->bufptr + kw.len);
      return true;
   }
   return false;
}

#if defined(POLYMAKE_GATHER_CODE_COVERAGE)
int prevent_unnecessary_scope(pTHX_ int (*parse_func)(pTHX_ OP**), OP** op_ptr)
{
   const auto noopt = PERLDB_NOOPT;
   PL_perldb &= ~PERLDBf_NOOPT;
   const int result = parse_func(aTHX_ op_ptr);
   PL_perldb |= noopt;
   return result;
}
#else
int prevent_unnecessary_scope(pTHX_ int (*parse_func)(pTHX_ OP**), OP** op_ptr)
{
   return parse_func(aTHX_ op_ptr);
}
#endif

}

int parse_enhanced_local(pTHX_ OP** op_ptr)
{
   lex_read_space(0);
   if (PL_parser->bufptr == PL_parser->bufend)
      return KEYWORD_PLUGIN_DECLINE;
   switch (*PL_parser->bufptr) {
   case 'b':
      if (following_keyword(aTHX_ "bless"))
         return parse_local_bless(aTHX_ op_ptr);
      break;
   case 'c':
      if (following_keyword(aTHX_ "caller", true))
         return parse_local_caller(aTHX_ op_ptr);
      if (following_keyword(aTHX_ "close"))
         return parse_local_open_close(aTHX_ op_ptr, local_close_op);
      break;
   case 'i':
      if (following_keyword(aTHX_ "if"))
         return prevent_unnecessary_scope(aTHX_ &parse_local_if, op_ptr);
      if (following_keyword(aTHX_ "interrupts", true))
         return parse_interrupts_op(aTHX_ true, op_ptr);
      break;
   case 'o':
      if (following_keyword(aTHX_ "open"))
         return parse_local_open_close(aTHX_ op_ptr, local_open_op);
      break;
   case 'p':
      if (following_keyword(aTHX_ "pop"))
         return parse_local_pop(aTHX_ op_ptr);
      if (following_keyword(aTHX_ "push"))
         return parse_local_push_unshift<false>(aTHX_ op_ptr);
      break;
   case 'r':
      if (following_keyword(aTHX_ "ref", true))
         return parse_local_ref(aTHX_ op_ptr);
      break;
   case 's':
      if (following_keyword(aTHX_ "scalar", true))
         return parse_local_scalar(aTHX_ op_ptr);
      if (following_keyword(aTHX_ "shift"))
         return parse_local_shift(aTHX_ op_ptr);
      if (following_keyword(aTHX_ "splice"))
         return parse_local_splice(aTHX_ op_ptr);
      if (following_keyword(aTHX_ "swap", true))
         return parse_local_swap(aTHX_ op_ptr);
      break;
   case 'u':
      if (following_keyword(aTHX_ "unshift"))
         return parse_local_push_unshift<true>(aTHX_ op_ptr);
      break;
   case 'w':
      if (following_keyword(aTHX_ "with", true))
         return prevent_unnecessary_scope(aTHX_ &parse_local_block, op_ptr);
      break;
   }
   return KEYWORD_PLUGIN_DECLINE;
}

}
namespace ops {

void init_globals(pTHX)
{
   def_pp_LEAVE = PL_ppaddr[OP_LEAVE];
   def_pp_OPEN  = PL_ppaddr[OP_OPEN];
}

} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
