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

struct ToRestore;

static Perl_check_t def_ck_CONST, def_ck_ENTERSUB, def_ck_LEAVESUB, def_ck_LEAVEEVAL, def_ck_GLOB, def_ck_READLINE,
                    def_ck_GV, def_ck_RV2SV, def_ck_RV2AV, def_ck_RV2HV, def_ck_ANONCODE;
static Perl_ppaddr_t def_pp_GV, def_pp_GVSV, def_pp_AELEMFAST, def_pp_PADAV, def_pp_SPLIT, def_pp_LEAVESUB, def_pp_ANONCODE,
                     def_pp_ENTEREVAL, def_pp_REGCOMP, def_pp_RV2GV, def_pp_NEXTSTATE, def_pp_DBSTATE, def_pp_ANONLIST, def_pp_SASSIGN;

#if PerlVersion >= 5220
static Perl_ppaddr_t def_pp_MULTIDEREF;
#endif

#if defined(PMCollectCoverage)
typedef void (*peep_fun_ptr)(pTHX_ OP*);
static peep_fun_ptr def_peep;
static HV* cov_stats=NULL;
static FILE* covfile=NULL;
#endif

#define LexCtxAutodeclare    0x80000000
#define LexCtxAllowReDeclare 0x40000000
#define LexCtxIndex          0x3fffffff

// compilation state to be saved during BEGIN processing
typedef struct ToRestore {
   ANY saved[3];
   struct ToRestore *begin;
   CV* cv;
   int cur_lex_imp, cur_lex_flags;
   int beginav_fill;
   I32 replaced, old_state, hints;
} ToRestore;

static AV *lexical_imports, *plugin_data;
static SV *plugin_code;
static int cur_lexical_import_ix=-1, cur_lexical_flags=0;
static int shadow_stash_cnt=0;
static HV *last_stash, *last_pkgLOOKUP;
static AV* last_dotLOOKUP;
static CV* declare_cv;
static ToRestore* active_begin=NULL;
static const char instanceof[]="instanceof";
static SV *dot_lookup_key, *dot_import_key, *dot_autolookup_key, *dot_subst_op_key, *dot_subs_key, *declare_key, *dot_dummy_pkg_key;
static SV *lex_imp_key, *sub_type_params_key, *scope_type_params_key, *anon_lvalue_key;
static SV *iv_hint, *uv_hint;
static const char TypeExpression_pkg[]="namespaces::TypeExpression";
static const char BeginAV_pkg[]="namespaces::BeginAV";
static HV *TypeExpression_stash, *args_lookup_stash, *special_imports;
static OP *last_typeof_arg=NULL;
static OP forw_decl_op;
static AV *type_param_names;

// TRUE if namespace mode active
int current_mode() { return PL_ppaddr[OP_GV] != def_pp_GV; }

static
void catch_ptrs(pTHX_ void* to_restore);

static
void reset_ptrs(pTHX_ void* to_restore);

static
int reset_ptrs_via_magic(pTHX_ SV* sv, MAGIC* mg);

static MGVTBL restore_holder_vtbl={ 0, 0, 0, 0, &reset_ptrs_via_magic };
static MGVTBL explicit_typelist_vtbl={ 0, 0, 0, 0, 0 };

static
OP* intercept_pp_gv(pTHX);

static
void establish_lex_imp_ix(pTHX_ int new_ix, int new_mode);

static
int clear_imported_flag(pTHX_ SV* sv, MAGIC* mg);

static MGVTBL clear_imported_flag_vtab = { 0, 0, 0, 0, &clear_imported_flag };

static inline
void set_lexical_scope_hint(pTHX)
{
   int new_hint=cur_lexical_flags | cur_lexical_import_ix;
   MAGIC hint_mg;
   hint_mg.mg_ptr=(char*)lex_imp_key;
   hint_mg.mg_len=HEf_SVKEY;
   if (new_hint) {
      SvIVX(iv_hint)= new_hint;
      Perl_magic_sethint(aTHX_ iv_hint, &hint_mg);
   } else {
      Perl_magic_clearhint(aTHX_ &PL_sv_undef, &hint_mg);
   }
}

static inline
ToRestore* newToRestore(pTHX_ int old_state)
{
   ToRestore *to_restore;
   New(0, to_restore, 1, ToRestore);
   to_restore->begin=active_begin;
   to_restore->beginav_fill=AvFILL(PL_beginav_save);
   to_restore->old_state=old_state;
   to_restore->hints=PL_hints;
   to_restore->cur_lex_imp=cur_lexical_import_ix;
   to_restore->cur_lex_flags=cur_lexical_flags;
   to_restore->replaced=0;
   return to_restore;
}

static
void finish_undo(pTHX_ ToRestore *to_restore)
{
   if (to_restore->replaced) {
      memcpy(PL_savestack+PL_savestack_ix, to_restore->saved, to_restore->replaced * sizeof(to_restore->saved[0]));
      PL_savestack_ix += to_restore->replaced;
   }
   cur_lexical_import_ix=to_restore->cur_lex_imp;
   cur_lexical_flags=to_restore->cur_lex_flags;
   if (to_restore->old_state) {
      while (AvFILL(PL_beginav_save) > to_restore->beginav_fill) {
         SV* begin_cv=av_pop(PL_beginav_save);
         SAVEFREESV(begin_cv);
      }
      PL_hints &= ~HINT_STRICT_VARS;
      if (cur_lexical_import_ix != to_restore->cur_lex_imp)
         set_lexical_scope_hint(aTHX);
   } else {
      PL_hints |= to_restore->hints & HINT_STRICT_VARS;
   }
   active_begin=to_restore->begin;
   Safefree(to_restore);
}

static
PERL_CONTEXT* find_undo_level(pTHX_ int skip_frames)
{
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;
   while (skip_frames--) {
      int t;
      do { t=CxTYPE(cx); --cx; } while (t!=CXt_SUB);
      assert(cx>=cx_bottom);
      if (pm_perl_skip_debug_cx) {
         while (CxTYPE(cx) != CXt_SUB || CvSTASH(cx->blk_sub.cv) == PL_debstash) {
            --cx;
            assert(cx>=cx_bottom);
         }
      }
   }
   if (CxTYPE(cx) == CXt_SUB && CvSPECIAL(cx->blk_sub.cv)) {
      for (;;) {
         --cx;
         assert(cx>=cx_bottom);
         switch (CxTYPE(cx)) {
         case CXt_BLOCK:
            if (pm_perl_skip_debug_cx) {
               COP *cop=cx->blk_oldcop;
               if (CopSTASH_eq(cop, PL_debstash))
                  continue;
            }
            break;
         case CXt_SUB:
            if (pm_perl_skip_debug_cx && CvSTASH(cx->blk_sub.cv) == PL_debstash)
               continue;
            break;
         case CXt_EVAL:
            if (cx == cx_bottom) {
               if (PL_curstackinfo->si_type == PERLSI_MAIN) {
                  /* perl < 5.20: reached the outermost scope in the main script */
                  return NULL;
               } else {
                  /* perl >= 5.20: "require" is handled in an own stack environment */
                  PERL_SI* prev_si=PL_curstackinfo->si_prev;
                  assert(prev_si != NULL);
                  return prev_si->si_cxix >= 0 ? prev_si->si_cxstack + prev_si->si_cxix : NULL;
               }
            }
            return cx-1;
         }
         break;
      }
   }
   Perl_croak(aTHX_ "namespaces::{un,}import may not be used directly; write 'use namespaces' or 'no namespaces' instead");
   /* UNREACHABLE */
   return NULL;
}

static
void insert_undo(pTHX_ int skip_frames)
{
   ANY* saves;
   PERL_CONTEXT* cx=find_undo_level(aTHX_ skip_frames);
   ToRestore* to_restore=newToRestore(aTHX_ FALSE);

   if (cx != NULL) {
      /* There is a useful ENTER at the beginning of yyparse() which marks the suitable position on the save stack.
       * In newer perls this seems to be the second ENTER executed within the context block,
       * while in the older versions one had to go deeper into the scope stack, for reasons long forgotten and obscure now */
      saves=PL_savestack+PL_scopestack[cx->blk_oldscopesp+1];
      to_restore->replaced=3;
      memcpy(to_restore->saved, saves, 3 * sizeof(to_restore->saved[0]));
      (saves++)->any_dxptr=&reset_ptrs;
      (saves++)->any_ptr=to_restore;
      (saves++)->any_uv=SAVEt_DESTRUCTOR_X;
   } else {
      // we are in the main script scope, no further enclosing contexts
      SV* restore_holder=newSV_type(SVt_PVMG);
      sv_magicext(restore_holder, Nullsv, PERL_MAGIC_ext, &restore_holder_vtbl, Nullch, 0);
      SvMAGIC(restore_holder)->mg_ptr=(char*)to_restore;
      to_restore->replaced=2;
      saves=PL_savestack;
      memcpy(to_restore->saved, saves, 2 * sizeof(to_restore->saved[0]));
      saves[0].any_ptr=restore_holder;
      saves[1].any_uv=SAVEt_FREESV;
   }
}

int reset_ptrs_via_magic(pTHX_ SV* sv, MAGIC* mg)
{
   reset_ptrs(aTHX_ mg->mg_ptr);
   return 0;
}

#if PerlVersion < 5220
# define Perl_op_convert_list Perl_convert
# define NewMETHOD_NAMED_OP(name, namelen) newSVOP(OP_METHOD_NAMED, 0, newSVpvn_share(name, namelen, 0))
#else
# define NewMETHOD_NAMED_OP(name, namelen) newMETHOP_named(OP_METHOD_NAMED, 0, newSVpvn_share(name, namelen, 0))
#endif

#if PerlVersion >= 5200
/* op_clear reaches into PL_comppad_name which points to something different during the execution phase */
# define SetPadnamesOfCurrentSub(savevar) \
  PADNAMELIST* savevar=PL_comppad_name; \
  PL_comppad_name=PadlistNAMES(CvPADLIST(pm_perl_get_cur_cv(aTHX)))
# define RestorePadnames(savevar) \
  PL_comppad_name=savevar
#else
# define SetPadnamesOfCurrentSub(savevar)
# define RestorePadnames(savevar)
#endif

static inline
int extract_lex_imp_ix(pTHX_ COP *cop)
{
   SV *sv=Perl_refcounted_he_fetch_sv(aTHX_ cop->cop_hints_hash, lex_imp_key, 0, 0);
   return SvIOK(sv) ? SvIVX(sv) & LexCtxIndex : 0;
}

static inline
int get_lex_flags(pTHX)
{
   SV *sv=Perl_refcounted_he_fetch_sv(aTHX_ PL_curcop->cop_hints_hash, lex_imp_key, 0, 0);
   return SvIOK(sv) ? SvIVX(sv) : 0;
}

#define get_lex_imp_ix extract_lex_imp_ix(aTHX_ PL_curcop)

#define get_lex_imp_ix_from_cv(cv) extract_lex_imp_ix(aTHX_ (COP*)CvSTART(cv))

static
GV* get_dotIMPORT_GV(pTHX_ HV *stash)
{
   GV* imp_gv=(GV*)HeVAL(hv_fetch_ent(stash, dot_import_key, TRUE, SvSHARED_HASH(dot_import_key)));
   AV* dotIMPORT=NULL;
   if (SvTYPE(imp_gv) != SVt_PVGV)
      gv_init(imp_gv, stash, SvPVX(dot_import_key), SvCUR(dot_import_key), GV_ADDMULTI);
   else
      dotIMPORT=GvAV(imp_gv);

   if (!dotIMPORT) {
      GV* declare_gv=(GV*)HeVAL(hv_fetch_ent(stash, declare_key, TRUE, SvSHARED_HASH(declare_key)));
      if (SvTYPE(declare_gv) != SVt_PVGV)
         gv_init(declare_gv, stash, SvPVX(declare_key), SvCUR(declare_key), GV_ADDMULTI);
      sv_setsv((SV*)declare_gv, sv_2mortal(newRV((SV*)declare_cv)));
      GvAV(imp_gv)=dotIMPORT=newAV();
      hv_delete_ent(stash, dot_dummy_pkg_key, G_DISCARD, SvSHARED_HASH(dot_dummy_pkg_key));
   }

   return imp_gv;
}

static inline
AV* get_dotIMPORT(pTHX_ HV* stash)
{
   return GvAV(get_dotIMPORT_GV(aTHX_ stash));
}

static
void set_dotIMPORT(pTHX_ HV *stash, AV *dotIMPORT)
{
   GV* imp_gv=(GV*)HeVAL(hv_fetch_ent(stash, dot_import_key, TRUE, SvSHARED_HASH(dot_import_key)));
   gv_init(imp_gv, stash, SvPVX(dot_import_key), SvCUR(dot_import_key), GV_ADDMULTI);
   GvAV(imp_gv)=(AV*)SvREFCNT_inc_simple_NN((SV*)dotIMPORT);
}

static
void set_dotDUMMY_PKG(pTHX_ HV *stash)
{
   GV* dummy_gv=(GV*)HeVAL(hv_fetch_ent(stash, dot_dummy_pkg_key, TRUE, SvSHARED_HASH(dot_dummy_pkg_key)));
   if (SvTYPE(dummy_gv) != SVt_PVGV) {
      gv_init(dummy_gv, stash, SvPVX(dot_dummy_pkg_key), SvCUR(dot_dummy_pkg_key), GV_ADDMULTI);
      sv_setiv(GvSVn(dummy_gv), 1);
   }
}

static
int is_dummy_pkg(pTHX_ HV *stash)
{
   HE* dummy_he=hv_fetch_ent(stash, dot_dummy_pkg_key, FALSE, SvSHARED_HASH(dot_dummy_pkg_key));
   SV* sv;
   return dummy_he && (sv=GvSV((GV*)HeVAL(dummy_he))) && SvIOK(sv) ? SvIV(sv) : 0;
}

static
int equal_arrays(AV *ar1, AV *ar2)
{
   if (AvFILLp(ar1) != AvFILLp(ar2)) return FALSE;
   if (AvFILLp(ar1)>=0) {
      SV **lookp=AvARRAY(ar1), **endp=lookp+AvFILLp(ar1), **lookp2=AvARRAY(ar2);
      for (; lookp<=endp; ++lookp, ++lookp2) {
         if (SvRV(*lookp) != SvRV(*lookp2)) return FALSE;
      }
   }
   return TRUE;
}

static inline
I32 find_stash_in_import_list(AV* import_av, HV* stash)
{
   SV **lookp=AvARRAY(import_av);
   if (lookp) {
      SV **endp;
      for (endp=lookp+AvFILLp(import_av); lookp<=endp; ++lookp)
         if ((HV*)SvRV(*lookp)==stash) return TRUE;
   }
   return FALSE;
}

static
int store_lex_lookup_stash(pTHX_ SV* stash_ref)
{
   SV *stash=SvRV(stash_ref);
   SV **lookp=AvARRAY(lexical_imports), **endp=lookp+AvFILLp(lexical_imports);
   while (++lookp<=endp) {
      if (SvRV(*lookp)==stash) return lookp-AvARRAY(lexical_imports);
   }
   av_push(lexical_imports, SvREFCNT_inc_simple_NN(stash_ref));
   return AvFILLp(lexical_imports);
}

static
void predeclare_sub(pTHX_ HV* stash, GV* cgv)
{
   GV* ngv=*(GV**)hv_fetch(stash, GvNAME(cgv), GvNAMELEN(cgv), TRUE);
   CV* forw_cv;
   if (SvTYPE(ngv)==SVt_PVGV) {
      if (GvCVu(ngv)) return;
   } else {
      gv_init(ngv, stash, GvNAME(cgv), GvNAMELEN(cgv), GV_ADDMULTI);
   }
   forw_cv=(CV*)newSV_type(SVt_PVCV);
   CvSTART(forw_cv)=&forw_decl_op;
   GvCV_set(ngv, forw_cv);
   CvGV_set(forw_cv, ngv);
   CvSTASH_set(forw_cv, stash);
}

static
void import_dotSUBS(pTHX_ HV* stash, AV* imp_dotSUBS)
{
   int i,e;
   for (i=0, e=AvFILLp(imp_dotSUBS); i<=e; ++i)
      predeclare_sub(aTHX_ stash, (GV*)(AvARRAY(imp_dotSUBS)[i]));
}

static inline
AV* get_dotARRAY(pTHX_ HV* stash, SV* arr_name_sv, I32 create)
{
   HE* arr_gve=hv_fetch_ent(stash,arr_name_sv,create,SvSHARED_HASH(arr_name_sv));
   if (create) {
      GV *arr_gv=(GV*)HeVAL(arr_gve);
      if (SvTYPE(arr_gv) != SVt_PVGV)
         gv_init(arr_gv, stash, SvPVX(arr_name_sv), SvCUR(arr_name_sv), GV_ADDMULTI);
      return GvAVn(arr_gv);
   }
   return arr_gve ? GvAV(HeVAL(arr_gve)) : Nullav;
}

#define get_dotSUBST_OP(stash,create) get_dotARRAY(aTHX_ stash, dot_subst_op_key, create)
#define get_dotSUBS(stash,create) get_dotARRAY(aTHX_ stash, dot_subs_key, create)

// elements of a const creation descriptor
#define PmConstCreationOpCode 0
#define PmConstCreationSubRef 1
#define PmConstCreationFirstArg 2
#define PmConstCreationReset 3
#define PmConstCreationCatch 4

static inline
AV* get_cur_dotSUBST_OP(pTHX)
{
   return cur_lexical_import_ix > 0 ? get_dotSUBST_OP((HV*)SvRV(AvARRAY(lexical_imports)[cur_lexical_import_ix]), FALSE) : 0;
}

static
AV* merge_dotSUBST_OP(pTHX_ HV* stash, AV* dotSUBST_OP, AV* imp_dotSUBST_OP)
{
   int i, j, e, k;
   if (!dotSUBST_OP) {
      dotSUBST_OP=get_dotSUBST_OP(stash, TRUE);
      for (i=0, e=AvFILLp(imp_dotSUBST_OP); i<=e; ++i)
         av_push(dotSUBST_OP, SvREFCNT_inc_NN(AvARRAY(imp_dotSUBST_OP)[i]));
   } else {
      for (i=0, e=AvFILLp(imp_dotSUBST_OP); i<=e; ++i) {
         AV *op_descr=(AV*)SvRV(AvARRAY(imp_dotSUBST_OP)[i]);
         for (j=0, k=AvFILLp(dotSUBST_OP); j<=k; ++j)
            if (SvIVX(AvARRAY(op_descr)[PmConstCreationOpCode])==SvIVX(AvARRAY((AV*)SvRV(AvARRAY(dotSUBST_OP)[j]))[PmConstCreationOpCode]))
               break;
         if (j>k) av_push(dotSUBST_OP, newRV((SV*)op_descr));
      }
   }
   return dotSUBST_OP;
}

static
AV* merge_dotSUBS(pTHX_ HV* stash, AV* dotSUBS, AV* imp_dotSUBS)
{
   int i, e;
   if (!dotSUBS) dotSUBS=get_dotSUBS(stash, TRUE);
   av_extend(dotSUBS, AvFILLp(dotSUBS)+AvFILLp(imp_dotSUBS)+1);
   for (i=0, e=AvFILLp(imp_dotSUBS); i<=e; ++i)
      av_push(dotSUBS, SvREFCNT_inc_NN(AvARRAY(imp_dotSUBS)[i]));
   return dotSUBS;
}

static
int store_shadow_lex_lookup_stash(pTHX_ AV* dotIMPORT)
{
   SV** lookp=AvARRAY(lexical_imports);
   SV** endp=lookp+AvFILLp(lexical_imports);
   HV* shadow_stash;
   AV* dotSUBST_OP=NULL;
   AV* imp_dotSUBST_OP;
   AV* dotSUBS=NULL;
   AV* imp_dotSUBS;

   while (++lookp<=endp) {
      HV *stash=(HV*)SvRV(*lookp);
      if (HvNAME(stash)[0] == '-') {
         if (equal_arrays(dotIMPORT, get_dotIMPORT(aTHX_ stash)))
            return lookp-AvARRAY(lexical_imports);
      }
   }

   // must create a new shadow stash
   shadow_stash=gv_stashpv(form("--namespace-lookup-%d", ++shadow_stash_cnt), TRUE);
   set_dotIMPORT(aTHX_ shadow_stash, dotIMPORT);
   av_push(lexical_imports, newRV_noinc((SV*)shadow_stash));
   for (lookp=AvARRAY(dotIMPORT), endp=lookp+AvFILLp(dotIMPORT); lookp<=endp; ++lookp) {
      if ((imp_dotSUBST_OP=get_dotSUBST_OP((HV*)SvRV(*lookp), FALSE)))
         dotSUBST_OP=merge_dotSUBST_OP(aTHX_ shadow_stash, dotSUBST_OP, imp_dotSUBST_OP);
      if ((imp_dotSUBS=get_dotSUBS((HV*)SvRV(*lookp), FALSE)))
         dotSUBS=merge_dotSUBS(aTHX_ shadow_stash, dotSUBS, imp_dotSUBS);
   }
   return AvFILLp(lexical_imports);
}

static
OP* switch_off_namespaces(pTHX)
{
   reset_ptrs(aTHX_ NULL);
   if (PL_op->op_flags & OPf_SPECIAL) {
      cur_lexical_import_ix=-1;
      cur_lexical_flags=0;
   }
   PL_op->op_ppaddr=&Perl_pp_null;
   return NORMAL;
}

static
I32 append_imp_stash(pTHX_ AV* import_av, HV* imp_stash)
{
   if (find_stash_in_import_list(import_av, imp_stash))
      return FALSE;
   av_push(import_av, newRV((SV*)imp_stash));
   return TRUE;
}

static
void remove_imp_stash(pTHX_ AV* dotLOOKUP, HV* imp_stash)
{
   SV **lookp, **endp;
   if ((lookp=AvARRAY(dotLOOKUP)))
      for (endp=lookp+AvFILLp(dotLOOKUP); lookp<=endp; ++lookp) {
         if ((HV*)SvRV(*lookp)==imp_stash) {
            SvREFCNT_dec(*lookp);
            if (lookp<endp) Move(lookp+1, lookp, endp-lookp, SV**);
            *endp=PmEmptyArraySlot;
            AvFILLp(dotLOOKUP)--;
            break;
         }
      }
}

