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

#ifndef POLYMAKE_PERL_EXT_H
#define POLYMAKE_PERL_EXT_H

#include <EXTERN.h>
#include <perl.h>
#if PerlVersion >= 5140
  START_EXTERN_C
# include <pp_proto.h>
  END_EXTERN_C
#endif

#ifdef PERL_IMPLICIT_CONTEXT
// pass the interpreter via my_perl variable as far as possible, avoid expensive pthread_getspecific
#  define PERL_NO_GET_CONTEXT
#  define getTHX aTHX
#else
#  ifdef dTHX
#    undef dTHX
#    define dTHX
#  endif
#  ifdef dTHXa
#    undef dTHXa
#    define dTHXa(a)
#  endif
#  define getTHX NULL
#endif
// avoid annoying compiler warnings about unused variables
#ifndef PERL_GLOBAL_STRUCT
#  ifdef dVAR
#    undef dVAR
#  endif
#endif
// perl < 5.18.0 does not meet new clang demands
#if defined(__cplusplus) && PerlVersion < 5180
#  ifdef dNOOP
#    undef dNOOP
#  endif
#  define dNOOP (void)0
#endif
#include <XSUB.h>

#include "polymake/perl/constants.h"

extern I32 pm_perl_skip_debug_cx;
#define SkipDebugSub(cv) (pm_perl_skip_debug_cx && CvSTASH(cv)==PL_debstash)
#define SkipDebugFrame(cx,plus_level) (pm_perl_skip_debug_cx && ((plus_level && CvSTASH(cx->blk_sub.cv)==PL_debstash) || CopSTASH_eq(cx->blk_oldcop,PL_debstash)))

#if PerlVersion >= 5120
#  define ForLoopArray(cx)      (cx)->blk_loop.state_u.ary.ary
EXTERN_C AV* Perl_av_fake(pTHX_ I32 size, SV **strp);
#  define SAVEt_PADSV SAVEt_PADSV_AND_MORTALIZE
#  define PM_svt_copy_klen_arg  I32
#else
#  define ForLoopArray(cx)      (cx)->blk_loop.iterary
#  define PM_svt_copy_klen_arg  int
#endif

// CvROOT and CvXSUB are in the same union
#define IsWellDefinedSub(x)   (CvROOT(x) != NULL)

// PerlVersion < 5101
#ifndef CxHASARGS
#  define CxHASARGS(cx)         (cx)->blk_sub.hasargs
#endif
#ifndef croak_xs_usage
#  define croak_xs_usage(cv, text) Perl_croak(aTHX_ "usage: %*.s(" text ")", (int)GvNAMELEN(CvGV(cv)), GvNAME(CvGV(cv)))
#endif

// PerlVersion < 5140
#ifndef GvCV_set
#  define GvCV_set(gv,cv) GvCV(gv)=cv
#endif
#ifndef CvGV_set
#  define CvGV_set(cv,gv) CvGV(cv)=gv
#endif
#ifndef CvSTASH_set
#  define CvSTASH_set(cv,st) CvSTASH(cv)=st
#endif
#ifndef op_append_elem
#  define op_append_elem(type, first, last) Perl_append_elem(aTHX_ type, first, last)
#endif
#ifndef op_prepend_elem
#  define op_prepend_elem(type, first, last) Perl_prepend_elem(aTHX_ type, first, last)
#endif
#if PerlVersion < 5140
MAGIC* pm_perl_mg_findext(const SV *sv, int type, const MGVTBL *vtbl);
#define mg_findext(sv,type,vtbl) pm_perl_mg_findext(sv, type, vtbl)
#endif

// PerlVersion < 5180
#ifndef PadlistARRAY
# define PadlistARRAY(x) ((AV**)AvARRAY(x))
# define PadARRAY(x)     ((SV**)AvARRAY(x))
# define PadMAX(x)       AvFILLp(x)
# define PadlistMAX(x)   AvFILLp(x)
#endif
#ifndef ReANY
# if PerlVersion < 5120
#  define ReANY(x) (x)
#  define RXp_PAREN_NAMES(rx) ((rx)->paren_names)
# else
#  define ReANY(x) ((struct regexp *)SvANY(x))
# endif
#endif

// PerlVersion < 5220 does not provide macros for manipulating op siblings
#ifndef OpHAS_SIBLING
# define OpHAS_SIBLING(o) ((o)->op_sibling != NULL)
# define OpSIBLING(o) ((o)->op_sibling)
# define OpMORESIB_set(o, sib) ((o)->op_sibling = (sib))
# define OpLASTSIB_set(o, parent) ((o)->op_sibling = NULL)