static
int merge_lexical_import_scopes(pTHX_ int lex_ix1, int lex_ix2)
{
   HV *imp_stash1, *imp_stash2;
   AV *dot_import1, *dot_import2;
   AV *new_imports;
   I32 is_shadow1, is_shadow2;

   if (lex_ix1==lex_ix2 || lex_ix2==0) return lex_ix1;
   if (lex_ix1==0) return lex_ix2;

   imp_stash1=(HV*)SvRV(AvARRAY(lexical_imports)[lex_ix1]);
   imp_stash2=(HV*)SvRV(AvARRAY(lexical_imports)[lex_ix2]);
   dot_import1=get_dotIMPORT(aTHX_ imp_stash1);
   dot_import2=get_dotIMPORT(aTHX_ imp_stash2);
   is_shadow1=HvNAME(imp_stash1)[0] == '-';
   is_shadow2=HvNAME(imp_stash2)[0] == '-';

   // maybe one stash is already contained in another's import list?
   if (!is_shadow2 && dot_import1 && find_stash_in_import_list(dot_import1, imp_stash2))
      return lex_ix1;
   if (!is_shadow1 && dot_import2 && find_stash_in_import_list(dot_import2, imp_stash1))
      return lex_ix2;

   // concatenate both import lists into a new one
   if (is_shadow1) {
      new_imports=av_make(AvFILLp(dot_import1)+1, AvARRAY(dot_import1));
   } else {
      new_imports=newAV();
      av_push(new_imports, newRV((SV*)imp_stash1));
   }
   if (is_shadow2) {
      SV **lookp2=AvARRAY(dot_import2), **endp2=lookp2+AvFILLp(dot_import2);
      if (is_shadow1) {
         for (; lookp2<endp2; ++lookp2)
            append_imp_stash(aTHX_ new_imports, (HV*)SvRV(*lookp2));
      } else {
         av_extend(new_imports, AvFILLp(dot_import2)+1);
         for (; lookp2<endp2; ++lookp2)
            av_push(new_imports, newSVsv(*lookp2));
      }
   } else {
      av_push(new_imports, newRV((SV*)imp_stash2));
   }

   lex_ix1=store_shadow_lex_lookup_stash(aTHX_ new_imports);
   SvREFCNT_dec(new_imports);
   return lex_ix1;
}

static
AV* get_dotLOOKUP(pTHX_ HV* stash);

static
void append_lookup(pTHX_ HV* stash, AV* dotLOOKUP, AV* import_from, int recurse)
{
   SV **impp=AvARRAY(import_from), **endp;
   if (impp) {
      for (endp=impp+AvFILLp(import_from); impp<=endp; ++impp) {
         HV* imp_stash=(HV*)SvRV(*impp);
         if (imp_stash != stash && append_imp_stash(aTHX_ dotLOOKUP, imp_stash) && recurse) {
            AV* imp_dotLOOKUP=get_dotLOOKUP(aTHX_ imp_stash);
            if (imp_dotLOOKUP) append_lookup(aTHX_ stash, dotLOOKUP, imp_dotLOOKUP, FALSE);
         }
      }
   }
}

static
AV* get_dotLOOKUP(pTHX_ HV* stash)
{
   AV* dotLOOKUP=NULL;
   HV* pkgLOOKUP=NULL;
   int i;
   GV* lookup_gv=(GV*)HeVAL(hv_fetch_ent(stash, dot_lookup_key, TRUE, SvSHARED_HASH(dot_lookup_key)));
   if (SvTYPE(lookup_gv) != SVt_PVGV) {
      gv_init(lookup_gv, stash, SvPVX(dot_lookup_key), SvCUR(dot_lookup_key), GV_ADDMULTI);
   } else {
      dotLOOKUP=GvAV(lookup_gv);
      pkgLOOKUP=GvHV(lookup_gv);
   }
   if (!dotLOOKUP) {
      char *st_name=HvNAME(stash);
      I32 st_name_len=HvNAMELEN_get(stash);
      AV *dotIMPORT;
      HE *imp_gve;

      if ( (imp_gve=hv_fetch_ent(stash, dot_import_key, FALSE, SvSHARED_HASH(dot_import_key))) &&
           (dotIMPORT=GvAV(HeVAL(imp_gve))) ) {
         dotLOOKUP=newAV();
         append_lookup(aTHX_ stash, dotLOOKUP, dotIMPORT, TRUE);

         for (i=st_name_len-2; i>0; --i) {
            if (st_name[i]==':' && st_name[i-1]==':') {
               HV *encl_stash=gv_stashpvn(st_name, --i, FALSE);
               if (encl_stash) {
                  if (append_imp_stash(aTHX_ dotLOOKUP, encl_stash)) {
                     if (hv_exists_ent(encl_stash, dot_import_key, SvSHARED_HASH(dot_import_key))) {
                        AV *encl_lookup=get_dotLOOKUP(aTHX_ encl_stash);
                        if (encl_lookup) {
                           append_lookup(aTHX_ stash, dotLOOKUP, encl_lookup, FALSE);
                           break;       /* encl_stash::.LOOKUP certainly contains all enclosing packages */
                        }
                     }
                  } else {
                     break;
                  }
               }
            }
         }

         GvAV(lookup_gv)=dotLOOKUP;
         if (AvFILLp(dotLOOKUP)<0) dotLOOKUP=NULL;
         GvHV(lookup_gv)=pkgLOOKUP=newHV();
      }
   }

   last_stash=stash;
   last_dotLOOKUP=dotLOOKUP;
   last_pkgLOOKUP=pkgLOOKUP;
   return dotLOOKUP;
}

static
OP* pp_popmark(pTHX)
{
   (void)POPMARK;
   return NORMAL;
}

static
OP* pp_popmark_and_nextstate(pTHX)
{
   (void)POPMARK;
   return def_pp_NEXTSTATE(aTHX);
}

#ifdef USE_ITHREADS

#define PullRepairedGV(o)     \
STMT_START {                          \
   SV* subst=cSVOPx_sv(o);            \
   int i=o->op_targ;                  \
   if (PAD_SV(i) != subst) {          \
      SvREFCNT_dec(PAD_SV(i));        \
      PAD_SVl(i)=SvREFCNT_inc_simple_NN(subst); \
   }                                  \
} STMT_END

static
OP* repaired_gv(pTHX)
{
   PullRepairedGV(OpSIBLING(PL_op));
   return Perl_pp_gv(aTHX);
}

static
OP* repaired_gvsv(pTHX)
{
   PullRepairedGV(OpSIBLING(PL_op));
   return Perl_pp_gvsv(aTHX);
}

static
OP* repaired_aelemefast(pTHX)
{
   PullRepairedGV(OpSIBLING(PL_op));
   return Perl_pp_aelemfast(aTHX);
}

static
OP* repaired_split(pTHX)
{
   PullRepairedGV(OpSIBLING(PL_op));
   return Perl_pp_split(aTHX);
}

#if PerlVersion >= 5220
static
OP* repaired_multideref(pTHX)
{
   OP* this_op=PL_op;
   OP* o=this_op;
   do {
      o=OpSIBLING(o);
      assert(o->op_type==OP_CONST && cSVOPo->op_sv != NULL);
      PullRepairedGV(o);
   } while (o->op_next==this_op);
   return Perl_pp_multideref(aTHX);
}
#endif

static
void do_repair_gvop(pTHX_ SV *old_sv, SV *new_sv, PADOFFSET pad_ix)
{
   CV *cv=pm_perl_get_cur_cv(aTHX);
   if (CvCLONED(cv)) {
      OP* this_op=PL_op;
      OP* helper=newSVOP(OP_CONST, 0, new_sv);
      helper->op_targ=pad_ix;
      PmOpCopySibling(helper, this_op);
      OpMORESIB_set(this_op, helper);
      switch (this_op->op_type) {
      case OP_GV:
         this_op->op_ppaddr=&repaired_gv;
         break;
      case OP_GVSV:
         this_op->op_ppaddr=&repaired_gvsv;
         break;
      case OP_AELEMFAST:
         this_op->op_ppaddr=&repaired_aelemefast;
         break;
      case OP_SPLIT:
         this_op->op_ppaddr=&repaired_split;
         break;
#if PerlVersion >= 5220
      case OP_MULTIDEREF:
         this_op->op_ppaddr=&repaired_multideref;
         helper->op_next=this_op;
         break;
#endif
      }
   } else {
      PADLIST *padlist=CvPADLIST(cv);
      PAD **padstart=PadlistARRAY(padlist), **pads, **epads;
      if (PL_comppad==padstart[CvDEPTH(cv)]) {
         PADOFFSET max = PadlistMAX(padlist);
#ifdef DEBUGGING
         PADNAMELIST* padnames = PadlistNAMES(padlist);
         if ((I32)pad_ix <= PadnamelistMAX(padnames)) {
#if PerlVersion < 5220
            SV* empty_slot=PadnamelistARRAY(padnames)[pad_ix];
            if (empty_slot != NULL && SvTYPE(empty_slot))
#else
            if (PadnameLEN(PadnamelistARRAY(padnames)[pad_ix]) != 0)
#endif
               Perl_croak(aTHX_ "namespaces::do_repair_gvop - internal error");
         }
#endif
         while (!PadlistARRAY(padlist)[max])
            max--;
         for (pads=padstart+1, epads=padstart+max; pads<=epads; ++pads) {
            SvREFCNT_dec(old_sv);
            if (pads < epads) SvREFCNT_inc_simple_void_NN(new_sv);       // the last increment is made after the loop
            AvARRAY(*pads)[pad_ix]=new_sv;
         }
      } else {
         // working with another PAD: probably re-eval
         SvREFCNT_dec(old_sv);
         PAD_SVl(pad_ix)=new_sv;
      }
   }
   if (SvTYPE(new_sv)==SVt_PVGV) {
      GvIN_PAD_on(new_sv);
      SvREFCNT_inc_simple_void_NN(new_sv);
   }
}

#define repair_gvop(old_sv, new_sv) \
   do_repair_gvop(aTHX_ (SV*)old_sv, (SV*)new_sv, cPADOP->op_padix)
#define repair_splitop(old_sv, new_sv) \
   do_repair_gvop(aTHX_ (SV*)old_sv, (SV*)new_sv, cPMOPx(cUNOP->op_first)->op_pmreplrootu.op_pmtargetoff)
#define repair_multideref(old_sv, new_sv, aux_item) \
   do_repair_gvop(aTHX_ (SV*)old_sv, (SV*)new_sv, (aux_item)->pad_offset)

#else  /* !ITHREADS */

#define repair_gvop(old_sv, new_sv) \
   SvREFCNT_dec(old_sv), \
   cSVOP->op_sv=SvREFCNT_inc_NN(new_sv)
#define repair_splitop(old_sv, new_sv) \
   SvREFCNT_dec(old_sv), \
   cPMOPx(cUNOP->op_first)->op_pmreplrootu.op_pmtargetgv=(GV*)SvREFCNT_inc_NN(new_sv)
#define repair_multideref(old_sv, new_sv, aux_item) \
   SvREFCNT_dec(old_sv), \
   (aux_item)->sv=SvREFCNT_inc_NN(new_sv)
#endif

#if PerlVersion >= 5220
# define aMultiDerefItem_ aux_item,
# define nullMultiDerefItem_ Null(UNOP_AUX_item*),
# define pMultiDerefItem_ UNOP_AUX_item* aux_item,
#else
# define aMultiDerefItem_
# define nullMultiDerefItem_
# define pMultiDerefItem_
#endif

static inline
void repair_pp_gv(pTHX_ pMultiDerefItem_ GV* old_gv, GV* new_gv)
{
   switch (PL_op->op_type) {
   case OP_SPLIT:
      repair_splitop(old_gv, new_gv);
      break;
   case OP_GVSV:
   case OP_AELEMFAST:
      repair_gvop(old_gv, new_gv);
      break;
#if PerlVersion >= 5220
   case OP_MULTIDEREF:
      repair_multideref(old_gv, new_gv, aux_item);
      break;
#endif
   default:
      {
         dSP;
         repair_gvop(old_gv, new_gv);
         SETs((SV*)new_gv);
      }
   }
}

static
GV* try_stored_lexical_gv(pTHX_ GV *var_gv, I32 type, I32 lex_imp_ix)
{
   MAGIC *mg=mg_find((SV*)var_gv, PERL_MAGIC_ext);
   GV **list_start, *imp_gv;
   if (mg && (list_start=(GV**)mg->mg_ptr)) {
      lex_imp_ix-=mg->mg_private;
      if (lex_imp_ix>=0 && lex_imp_ix<mg->mg_len && (imp_gv=list_start[lex_imp_ix])) {
         switch (type) {
         case SVt_PV:
            if (GvIMPORTED_SV(imp_gv)) return imp_gv;
            break;
         case SVt_PVAV:
            if (GvIMPORTED_AV(imp_gv)) return imp_gv;
            break;
         case SVt_PVHV:
            if (GvIMPORTED_HV(imp_gv)) return imp_gv;
            break;
         case SVt_PVCV: {
            CV *cv=GvCV(imp_gv);
            if (cv && IsWellDefinedSub(cv)) return imp_gv;
         }}
      }
   }
   return Nullgv;
}

static
void store_lexical_gv(pTHX_ GV *var_gv, GV *imp_gv, I32 lex_imp_ix)
{
   MAGIC* mg=mg_find((SV*)var_gv, PERL_MAGIC_ext);
   GV **list_start;
   if (mg && (list_start=(GV**)mg->mg_ptr)) {
      lex_imp_ix-=mg->mg_private;
      if (lex_imp_ix<0) {
         Newz(0, list_start, mg->mg_len-lex_imp_ix, GV*);
         Copy(mg->mg_ptr, list_start, mg->mg_len, GV*);
         Safefree(mg->mg_ptr);
         mg->mg_ptr=(char*)list_start;
         mg->mg_len-=lex_imp_ix;
         list_start[0]=imp_gv;
         mg->mg_private+=lex_imp_ix;
      } else if (lex_imp_ix>=mg->mg_len) {
         Renewc(mg->mg_ptr, lex_imp_ix+1, GV*, char);
         list_start=(GV**)mg->mg_ptr;
         Zero(list_start+mg->mg_len, lex_imp_ix-mg->mg_len, GV*);
         list_start[lex_imp_ix]=imp_gv;
         mg->mg_len=lex_imp_ix+1;
      } else if (list_start[lex_imp_ix]) {
         if (list_start[lex_imp_ix] != imp_gv)
            Perl_croak(aTHX_ "ambiguous name resolution in package %s, lexical scope %d: conflict between %s::%.*s in and %s::%.*s",
                       HvNAME(GvSTASH(var_gv)), (int)(lex_imp_ix+mg->mg_private),
                       HvNAME(GvSTASH(imp_gv)), (int)GvNAMELEN(imp_gv), GvNAME(imp_gv),
                       HvNAME(GvSTASH(list_start[lex_imp_ix])), (int)GvNAMELEN(imp_gv), GvNAME(imp_gv));
      } else {
         list_start[lex_imp_ix]=imp_gv;
      }
   } else {
      if (!mg) mg=sv_magicext((SV*)var_gv, Nullsv, PERL_MAGIC_ext, Null(MGVTBL*), Nullch, 1);
      Newz(0, list_start, 1, GV*);
      mg->mg_ptr=(char*)list_start;
      list_start[0]=imp_gv;
      mg->mg_private=lex_imp_ix;
   }
}

static
void store_package_gv(pTHX_ GV *var_gv, GV *imp_gv)
{
   MAGIC *mg=mg_find((SV*)var_gv, PERL_MAGIC_ext);
   if (mg) {
      if (mg->mg_obj) {
         if ((GV*)mg->mg_obj != imp_gv)
            Perl_croak(aTHX_ "ambiguous name resolution in package %s: conflict between %s::%.*s in and %s::%.*s",
                       HvNAME(GvSTASH(var_gv)),
                       HvNAME(GvSTASH(imp_gv)), (int)GvNAMELEN(imp_gv), GvNAME(imp_gv),
                       HvNAME(GvSTASH(mg->mg_obj)), (int)GvNAMELEN(imp_gv), GvNAME(imp_gv));
      } else {
         mg->mg_obj=(SV*)imp_gv;
      }
   } else {
      mg=sv_magicext((SV*)var_gv, Nullsv, PERL_MAGIC_ext, Null(MGVTBL*), Nullch, 1);
      mg->mg_obj=(SV*)imp_gv;
   }
}

static inline
GV* test_imported_gv(pTHX_ GV *gv, I32 type, int ignore_methods)
{
   switch (type) {
   case SVt_PV:
      return GvIMPORTED_SV(gv) ? gv : Nullgv;
   case SVt_PVAV:
      return GvIMPORTED_AV(gv) ? gv : Nullgv;
   case SVt_PVHV:
      return GvIMPORTED_HV(gv) ? gv : Nullgv;
   case SVt_PVCV: {
      CV* cv=GvCV(gv);
      if (cv != Nullcv && CvSTART(cv) != &forw_decl_op) {
         if (ignore_methods && CvMETHOD(cv))
            /* may not discover methods in object-less call */
            return (GV*)-1UL;
         if (IsWellDefinedSub(cv) || GvASSUMECV(gv))
            /* If only promised - let's try later, or die if the next op is ENTERSUB.
               For inherited static methods return the gv from the basis class! */
            return GvCVGEN(gv) ? CvGV(cv) : gv;
      }
   }}
   return Nullgv;
}

static inline
GV* try_stored_package_gv(pTHX_ GV *gv, I32 type, int ignore_methods)
{
   MAGIC *mg=mg_find((SV*)gv, PERL_MAGIC_ext);
   if (mg && (gv=(GV*)mg->mg_obj)) {
      gv=test_imported_gv(aTHX_ gv, type, ignore_methods);
      return gv==(GV*)-1UL ? Nullgv : gv;
   }
   return Nullgv;
}

static
GV* lookup_name_in_stash(pTHX_ HV *stash, const char *name, STRLEN namelen, I32 type, int ignore_methods)
{
   GV **gvp=(GV**)hv_fetch(stash,name,namelen,FALSE);
   if (gvp) {
      GV *gv=*gvp;
      if (SvTYPE(gv)==SVt_PVGV) {
         GV *imp_gv=test_imported_gv(aTHX_ gv, type, ignore_methods);
         if (imp_gv) return imp_gv==(GV*)-1UL ? Nullgv : imp_gv;
         return try_stored_package_gv(aTHX_ gv, type, ignore_methods);
      }
   }
   return Nullgv;
}

static
GV* lookup_name_in_list(pTHX_ HV *stash, GV *var_gv, const char *name, STRLEN namelen, I32 type, int ignore_methods)
{
   AV *dotLOOKUP= stash==last_stash ? last_dotLOOKUP : get_dotLOOKUP(aTHX_ stash);
   SV **lookp, **endp;
   GV *imp_gv;
   if (dotLOOKUP && (lookp=AvARRAY(dotLOOKUP))) {
      for (endp=lookp+AvFILLp(dotLOOKUP); lookp<=endp; ++lookp)
         if ((imp_gv=lookup_name_in_stash(aTHX_ (HV*)SvRV(*lookp), name, namelen, type, ignore_methods))) {
            if (type != SVt_PVCV || IsWellDefinedSub(GvCV(imp_gv))) {
               if (!var_gv) {
                  var_gv=*(GV**)hv_fetch(stash, name, namelen, TRUE);
                  if (SvTYPE(var_gv) != SVt_PVGV)
                     gv_init(var_gv, stash, name, namelen, GV_ADDMULTI);
               }
               store_package_gv(aTHX_ var_gv, imp_gv);
            }
            return imp_gv;
         }
   }
   return Nullgv;
}

/* performs only package-based lookup, no lexical context taken into accout */
static inline
GV* lookup_var(pTHX_ HV *stash, const char *name, STRLEN namelen, I32 type, int ignore_methods)
{
   GV* imp_gv=lookup_name_in_stash(aTHX_ stash, name, namelen, type, ignore_methods);
   if (!imp_gv) {
      HV *save_last_stash=last_stash, *save_last_pkgLOOKUP=last_pkgLOOKUP;
      AV *save_last_dotLOOKUP=last_dotLOOKUP;
      imp_gv=lookup_name_in_list(aTHX_ stash, Nullgv, name, namelen, type, ignore_methods);
      last_stash=save_last_stash; last_dotLOOKUP=save_last_dotLOOKUP; last_pkgLOOKUP=save_last_pkgLOOKUP;
   }
   return imp_gv;
}

/* new Package(arg,...) is often misinterpreted as new(Package(arg,...)) */
static
void resolve_static_method(pTHX_ HV* stash, GV* var_gv, OP* next_gv_op)
{
   OP* next_sub_op;
   if (next_gv_op->op_type == OP_ENTERSUB  &&
       (next_gv_op=next_gv_op->op_next)->op_type == OP_GV  &&
       (next_sub_op=next_gv_op->op_next)->op_type == OP_ENTERSUB) {
      OP* pushmark2_op=cUNOPx(next_sub_op)->op_first;
      OP* pushmark1_op;
      if (!OpHAS_SIBLING(pushmark2_op))
         pushmark2_op=cUNOPx(pushmark2_op)->op_first;
      pushmark1_op=pushmark2_op->op_next;
      if (
#if PerlVersion >= 5180
          pushmark1_op->op_type == OP_PADRANGE ||
#endif
          pushmark1_op->op_type == OP_PUSHMARK) {
         HV* pkg_stash;
         char smallbuf[64];
         char* buf=NULL;
         const char* n=GvNAME(var_gv);
         size_t l=GvNAMELEN(var_gv);
         HV* var_gv_stash=GvSTASH(var_gv);
         if (var_gv_stash != stash) {
            const char *hn=HvNAME(var_gv_stash);
            size_t hnl=HvNAMELEN_get(var_gv_stash);
            size_t tl=hnl+2+l;
            if (tl<sizeof(smallbuf))
               buf=smallbuf;
            else
               New(0, buf, tl+1, char);
            Copy(hn, buf, hnl, char);
            buf[hnl]=':'; buf[hnl+1]=':';
            Copy(n, buf+hnl+2, l, char);
            buf[tl]=0;
            n=buf; l=tl;
         }
         if ((pkg_stash=pm_perl_namespace_lookup_class_autoload(aTHX_ stash, n, l, get_lex_imp_ix))) {
            GV *next_sub=cGVOPx_gv(next_gv_op);
            GV *method_gv=gv_fetchmethod(pkg_stash, GvNAME(next_sub));
            if (method_gv) {
               SV *class_name_sv=newSVpvn_share(HvNAME(pkg_stash), HvNAMELEN_get(pkg_stash), 0);
               OP *class_name_op=newSVOP(OP_CONST, 0, class_name_sv);
               SV **bottom;
               dSP;
               EXTEND(SP, 2);
               // push the package name under the args, and the method GV on the top
               bottom=PL_stack_base+POPMARK;
               while (--SP > bottom) SP[1]=*SP;
               SP[1]=class_name_sv;
               *++PL_stack_sp=(SV*)method_gv;
               GvIMPORTED_CV_on(var_gv);                // but without ASSUME_CV!

               // reorganize the op tree as to get the package name at the beginning of the argument list,
               // and get rid of the second pushmark
               PmOpCopySibling(class_name_op, pushmark2_op);
               OpMORESIB_set(pushmark2_op, class_name_op);
               pushmark2_op->op_next=class_name_op;
#if PerlVersion >= 5180
               if (pushmark1_op->op_type == OP_PADRANGE)
                  // op_sibling points to the first padXv it has replaced
                  class_name_op->op_next=OpSIBLING(pushmark1_op);
               else
#endif
               class_name_op->op_next=pushmark1_op->op_next;
               PL_op->op_next=next_sub_op;
               repair_gvop(var_gv, method_gv);
            }
         }
         if (buf && buf!=smallbuf) Safefree(buf);
      }
   }
}

static
void lookup(pTHX_ pMultiDerefItem_ GV* var_gv, I32 type, OP** pnext_op, OP* access_op)
{
   HV* stash=GvSTASH(var_gv);
   if (stash != PL_defstash && stash != PL_debstash) {
      const char *varname=GvNAME(var_gv);
      STRLEN varnamelen=GvNAMELEN(var_gv);
      OP* assign_op=NULL;
      OP* declare_op=NULL;
      int defer_defuse_declare=FALSE, ignore_methods=FALSE, declare_local=FALSE;
      int lex_imp_ix=0;
      GV* imp_gv;

      if (access_op) {
         OP* o_next;
         while ((o_next=access_op->op_next) != NULL) {
            if (o_next->op_type==OP_GVSV) {
               defer_defuse_declare=TRUE;
               access_op=o_next;
               continue;
            }
            if (o_next->op_type==OP_AASSIGN ||
                (type==SVt_PV && !defer_defuse_declare && o_next->op_type==OP_SASSIGN)) {
               assign_op=o_next;
               o_next=o_next->op_next;
            } else if (access_op->op_type==OP_SPLIT) {
               assign_op=access_op;
            }
            if (o_next->op_type==OP_GV) {
               if (GvCV(cGVOPx_gv(o_next)) == declare_cv) {
                  declare_op=o_next;
               } else {
                  access_op=o_next->op_next;
                  if (access_op->op_type==OP_RV2SV || access_op->op_type==OP_RV2AV || access_op->op_type==OP_RV2HV) {
                     defer_defuse_declare=TRUE;
                     continue;
                  }
               }
            }
            break;
         }
      }
      if (!pnext_op || CopSTASH_eq(PL_curcop, stash)) {
         // unqualified
         if (declare_op) {
            int allow_redeclare=get_lex_flags(aTHX) & LexCtxAllowReDeclare;

            if ((imp_gv=try_stored_package_gv(aTHX_ var_gv, type, FALSE))) {
               *pnext_op=die("declaration conflicts with imported variable %c%s::%.*s at %s line %d.\n",
                             type==SVt_PV ? '$' : type==SVt_PVAV ? '@' : '%',
                             HvNAME(GvSTASH(imp_gv)), (int)varnamelen, varname, CopFILE(PL_curcop), (int)CopLINE(PL_curcop));
               return;
            }
            if (access_op->op_type == OP_GVSV || access_op->op_type==OP_RV2SV || access_op->op_type==OP_RV2AV || access_op->op_type==OP_RV2HV) {
               declare_local= access_op->op_private & OPpLVAL_INTRO;
               if (declare_local && allow_redeclare) {
                  *pnext_op=die("declare local %c%s::%.*s is not allowed in the scope of allow_redeclare at %s line %d.",
                                type==SVt_PV ? '$' : type==SVt_PVAV ? '@' : '%',
                                HvNAME(GvSTASH(imp_gv)), (int)varnamelen, varname, CopFILE(PL_curcop), (int)CopLINE(PL_curcop));
                  return;
               }
            }
            if (allow_redeclare || declare_local) {
               SV* guard;
               OP* guard_op=OpSIBLING(declare_op);
               if (guard_op) {
                  guard=cSVOPx_sv(guard_op);
               } else {
                  guard=newSV_type(SVt_NULL);
                  guard_op=Perl_newSVOP(aTHX_ OP_CONST, 0, guard);
                  PmOpCopySibling(guard_op, declare_op);
                  OpMORESIB_set(declare_op, guard_op);
               }
               sv_magicext(guard, (SV*)var_gv, PERL_MAGIC_ext, &clear_imported_flag_vtab, Nullch, type);
            }
            switch (type) {
            case SVt_PV:
               GvIMPORTED_SV_on(var_gv);
               break;
            case SVt_PVAV:
               GvIMPORTED_AV_on(var_gv);
               break;
            case SVt_PVHV:
               GvIMPORTED_HV_on(var_gv);
               break;
            }
            if (!defer_defuse_declare) {
               if (assign_op) {
                  /* change to void context */
                  assign_op->op_flags &= ~OPf_WANT;
                  assign_op->op_flags |= OPf_WANT_VOID;
               }
               declare_op->op_ppaddr=&pp_popmark;
               declare_op->op_next=declare_op->op_next->op_next;        /* skip entersub */
            }
            return;
         }
         if (assign_op) {
            OPCODE after_assign=assign_op->op_next->op_type;
            if ((after_assign==OP_LEAVEEVAL || after_assign==OP_NEXTSTATE || after_assign==OP_DBSTATE) &&
                (get_lex_flags(aTHX) & LexCtxAutodeclare)) {
               switch (type) {
               case SVt_PV:
                  if (!GvSV(var_gv) || !SvTYPE(GvSV(var_gv))) {
                     GvIMPORTED_SV_on(var_gv);
                     return;
                  }
                  break;
               case SVt_PVAV:
                  if (!GvAV(var_gv) || !AvARRAY(GvAV(var_gv))) {
                     GvIMPORTED_AV_on(var_gv);
                     return;
                  }
                  break;
               case SVt_PVHV:
                  if (!GvHV(var_gv) || !HvARRAY(GvHV(var_gv))) {
                     GvIMPORTED_HV_on(var_gv);
                     return;
                  }
                  break;
               }
            }
         }

         ignore_methods= type==SVt_PVCV && pnext_op && (*pnext_op)->op_type == OP_ENTERSUB;

         if ((imp_gv=try_stored_package_gv(aTHX_ var_gv, type, ignore_methods))) {
            repair_pp_gv(aTHX_ aMultiDerefItem_ var_gv, imp_gv);
            return;
         }

         lex_imp_ix=get_lex_imp_ix;
         if (lex_imp_ix>0 && (imp_gv=try_stored_lexical_gv(aTHX_ var_gv, type, lex_imp_ix))) {
            repair_pp_gv(aTHX_ aMultiDerefItem_ var_gv, imp_gv);
            return;
         }

         if (type != SVt_PVCV ||
             (GvFLAGS(var_gv) & (GVf_ASSUMECV | GVf_IMPORTED_CV)) != GVf_IMPORTED_CV) {

            /* first try: the package-scope lookup list */
            if ((imp_gv=lookup_name_in_list(aTHX_ stash, var_gv, varname, varnamelen, type, ignore_methods))) {
               repair_pp_gv(aTHX_ aMultiDerefItem_ var_gv, imp_gv);
               return;
            }
            if (pnext_op && lex_imp_ix>0) {
               /* second try: the lexical scope lookup list */
               if ((imp_gv=lookup_var(aTHX_ (HV*)SvRV(AvARRAY(lexical_imports)[lex_imp_ix]),
                                      varname, varnamelen, type, ignore_methods))) {
                  store_lexical_gv(aTHX_ var_gv, imp_gv, lex_imp_ix);
                  repair_pp_gv(aTHX_ aMultiDerefItem_ var_gv, imp_gv);
                  return;
               }
            }
         }

         if (pnext_op) {
            /* Nothing found: time to croak... But let's check for exceptions first */
            if (type==SVt_PVCV) {
               resolve_static_method(aTHX_ stash, var_gv, *pnext_op);
               /* pp_entersub will produce a suitable message when it gets stuck on the undefined sub */
               return;
            }

            *pnext_op=die("reference to an undeclared variable %c%.*s at %s line %d.\n",
                          type==SVt_PV ? '$' : type==SVt_PVAV ? '@' : '%',
                          (int)varnamelen, varname, CopFILE(PL_curcop), (int)CopLINE(PL_curcop));
         }

      } else {
         /* full qualified, but undeclared */
         HV *cur_stash, *other_stash;

         if (declare_op) {
            *pnext_op=die("can't declare variables from other packages: please use namespaces::declare_var() instead");
            return;
         }

         /* check for exceptions */
         switch (type) {
         case SVt_PVAV:
            /* allow to refer to the ISA array of a defined package */
            if (varnamelen==3 && varname[0]=='I' && varname[1]=='S' && varname[2]=='A')
               return;

         case SVt_PVHV:
            /* allow to refer to the symbol table of a defined package */
            if (varnamelen>=3 && varname[varnamelen-2]==':' && varname[varnamelen-1]==':'
                && GvHV(var_gv) && HvNAME(GvHV(var_gv)))
               return;
            break;

         case SVt_PVCV:
            /* argument-dependent lookup */
            if (stash==args_lookup_stash && pnext_op) {
               dSP;
               SV **args=PL_stack_base+TOPMARK;
               while (++args<SP) {
                  SV *arg=*args;
                  if (SvROK(arg) && (arg=SvRV(arg), SvOBJECT(arg)) &&
                      (imp_gv=lookup_var(aTHX_ SvSTASH(arg), varname, varnamelen, type, TRUE))) {
                     SETs((SV*)imp_gv);
                     PL_op->op_ppaddr=&intercept_pp_gv;
                     return;
                  }
               }
            }
            break;
         }

         cur_stash=CopSTASH(PL_curcop);
         lex_imp_ix=get_lex_imp_ix;
         other_stash=pm_perl_namespace_lookup_class_autoload(aTHX_ cur_stash, HvNAME(stash), HvNAMELEN_get(stash), lex_imp_ix);
         if (other_stash) {
            if (other_stash==stash) {
               MAGIC* mg=mg_find((SV*)var_gv, PERL_MAGIC_ext);
               if (mg && (imp_gv=(GV*)mg->mg_obj) &&
                   (imp_gv=test_imported_gv(aTHX_ imp_gv, type, FALSE))) {
                  repair_pp_gv(aTHX_ aMultiDerefItem_ var_gv, imp_gv);
                  return;
               }
            } else if ((imp_gv=lookup_var(aTHX_ other_stash, varname, varnamelen, type, FALSE))) {
               repair_pp_gv(aTHX_ aMultiDerefItem_ var_gv, imp_gv);
               return;
            } else if (type==SVt_PVCV && pnext_op) {
               resolve_static_method(aTHX_ cur_stash, var_gv, *pnext_op);
               return;
            }
         }

         if (type != SVt_PVCV &&
             (hv_exists_ent(stash, dot_import_key, SvSHARED_HASH(dot_import_key)) ||
              is_dummy_pkg(aTHX_ stash))) {
            /* complain now if the addressed package is compiled with namespace mode
               and we are not looking for a subroutine (otherwise OP_ENTERSUB makes a nicer message) */
            *pnext_op=die("reference to an undeclared variable %c%s::%.*s at %s line %d.\n",
                          type==SVt_PV ? '$' : type==SVt_PVAV ? '@' : '%',
                          HvNAME(stash), (int)varnamelen, varname, CopFILE(PL_curcop), (int)CopLINE(PL_curcop));
         }
      }
   }
}

int clear_imported_flag(pTHX_ SV* sv, MAGIC* mg)
{
   GV* gv=(GV*)mg->mg_obj;
   switch (mg->mg_len)
   {
   case SVt_PV:
      GvIMPORTED_SV_off(gv);
      break;
   case SVt_PVAV:
      GvIMPORTED_AV_off(gv);
      break;
   case SVt_PVHV:
      GvIMPORTED_HV_off(gv);
      break;
   }
   return 0;
}

SV* pm_perl_namespace_try_lookup(pTHX_ HV *stash, SV *name, I32 type)
{
   if (get_dotLOOKUP(aTHX_ stash)) {
      STRLEN l;
      const char *n=SvPV(name,l);
      GV* gv=*(GV**)hv_fetch(stash, n, l, TRUE);
      if (SvTYPE(gv) != SVt_PVGV)
         gv_init(gv, stash, n, l, GV_ADDMULTI);
      lookup(aTHX_ nullMultiDerefItem_ gv, type, Null(OP**), Null(OP*));
      switch (type) {
      case SVt_PV:
         return GvSV(gv);
      case SVt_PVAV:
         return (SV*)GvAV(gv);
      case SVt_PVHV:
         return (SV*)GvHV(gv);
      case SVt_PVCV:
         return (SV*)GvCV(gv);
      case SVt_PVGV:
         return (SV*)gv;
      }
   }
   return 0;
}

static
void resolve_scalar_gv(pTHX_ pMultiDerefItem_ GV* var_gv, OP** pnext_op, OP* access_op)
{
   if (!GvIMPORTED_SV(var_gv)) {
      const char* name;
      if (GvNAMELEN(var_gv)==8) {
         name=GvNAME(var_gv);
         if (*name=='A' && !memcmp(name, "AUTOLOAD", 8) && GvCV(var_gv)) {
            // $AUTOLOAD must not be predeclared if there is sub AUTOLOAD too
            GvIMPORTED_SV_on(var_gv);
            return;
         }
      } else if (GvNAMELEN(var_gv)==1 && PL_curstackinfo->si_type == PERLSI_SORT) {
         name=GvNAME(var_gv);
         if (*name=='a' || *name=='b')
            // sort sub placeholders must not be predeclared
            return;
      }
      lookup(aTHX_ aMultiDerefItem_ var_gv, SVt_PV, pnext_op, access_op);
   }
}

static
void resolve_array_gv(pTHX_ pMultiDerefItem_ GV* var_gv, OP** pnext_op, OP* access_op)
{
   if (!GvIMPORTED_AV(var_gv)) {
      const char* name;
      if (GvNAMELEN(var_gv)==3) {
         name=GvNAME(var_gv);
         if (name[0]=='I' && name[1]=='S' && name[2]=='A' && CopSTASH_eq(PL_curcop, GvSTASH(var_gv))) {
            // @ISA must not be predeclared
            GvIMPORTED_AV_on(var_gv);
            return;
         }
      }
      lookup(aTHX_ aMultiDerefItem_ var_gv, SVt_PVAV, pnext_op, access_op);
   }
}

static
void resolve_hash_gv(pTHX_ pMultiDerefItem_ GV* var_gv, OP** pnext_op, OP* access_op)
{
   if (!GvIMPORTED_HV(var_gv)) {
      const char* name=GvNAME(var_gv);
      STRLEN namelen=GvNAMELEN(var_gv);
      if (namelen>2 && name[namelen-1]==':' && name[namelen-2]==':') {
         HV* stash=GvHV(var_gv);
         if (stash && HvNAME(stash)) {
            // nested package stashes must not be predeclared
            GvIMPORTED_HV_on(var_gv);
            return;
         }
      }
      lookup(aTHX_ aMultiDerefItem_ var_gv, SVt_PVHV, pnext_op, access_op);
   }
}

static
OP* intercept_pp_gv(pTHX)
{
   OP* next_op=def_pp_GV(aTHX);
   OP* orig_next_op=next_op;
   dSP;
   GV* var_gv=(GV*)TOPs;
   CV* cv;
   OP* this_op=PL_op;

   switch (next_op->op_type) {
   case OP_RV2SV:
      resolve_scalar_gv(aTHX_ nullMultiDerefItem_ var_gv, &next_op, next_op);
      if (next_op == orig_next_op && this_op->op_ppaddr == &intercept_pp_gv)  // not died
         this_op->op_ppaddr=def_pp_GV;
      break;
   case OP_RV2AV:
      resolve_array_gv(aTHX_ nullMultiDerefItem_ var_gv, &next_op, next_op);
      if (next_op == orig_next_op && this_op->op_ppaddr == &intercept_pp_gv)  // not died
         this_op->op_ppaddr=def_pp_GV;
      break;
   case OP_RV2HV:
      resolve_hash_gv(aTHX_ nullMultiDerefItem_ var_gv, &next_op, next_op);
      if (next_op == orig_next_op && this_op->op_ppaddr == &intercept_pp_gv)  // not died
         this_op->op_ppaddr=def_pp_GV;
      break;
   case OP_RV2CV:
      this_op->op_ppaddr=def_pp_GV;  // lookup() never dies on unknown CVs
#if PerlVersion >= 5220
      if (SvROK(var_gv)) break;
#endif
      if ((cv=GvCV(var_gv)) && (next_op->op_next->op_type != OP_REFGEN || IsWellDefinedSub(cv)))
         break;
      lookup(aTHX_ nullMultiDerefItem_ var_gv, SVt_PVCV, &next_op, Null(OP*));
      break;
   case OP_ENTERSUB:
      this_op->op_ppaddr=def_pp_GV;  // lookup() never dies on unknown CVs
#if PerlVersion >= 5220
      if (SvROK(var_gv)) break;
#endif
      if ((cv=GvCV(var_gv)) != Nullcv && CvSTART(cv) != &forw_decl_op) {
         OP *pushmark=cUNOPx(next_op)->op_first, *meth_op, *first_arg;
         if (!OpHAS_SIBLING(pushmark)) pushmark=cUNOPx(pushmark)->op_first;
         first_arg=OpSIBLING(pushmark);

         if (first_arg->op_next==this_op && first_arg->op_type==OP_CONST && (first_arg->op_private & OPpCONST_BARE)) {
            /* a very special case: `method XXX;' where (another) sub `method' is defined in the current package too */
            SV *pkg_name_sv=cSVOPx_sv(first_arg);
            HV *pkg_stash=pm_perl_namespace_lookup_class_autoload(aTHX_ GvSTASH(var_gv), SvPVX(pkg_name_sv), SvCUR(pkg_name_sv), get_lex_imp_ix);
            if (pkg_stash) {
               GV *method_gv=gv_fetchmethod(pkg_stash, GvNAME(var_gv));
               if (method_gv) {
                  if (cSVOPx(first_arg)->op_sv == pkg_name_sv)  // otherwise it's stored in PAD and will be disposed of in due course
                     SvREFCNT_dec(pkg_name_sv);
                  cSVOPx(first_arg)->op_sv=TOPm1s=newSVpvn_share(HvNAME(pkg_stash), HvNAMELEN_get(pkg_stash), 0);
                  repair_gvop(var_gv, method_gv);
                  SETs((SV*)method_gv);
               }
            }
         } else if (pushmark->op_next==this_op && (meth_op=this_op->op_next->op_next)->op_type==OP_METHOD_NAMED && CvMETHOD(GvCV(var_gv))) {
            /* another suspicious case: `name->method' where sub name is defined as method: look for a namespace `name' first */
            HV *pkg_stash=pm_perl_namespace_lookup_class_autoload(aTHX_ GvSTASH(var_gv), GvNAME(var_gv), GvNAMELEN(var_gv), get_lex_imp_ix);
            if (pkg_stash) {
               SV *pkg_name=newSVpvn_share(HvNAME(pkg_stash), HvNAMELEN_get(pkg_stash), 0);
               SvREADONLY_on(pkg_name);
               repair_gvop(var_gv, pkg_name);
               SETs(pkg_name);
               (void)POPMARK;
               pushmark->op_ppaddr=&Perl_pp_null;       /* skip pushmark and entersub */
               this_op->op_next=meth_op;
               this_op->op_ppaddr=def_pp_GV;
            }
         }
      } else {
         lookup(aTHX_ nullMultiDerefItem_ var_gv, SVt_PVCV, &next_op, Null(OP*));
      }
      return PL_op->op_next;
   }
   return next_op;
}

static
OP* intercept_pp_rv2gv(pTHX)
{
   OP* next_op=def_pp_RV2GV(aTHX);
   OP* declare_op=Nullop;
   I32 defuse=FALSE;
   
   if (next_op->op_type==OP_SASSIGN) {
      declare_op=next_op->op_next;
      if (declare_op->op_type == OP_GV && GvCV(cGVOPx_gv(declare_op)) == declare_cv) {
         dSP;
         GV *dst_gv=(GV*)TOPs;
         SV *src_sv=TOPm1s;
         if (SvROK(src_sv)) {
            I32 src_type=SvTYPE(SvRV(src_sv));
            switch (src_type) {
            case SVt_PVAV:
               GvIMPORTED_AV_on(dst_gv);
               defuse=TRUE;
               break;
            case SVt_PVHV:
               GvIMPORTED_HV_on(dst_gv);
               defuse=TRUE;
               break;
            default:
               if (src_type >= SVt_IV && src_type <= SVt_PVMG) {
                  GvIMPORTED_SV_on(dst_gv);
                  defuse=TRUE;
               }
               break;
            }
         } else if (SvTYPE(src_sv)==SVt_PVGV) {
            defuse=TRUE;
         }
      }
   }
   if (defuse) {
      if (declare_op->op_ppaddr != &pp_popmark) {
         /* change to void context */
         next_op->op_flags &= ~OPf_WANT;
         next_op->op_flags |= OPf_WANT_VOID;
         /* skip entersub */
         declare_op->op_ppaddr=&pp_popmark;
         declare_op->op_next=declare_op->op_next->op_next;
      }
   } else {
      PL_op->op_ppaddr=def_pp_RV2GV;
   }
   return next_op;
}

static
OP* intercept_pp_gvsv(pTHX)
{
   GV* var_gv=cGVOP_gv;
   OP* this_op=PL_op;
   OP* next_op=this_op;
   resolve_scalar_gv(aTHX_ nullMultiDerefItem_ var_gv, &next_op, next_op);
   if (next_op == this_op && this_op->op_ppaddr == &intercept_pp_gvsv)  // not died
      this_op->op_ppaddr=def_pp_GVSV;
   return next_op;
}