# define PmOpCopySibling(to, from) ((to)->op_sibling=(from)->op_sibling)
#else
# if defined PERL_OP_PARENT
#  define PmOpCopySibling(to, from) ((to)->op_moresib=(from)->op_moresib, (to)->op_sibparent=(from)->op_sibparent)
# else
#  define PmOpCopySibling(to, from) ((to)->op_moresib=(from)->op_moresib, (to)->op_sibling=(from)->op_sibling)
# endif
#endif

#if PerlVersion >= 5200
# define PmEmptyArraySlot Nullsv
#else
# define PmEmptyArraySlot &PL_sv_undef
#endif

// these values have to be checked in toke.c for each new perl release
#if PerlVersion < 5250
# define LEX_KNOWNEXT 0
#endif
#define LEX_NORMAL 10

// check whether this private flag is not used in OP_METHOD_NAMED for each new perl release
#define MethodIsCalledOnLeftSideOfArrayAssignment 1

START_EXTERN_C

CV* pm_perl_get_cur_cv(pTHX);
SV **pm_perl_get_cx_curpad(pTHX_ PERL_CONTEXT* cx, PERL_CONTEXT* cx_bottom);

// public export from Poly
OP* pm_perl_select_method_helper_op(pTHX);
MAGIC* pm_perl_array_flags_magic(pTHX_ SV*);
SV* pm_perl_name_of_ret_var(pTHX);
int pm_perl_canned_dup(pTHX_ MAGIC* mg, CLONE_PARAMS* param);

// public export from RefHash
HE* pm_perl_refhash_fetch_ent(pTHX_ HV* hv, SV* keysv, I32 lval);

// public export from namespaces
SV* pm_perl_namespace_try_lookup(pTHX_ HV* stash, SV* name, I32 type);
HV* pm_perl_namespace_lookup_class(pTHX_ HV* stash, const char* class_name, STRLEN class_namelen, int lex_lookup_ix);
CV* pm_perl_namespace_lookup_sub(pTHX_ HV* stash, const char* name, STRLEN namelen, CV* lex_context_cv);
typedef void (*namespace_plugin_fun_ptr)(pTHX_ SV*);
void pm_perl_namespace_register_plugin(pTHX_ namespace_plugin_fun_ptr enabler, namespace_plugin_fun_ptr disabler, SV *data);

// public export from Scope
void pm_perl_localize_scalar(pTHX_ SV* var);
void pm_perl_localize_array(pTHX_ SV* av, SV* ar_ref);
 
// public export from CPlusPlus
OP* pm_perl_cpp_helem(pTHX_ HV* hv, const MAGIC* mg);
OP* pm_perl_cpp_hslice(pTHX_ HV* hv, const MAGIC* mg);
OP* pm_perl_cpp_exists(pTHX_ HV* hv, const MAGIC* mg);
OP* pm_perl_cpp_delete_hslice(pTHX_ HV* hv, const MAGIC* mg);
OP* pm_perl_cpp_delete_helem(pTHX_ HV* hv, const MAGIC* mg);
OP* pm_perl_cpp_keycnt(pTHX_ HV* hv, const MAGIC* mg);
int pm_perl_cpp_hassign(pTHX_ HV* hv, MAGIC* mg, I32* firstRp, I32 lastR, int return_size);
int pm_perl_cpp_has_assoc_methods(const MAGIC* mg);

END_EXTERN_C

#ifndef __cplusplus
static inline
I32 min (I32 a, I32 b) { return a<b ? a : b; }

static inline
I32 max (I32 a, I32 b) { return a>b ? a : b; }

static inline
void write_protect_on(pTHX_ SV* x)
{
   if (x != &PL_sv_undef) SvREADONLY_on(x);
}

static inline
void write_protect_off(pTHX_ SV* x)
{
   if (x != &PL_sv_undef) SvREADONLY_off(x);
}

// for given OP_ENTERSUB, find the corresponding OP_METHOD_NAMED, or return NULL
static inline
OP* method_named_op(OP* o)
{
   return ((o->op_flags & OPf_KIDS) && (o=cLISTOPo->op_last) && o->op_type == OP_METHOD_NAMED) ? o : 0;
}
#endif

static inline
MAGIC* pm_perl_get_cpp_magic(SV* sv)
{
   MAGIC* mg;
   for (mg=SvMAGIC(sv); mg && mg->mg_virtual->svt_dup != &pm_perl_canned_dup; mg=mg->mg_moremagic) ;
   return mg;
}

#endif // POLYMAKE_PERL_EXT_H

// Local Variables:
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