static
OP* intercept_pp_aelemfast(pTHX)
{
   OP* this_op=PL_op;
   OP* next_op=this_op;
   if (next_op->op_type != OP_AELEMFAST_LEX)
      resolve_array_gv(aTHX_ nullMultiDerefItem_ cGVOP_gv, &next_op, Null(OP*));
   if (next_op == this_op && this_op->op_ppaddr == &intercept_pp_aelemfast)  // not died
      this_op->op_ppaddr=def_pp_AELEMFAST;
   return next_op;
}

static
OP* intercept_pp_split(pTHX)
{
   PMOP* pushre;
   GV* var_gv = Nullgv;
   OP* this_op=PL_op;
   OP* next_op=this_op;
#if PerlVersion >= 5256
   if ((this_op->op_private & (OPpSPLIT_ASSIGN | OPpSPLIT_LEX)) == OPpSPLIT_ASSIGN
       && !(this_op->op_flags & OPf_STACKED)) {
      pushre=cPMOPx(this_op);
# ifdef USE_ITHREADS
      var_gv=(GV*)PAD_SVl(pushre->op_pmreplrootu.op_pmtargetoff);
# else
      var_gv=pushre->op_pmreplrootu.op_pmtargetgv;
# endif
   }
#else  // PerlVersion <= 5256
   pushre=cPMOPx(cUNOP->op_first);
# ifdef USE_ITHREADS
   if (pushre->op_pmreplrootu.op_pmtargetoff) {
      var_gv=(GV*)PAD_SVl(pushre->op_pmreplrootu.op_pmtargetoff);
   }
# else
   if (pushre->op_pmreplrootu.op_pmtargetgv) {
      var_gv=pushre->op_pmreplrootu.op_pmtargetgv;
   }
# endif
#endif
   if (var_gv && !GvIMPORTED_AV(var_gv))
      lookup(aTHX_ nullMultiDerefItem_ var_gv, SVt_PVAV, &next_op, next_op);
   if (next_op == this_op && this_op->op_ppaddr == &intercept_pp_split)  // not died
      this_op->op_ppaddr=def_pp_SPLIT;
   return next_op;
}

// Locate the NEXTSTATE op following the statement in the caller that calls the current sub.
static
OP* next_statement_in_caller(pTHX_ PERL_CONTEXT** cx_ret)
{
   OP* op_next_state=NULL;
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;
   for (; cx > cx_bottom; --cx) {
      if (CxTYPE(cx)==CXt_SUB && !SkipDebugFrame(cx, 0)) {
         op_next_state=(OP*)cx->blk_oldcop;
         break;
      }
   }
   // op_next_state => NEXTSTATE op initiating the statement where the current sub is called.
   if (op_next_state != NULL) {
      while ((op_next_state=OpSIBLING(op_next_state)) != NULL && op_next_state->op_type != OP_NEXTSTATE && op_next_state->op_type != OP_DBSTATE) ;
   }
   *cx_ret=cx;
   return op_next_state;
}


// Return to the next full statement following the call; assuming that the call is made from a `return' expression.
static
OP* pp_fall_off_to_nextstate(pTHX)
{
   PERL_CONTEXT* cx;
   OP* op_next_state=next_statement_in_caller(aTHX_ &cx);
   OP* ret=def_pp_LEAVESUB(aTHX);
   if (op_next_state != NULL) {
      if (pm_perl_skip_debug_cx) {
         op_next_state->op_ppaddr=&pp_popmark_and_nextstate;
         cx->blk_sub.retop=op_next_state;
      } else {
         (void)POPMARK;  // discard the MARK created for the return statement in the caller
         ret=op_next_state;
      }
   }
   return ret;
}

static
void import_subs_into_pkg(pTHX_ HV *stash, GV *imp_gv, int lex_imp_ix)
{
   int offset=lex_imp_ix>>3, bit=1<<(lex_imp_ix&7);
   SV *imp_sv=GvSVn(imp_gv);
   AV *dotSUBS;

   if (SvIOKp(imp_sv) && SvIVX(imp_sv)==lex_imp_ix) {
      return;
   } else if (SvPOKp(imp_sv)) {
      if (SvCUR(imp_sv) > offset && (SvPVX(imp_sv)[offset] & bit)) return;
   } else {
      (void)SvUPGRADE(imp_sv, SVt_PVIV);
      SvPOKp_on(imp_sv);
   }

   dotSUBS=get_dotSUBS((HV*)SvRV(AvARRAY(lexical_imports)[lex_imp_ix]), FALSE);
   if (dotSUBS) import_dotSUBS(aTHX_ stash, dotSUBS);
   if (SvCUR(imp_sv) <= offset) {
      SvGROW(imp_sv, offset+1);
      while (SvCUR(imp_sv) <= offset) {
         SvPVX(imp_sv)[SvCUR(imp_sv)++]=0;
      }
   }
   SvPVX(imp_sv)[offset] |= bit;
}

static inline
void check_explicit_pkg(pTHX_ GV* gv)
{
   HV* stash=GvSTASH(gv);
   if (stash != PL_curstash && stash != PL_defstash && HvTOTALKEYS(stash)==1) {
      set_dotDUMMY_PKG(aTHX_ stash);
   }
}

static inline
void check_explicit_pkg_in_kid(pTHX_ OP* o)
{
   if (o->op_flags & OPf_KIDS) {
      o=cUNOPo->op_first;
      if (o->op_type == OP_GV)
         check_explicit_pkg(aTHX_ cGVOPo_gv);
   }
}

static
OP* intercept_ck_gv(pTHX_ OP* o)
{
   o=def_ck_GV(aTHX_ o);
   check_explicit_pkg(aTHX_ cGVOPo_gv);
   return o;
}

static
OP* intercept_ck_rv2sv(pTHX_ OP* o)
{
   o=def_ck_RV2SV(aTHX_ o);
   check_explicit_pkg_in_kid(aTHX_ o);
   return o;
}

static
OP* intercept_ck_rv2av(pTHX_ OP* o)
{
   o=def_ck_RV2AV(aTHX_ o);
   check_explicit_pkg_in_kid(aTHX_ o);
   return o;
}

static
OP* intercept_ck_rv2hv(pTHX_ OP* o)
{
   o=def_ck_RV2HV(aTHX_ o);
   check_explicit_pkg_in_kid(aTHX_ o);
   return o;
}

static inline
AV* find_const_creation_descriptor(pTHX_ int signature)
{
   AV* dotSUBST_OP=get_cur_dotSUBST_OP(aTHX);
   if (dotSUBST_OP) {
      SV** descrp=AvARRAY(dotSUBST_OP);
      SV** endp=descrp+AvFILLp(dotSUBST_OP);
      for (; descrp<=endp; ++descrp) {
         AV* op_descr=(AV*)SvRV(*descrp);
         if (SvIVX(AvARRAY(op_descr)[PmConstCreationOpCode]) == signature)
            return op_descr;
      }
   }
   return Nullav;
}

static inline
OP* store_in_state_var(pTHX)
{
   OP* store_op=newOP(OP_PADSV, (OPpPAD_STATE | OPpLVAL_INTRO) << 8);
   store_op->op_targ=pad_add_name_pvn("", 0, padadd_STATE | padadd_NO_DUP_CHECK, Nullhv, Nullhv);
   return store_op;
}

static inline
OP* construct_const_creation_optree(pTHX_ AV* op_descr, OP* o, OP* parent)
{
   SV* sub_ref=AvARRAY(op_descr)[PmConstCreationSubRef];
   SV* first_arg=AvARRAY(op_descr)[PmConstCreationFirstArg];
   OP* list_op=op_append_elem(OP_LIST, o, Perl_newSVOP(aTHX_ OP_CONST, 0, SvREFCNT_inc_simple_NN(sub_ref)));
   if (first_arg != PmEmptyArraySlot)
      op_prepend_elem(OP_LIST, Perl_newSVOP(aTHX_ OP_CONST, 0, SvREFCNT_inc_simple_NN(first_arg)), list_op);
   o=Perl_op_convert_list(aTHX_ OP_ENTERSUB, OPf_STACKED, list_op);
   if (parent) {
      OpLASTSIB_set(o, parent);
   } else {
      o=newASSIGNOP(0, store_in_state_var(aTHX), 0, o);
      assert(o->op_type==OP_NULL && cUNOPo->op_first->op_type==OP_ONCE && o->op_private==1);
      o->op_private=4;
   }
   return o;
}

static inline
I32 is_creating_constant(OP* o)
{
   return o->op_type==OP_NULL && cUNOPo->op_first->op_type==OP_ONCE && o->op_private==4;
}

static inline
const char* looks_like_bigint(SV* sv, const char* buf)
{
   int negative;
   while (isSPACE(*buf)) ++buf;
   negative = *buf == '-';
   if (negative || *buf == '+') ++buf;
   if (!isDIGIT(*buf))
      // slipped off the line end - no chance to reconstruct the number,
      // otherwise it's not an integral number and hence not interesting as well
      return NULL;

   // check for integer overflow as well
   if (SvIOK(sv) && (SvIVX(sv) == 0 || (SvIVX(sv) < 0) == negative))
      return NULL;

   do ++buf; while (isDIGIT(*buf));
   // no conversion for hexadecimal numbers and floating-point numbers
   return strchr(".eExX", *buf) ? NULL : buf;
}

static
OP* intercept_ck_const(pTHX_ OP* o)
{
   if (PL_curcop == &PL_compiling && !PL_parser->lex_inwhat) {
      SV *sv=cSVOPo->op_sv;
      const char *buf=PL_parser->bufptr;
      const char *buf_end;
      if (buf && SvPOKp(sv) && buf[0] == 'p' && !strncmp(buf, "package ", 8)) {
         HV *stash;
         char* p=SvPVX(sv);
         if (p[0]=='_') {
            const STRLEN pl=SvCUR(sv);
            if (pl>3 && p[2]==':') {
               if (p[1]==':') {
                  // subpackage of the current package
                  const int cur_pkg_len=SvCUR(PL_curstname);
                  SvPV_set(sv, (char*)safemalloc(pl+cur_pkg_len));
                  SvCUR_set(sv, 0);
                  SvLEN_set(sv, pl+cur_pkg_len);
                  sv_setsv(sv, PL_curstname);
                  sv_catpvn(sv, p+1, pl-1);
                  safefree(p);
               } else if (p[1]=='_' && p[3]==':') {
                  // sibling of the current package
                  const char* cur_pkg=SvPVX(PL_curstname);
                  const char* last_colon=strrchr(cur_pkg, ':');
                  if (last_colon != NULL && last_colon > cur_pkg+2 && last_colon[-1]==':') {
                     const int parent_pkg_len=last_colon-cur_pkg-1;
                     SvPV_set(sv, (char*)safemalloc(pl-1+parent_pkg_len));
                     SvCUR_set(sv, 0);
                     SvLEN_set(sv, pl-1+parent_pkg_len);
                     sv_setpvn(sv, cur_pkg, parent_pkg_len);
                     sv_catpvn(sv, p+2, pl-2);
                     safefree(p);
                  }
               }
            } else if (pl==2 && p[1]=='_') {
               // back to enclosing package
               const char* cur_pkg=SvPVX(PL_curstname);
               const char* last_colon=strrchr(cur_pkg, ':');
               if (last_colon != NULL && last_colon > cur_pkg+2 && last_colon[-1]==':') {
                  const int parent_pkg_len=last_colon-cur_pkg-1;
                  SvPV_set(sv, (char*)safemalloc(1+parent_pkg_len));
                  SvCUR_set(sv, 0);
                  SvLEN_set(sv, 1+parent_pkg_len);
                  sv_setpvn(sv, cur_pkg, parent_pkg_len);
                  safefree(p);
               }
            } else if (pl==1) {
               // staying in the current package, enforce saving it on the savestack
               sv_setsv(sv, PL_curstname);
            }
         }
         stash=gv_stashpvn(SvPVX(sv), SvCUR(sv), TRUE);
         if (stash != PL_defstash && stash != PL_debstash) {
            GV* imp_gv=get_dotIMPORT_GV(aTHX_ stash);
            SV* imp_sv;
            if (cur_lexical_import_ix > 0)
               import_subs_into_pkg(aTHX_ stash, imp_gv, cur_lexical_import_ix);
            if ((imp_sv=GvSV(imp_gv)) && SvIOKp(imp_sv)) {
               // the re-entered package already memorized its lexical import scope: must merge both together
               int new_lex_ix=merge_lexical_import_scopes(aTHX_ SvIV(GvSV(imp_gv)), cur_lexical_import_ix);
               if (new_lex_ix != cur_lexical_import_ix) {
                  SAVEINT(cur_lexical_import_ix);
                  establish_lex_imp_ix(aTHX_ new_lex_ix, TRUE);
               }
            }
         }
      }
      else if (buf && buf==PL_parser->oldbufptr && (SvFLAGS(sv) & (SVf_IOK | SVf_NOK)) && (buf_end=looks_like_bigint(sv, buf)) != NULL) {
         AV* op_descr=find_const_creation_descriptor(aTHX_ 'I'+('I'<<8));
         if (op_descr) {
            const char* minus=PL_parser->oldoldbufptr;
            while (minus < buf) {
               if (isSPACE(*minus)) {
                  ++minus;
               } else {
                  if (*minus == '-') buf=minus;
                  break;
               }
            }
            SvREADONLY_off(sv);
            sv_setpvn(sv, buf, buf_end-buf);
            SvREADONLY_on(sv);
            return construct_const_creation_optree(aTHX_ op_descr, o, Nullop);
         }
      }
   }
   return def_ck_CONST(aTHX_ o);
}

static
OP* intercept_ck_const_op(pTHX_ OP *o)
{
   OP* a=cBINOPo->op_first;
   OP* b=OpSIBLING(a);
   if (( (a->op_type==OP_CONST && SvIOK(cSVOPx_sv(a))) || is_creating_constant(a) )
       &&
       ( (b->op_type==OP_CONST && SvIOK(cSVOPx_sv(b))) || is_creating_constant(b) )) {
      AV* op_descr=find_const_creation_descriptor(aTHX_ o->op_type);
      if (op_descr) {
         OP* new_op=construct_const_creation_optree(aTHX_ op_descr, op_prepend_elem(OP_LIST, a, b), Nullop);
         o->op_flags &= ~OPf_KIDS;
         FreeOp(o);
         return new_op;
      }
   }
   return o;
}

static
OP* intercept_ck_negate_op(pTHX_ OP *o)
{
   OP* a=cUNOPo->op_first;
   if (is_creating_constant(a)) {
      // minus sign is already included in the string constant
      o->op_flags &= ~OPf_KIDS;
      FreeOp(o);
      return a;
   }
   return o;
}

static
OP* intercept_ck_anonlist_op(pTHX_ OP *o)
{
   OP* a=cUNOPo->op_first;
   if (a->op_type==OP_ANONLIST && (a->op_flags & OPf_SPECIAL)) {
      AV* op_descr=find_const_creation_descriptor(aTHX_ o->op_type);
      if (op_descr)
         cUNOPo->op_first=construct_const_creation_optree(aTHX_ op_descr, a, o);
   }
   return o;
}

static
OP* inject_switch_op(pTHX_ OP *o, int flags)
{
   OP* sw_op=newOP(OP_CUSTOM, flags);
   sw_op->op_ppaddr=&switch_off_namespaces;
   cUNOPo->op_first=op_prepend_elem(OP_LINESEQ, sw_op, cUNOPo->op_first);
   return sw_op;
}

static
OP* intercept_ck_leaveeval(pTHX_ OP *o)
{
   inject_switch_op(aTHX_ o, OPf_SPECIAL);
   return def_ck_LEAVEEVAL(aTHX_ o);
}

static
OP* intercept_pp_leavesub(pTHX)
{
   catch_ptrs(aTHX_ active_begin);
   return def_pp_LEAVESUB(aTHX);
}

static inline
MAGIC* fetch_explicit_typelist_magic(pTHX_ SV* args)
{
   return mg_findext(args, PERL_MAGIC_ext, &explicit_typelist_vtbl);
}

static
OP* fetch_sub_scope_type_param(pTHX)
{
   dSP;
   AV* typelist;
   MAGIC* mg=fetch_explicit_typelist_magic(aTHX_ (SV*)GvAV(PL_defgv));
   assert(mg);
   typelist=(AV*)SvRV(mg->mg_obj);
   assert(SvTYPE(typelist)==SVt_PVAV && PL_op->op_private <= AvFILLp(typelist));
   XPUSHs(AvARRAY(typelist)[PL_op->op_private]);
   RETURN;
}

static
OP* fetch_sub_scope_type_param_via_lex(pTHX)
{
   dSP;
   AV* typelist;
   SV* typelist_ref=PAD_SVl(PL_op->op_targ);
   assert(SvROK(typelist_ref));
   typelist=(AV*)SvRV(typelist_ref);
   assert(SvTYPE(typelist)==SVt_PVAV);
   // this is used in final typecheck routines, where some type parameters may be not deduced yet
   XPUSHs(*av_fetch(typelist, PL_op->op_private, TRUE));
   RETURN;
}

static
OP* localize_scope_type_list(pTHX)
{
   dSP;
   SV* cur_list=POPs;
   GV* gv=(GV*)POPs;
   pm_perl_localize_array(aTHX_ (SV*)GvAV(gv), cur_list);
   RETURN;
}

static
OP* pp_instance_of(pTHX)
{
   dSP; dTOPss;
   SV *obj;
   HV *class=(HV*)cSVOP_sv;
   if (SvROK(sv) && (obj=SvRV(sv), SvOBJECT(obj) && SvSTASH(obj)==class)) {
      SETs(&PL_sv_yes);
   } else {
      I32 answer=sv_derived_from(sv, HvNAME(class));
      SPAGAIN;
      SETs(answer ? &PL_sv_yes : &PL_sv_no);
   }
   return NORMAL;
}

static
OP* pp_class_method(pTHX)
{
   dSP;
   SV* method_name=cSVOP_sv;
   SV* first_arg=PL_stack_base[TOPMARK+1];
   const char* class_name=SvPVX(first_arg);
   STRLEN l=SvCUR(first_arg), prefix=0;
   HV* class;
   GV* method_gv;
   GV* io_gv=NULL;
   if (class_name[0]==':' && class_name[1]==':') {
      prefix=2;
   } else if (l>6 && class_name[4]==':' && !memcmp(class_name, "main::", 6)) {
      prefix=6;
   }
   class= prefix ? gv_stashpvn(class_name+prefix, l-prefix, FALSE)
                 : pm_perl_namespace_lookup_class_autoload(aTHX_ CopSTASH(PL_curcop), class_name, l, get_lex_imp_ix);
   SPAGAIN;
   if (!class) {
      /* maybe a file handle method? */
      IO *io_sv;
      if ((io_gv=gv_fetchpv(class_name, FALSE, SVt_PVIO)) && (io_sv=GvIOp(io_gv)) && (IoIFP(io_sv) || IoOFP(io_sv)))
         class=SvSTASH(io_sv);
      else
         Perl_croak(aTHX_ "Package \"%.*s\" does not exist", (int)SvCUR(first_arg), SvPVX(first_arg));

   } else if (SvCUR(method_name)==sizeof(instanceof)-1 && PL_stack_base+TOPMARK+2==SP &&
              !memcmp(SvPVX(method_name),instanceof,sizeof(instanceof)-1)) {
      OP *o=PL_op, *sub_op=o->op_next;
      SetPadnamesOfCurrentSub(padnames_save);
      Perl_op_clear(aTHX_ o);
      o->op_ppaddr=&pp_instance_of;
      cSVOPo->op_sv=SvREFCNT_inc_simple_NN((SV*)class);
      o->op_next=sub_op->op_next;               // skip ENTERSUB
      o=cUNOPx(sub_op)->op_first;
      if (!OpHAS_SIBLING(o)) o=cUNOPo->op_first;
      o->op_ppaddr=&Perl_pp_null;               // suppress PUSHMARK, skip CONST(package_name)
      o->op_next=o->op_next->op_next;
      RestorePadnames(padnames_save);
      SP[-1]=SP[0]; --SP;
      (void)POPMARK; PUTBACK;
      return pp_instance_of(aTHX);
   }

   if ((method_gv=gv_fetchmethod(class, SvPVX(method_name)))) {
      CV* method_cv=GvCV(method_gv);
      OP* o=PL_op;
      SetPadnamesOfCurrentSub(padnames_save);
      Perl_op_clear(aTHX_ o);
      o->op_ppaddr=PL_ppaddr[OP_CONST];
      o->op_type=OP_CONST;
      o->op_flags=OPf_WANT_SCALAR;
      cSVOPo->op_sv=SvREFCNT_inc_simple_NN((SV*)method_cv);
      SPAGAIN;
      XPUSHs((SV*)method_cv);
      if (o->op_next->op_type==OP_RV2CV)
         o->op_next=o->op_next->op_next;
      if (io_gv || (!prefix && (l=HvNAMELEN_get(class)) != SvCUR(first_arg))) {
         o=OpSIBLING(cUNOPx(o->op_next)->op_first);
         Perl_op_clear(aTHX_ o);
         cSVOPo->op_sv=PL_stack_base[TOPMARK+1]= io_gv ? newRV((SV*)io_gv) : newSVpvn_share(HvNAME(class), l, 0);
      }
      RestorePadnames(padnames_save);
   } else {
      Perl_croak(aTHX_ "Can't locate object method \"%.*s\" via package \"%s\"", (int)SvCUR(method_name), SvPVX(method_name), HvNAME(class));
   }
   RETURN;
}

static inline
int find_among_parameter_names(pTHX_ AV* param_names_av, const char* pkg_name, STRLEN pkg_name_len)
{
   SV** param_names=AvARRAY(param_names_av);
   SV** param_names_last=param_names+AvFILLp(param_names_av);
   int name_ix=0;
   for (;  param_names <= param_names_last;  ++param_names, ++name_ix)
      if (pkg_name_len == SvCUR(*param_names) && !strncmp(pkg_name, SvPVX(*param_names), pkg_name_len))
         return name_ix;
   return -1;
}

static
OP* fetch_type_param_proto_pvn(pTHX_ const char* pkg_name, STRLEN pkg_name_len)
{
   SV* hint_sv;
   GV* sub_type_params_gv=NULL;
   if ((hint_sv=Perl_refcounted_he_fetch_sv(aTHX_ PL_compiling.cop_hints_hash, sub_type_params_key, 0, 0)) &&
       SvIOK(hint_sv)) {
      sub_type_params_gv=(GV*)SvUVX(hint_sv);
      if (sub_type_params_gv != PL_defgv) {
         // it does not refer to @_
         int name_ix=find_among_parameter_names(aTHX_ type_param_names, pkg_name, pkg_name_len);
         if (name_ix>=0) {
            OP* o;
            if (sub_type_params_gv != NULL) {
               if ((size_t)sub_type_params_gv <= 10) {
                  // The package name found among the subroutine-local parameters.
                  // At runtime, the prototypes will sit in an array referred by a lexical variable
                  o=newOP(OP_CUSTOM, 0);
                  o->op_ppaddr=&fetch_sub_scope_type_param_via_lex;
                  o->op_targ=(size_t)sub_type_params_gv;
               } else {
                  // The package name found among the placeholders.
                  o=newGVOP(OP_AELEMFAST, 0, sub_type_params_gv);
                  o->op_ppaddr=def_pp_AELEMFAST;
               }
            } else {
               // The package name found among the subroutine-local parameters.
               // At runtime, the prototypes will sit in an array magically attached to @_.
               o=newOP(OP_CUSTOM, 0);
               o->op_ppaddr=&fetch_sub_scope_type_param;
            }
            o->op_private=name_ix;
            return o;
         }
         sub_type_params_gv=NULL;
      }
   }
   if ((hint_sv=Perl_refcounted_he_fetch_sv(aTHX_ PL_compiling.cop_hints_hash, scope_type_params_key, 0, 0)) &&
       SvIOK(hint_sv)) {
      GV* scope_type_params_gv=(GV*)SvUVX(hint_sv);
      int name_ix=find_among_parameter_names(aTHX_ GvAV(scope_type_params_gv), pkg_name, pkg_name_len);
      if (name_ix>=0) {
         // The package name found among the scope parameters.
         // At runtime, the prototypes will sit in the array attached to this glob, unless sub_type_params_gv == \*_.
         OP* o=newGVOP(OP_AELEMFAST, 0, sub_type_params_gv ? sub_type_params_gv : scope_type_params_gv);
         o->op_ppaddr=def_pp_AELEMFAST;
         o->op_private=name_ix;
         // mark for modification in intercept_ck_leavesub
         if (!CvUNIQUE(PL_compcv) && sub_type_params_gv==NULL) CvDEPTH(PL_compcv)=1;
         return o;
      }
   }
   return NULL;
}

static inline
OP* fetch_type_param_proto_sv(pTHX_ SV* pkg_name_sv)
{
   return fetch_type_param_proto_pvn(aTHX_ SvPVX(pkg_name_sv), SvCUR(pkg_name_sv));
}

static inline
I32 is_typeof_call_pvn(const char* meth_name, STRLEN meth_name_len)
{
   return meth_name[0]=='t' && ((meth_name_len==6 && !strncmp(meth_name, "typeof", 6)) ||
                                (meth_name_len==10 && !strncmp(meth_name, "typeof_gen", 10)));
}

static inline
I32 is_typeof_call(pTHX_ OP* o)
{
   SV* meth_name_sv=cSVOPo_sv;
   return is_typeof_call_pvn(SvPVX(meth_name_sv), SvCUR(meth_name_sv));
}

static
OP* intercept_ck_sub(pTHX_ OP* o)
{
   if (PL_curstash!=PL_defstash &&
       (o->op_flags & (OPf_STACKED | OPf_KIDS)) == (OPf_STACKED | OPf_KIDS)) {
      OP* pushmark=cUNOPo->op_first;
      if (pushmark->op_type==OP_PUSHMARK) {
         OP* const_op=OpSIBLING(pushmark);
         if (const_op && const_op->op_type==OP_CONST && (const_op->op_private & OPpCONST_BARE)) {
            OP* meth=cLISTOPo->op_last;
            if (meth->op_type == OP_METHOD_NAMED) {
               OP* fetch_proto=fetch_type_param_proto_sv(aTHX_ cSVOPx_sv(const_op));
               if (fetch_proto != NULL) {
                  if (OpSIBLING(const_op)==meth && is_typeof_call(aTHX_ meth)) {
                     // don't need the entersub at all: "typeof Placeholder" can be replaced with the bare retrieval op
                     op_free(o);
                     return fetch_proto;
                  } else {
                     // insert a call to proto->pkg
                     OP* call_pkg_method;
                     PL_check[OP_ENTERSUB]=def_ck_ENTERSUB;
                     // TODO: optimization: replace with package_retrieval like in Overload.xs
                     call_pkg_method=Perl_op_convert_list(aTHX_ OP_ENTERSUB, OPf_STACKED, op_append_elem(OP_LIST, fetch_proto, NewMETHOD_NAMED_OP("pkg", 3)));
                     PL_check[OP_ENTERSUB]=&intercept_ck_sub;
                     PmOpCopySibling(call_pkg_method, const_op);
                     OpMORESIB_set(pushmark, call_pkg_method);
                     op_free(const_op);
                  }
               } else if (OpSIBLING(const_op)==last_typeof_arg && OpSIBLING(last_typeof_arg)==meth && is_typeof_call(aTHX_ meth)) {
                  // don't need the entersub at all
                  OpMORESIB_set(const_op, meth);
                  op_free(o);
                  OpLASTSIB_set(last_typeof_arg, NULL);
                  return last_typeof_arg;
               } else {
                  // The package name is static.  It will be resolved at runtime.
                  meth->op_ppaddr=&pp_class_method;
               }
            }
         }
      }
   }
   return def_ck_ENTERSUB(aTHX_ o);
}

static
OP* intercept_ck_leavesub(pTHX_ OP *op)
{
   CV *cv=PL_compcv;
   if (cv && SvTYPE(cv)==SVt_PVCV) {
      // it can be a BEGIN sub, prepare for capturing it befre execution
      PL_savebegin=1;
      if (!CvSPECIAL(cv) && CvDEPTH(cv)) {
         // marked in fetch_type_param_proto_pvn :
         // construct a localizing assignment for the outer scope type array,
         // the list of concrete types is delivered by a sub attached to the glob holding the type array
         SV* hint_sv=Perl_refcounted_he_fetch_sv(aTHX_ PL_compiling.cop_hints_hash, scope_type_params_key, 0, 0);
         GV* scope_type_params_gv=(GV*)SvUVX(hint_sv);
         OP *gvop1, *gvop2, *call_typelist_sub, *localize_op;
         OP* o=cUNOPx(op)->op_first;    // lineseq?
         if (!OpHAS_SIBLING(o)) o=cUNOPo->op_first;
         assert(o->op_type == OP_NEXTSTATE || o->op_type == OP_DBSTATE);
         gvop1=newGVOP(OP_GV, 0, scope_type_params_gv);
         gvop1->op_ppaddr=def_pp_GV;
         gvop2=newGVOP(OP_GV, 0, scope_type_params_gv);
         gvop2->op_ppaddr=def_pp_GV;
         PL_check[OP_ENTERSUB]=def_ck_ENTERSUB;
         call_typelist_sub=Perl_op_convert_list(aTHX_ OP_ENTERSUB, OPpENTERSUB_AMPER, newLISTOP(OP_LIST, 0, gvop2, 0));
         PL_check[OP_ENTERSUB]=&intercept_ck_sub;
         localize_op=newBINOP(OP_NULL, OPf_STACKED, Perl_scalar(aTHX_ gvop1), Perl_scalar(aTHX_ call_typelist_sub));
         localize_op->op_type=OP_CUSTOM;
         localize_op->op_ppaddr=&localize_scope_type_list;
         PmOpCopySibling(localize_op, o);
         OpMORESIB_set(o, localize_op);
         CvDEPTH(cv)=0;
      }
   }
   return def_ck_LEAVESUB(aTHX_ op);
}

static
OP* pp_bless_type_expr(pTHX)
{
   OP* next=def_pp_ANONLIST(aTHX);
   dSP;
   SV* list_ref=TOPs;
   AV* list=(AV*)SvRV(list_ref);
   SV** type_ptr=AvARRAY(list);
   SV** type_last=type_ptr+AvFILLp(list);
   sv_bless(list_ref, TypeExpression_stash);
   for (; type_ptr <= type_last; ++type_ptr)
      SvREADONLY_on(*type_ptr);
   return next;
}

static
OP* pp_assign_ro(pTHX)
{
   OP* next=def_pp_SASSIGN(aTHX);
   dSP;
   SvREADONLY_on(TOPs);
   return next;
}

static inline
OP* start_type_op_subtree(pTHX_ OP* interpolated, const char* name, const char* name_end)
{
   STRLEN name_len=name_end-name;
   OP* result=fetch_type_param_proto_pvn(aTHX_ name, name_len);
   if (result) {
      interpolated->op_flags=1;
   } else {
      OP* const_op=newSVOP(OP_CONST, OPf_WANT_SCALAR, newSVpvn_share(name, name_len, 0));
      const_op->op_private=OPpCONST_BARE;
      result=newLISTOP(OP_LIST, 0, const_op, 0);
   }
   return result;
}

static
OP* finalize_type_op_subtree(pTHX_ OP* list_op)
{
   if (list_op->op_type == OP_LIST) {
      OP* meth_op=NewMETHOD_NAMED_OP("typeof", 6);
      meth_op->op_ppaddr=&pp_class_method;
      return Perl_op_convert_list(aTHX_ OP_ENTERSUB, OPf_STACKED, op_append_elem(OP_LIST, list_op, meth_op));
   }
   return list_op;
}

static
const char* skip_interpolated_expression(const char* expr, const char* expr_end)
{
   // skip an expression interpolated in a string:  var or var[expr] or var{$expr} or $var or ${expr}
   // leading $ or @ sign has already been consumed by the caller
   int open_brackets=0;
   int allow_id=TRUE;
   for (; expr < expr_end; ++expr) {
      char c=*expr;
      switch (c) {
      case '[':
      case '{':
      case '(':
         ++open_brackets;
         allow_id=FALSE;
         break;
      case ']':
      case '}':
      case ')':
         if (--open_brackets < 0) return NULL;  // unbalanced brackets?
         break;
      case '>':
      case ',':
         if (open_brackets==0) return expr;  // valid delimiter
         break;
      case '$':
         if (open_brackets==0 && !allow_id) return NULL;
         break;
      default:
         if (open_brackets==0) {
            if (isSPACE(c)) {
               allow_id=FALSE;
            } else {
               if (!(allow_id && isALNUM(c))) return NULL;  // wrong delimiter?
            }
         }
         break;
      }
   }
   return NULL;  // no delimiter found
}

/* Translate a string like "NAME1, NAME2<PARAM,...>, ... "
 * into an op sequence representing the expression NAME1->typeof, NAME2->typeof(PARAM->typeof,...), ...
 * recursively applying the transformation to each parameter.
 * When compiling a parameterized function or a method of a parameterized object type,
 * names occurring in the current parameter lists are replaced with direct references to the array holding them.
 * @param[in,out] outer_list_op a LISTOP to append the generated OPs to.
 * @param[in] interpolated head of the op list of interpolated expressions, they are removed one by one and inserted into the resulting tree
 * @return pointer behind the last consumed char, or NULL if parsing failed
 */
static
const char* construct_type_op_tree(pTHX_ OP* outer_list_op, OP* interpolated, const char* expr, const char* expr_end)
{
   const char* name_start=NULL;
   const char* name_end=NULL;
   OP* cur_list_op=NULL;
   for (;; ++expr) {
      char c;
      if (expr<expr_end) {
         c=*expr;
      } else if (!name_start) {
         break;
      } else {
         c='>';
      }
      if (!name_start) {
         // NAME must start with a letter or an underscore
         if (isIDFIRST(c)) {
            name_start=expr;
         } else if (c=='$' || c=='@') {
            // interpolated expression
            OP* subexpr_op=OpSIBLING(interpolated);
            if (!subexpr_op) {
               expr=NULL;  break;   // something got wrong
            }
            PmOpCopySibling(interpolated, subexpr_op);
            OpLASTSIB_set(subexpr_op, NULL);
            op_append_elem(OP_LIST, outer_list_op, subexpr_op);
            expr=skip_interpolated_expression(expr+1, expr_end);
            if (!expr) return NULL;
            if (*expr == '>') return expr;
            if (*expr != ',') return NULL;

         } else if (!isSPACE(c)) {
            if (c != '>') expr=NULL;   // empty brackets are allowed, otherwise it's something invalid
            break;
         }
      } else {
         if (!name_end) {
            if (isALNUM(c) || c==':') continue;
            name_end=expr;
            cur_list_op=start_type_op_subtree(aTHX_ interpolated, name_start, name_end);
         }
         if (isSPACE(c)) continue;
         if (c =='<') {
            if (cur_list_op->op_type != OP_LIST) {
               Perl_qerror(aTHX_ Perl_mess(aTHX_ "type parameter %.*s can't be parameterized", (int)(name_end-name_start), name_start));
               break;
            }
            expr=construct_type_op_tree(aTHX_ cur_list_op, interpolated, expr+1, expr_end);
            if (!expr) break;
         } else {
            op_append_elem(OP_LIST, outer_list_op, finalize_type_op_subtree(aTHX_ cur_list_op));
            if (c == '>') return expr;
            cur_list_op=NULL;
            name_start=NULL;
            name_end=NULL;
            if (c != ',') return NULL;
         }
      }
   }
   if (cur_list_op) {
      op_free(cur_list_op);
      return NULL;
   }
   return expr;
}

// find variables and expressions interpolated into the type expression,
// detach them from the GLOB op tree and link them as siblings in the tree DFS traversal order
static
OP* collect_interpolated(pTHX_ OP* interpolated, OP* parent)
{
   OP* o=cUNOPx(parent)->op_first;
   OP* pred_sibling=NULL;
   for (;;) {
      OP* next_sibling=OpSIBLING(o);
      // At this early phase optimized OPs like AELEMFAST, AELEMFAST_LEX, GVSV, or MULTIDEREF do not appear in the tree
      // Should this change in the future, corresponding codes must be added here
      if (o->op_type == OP_PADSV ||
          o->op_type == OP_PADAV ||
          o->op_type == OP_AELEM ||
          o->op_type == OP_ASLICE ||
          o->op_type == OP_HELEM ||
          o->op_type == OP_HSLICE ||
          // Ignore the $" reference inserted for an implicit join
          (o->op_type == OP_RV2SV && parent->op_type != OP_JOIN)) {
         if (o->op_next) {
            // reset the next OP pointer to the leftmost kid
            OP* kid=o;
            while (kid->op_flags & OPf_KIDS) kid=cUNOPx(kid)->op_first;
            o->op_next=kid;
         } else if (o->op_type != OP_PADAV && o->op_type != OP_PADAV) {
            Perl_op_linklist(aTHX_ o);
         }
         // append o to the list of interpolated expressions
         OpMORESIB_set(interpolated, o);
         OpLASTSIB_set(o, NULL);
         interpolated=o;

         // remove o from the sibling list
         if (pred_sibling) {
            OpMAYBESIB_set(pred_sibling, next_sibling, parent);
         } else {
            cUNOPx(parent)->op_first=next_sibling;
            if (!next_sibling) {
               parent->op_flags &= ~OPf_KIDS;
               break;
            }
         }
      } else if (o->op_flags & OPf_KIDS) {
         interpolated=collect_interpolated(aTHX_ interpolated, o);
      }
      if (!next_sibling) break;
      pred_sibling=o;
      o=next_sibling;
   }

   return interpolated;
}

static
OP* recognize_template_expr(pTHX_ OP* o)
{
   /* Recognizes:  METHOD CLASS <TYPE_PARAM,...> ( arg, ... )
              or   METHOD CLASS <TYPE_PARAM,...> arg, ...;
              or   FUNCTION<TYPE_PARAM,...> ( arg, ... )

      Expected:
         for GLOB:     bufptr -> '(' , the first arg, or one of expression terminators: ',' ')' ';' '}'
         for READLINE: bufptr -> '<'
         oldbufptr -> '<'
         oldoldbufptr -> METHOD

      Returns: an OP tree to substitute o, or NULL if it's really a readline/glob.
   */
   char* p=PL_parser->bufptr;
   char* m;
   char* m_start;
   char* expr;
   char* expr_end=p;
   OPCODE otype=o->op_type;
   int function_case=FALSE;
   int is_typeof=FALSE;
   OP interpolated;  // dummy head of the interpolated expression list
   OP* result=NULL;

   if (otype==OP_READLINE) {
      if (*p != '<') return NULL;
   } else {
      do --p; while (isSPACE(*p));
      if (*p != '>') return NULL;
   }

   p=PL_parser->oldbufptr;
   while (isSPACE(*p)) --p;
   if (*p != '<') return NULL;
   expr=p+1;                       // for function case
   do --p; while (isSPACE(*p));
   if (!isALNUM(*p)) return NULL;  // p -> last letter in CLASS or FUNCTION

   m=PL_parser->oldoldbufptr;
   while (isSPACE(*m)) ++m;
   if (!isALPHA(*m)) return NULL;
   m_start=m;
   do {
      if (!isALNUM(*m) && *m!=':') return NULL;
      if (m==p) {
         function_case=TRUE; break;
      }
      ++m;
   } while (!isSPACE(*m));

   if (!function_case) {
      is_typeof=is_typeof_call_pvn(m_start, m-m_start);
      do ++m; while (isSPACE(*m));
      if (m>p) return NULL;
      expr=m;                      // -> first letter in CLASS
      if (!isALPHA(*m)) return NULL;
      while (m<p) {
         if (!isALNUM(*m) && *m!=':') return NULL;
         ++m;
      }
   }

   OpLASTSIB_set(&interpolated, NULL);

   if (otype==OP_READLINE) {
      I32 nonempty=FALSE;
      while (*(++expr_end) != '>')
         if (isALNUM(*expr_end)) nonempty=TRUE;
      ++expr_end;
      if (nonempty) {
         OP* operand=cUNOPo->op_first;
         if (operand->op_type == OP_GV) {
            GV* expr_gv=cGVOPx_gv(cUNOPo->op_first);
            IO* io_sv=GvIOp(expr_gv);
            if (io_sv) {
               // the vicious parser created an unsolicited IO handle which will mislead method resolution later
               if (IoIFP(io_sv) || IoOFP(io_sv)) {
                  Perl_qerror(aTHX_ Perl_mess(aTHX_ "Name %.*s used both as a type and a file handle", (int)GvNAMELEN(expr_gv), GvNAME(expr_gv)));
                  return NULL;
               }
               SvREFCNT_dec(io_sv);
               GvIOp(expr_gv)=NULL;
            }
         } else {
            collect_interpolated(aTHX_ &interpolated, o);
         }
      }
   } else {
      collect_interpolated(aTHX_ &interpolated, o);
   }
   interpolated.op_flags=OpHAS_SIBLING(&interpolated) ? 1 : 0;

   // temporarily disable superfluous check
   PL_check[OP_ENTERSUB]=def_ck_ENTERSUB;
   result=newLISTOP(OP_LIST, 0, 0, 0);
   if (construct_type_op_tree(aTHX_ result, &interpolated, expr, expr_end)) {
      OP* store_op= !interpolated.op_flags && !CvUNIQUE(PL_compcv) ? store_in_state_var(aTHX) : NULL;
      if (is_typeof) {
         last_typeof_arg=OpSIBLING(cLISTOPx(result)->op_first);  // recognize it later in ck_entersub
         if (store_op) {
            assert(OpSIBLING(last_typeof_arg)==NULL);
            last_typeof_arg=newASSIGNOP(0, store_op, 0, last_typeof_arg);
            OpMORESIB_set(cLISTOPx(result)->op_first, last_typeof_arg);
            cLISTOPx(result)->op_last=last_typeof_arg;
            OpLASTSIB_set(last_typeof_arg, result);
         }
      } else {
         result=newANONLIST(result);
         result->op_ppaddr=&pp_bless_type_expr;
         if (store_op) {
            result=newASSIGNOP(0, store_op, 0, result);
            OpSIBLING(cLOGOPx(cUNOPx(result)->op_first)->op_first)->op_ppaddr=&pp_assign_ro;
         }
      }
   } else {
      for (o=OpSIBLING(&interpolated); o; ) {
         OP* next=OpSIBLING(o);
         op_free(o);
         o=next;
      }
      op_free(result);
      result=NULL;
      Perl_qerror(aTHX_ Perl_mess(aTHX_ "invalid type expression"));
   }
   PL_check[OP_ENTERSUB]=&intercept_ck_sub;
   return result;
}

static
OP* intercept_ck_glob(pTHX_ OP* o)
{
   OP* const_op=recognize_template_expr(aTHX_ o);
   if (const_op) {
      *--(PL_parser->bufptr)=',';
      op_free(o);
      return const_op;
   }
   reset_ptrs(aTHX_ NULL);
   o=def_ck_GLOB(aTHX_ o);
   catch_ptrs(aTHX_ NULL);
   return o;
}

static
OP* intercept_ck_readline(pTHX_ OP* o)
{
   OP* const_op=recognize_template_expr(aTHX_ o);
   if (const_op) {
      PL_parser->nextval[PL_parser->nexttoke].ival=0;
      PL_parser->nexttype[PL_parser->nexttoke++]=',';
#ifdef LEX_KNOWNEXT
      // these seem to have disappeared since perl 5.25
      PL_parser->lex_defer=PL_parser->lex_state;
      PL_parser->lex_expect=XTERM;
      PL_parser->lex_state=LEX_KNOWNEXT;
#endif
      op_free(o);
      return const_op;
   }
   return def_ck_READLINE(aTHX_ o);
}

static
OP* intercept_pp_entereval(pTHX)
{
   int lex_imp_ix=get_lex_imp_ix;
   OP *next;

   if (current_mode())
      Perl_croak(aTHX_ "namespace mode internal error: compilation mode active during execution");
   cur_lexical_import_ix=lex_imp_ix;
   catch_ptrs(aTHX_ NULL);
   next=def_pp_ENTEREVAL(aTHX);
   if (next != NULL && next->op_ppaddr != &switch_off_namespaces) {
      reset_ptrs(aTHX_ NULL);
      cur_lexical_import_ix=-1;
      cur_lexical_flags=0;
   }
   return next;
}

static
OP* intercept_pp_regcomp(pTHX)
{
   int lex_imp_ix=get_lex_imp_ix;
   OP *next;

   if (current_mode())
      Perl_croak(aTHX_ "namespace mode internal error: compilation mode active during execution");
   cur_lexical_import_ix=lex_imp_ix;
   catch_ptrs(aTHX_ NULL);
   next=def_pp_REGCOMP(aTHX);
   reset_ptrs(aTHX_ NULL);
   cur_lexical_import_ix=-1;
   cur_lexical_flags=0;
   assert(next==NULL || next->op_ppaddr != &switch_off_namespaces);
   return next;
}

#if PerlVersion >= 5220
static
OP* intercept_pp_multideref(pTHX)
{
   OP* o=PL_op;
   OP* next_op=o;
   GV* var_gv=NULL;

   // The following voodoo is a stripped down code from pp_multideref.
   // It has to be aligned with the future development of that monstrous op.

   UNOP_AUX_item* items = cUNOP_AUXo->op_aux;
   UV actions = items->uv;
   o->op_ppaddr=def_pp_MULTIDEREF;

   while (1) {
      switch (actions & MDEREF_ACTION_MASK) {

      case MDEREF_reload:
         actions = (++items)->uv;
         continue;

      case MDEREF_AV_padav_aelem:                 /* $lex[...] */
      case MDEREF_HV_padhv_helem:                 /* $lex{...} */
      case MDEREF_AV_padsv_vivify_rv2av_aelem:    /* $lex->[...] */
      case MDEREF_HV_padsv_vivify_rv2hv_helem:    /* $lex->{...} */
         ++items;
         break;

      case MDEREF_AV_gvav_aelem:                  /* $pkg[...] */
         var_gv=(GV*)UNOP_AUX_item_sv(++items);
         resolve_array_gv(aTHX_ items, var_gv, &next_op, Null(OP*));
         if (next_op != o) return next_op;
         break;

      case MDEREF_HV_gvhv_helem:                  /* $pkg{...} */
         var_gv=(GV*)UNOP_AUX_item_sv(++items);
         resolve_hash_gv(aTHX_ items, var_gv, &next_op, Null(OP*));
         if (next_op != o) return next_op;
         break;

      case MDEREF_AV_gvsv_vivify_rv2av_aelem:     /* $pkg->[...] */
      case MDEREF_HV_gvsv_vivify_rv2hv_helem:     /* $pkg->{...} */
         var_gv=(GV*)UNOP_AUX_item_sv(++items);
         resolve_scalar_gv(aTHX_ items, var_gv, &next_op, Null(OP*));
         if (next_op != o) return next_op;
         break;

      case MDEREF_AV_pop_rv2av_aelem:             /* expr->[...] */
      case MDEREF_HV_pop_rv2hv_helem:             /* expr->{...} */
      case MDEREF_AV_vivify_rv2av_aelem:          /* vivify, ->[...] */
      case MDEREF_HV_vivify_rv2hv_helem:          /* vivify, ->{...} */
         break;

      default:
         Perl_croak(aTHX_ "unknown MULTIDEREF action %d", actions & MDEREF_ACTION_MASK);
      }

      switch (actions & MDEREF_INDEX_MASK) {
      case MDEREF_INDEX_none:
         return next_op;
      case MDEREF_INDEX_const:
      case MDEREF_INDEX_padsv:
         ++items;
         break;
      case MDEREF_INDEX_gvsv:
         var_gv=(GV*)UNOP_AUX_item_sv(++items);
         resolve_scalar_gv(aTHX_ items, var_gv, &next_op, Null(OP*));
         if (next_op != o) return next_op;
         break;
      default:
         Perl_croak(aTHX_ "unknown MULTIDEREF index action %d", actions & MDEREF_INDEX_MASK);
      }

      if (actions & MDEREF_FLAG_last) break;
      actions >>= MDEREF_SHIFT;
   }
   return next_op;
}
#endif

static inline
OP* leave_with_magic_lvalue(pTHX)
{
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
   return Perl_pp_leavesub(aTHX);
}

static
OP* pp_leave_with_magic_lvalue(pTHX)
{
   if (cxstack[cxstack_ix].blk_gimme == G_SCALAR)
      return leave_with_magic_lvalue(aTHX);
   else
      return Perl_pp_leavesub(aTHX);
}

static
OP* pp_leave_maybe_with_lvalue(pTHX)
{
   if (cxstack[cxstack_ix].blk_gimme == G_SCALAR) {
      OP* flag_op=PL_op->op_next;
      SV* flag_sv=PAD_SVl(flag_op->op_targ);
      // the codes 1 and 2 correspond to the return values of is_lvalue in Poly.xs
      if (SvIOK(flag_sv) && SvIVX(flag_sv) != 0) {
         return SvIVX(flag_sv) == 1 ? leave_with_magic_lvalue(aTHX) : Perl_pp_leavesublv(aTHX);
      }
   }
   return Perl_pp_leavesub(aTHX);
}

static
OP* intercept_pp_anoncode(pTHX)
{
   OP* next_op=def_pp_ANONCODE(aTHX);
   if (next_op == PL_op->op_next) {   // not died
      dSP;
      CV* sub=(CV*)TOPs;
      OP* leave=CvROOT(sub);
      OP* flag_op=leave->op_next;
      SV* flag_sv=PAD_BASE_SV(CvPADLIST(sub), flag_op->op_targ);
      if (SvIOK(flag_sv) && SvIVX(flag_sv) != 0) {
         CvFLAGS(sub) |= CVf_LVALUE | CVf_NODEBUG;
      }
   }
   return next_op;
}

static
OP* intercept_ck_anoncode(pTHX_ OP* o)
{
   SV* hint_sv=Perl_refcounted_he_fetch_sv(aTHX_ PL_compiling.cop_hints_hash, anon_lvalue_key, 0, 0);
   CV* sub;
   OP* leave;
   o=def_ck_ANONCODE(aTHX_ o);
   if (!hint_sv || hint_sv==&PL_sv_placeholder) {
      // left the scope
      PL_check[OP_ANONCODE]=def_ck_ANONCODE;
      return o;
   }
   sub=(CV*)PAD_SVl(o->op_targ);
   leave=CvROOT(sub);
   assert(leave->op_type == OP_LEAVESUB);
   if (SvIOK(hint_sv)) {
      // this sub or all its clones are always returning an lvalue
      CvFLAGS(sub) |= CVf_LVALUE | CVf_NODEBUG;
      leave->op_ppaddr= SvIVX(hint_sv) == 1 ? &pp_leave_with_magic_lvalue : PL_ppaddr[OP_LEAVESUBLV];
   } else {
      // the lvalue status depends on the outer context of the closure
      OP* start=CvSTART(sub);
      OP* flag_op=start->op_next;
      PADLIST* sub_padlist=CvPADLIST(sub);
      start=flag_op->op_next;

      if (flag_op->op_type != OP_PADSV || !start || (start->op_type != OP_NEXTSTATE && start->op_type != OP_DBSTATE))
         Perl_croak(aTHX_ "First op in an lvalue anon sub must be a single lex variable");
      {
         PADNAME* flag_name=PadlistNAMESARRAY(sub_padlist)[flag_op->op_targ];
         if (PadnameLEN(flag_name) != SvCUR(hint_sv) || strncmp(PadnamePV(flag_name), SvPVX(hint_sv), SvCUR(hint_sv)))
            Perl_croak(aTHX_ "found flag lexical variable %.*s while %.*s expected",
                       (int)SvCUR(hint_sv), SvPVX(hint_sv), (int)PadnameLEN(flag_name), PadnamePV(flag_name));
#if PerlVersion >= 5180
         if (!PadnameOUTER(flag_name))
            Perl_croak(aTHX_ "flag lexical variable must be captured from outer scope");
#endif
      }

      // the flag variable itself does not contribute to the result, can be short-cut
      CvSTART(sub)=start;
      leave->op_ppaddr=&pp_leave_maybe_with_lvalue;
      leave->op_next=flag_op;
      o->op_ppaddr=&intercept_pp_anoncode;
   }
   return o;
}

static
void store_anon_lvalue_flag(pTHX_ SV* flag_sv)
{
   MAGIC hint_mg;
   hint_mg.mg_len=HEf_SVKEY;
   hint_mg.mg_ptr=(char*)anon_lvalue_key;
   Perl_magic_sethint(aTHX_ flag_sv, &hint_mg);
   PL_check[OP_ANONCODE]=&intercept_ck_anoncode;
}


static
HV* lookup_class_in_pkg(pTHX_ HV* stash, const char* class_name, const char* first_colon, const char* buf, size_t buflen)
{
   const char *colon=first_colon;
   GV **imp_class_gvp;

   if (colon) {
      const char *class_name_part=class_name, *next_colon=colon;
      do {
         size_t l=(colon=next_colon+2)-class_name_part;
         imp_class_gvp=(GV**)hv_fetch(stash, class_name_part, l, FALSE);
         if (!imp_class_gvp || SvTYPE(*imp_class_gvp)!=SVt_PVGV || !(stash=GvHV(*imp_class_gvp)))
            return Nullhv;
      } while ((next_colon=strchr((class_name_part=colon), ':')));
      buf+=class_name_part-class_name;
      buflen-=(class_name_part-class_name);
   }
   if ((imp_class_gvp=(GV**)hv_fetch(stash, buf, buflen, FALSE)) && SvTYPE(*imp_class_gvp)==SVt_PVGV)
       return GvHV(*imp_class_gvp);
   return Nullhv;
}

HV* pm_perl_namespace_lookup_class(pTHX_ HV* stash, const char* class_name, STRLEN class_namelen, int lex_imp_ix)
{
   HV* imp_class=NULL;
   HV* glob_class=NULL;
   SV* cached_stash;
   AV* dotLOOKUP= stash==last_stash ? last_dotLOOKUP : get_dotLOOKUP(aTHX_ stash);
   HV* pkgLOOKUP=last_pkgLOOKUP;
   if (pkgLOOKUP == NULL) return gv_stashpvn(class_name, class_namelen, FALSE);

   cached_stash=*hv_fetch(pkgLOOKUP, class_name, class_namelen, TRUE);
   if (SvROK(cached_stash))
      return (HV*)SvRV(cached_stash);
   if (SvIOK(cached_stash)) {
      return lex_imp_ix<=0 ? Nullhv
                           : pm_perl_namespace_lookup_class(aTHX_ (HV*)SvRV(AvARRAY(lexical_imports)[lex_imp_ix]), class_name, class_namelen, -1);
   }
   {
      const char* first_colon=memchr(class_name, ':', class_namelen);
      size_t l=class_namelen+2;
      char smallbuf[64];
      char *buf;
      if (l<sizeof(smallbuf))
         buf=smallbuf;
      else
         New(0, buf, l+1, char);
      Copy(class_name, buf, l-2, char);
      buf[l-2]=':';  buf[l-1]=':';  buf[l]=0;

      if (!(imp_class=lookup_class_in_pkg(aTHX_ stash, class_name, first_colon, buf, l)) && dotLOOKUP) {
         SV **lookp=AvARRAY(dotLOOKUP), **endp;
         if (lookp) {
            for (endp=lookp+AvFILLp(dotLOOKUP); lookp<=endp &&
                    !(imp_class=lookup_class_in_pkg(aTHX_ (HV*)SvRV(*lookp), class_name, first_colon, buf, l)); ++lookp) ;
         }
      }
      if (buf != smallbuf) Safefree(buf);

      if (!imp_class && lex_imp_ix>=0) {
         if (lex_imp_ix>0)
            imp_class=pm_perl_namespace_lookup_class(aTHX_ (HV*)SvRV(AvARRAY(lexical_imports)[lex_imp_ix]), class_name, class_namelen, -1);
         if ((glob_class=gv_stashpvn(class_name, class_namelen, FALSE)) != NULL && is_dummy_pkg(aTHX_ glob_class))
            glob_class=NULL;
         if (imp_class) {
            if (!glob_class || glob_class!=imp_class) {
               // lexical scope prevails over global lookup
               sv_setiv(cached_stash,1);
               return imp_class;
            }
         }
         imp_class=glob_class;
      }

      if (imp_class) {
         (void)SvUPGRADE(cached_stash, SVt_RV);
         SvRV_set(cached_stash, SvREFCNT_inc_simple_NN(imp_class));
         SvROK_on(cached_stash);
      } else if (lex_imp_ix>=0) {
         sv_setiv(cached_stash,0);
      }
   }
   return imp_class;
}

HV* pm_perl_namespace_lookup_class_autoload(pTHX_ HV* stash, const char* class_name, STRLEN class_namelen, int lex_imp_ix)
{
   HV* result=pm_perl_namespace_lookup_class(aTHX_ stash, class_name, class_namelen, lex_imp_ix);
   if (!result) {
      HE* he=hv_fetch_ent(stash, dot_autolookup_key, FALSE, SvSHARED_HASH(dot_autolookup_key));
      GV* gv;
      CV* auto_sub;
      if (he && (gv=(GV*)HeVAL(he), SvTYPE(gv)==SVt_PVGV && (auto_sub=GvCV(gv)) != NULL)) {
         int found=FALSE;
         dSP;
         PUSHMARK(SP);
         mXPUSHp(class_name, class_namelen);
         PUTBACK;
         if (call_sv((SV*)auto_sub, G_SCALAR | G_EVAL)) {
            SV* ret;
            SPAGAIN;
            ret=POPs;
            found=SvTRUE(ret);
            PUTBACK;
         }
         if (found)
            result=pm_perl_namespace_lookup_class(aTHX_ stash, class_name, class_namelen, lex_imp_ix);
      }
   }
   return result;
}

CV* pm_perl_namespace_lookup_sub(pTHX_ HV* stash, const char* name, STRLEN namelen, CV* lex_context_cv)
{
   GV *gv;
   const int lex_imp_ix=lex_context_cv ? get_lex_imp_ix_from_cv(lex_context_cv) : 0;

   const char *colon=strrchr(name,':');
   if (colon && --colon > name && *colon==':') {
      // (at least partially) qualified: look for the package first
      stash=pm_perl_namespace_lookup_class_autoload(aTHX_ stash, name, colon-name, lex_imp_ix);
      if (stash == Nullhv) return Nullcv;
      colon+=2;
      namelen-=colon-name;
      name=colon;
   }

   gv=lookup_var(aTHX_ stash, name, namelen, SVt_PVCV, TRUE);
   if (!gv && !colon && lex_imp_ix) {
      // unqualified and not found in the given package: look into the lexical scope
      gv=lookup_var(aTHX_ (HV*)SvRV(AvARRAY(lexical_imports)[lex_imp_ix]), name, namelen, SVt_PVCV, TRUE);
   }

   return gv ? GvCV(gv) : Nullcv;
}

static inline
void switch_check_const_op(pTHX_ AV* dotSUBST_OP, I32 enable)
{
   if (dotSUBST_OP) {
      int method_index=PmConstCreationReset+enable;
      SV** descrp=AvARRAY(dotSUBST_OP);
      SV** endp=descrp+AvFILLp(dotSUBST_OP);
      for (; descrp<=endp; ++descrp) {
         AV* op_descr=(AV*)SvRV(*descrp);
         SV* method_sv=AvARRAY(op_descr)[method_index];
         if (method_sv != PmEmptyArraySlot)
            PL_check[SvIVX(AvARRAY(op_descr)[PmConstCreationOpCode])]=(Perl_check_t)SvUVX(method_sv);
      }
   }
}

static
void establish_lex_imp_ix(pTHX_ int new_ix, int new_mode)
{
   cur_lexical_flags |= new_mode & (LexCtxAutodeclare | LexCtxAllowReDeclare);
   if (!current_mode()) {
      cur_lexical_import_ix=new_ix;
      catch_ptrs(aTHX_ NULL);
   } else if (new_mode) {
      AV *old_dotSUBST_OP=get_cur_dotSUBST_OP(aTHX);
      switch_check_const_op(aTHX_ old_dotSUBST_OP, FALSE);
      cur_lexical_import_ix=new_ix;
      switch_check_const_op(aTHX_ get_cur_dotSUBST_OP(aTHX), TRUE);
   } else {
      reset_ptrs(aTHX_ NULL);
      cur_lexical_import_ix=new_ix;
   }
   set_lexical_scope_hint(aTHX);
}

static
OP* mark_dbstate(pTHX)
{
   return def_pp_DBSTATE(aTHX);
}

#if defined(PMCollectCoverage)
static inline
void store_cov_line(pTHX_ COP* cop, int cnt)
{
   // skip "(eval NNN)" and anonymous filtered code
   const char* filename=CopFILE(cop);
   if (filename[0] != '(' && strncmp(filename, "/loader/0x", 10)) {
      AV* hits_av;
      SV* hitcnt;
      SV* file_entry=*hv_fetch(cov_stats, filename, strlen(filename), TRUE);
      if (SvROK(file_entry)) {
         hits_av=(AV*)SvRV(file_entry);
      } else {
         hits_av=newAV();
         sv_upgrade(file_entry, SVt_RV);
         SvRV_set(file_entry, (SV*)hits_av);
         SvROK_on(file_entry);
      }
      hitcnt=*av_fetch(hits_av, CopLINE(cop)-1, TRUE);
      if (SvIOK(hitcnt)) {
         SvIV_set(hitcnt, SvIVX(hitcnt)+cnt);
      } else {
         sv_setiv(hitcnt, cnt);
      }
   }
}

static
void scan_op_tree(pTHX_ OP* o)
{
   // recursively visit all OP nodes and announce all NEXTSTATEs because they carry the line numbers.
   while (o) {
      if (o->op_type == OP_NEXTSTATE) {
         store_cov_line(aTHX_ (COP*)o, 0);
      } else if (o->op_flags & OPf_KIDS) {
         scan_op_tree(aTHX_ cUNOPo->op_first);
      }
      o=OpSIBLING(o);
   }
}

static
void intercept_peep(pTHX_ OP* o)
{
   def_peep(aTHX_ o);
   scan_op_tree(aTHX_ o);
}

static
OP* intercept_pp_nextstate(pTHX)
{
   COP* o=(COP*)PL_op;
   store_cov_line(aTHX_ o, 1);
   return def_pp_NEXTSTATE(aTHX);
}
#endif

static
void catch_ptrs(pTHX_ void* to_restore)
{
   if (to_restore) {
      finish_undo(aTHX_ (ToRestore*)to_restore);
   } else {
      PL_hints &= ~HINT_STRICT_VARS;
   }

   if (!to_restore || !current_mode()) {
      SV* beginav=(SV*)PL_beginav_save;
      SvRMAGICAL_on(beginav);

      PL_ppaddr[OP_GV]       =&intercept_pp_gv;
      PL_ppaddr[OP_GVSV]     =&intercept_pp_gvsv;
      PL_ppaddr[OP_AELEMFAST]=&intercept_pp_aelemfast;
      PL_ppaddr[OP_SPLIT]    =&intercept_pp_split;
      PL_ppaddr[OP_ENTEREVAL]=&intercept_pp_entereval;
      PL_ppaddr[OP_REGCOMP]  =&intercept_pp_regcomp;
      PL_ppaddr[OP_RV2GV]    =&intercept_pp_rv2gv;
      PL_ppaddr[OP_DBSTATE]  =&mark_dbstate;
#if PerlVersion >= 5220
      PL_ppaddr[OP_MULTIDEREF]=&intercept_pp_multideref;
#endif
      PL_check[OP_CONST]     =&intercept_ck_const;
      PL_check[OP_ENTERSUB]  =&intercept_ck_sub;
      PL_check[OP_LEAVESUB]  =&intercept_ck_leavesub;
      PL_check[OP_LEAVEEVAL] =&intercept_ck_leaveeval;
      PL_check[OP_GLOB]      =&intercept_ck_glob;
      PL_check[OP_READLINE]  =&intercept_ck_readline;
      PL_check[OP_GV]        =&intercept_ck_gv;
      PL_check[OP_RV2SV]     =&intercept_ck_rv2sv;
      PL_check[OP_RV2AV]     =&intercept_ck_rv2av;
      PL_check[OP_RV2HV]     =&intercept_ck_rv2hv;
#if defined(PMCollectCoverage)
      if (cov_stats) {
         PL_peepp               =&intercept_peep;
         PL_ppaddr[OP_NEXTSTATE]=&intercept_pp_nextstate;
         PL_perldb |= PERLDBf_NOOPT;
      }
#endif
      if (cur_lexical_import_ix > 0)
         switch_check_const_op(aTHX_ get_cur_dotSUBST_OP(aTHX), TRUE);
      if (AvFILLp(plugin_data) >= 0) {
         SV **pl, **ple;
         namespace_plugin_fun_ptr *pf=(namespace_plugin_fun_ptr*)SvPVX(plugin_code);
         for (pl=AvARRAY(plugin_data), ple=pl+AvFILLp(plugin_data); pl<=ple; ++pl, pf+=2)
            (*pf)(aTHX_ *pl);
      }
   }
}

static
void reset_ptrs(pTHX_ void* to_restore)
{
   if (to_restore) {
      finish_undo(aTHX_ (ToRestore*)to_restore);
   } else {
      PL_hints |= HINT_STRICT_VARS;
   }
   if (!to_restore || current_mode()) {
      SV* beginav=(SV*)PL_beginav_save;
      SvRMAGICAL_off(beginav);
      PL_savebegin=0;

      PL_ppaddr[OP_GV]       =def_pp_GV;
      PL_ppaddr[OP_GVSV]     =def_pp_GVSV;
      PL_ppaddr[OP_AELEMFAST]=def_pp_AELEMFAST;
      PL_ppaddr[OP_SPLIT]    =def_pp_SPLIT;
      PL_ppaddr[OP_ENTEREVAL]=def_pp_ENTEREVAL;
      PL_ppaddr[OP_REGCOMP]  =def_pp_REGCOMP;
      PL_ppaddr[OP_RV2GV]    =def_pp_RV2GV;
      PL_ppaddr[OP_DBSTATE]  =def_pp_DBSTATE;
#if PerlVersion >= 5220
      PL_ppaddr[OP_MULTIDEREF]=def_pp_MULTIDEREF;
#endif
      PL_check[OP_CONST]     =def_ck_CONST;
      PL_check[OP_ENTERSUB]  =def_ck_ENTERSUB;
      PL_check[OP_LEAVESUB]  =def_ck_LEAVESUB;
      PL_check[OP_LEAVEEVAL] =def_ck_LEAVEEVAL;
      PL_check[OP_GLOB]      =def_ck_GLOB;
      PL_check[OP_READLINE]  =def_ck_READLINE;
      PL_check[OP_GV]        =def_ck_GV;
      PL_check[OP_RV2SV]     =def_ck_RV2SV;
      PL_check[OP_RV2AV]     =def_ck_RV2AV;
      PL_check[OP_RV2HV]     =def_ck_RV2HV;
      PL_check[OP_ANONCODE]  =def_ck_ANONCODE;
#if defined(PMCollectCoverage)
      if (cov_stats) {
         PL_peepp               =def_peep;
         PL_ppaddr[OP_NEXTSTATE]=def_pp_NEXTSTATE;
         PL_perldb &= ~PERLDBf_NOOPT;
      }
#endif
      if (cur_lexical_import_ix > 0)
         switch_check_const_op(aTHX_ get_cur_dotSUBST_OP(aTHX), FALSE);
      if (AvFILLp(plugin_data) >= 0) {
         SV **pl, **ple;
         namespace_plugin_fun_ptr *pf=(namespace_plugin_fun_ptr*)SvPVX(plugin_code); ++pf;
         for (pl=AvARRAY(plugin_data), ple=pl+AvFILLp(plugin_data); pl<=ple; ++pl, pf+=2)
            (*pf)(aTHX_ *pl);
      }
   }
}

// TRUE if executing a BEGIN { } block called from a scope enabled with namespace mode
static inline
int imported_from_mode(pTHX)
{
   int answer=FALSE;
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;
   if (active_begin && active_begin->old_state) {
      while (cx > cx_bottom) {
         CV *beg_cv;
         if (CxTYPE(cx)==CXt_SUB && (beg_cv=cx->blk_sub.cv, CvSPECIAL(beg_cv))) {
            --cx;
            if (pm_perl_skip_debug_cx) {
               while ((CxTYPE(cx)==CXt_BLOCK && CopSTASH_eq(cx->blk_oldcop,PL_debstash)) ||
                      (CxTYPE(cx)==CXt_SUB && CvSTASH(cx->blk_sub.cv)==PL_debstash)) --cx;
            }
            if (CxTYPE(cx)==CXt_EVAL && beg_cv == active_begin->cv) {
               answer=TRUE;
            }
            break;
         }
         --cx;
      }
   }
   return answer;
}

void pm_perl_namespace_register_plugin(pTHX_ namespace_plugin_fun_ptr enabler, namespace_plugin_fun_ptr disabler, SV *data)
{
   namespace_plugin_fun_ptr *pf;
   STRLEN pl=SvCUR(plugin_code);
   SvGROW(plugin_code, pl+sizeof(namespace_plugin_fun_ptr)*2);
   pf=(namespace_plugin_fun_ptr*)(SvPVX(plugin_code)+pl);
   pf[0]=enabler; pf[1]=disabler;
   SvCUR_set(plugin_code,pl+sizeof(namespace_plugin_fun_ptr)*2);
   av_push(plugin_data, data);
}

static
OP* db_caller_scope(pTHX)
{
   PERL_CONTEXT *cx_bottom=cxstack, *cx=cx_bottom+cxstack_ix;
   while (cx > cx_bottom) {
      if (CxTYPE(cx)==CXt_SUB) {
         COP *o=cx->blk_oldcop;
         if (o->op_ppaddr==&mark_dbstate) {
            dSP;
            SV *sv=TOPs;
            if (SvREADONLY(sv)) { sv=sv_mortalcopy(sv); SETs(sv); }
            sv_catpvf(sv, " use namespaces %d (); ", extract_lex_imp_ix(aTHX_ o));
         }
         break;
      }
      --cx;
   }
   return NORMAL;
}

MODULE = namespaces             PACKAGE = namespaces

PROTOTYPES: DISABLE

void
import(...)
PPCODE:
{
   AV* new_imports=NULL;
   int i=1;
   const char* n=NULL;
   int remove=FALSE, new_ix=0, skip_frames=0, flags=0;
   STRLEN l;
   SV *arg;

   if (items>=1 && (arg=ST(1), SvIOK(arg))) {
      // special call from another import routine: skip that many stack frames
      flags=SvIVX(arg) & (LexCtxAutodeclare | LexCtxAllowReDeclare);
      skip_frames=SvIVX(arg) & ~(LexCtxAutodeclare | LexCtxAllowReDeclare);
      ++i;
   }
   if (cur_lexical_import_ix<0) {
      // first call in this compilation unit: must prepare the restore destructor
      insert_undo(aTHX_ skip_frames);
      if (items==i) {
         // no lexical-scope lookup list specified
         establish_lex_imp_ix(aTHX_ 0, TRUE | flags);
         XSRETURN_EMPTY;
      }
      arg=ST(i);
      if (SvPOK(arg)) {
         n=SvPV(arg,l);
         if (l==1 && (*n=='+' || *n=='-'))
            Perl_croak(aTHX_ "the namespace lookup list cannot be modified in the very first 'use namespaces' call");
      }

   } else {
      if (items==i) {
         // reset to an empty lookup list
         establish_lex_imp_ix(aTHX_ 0, TRUE | flags);
         XSRETURN_EMPTY;
      }
      arg=ST(i);
      if (SvPOK(arg)) {
         n=SvPV(arg,l);
         if (l==1 && (*n=='+' || *n=='-')) {
            SV *cur_entry=AvARRAY(lexical_imports)[cur_lexical_import_ix];
            if (items==2)
               Perl_croak(aTHX_ "empty namespace lookup modification list");

            if (SvROK(cur_entry)) {
               HV *imp_stash=(HV*)SvRV(cur_entry);
               if (HvNAME(imp_stash)[0] == '-') {
                  // already one of our shadow stashes
                  AV *prev_import=get_dotIMPORT(aTHX_ imp_stash);
                  new_imports=av_make(AvFILLp(prev_import)+1, AvARRAY(prev_import));
               } else {
                  // a regular stash
                  new_imports=newAV();
                  av_push(new_imports, newRV((SV*)imp_stash));
               }
            }
            remove= *n=='-';
            ++i;
         }
      }
   }

   if (i+1==items && SvROK(arg) && (arg=SvRV(arg), SvTYPE(arg)==SVt_PVCV)) {
      // shortcut for lexical-scope lookup inheritance
      new_ix=get_lex_imp_ix_from_cv(arg);
      establish_lex_imp_ix(aTHX_ new_ix, TRUE | flags);
      XSRETURN_EMPTY;
   }

   if (!new_imports) new_imports=newAV();

   for (; i<items; ++i) {
      const char *n=SvPV(ST(i),l);
      HV *imp_stash=gv_stashpvn(n, l, FALSE);
      if (imp_stash) {
         if (remove)
            remove_imp_stash(aTHX_ new_imports, imp_stash);
         else
            append_imp_stash(aTHX_ new_imports, imp_stash);
      }
   }

   switch (AvFILLp(new_imports)) {
   case -1:
      // the lookup list became empty
      new_ix=0;
      break;
   case 0:
      // exactly one stash to look up in
      new_ix=store_lex_lookup_stash(aTHX_ AvARRAY(new_imports)[0]);
      break;
   default:
      new_ix=store_shadow_lex_lookup_stash(aTHX_ new_imports);
      break;
   }
   SvREFCNT_dec(new_imports);
   establish_lex_imp_ix(aTHX_ new_ix, TRUE | flags);
}

void
import_subs()
PPCODE:
{
   if (cur_lexical_import_ix > 0) {
      HV *stash=CopSTASH(PL_curcop);
      import_subs_into_pkg(aTHX_ stash, (GV*)HeVAL(hv_fetch_ent(stash, dot_import_key, TRUE, SvSHARED_HASH(dot_import_key))), cur_lexical_import_ix);
   }
}

void
import_subs_from(sub)
   SV *sub;
PPCODE:
{
   if (!SvROK(sub) || (sub=SvRV(sub), SvTYPE(sub)!=SVt_PVCV))
      croak_xs_usage(cv, "\\&sub");
   else {
      HV *stash=CopSTASH(PL_curcop);
      import_subs_into_pkg(aTHX_ stash, (GV*)HeVAL(hv_fetch_ent(stash, dot_import_key, TRUE, SvSHARED_HASH(dot_import_key))), get_lex_imp_ix_from_cv(sub));
   }
}

void
unimport(...)
PPCODE:
{
   if (!current_mode()) XSRETURN_EMPTY;
   if (items>1) Perl_croak(aTHX_ "'no namespaces' cannot have any arguments");
   establish_lex_imp_ix(aTHX_ 0, FALSE);
}

void
VERSION(self,ix)
   SV* self;
   I32 ix;
PPCODE:
{
   PERL_UNUSED_ARG(self);
   if (ix<0 || ix>AvFILLp(lexical_imports))
      Perl_croak(aTHX_ "namespaces: lexical scope index %d out of range", (int)ix);
   establish_lex_imp_ix(aTHX_ ix, TRUE);
}

void
memorize_lexical_scope()
PPCODE:
{
   HE *imp_gve=hv_fetch_ent(CopSTASH(PL_curcop), dot_import_key, FALSE, SvSHARED_HASH(dot_import_key));
   if (imp_gve) {
      sv_setiv(GvSVn((GV*)HeVAL(imp_gve)), get_lex_imp_ix);
   } else {
      Perl_croak(aTHX_ "package %s was defined in a non-namespace enviromnent", CopSTASHPV(PL_curcop));
   }
}

void
tell_lexical_scope()
PPCODE:
{
   dTARGET;
   XPUSHi(get_lex_imp_ix);
}

void
temp_disable()
CODE:
{
   if (current_mode()) {
      reset_ptrs(aTHX_ NULL);
      LEAVE;
      SAVEDESTRUCTOR_X(&catch_ptrs, NULL);
      SAVEINT(cur_lexical_import_ix);
      SAVEINT(cur_lexical_flags);
      SAVEVPTR(PL_compcv);
      cur_lexical_import_ix=-1;
      cur_lexical_flags=0;
      PL_compcv=Nullcv;       // new OPs needed for code restructuring must not be allocated in the op-slabs of the current cv
      ENTER;
   }
}

void
is_active()
PPCODE:
{
   dTARGET;
   PUSHi(current_mode());
}

void
using(dst, ...)
   SV* dst;
CODE:
{
   HV* caller_stash=
      (SvCUR(dst)==10 && !memcmp(SvPVX(dst),"namespaces",10))
      ? (last_stash=NULL, CopSTASH(PL_curcop))
      : gv_stashpvn(SvPVX(dst), SvCUR(dst), TRUE);
   I32 i;
   AV* dotLOOKUP=NULL;
   AV* dotIMPORT=NULL;
   AV* dotSUBST_OP=NULL;
   AV* imp_dotSUBST_OP;
   AV* dotSUBS=NULL;
   AV* imp_dotSUBS;
   GV* av_gv;
   HE* av_gve=hv_fetch_ent(caller_stash, dot_lookup_key, FALSE, SvSHARED_HASH(dot_lookup_key));
   if (!(av_gve != NULL && (av_gv=(GV*)HeVAL(av_gve), SvTYPE(av_gv)==SVt_PVGV && (dotLOOKUP=GvAV(av_gv)) != NULL)))
      dotIMPORT=get_dotIMPORT(aTHX_ caller_stash);

   for (i=1; i<items; ++i) {
      HV *imp_stash=gv_stashsv(ST(i), FALSE);
      if (!imp_stash) continue;
      if (imp_stash != caller_stash) {
         if (dotIMPORT) {
            av_push(dotIMPORT, newRV((SV*)imp_stash));
         } else if (append_imp_stash(aTHX_ dotLOOKUP, imp_stash)) {
            AV *imp_dotLOOKUP=get_dotLOOKUP(aTHX_ imp_stash);
            if (imp_dotLOOKUP) append_lookup(aTHX_ caller_stash, dotLOOKUP, imp_dotLOOKUP, FALSE);
         }
         if ((imp_dotSUBST_OP=get_dotSUBST_OP(imp_stash,FALSE)))
            dotSUBST_OP=merge_dotSUBST_OP(aTHX_ caller_stash, dotSUBST_OP, imp_dotSUBST_OP);
         if ((imp_dotSUBS=get_dotSUBS(imp_stash,FALSE))) {
            import_dotSUBS(aTHX_ caller_stash, imp_dotSUBS);
            dotSUBS=merge_dotSUBS(aTHX_ caller_stash, dotSUBS, imp_dotSUBS);
         }
      }
   }

   if (dotSUBST_OP && cur_lexical_import_ix > 0 && (HV*)SvRV(AvARRAY(lexical_imports)[cur_lexical_import_ix])==caller_stash)
      switch_check_const_op(aTHX_ dotSUBST_OP, TRUE);
   if (dotIMPORT && last_stash==caller_stash)
      last_stash=NULL;
}

void
lookup(pkg, item_name)
   SV *pkg;
   SV *item_name;
PPCODE:
{
   STRLEN namelen;
   const char *name;
   HV *stash;
   GV *gv;
   if (SvROK(pkg)) {
      stash=SvSTASH(SvRV(pkg));
   } else {
      name=SvPV(pkg,namelen);
      stash=gv_stashpvn(name, namelen, FALSE);
   }
   if (stash) {
      I32 type=0;

      name=SvPV(item_name,namelen);
      switch (name[0]) {
      case '$':
         type=SVt_PV,   ++name, --namelen;  break;
      case '@':
         type=SVt_PVAV, ++name, --namelen;  break;
      case '%':
         type=SVt_PVHV, ++name, --namelen;  break;
      case '&':
         type=SVt_PVCV, ++name, --namelen;  break;
      default:
         if (isIDFIRST(name[0])) {
            type=SVt_PVCV;  break;
         } else {
            Perl_croak(aTHX_ "namespaces::lookup internal error: unknown name type %c", name[0]);
         }
      }

      gv=lookup_var(aTHX_ stash, name, namelen, type, TRUE);
      if (gv) {
         SV* found=NULL;
         switch (type) {
         case SVt_PV: {
            found=GvSV(gv);
            break;
         }
         case SVt_PVAV: {
            found=(SV*)GvAV(gv);
            break;
         }
         case SVt_PVHV: {
            found=(SV*)GvHV(gv);
            break;
         }
         case SVt_PVCV: {
            CV *cv=GvCV(gv);
            if (IsWellDefinedSub(cv)) found=(SV*)cv;
            break;
         }
         }
         if (found) {
            PUSHs(sv_2mortal(newRV(found)));
            XSRETURN(1);
         }
      }
   }
   XSRETURN_UNDEF;
}

void
lookup_class(pkg, class, ...)
   SV* pkg;
   SV* class;
PPCODE:
{
   STRLEN classl, pkgl;
   const char* classn;
   const char* pkgn;
   HV* stash;
   HV* class_stash;
   HV* lex_ctx_stash;
   if (items>3) croak_xs_usage(cv, "\"pkg\", \"class\" [, \"lex_scope_pkg\" ]");

   classn=SvPV(class, classl);
   pkgn=SvPV(pkg, pkgl);
   stash=gv_stashpvn(pkgn, pkgl, FALSE);
   if (stash) {
      I32 lex_ix=0;
      GV *imp_gv;
      HE *imp_gve;
      if (items==3 && (pkg=ST(2), SvPOK(pkg))) {
         pkgn=SvPV(pkg, pkgl);
         lex_ctx_stash=gv_stashpvn(pkgn, pkgl, FALSE);
      } else {
         lex_ctx_stash=stash;
      }
      imp_gve=hv_fetch_ent(lex_ctx_stash, dot_import_key, FALSE, SvSHARED_HASH(dot_import_key));
      if (imp_gve && (imp_gv=(GV*)HeVAL(imp_gve), SvIOKp(GvSVn(imp_gv))))
         lex_ix=SvIVX(GvSV(imp_gv));
      if ((class_stash=pm_perl_namespace_lookup_class_autoload(aTHX_ stash, classn, classl, lex_ix))) {
         dTARGET;
         sv_setpv(TARG, HvNAME(class_stash));
         PUSHs(TARG);
         XSRETURN(1);
      }
   }
   class_stash=gv_stashpvn(classn, classl, FALSE);
   if (class_stash && !is_dummy_pkg(aTHX_ class_stash)) {
      ST(0)=ST(items-1);
      XSRETURN(1);
   }
   XSRETURN_UNDEF;
}

void
lookup_class_in_caller_scope(stash_ref, class)
   SV* stash_ref;
   SV* class;
PPCODE:
{
   STRLEN classl;
   const char* classn=SvPV(class, classl);
   HV* stash=(HV*)SvRV(stash_ref);
   HV* class_stash=pm_perl_namespace_lookup_class(aTHX_ stash, classn, classl, active_begin->cur_lex_imp);
   if (class_stash != NULL) {
      dTARGET;
      sv_setpv(TARG, HvNAME(class_stash));
      PUSHs(TARG);
      XSRETURN(1);
   }
   class_stash=gv_stashpvn(classn, classl, FALSE);
   if (class_stash != NULL && !is_dummy_pkg(aTHX_ class_stash)) {
      ST(0)=ST(1);
      XSRETURN(1);
   }
   XSRETURN_UNDEF;
}

void
create_dummy_pkg(pkg_name)
   SV* pkg_name;
PPCODE:
{
   HV* stash=gv_stashsv(pkg_name, TRUE);
   set_dotDUMMY_PKG(aTHX_ stash);
}

void
declare(...)
PPCODE:
{
   PERL_UNUSED_VAR(items);
   if (!(get_lex_flags(aTHX) & LexCtxAutodeclare))
   {
      // detect declare local with optional enclosed assigment and defuse it,
      // otherwise complain
      OP* o=PL_op;
      OP* assign_op=Nullop;
      OP* first_arg=Nullop;
      o=cUNOPo->op_first;
      if (o->op_type==OP_NULL) o=cLISTOPo->op_first;
      assert(o->op_type==OP_PUSHMARK);
      o=OpSIBLING(o);
      if ((o->op_type==OP_SASSIGN || o->op_type==OP_AASSIGN) && !(o->op_private & OPpASSIGN_BACKWARDS)) {
         assign_op=o;
         first_arg=cBINOPo->op_last;
         if (o->op_type==OP_AASSIGN) {
            if (first_arg->op_type==OP_NULL) first_arg=cUNOPx(first_arg)->op_first;
            assert(first_arg->op_type==OP_PUSHMARK);
            first_arg=OpSIBLING(first_arg);
         }
      } else {
         first_arg=o;
      }
      if (first_arg->op_type==OP_NULL) first_arg=cUNOPx(first_arg)->op_first;
      if ((first_arg->op_type==OP_GVSV || first_arg->op_type==OP_RV2AV || first_arg->op_type == OP_RV2HV)
          && (first_arg->op_private & OPpLVAL_INTRO)) {

         if (assign_op) {
            /* change to void context */
            assign_op->op_flags &= ~OPf_WANT;
            assign_op->op_flags |= OPf_WANT_VOID;
         }
         while (OpHAS_SIBLING(o)) o=OpSIBLING(o);
         if (o->op_type==OP_NULL) o=cUNOPo->op_first;
         assert(o->op_type == OP_GV && GvCV(cGVOPo_gv) == declare_cv && o->op_next == PL_op);
         /* skip entersub */
         o->op_ppaddr=&pp_popmark;
         o->op_next=o->op_next->op_next;
         XSRETURN_EMPTY;
      }
   }
   Perl_croak(aTHX_ "multiple declaration of a variable");
}

void
declare_const(gv, value)
   GV *gv=(GV*)ST(0);
   SV *value;
PPCODE:
{
   SV *const_sv;
   if (SvROK(gv)) gv=(GV*)SvRV(gv);
   const_sv=GvSVn(gv);
   sv_setsv(const_sv, value);
   write_protect_on(aTHX_ const_sv);
   GvIMPORTED_SV_on(gv);
}

void
declare_var(pkg, var)
   SV *pkg;
   SV *var;
PPCODE:
{
   STRLEN varnamelen;
   const char *varname=SvPV(var,varnamelen);
   HV *stash;
   GV *gv;
   SV *sv;
   if (SvROK(pkg)) {
      stash=(HV*)SvRV(pkg);
      if (SvTYPE(stash) != SVt_PVHV) croak_xs_usage(cv, "\\stash, \"[$@%%]varname\"");
   } else if (SvPOK(pkg)) {
      stash=gv_stashsv(pkg, FALSE);
      if (!stash) Perl_croak(aTHX_ "package %.*s does not exist", (int)SvCUR(pkg), SvPVX(pkg));
   } else {
      croak_xs_usage(cv, "\"pkg\", \"[$@%%]varname\"");
   }
   gv=*(GV**)hv_fetch(stash,varname+1,varnamelen-1,TRUE);
   if (SvTYPE(gv) != SVt_PVGV)
      gv_init(gv, stash, varname+1, varnamelen-1, GV_ADDMULTI);
   switch (varname[0]) {
   case '$':
      sv=GvSVn(gv);
      GvIMPORTED_SV_on(gv);
      break;
   case '@':
      sv=(SV*)GvAVn(gv);
      GvIMPORTED_AV_on(gv);
      break;
   case '%':
      sv=(SV*)GvHVn(gv);
      GvIMPORTED_HV_on(gv);
      break;
   default:
      Perl_croak(aTHX_ "unknown variable type '%c': one of [$@%%] expected", varname[0]);
   }
   if (GIMME_V != G_VOID) PUSHs(sv_2mortal(newRV(sv)));
}

void
intercept_const_creation(pkg, op_sign, subr, ...)
   SV* pkg;
   const char* op_sign;
   SV* subr;
PPCODE:
{
   HV* stash= SvPOK(pkg) ? gv_stashsv(pkg, FALSE) : SvROK(pkg) ? (HV*)SvRV(pkg) : CopSTASH(PL_curcop);
   SV* first_arg= items==4 ? ST(3) : NULL;
   AV* dotSUBST_OP;

   if (!stash || SvTYPE(stash) != SVt_PVHV || !SvROK(subr) || SvTYPE(SvRV(subr)) != SVt_PVCV || items>4)
      croak_xs_usage(cv, "\"pkg\" | undef, \"op_sign\", \\&sub [, first_arg ]");

   dotSUBST_OP=get_dotSUBST_OP(stash, TRUE);

   switch (*op_sign) {
   case '/': {
      AV* op_descr1=newAV();
      AV* op_descr2=newAV();
      SV* reset=newSVuv((UV)PL_check[OP_DIVIDE]);
      SV* catch=newSVuv((UV)&intercept_ck_const_op);
      av_extend(op_descr1, PmConstCreationCatch);
      av_extend(op_descr2, PmConstCreationCatch);
      av_store(op_descr1, PmConstCreationOpCode, newSViv(OP_DIVIDE));
      av_store(op_descr2, PmConstCreationOpCode, newSViv(OP_I_DIVIDE));
      av_store(op_descr1, PmConstCreationSubRef, SvREFCNT_inc_simple_NN(subr));
      av_store(op_descr2, PmConstCreationSubRef, SvREFCNT_inc_simple_NN(subr));
      if (first_arg) {
         av_store(op_descr1, PmConstCreationFirstArg, newSVsv(first_arg));
         av_store(op_descr2, PmConstCreationFirstArg, newSVsv(first_arg));
      }
      av_store(op_descr1, PmConstCreationReset, reset);
      av_store(op_descr2, PmConstCreationReset, SvREFCNT_inc_simple_NN(reset));
      av_store(op_descr1, PmConstCreationCatch, catch);
      av_store(op_descr2, PmConstCreationCatch, SvREFCNT_inc_simple_NN(catch));
      av_push(dotSUBST_OP, newRV_noinc((SV*)op_descr1));
      av_push(dotSUBST_OP, newRV_noinc((SV*)op_descr2));
      break;
   }
   case '~': {
      AV* op_descr=newAV();
      SV* reset=newSVuv((UV)PL_check[OP_COMPLEMENT]);
      SV* catch=newSVuv((UV)&intercept_ck_anonlist_op);
      av_extend(op_descr, PmConstCreationCatch);
      av_store(op_descr, PmConstCreationOpCode, newSViv(OP_COMPLEMENT));
      av_store(op_descr, PmConstCreationSubRef, SvREFCNT_inc_simple_NN(subr));
      if (first_arg)
         av_store(op_descr, PmConstCreationFirstArg, newSVsv(first_arg));
      av_store(op_descr, PmConstCreationReset, reset);
      av_store(op_descr, PmConstCreationCatch, catch);
      av_push(dotSUBST_OP, newRV_noinc((SV*)op_descr));
      break;
   }
   case 'I': {
      AV* op_descr1=newAV();
      AV* op_descr2=newAV();
      SV* reset=newSVuv((UV)PL_check[OP_NEGATE]);
      SV* catch=newSVuv((UV)&intercept_ck_negate_op);
      av_extend(op_descr1, PmConstCreationCatch);
      av_extend(op_descr2, PmConstCreationCatch);
      av_store(op_descr1, PmConstCreationOpCode, newSViv('I'+('I'<<8)));
      av_store(op_descr2, PmConstCreationOpCode, newSViv(OP_NEGATE));
      av_store(op_descr1, PmConstCreationSubRef, SvREFCNT_inc_simple_NN(subr));
      if (first_arg)
         av_store(op_descr1, PmConstCreationFirstArg, newSVsv(first_arg));
      av_store(op_descr2, PmConstCreationReset, reset);
      av_store(op_descr2, PmConstCreationCatch, catch);
      av_push(dotSUBST_OP, newRV_noinc((SV*)op_descr1));
      av_push(dotSUBST_OP, newRV_noinc((SV*)op_descr2));
      break;
   }
   default:
      Perl_croak(aTHX_ "intercepting '%s' operation is not supported", op_sign);
   }
}

void
export_sub(pkg, subr)
   SV* pkg;
   SV* subr;
PPCODE:
{
   HV* stash= SvPOK(pkg) ? gv_stashsv(pkg, FALSE) : SvROK(pkg) ? (HV*)SvRV(pkg) : CopSTASH(PL_curcop);

   if (!stash || SvTYPE(stash) != SVt_PVHV || !SvROK(subr) || SvTYPE(SvRV(subr)) != SVt_PVCV)
      croak_xs_usage(cv, "\"pkg\", \\&sub");
   {
      // export into the (fake) packages with partial names, so that the sub is found via qualified lookup
      const char* pkgname=HvNAME(stash);
      const char* colon = pkgname + HvNAMELEN_get(stash) - 1;
      AV* dotSUBS=get_dotSUBS(stash, TRUE);
      SV* cgv=SvREFCNT_inc_simple_NN(CvGV(SvRV(subr)));
      int taillen=0;
      for (; colon>pkgname; --colon, ++taillen) {
         if (colon[0]==':' && colon[-1]==':') {
            predeclare_sub(aTHX_ gv_stashpvn(colon+1, taillen, TRUE), (GV*)cgv);
            colon-=2;  taillen+=2;
         }
      }
      av_push(dotSUBS, cgv);
   }
}

void
caller_scope()
PPCODE:
{
   dTARGET;
   if (imported_from_mode(aTHX))
      sv_setpvf(TARG, "use namespaces %d ();", active_begin->cur_lex_imp);
   else
      sv_setpvn(TARG, "no namespaces;", 14);
   XPUSHs(TARG);
}

void
fall_off_to_nextstate(subr)
   SV *subr;
PPCODE:
{
   SV* sub;
   if (SvROK(subr) && (sub=SvRV(subr), SvTYPE(sub)==SVt_PVCV) && !CvISXSUB(sub) && CvROOT(sub)->op_type==OP_LEAVESUB) {
      CvROOT(sub)->op_ppaddr=&pp_fall_off_to_nextstate;
   } else {
      croak_xs_usage(cv, "\\&sub");
   }
}

void
skip_return()
PPCODE:
{
   PERL_CONTEXT* cx;
   OP* op_next_state=next_statement_in_caller(aTHX_ &cx);
   if (op_next_state != NULL) {
      op_next_state->op_ppaddr=&pp_popmark_and_nextstate;
      cx->blk_sub.retop=op_next_state;
   }
}

void
store_explicit_typelist(args_ref)
   SV* args_ref;
PPCODE:
{
   AV* args=(AV*)SvRV(args_ref);
   MAGIC* mg=fetch_explicit_typelist_magic(aTHX_ (SV*)args);
   dTARGET;
   if (mg==NULL) {
      SV* list_ref;
      AV* src_av;
      AV* dst_av=NULL;
      I32 num_types=0;
      if (AvFILLp(args)>=0 &&
          (list_ref=AvARRAY(args)[0], SvROK(list_ref)) &&
          (src_av=(AV*)SvRV(list_ref),
           SvTYPE(src_av)==SVt_PVAV && SvSTASH(src_av)==TypeExpression_stash)) {
         list_ref=av_shift(args);
         if (AvREAL(args)) SvREFCNT_dec(list_ref);  // account for shift()
         num_types=AvFILLp(src_av)+1;
         assert(num_types != 0);
         if (SvREADONLY(list_ref)) {
            // the type list constructed once; make a temporary copy, because it can be changed during type deduction
            SV **src, **src_end, **dst;
            dst_av=newAV();
            av_fill(dst_av, num_types-1);
            dst=AvARRAY(dst_av);
            for (src=AvARRAY(src_av), src_end=src+num_types; src < src_end; ++src, ++dst)
               *dst=SvREFCNT_inc_simple_NN(*src);
            list_ref=newRV_noinc((SV*)dst_av);
         }
      } else {
         dst_av=newAV();
         list_ref=newRV_noinc((SV*)dst_av);
      }
      mg=sv_magicext((SV*)args, list_ref, PERL_MAGIC_ext, &explicit_typelist_vtbl, Nullch, 0);
      if (dst_av) SvREFCNT_dec(list_ref);  // list_ref is exclusively owned by MAGIC, but sv_magicext always bumps the refcounter
      mg->mg_private=num_types;
   }
   PUSHi(mg->mg_private);
   if (GIMME_V == G_ARRAY) XPUSHs(mg->mg_obj);
}

void
fetch_explicit_typelist(args_ref)
   SV* args_ref;
PPCODE:
{
   MAGIC* mg=fetch_explicit_typelist_magic(aTHX_ SvRV(args_ref));
   if (mg) {
      PUSHs(mg->mg_obj);
      if (GIMME_V == G_ARRAY) {
         dTARGET;
         XPUSHi(mg->mg_private);
      }
   }
}

void
collecting_coverage()
PPCODE:
{
#if defined(PMCollectCoverage)
   if (cov_stats)
      XSRETURN_YES;
#endif
   XSRETURN_NO;
}

void
flush_coverage_stats()
PPCODE:
{
#if defined(PMCollectCoverage)
   if (covfile) {
      HE *entry;
      hv_iterinit(cov_stats);
      while ((entry = hv_iternext(cov_stats))) {
         STRLEN srcfile_len;
         AV* hits_av=(AV*)SvRV(HeVAL(entry));
         if (AvFILLp(hits_av) >= 0) {
            SV **hit, **hit_last;
            const char* srcfile=HePV(entry, srcfile_len);
            fwrite(srcfile, 1, srcfile_len, covfile);
            for (hit=&AvARRAY(hits_av)[0], hit_last=hit+AvFILLp(hits_av);
                 hit <= hit_last; ++hit) {
               if ((PerlVersion < 5200 || *hit != Nullsv) && SvIOK(*hit)) {
                  fprintf(covfile, " %d", (int)SvIVX(*hit));
               } else {
                  fwrite(" -", 1, 2, covfile);
               }
            }
            fputc('\n', covfile);
         }
      }
      fclose(covfile);
   }
#endif
}

MODULE = namespaces             PACKAGE = namespaces::AnonLvalue

void
import(pkg, varname)
   SV* pkg;
   SV* varname;
PPCODE:
{
   if (!SvPOK(varname) || SvCUR(varname)<2 || SvPVX(varname)[0]!='$')
      croak_xs_usage(cv, "$varname");
   store_anon_lvalue_flag(aTHX_ varname);
   PERL_UNUSED_ARG(pkg);
}

void
VERSION(pkg, flag)
   SV* pkg;
   SV* flag;
PPCODE:
{
   if (!SvIOK(flag) || SvIVX(flag) < 1 || SvIVX(flag) > 2)
      croak_xs_usage(cv, "1 | 2");
   store_anon_lvalue_flag(aTHX_ flag);
   PERL_UNUSED_ARG(pkg);
}
   

MODULE = namespaces             PACKAGE = namespaces::Params

void
import(...)
PPCODE:
{
   AV* store_names_in=NULL;
   int first_name=0;
   SV* lead=ST(1);
   GV* list_gv=NULL;

   MAGIC hint_mg;
   hint_mg.mg_len=HEf_SVKEY;

   if (items<=1)
      croak_xs_usage(cv, "[ *glob | \\*glob ] 'PARAM1' ...");

   if (SvTYPE(lead)==SVt_PVGV) {
      // scope level
      list_gv=(GV*)lead;

      if (items==2) {
         // reopening an object scope
         if (!GvAV(list_gv)) XSRETURN_EMPTY;
      } else {
         // declaring a new type
         store_names_in=GvAVn(list_gv);
         first_name=2;
      }
      hint_mg.mg_ptr=(char*)scope_type_params_key;
      SvUVX(uv_hint)=(size_t)list_gv;
      Perl_magic_sethint(aTHX_ uv_hint, &hint_mg);

   } else {
      // sub level
      if (SvROK(lead)) {
         // prototype objects stored in a persistent array or passed directly in @_
         list_gv=(GV*)SvRV(lead);
         if (SvTYPE(list_gv) != SVt_PVGV ||
             (items==2) != (list_gv==PL_defgv))
            croak_xs_usage(cv, "[ *glob | \\*glob ] 'PARAM1' ... or \\*_");

         if (items > 2) {
            store_names_in=type_param_names;
            first_name=2;
         }
      } else {
         if (items > 2 && !SvOK(lead) && SvPADMY(lead)) {
            // prototype object array reference stored in a local variable
#if PerlVersion >= 5180
            CV* compiled_cv=PL_compcv;
            I32 my_var_padix=PL_comppad_name_fill;
#else
            // For BEGIN block a separate compcv was created
            CV* compiled_cv=PL_compcv->sv_any->xcv_outside;
            I32 my_var_padix=AvFILLp(AvARRAY(compiled_cv->sv_any->xcv_padlist)[0]);
#endif
            for (; my_var_padix>0; --my_var_padix) {
               SV* my_var=PAD_BASE_SV(CvPADLIST(compiled_cv), my_var_padix);
               if (my_var == lead) {
                  list_gv=(GV*)(size_t)my_var_padix;
                  break;
               }
            }
            if (my_var_padix==0)
               Perl_croak(aTHX_ "passed lexical variable not found in the current PAD");
            first_name=2;
         } else {
            // prototype objects MAGICally attached to @_
            first_name=1;
         }
         store_names_in=type_param_names;
      }
      hint_mg.mg_ptr=(char*)sub_type_params_key;
      SvUVX(uv_hint)=(size_t)list_gv;
      Perl_magic_sethint(aTHX_ uv_hint, &hint_mg);
   }
   if (store_names_in) {
      SV** store_names_at;
      av_fill(store_names_in, items-first_name-1);
      for (store_names_at=AvARRAY(store_names_in);  first_name<items;  ++store_names_at, ++first_name)
         *store_names_at=SvREFCNT_inc_simple_NN(ST(first_name));
   }
}

MODULE = namespaces             PACKAGE = namespaces::BeginAV

void
PUSH(avref, sv)
   SV* avref;
   SV* sv;
PPCODE:
{
   // This is called immediately before execution of the BEGIN subroutine.
   // Its task is to temporarily switch off the compilation mode unless this is the follow-up 'use namespaces'
   SV* beginav=SvRV(avref);
   CV* cv=(CV*)sv;
   ToRestore* to_restore;
   int require_seen=FALSE;
   OP* rootop=CvROOT(cv);
   OP* o;
   assert(beginav==(SV*)PL_beginav_save);
   assert(rootop->op_type==OP_LEAVESUB);
   o=cUNOPx(rootop)->op_first;    // lineseq?
   if (!OpHAS_SIBLING(o)) o=cUNOPo->op_first;
   while ((o=OpSIBLING(o))) {
      if (o->op_type == OP_REQUIRE) {
         SV* filename;
         o=cUNOPo->op_first;
         filename=cSVOPo->op_sv;
         if (filename == NULL)
            filename=PadARRAY((PadlistARRAY(CvPADLIST(cv)))[1])[o->op_targ];
         if (hv_exists_ent(special_imports, filename, 0)) {
            SvRMAGICAL_off(beginav);
            av_push((AV*)beginav, sv);
            SvRMAGICAL_on(beginav);
            return;
         }
         require_seen=TRUE;
         break;
      }
   }
   to_restore=newToRestore(aTHX_ TRUE);
   active_begin=to_restore;
   reset_ptrs(aTHX_ NULL);
   rootop->op_ppaddr=&intercept_pp_leavesub;
   if (require_seen) {
      to_restore->cv=cv;
      cur_lexical_import_ix=-1;
      cur_lexical_flags=0;
   }
   av_push((AV*)beginav, sv);
}


BOOT:
{
   SV* cvar;
   last_stash=NULL;
   lexical_imports=get_av("namespaces::LEXICAL_IMPORTS", TRUE);
   plugin_data=get_av("namespaces::PLUGINS", TRUE);
   plugin_code=get_sv("namespaces::PLUGINS", TRUE);
   sv_setpvn(plugin_code,"",0);
   declare_cv=get_cv("namespaces::declare", FALSE);
   cvar=get_sv("namespaces::auto_declare", TRUE);
   sv_setiv(cvar, LexCtxAutodeclare);
   SvREADONLY_on(cvar);
   cvar=get_sv("namespaces::allow_redeclare", TRUE);
   sv_setiv(cvar, LexCtxAllowReDeclare);
   SvREADONLY_on(cvar);
   TypeExpression_stash=gv_stashpvn(TypeExpression_pkg, sizeof(TypeExpression_pkg)-1, TRUE);
   args_lookup_stash=gv_stashpvn("args", 4, TRUE);
   special_imports=get_hv("namespaces::special_imports", TRUE);
   if (PL_DBgv) {
      // find the initialization of $usercontext in sub DB::DB and inject our code there
      static const char usercontext[]="usercontext";
      OP* o=CvSTART(GvCV(PL_DBgv));
      for (; o; o=OpSIBLING(o)) {
         if (o->op_type==OP_SASSIGN) {
            OP *gvop=cBINOPo->op_last;
            if (gvop->op_type==OP_NULL) gvop=cUNOPx(gvop)->op_first;
            if (gvop->op_type==OP_GVSV) {
               GV *gv;
#ifdef USE_ITHREADS
               SV **saved_curpad=PL_curpad;
               PL_curpad=PadARRAY((PadlistARRAY(CvPADLIST(GvCV(PL_DBgv))))[1]);
#endif
               gv=cGVOPx_gv(gvop);
#ifdef USE_ITHREADS
               PL_curpad=saved_curpad;
#endif
               if (GvNAMELEN(gv)==sizeof(usercontext)-1 && !strncmp(GvNAME(gv),usercontext,sizeof(usercontext)-1)) {
                  o=cBINOPo->op_first;
                  if (o->op_type == OP_CONCAT) {
                     // perl <= 5.16
                     OP* const_op=cBINOPo->op_first;
                     OP* null_op=cBINOPo->op_last;
                     if (null_op->op_type==OP_NULL) {
                        null_op->op_ppaddr=&db_caller_scope;
                        null_op->op_next=const_op->op_next;
                        const_op->op_next=null_op;
                     }
                  } else if (o->op_type == OP_ENTERSUB) {
                     // perl >= 5.18
                     OP* null_op=cUNOPo->op_first;
                     if (null_op->op_type==OP_NULL) {
                        null_op->op_ppaddr=&db_caller_scope;
                        null_op->op_next=o->op_next;
                        o->op_next=null_op;
                     }
                  }
                  break;
               }
            }
         }
      }
      CvNODEBUG_on(get_cv("namespaces::import", FALSE));
      CvNODEBUG_on(get_cv("namespaces::unimport", FALSE));
      CvNODEBUG_on(get_cv("namespaces::temp_disable", FALSE));
      CvNODEBUG_on(get_cv("namespaces::intercept_const_creation", FALSE));
      CvNODEBUG_on(get_cv("namespaces::caller_scope", FALSE));
      CvNODEBUG_on(get_cv("namespaces::skip_return", FALSE));
      CvNODEBUG_on(get_cv("namespaces::store_explicit_typelist", FALSE));
      CvNODEBUG_on(get_cv("namespaces::fetch_explicit_typelist", FALSE));
      CvNODEBUG_on(get_cv("namespaces::Params::import", FALSE));
      CvNODEBUG_on(get_cv("namespaces::BeginAV::PUSH", FALSE));
   }
   def_pp_GV       =PL_ppaddr[OP_GV];
   def_pp_GVSV     =PL_ppaddr[OP_GVSV];
   def_pp_AELEMFAST=PL_ppaddr[OP_AELEMFAST];
   def_pp_PADAV    =PL_ppaddr[OP_PADAV];
   def_pp_SPLIT    =PL_ppaddr[OP_SPLIT];
   def_pp_LEAVESUB =PL_ppaddr[OP_LEAVESUB];
   def_pp_ENTEREVAL=PL_ppaddr[OP_ENTEREVAL];
   def_pp_REGCOMP  =PL_ppaddr[OP_REGCOMP];
   def_pp_RV2GV    =PL_ppaddr[OP_RV2GV];
   def_pp_NEXTSTATE=PL_ppaddr[OP_NEXTSTATE];
   def_pp_DBSTATE  =PL_ppaddr[OP_DBSTATE];
   def_pp_ANONLIST =PL_ppaddr[OP_ANONLIST];
   def_pp_ANONCODE =PL_ppaddr[OP_ANONCODE];
   def_pp_SASSIGN  =PL_ppaddr[OP_SASSIGN];
#if PerlVersion >= 5220
   def_pp_MULTIDEREF=PL_ppaddr[OP_MULTIDEREF];
#endif
   def_ck_CONST    =PL_check[OP_CONST];
   def_ck_ENTERSUB =PL_check[OP_ENTERSUB];
   def_ck_LEAVESUB =PL_check[OP_LEAVESUB];
   def_ck_LEAVEEVAL=PL_check[OP_LEAVEEVAL];
   def_ck_GLOB     =PL_check[OP_GLOB];
   def_ck_READLINE =PL_check[OP_READLINE];
   def_ck_GV       =PL_check[OP_GV];
   def_ck_RV2SV    =PL_check[OP_RV2SV];
   def_ck_RV2AV    =PL_check[OP_RV2AV];
   def_ck_RV2HV    =PL_check[OP_RV2HV];
   def_ck_ANONCODE =PL_check[OP_ANONCODE];

   if (PL_beginav_save == NULL)
      PL_beginav_save=newAV();
   {
      SV* beginav=(SV*)PL_beginav_save;
      HV* beginav_stash=gv_stashpvn(BeginAV_pkg, sizeof(BeginAV_pkg)-1, TRUE);
      SV* beginav_ref=sv_2mortal(newRV(beginav));
      sv_bless(beginav_ref, beginav_stash);
      sv_magicext(beginav, Nullsv, PERL_MAGIC_tied, Null(MGVTBL*), Nullch, 0);
      SvMAGICAL_off(beginav);
   }
#if defined(PMCollectCoverage)
   {
      const char* covfilename=getenv("POLYMAKE_COVERAGE_FILE");
      if (covfilename) {
         const char* open_mode="w";
         if (covfilename[0] == '+') {
            open_mode="a";
            ++covfilename;
         }
         covfile=fopen(covfilename, open_mode);
         if (!covfile)
            Perl_croak(aTHX_ "can't create coverage file %s: %s\n", covfilename, Strerror(errno));
         def_peep=PL_peepp;
         cov_stats=newHV();
         Perl_av_create_and_push(aTHX_ &PL_endav, SvREFCNT_inc(get_cv("namespaces::flush_coverage_stats", FALSE)));
      }
   }
#endif
   dot_lookup_key=newSVpvn_share(".LOOKUP",7,0);
   dot_import_key=newSVpvn_share(".IMPORT",7,0);
   dot_autolookup_key=newSVpvn_share(".AUTOLOOKUP",11,0);
   dot_dummy_pkg_key=newSVpvn_share(".DUMMY_PKG",10,0);
   dot_subst_op_key=newSVpvn_share(".SUBST_OP",9,0);
   dot_subs_key=newSVpvn_share(".SUBS",5,0);
   declare_key=newSVpvn_share("declare",7,0);
   lex_imp_key=newSVpvn_share("lex_imp",7,0);
   sub_type_params_key=newSVpvn_share("sub_typp",8,0);
   scope_type_params_key=newSVpvn_share("scp_typp",8,0);
   anon_lvalue_key=newSVpvn_share("anonlval",8,0);
   type_param_names=newAV();
   iv_hint=newSViv(0);
   uv_hint=newSVuv(0);
}

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
