/* Copyright (c) 1997-2023
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

#pragma once

#if POLYMAKE_DEBUG && defined(_FORTIFY_SOURCE)
// required for weird Manjaro system library
#undef _FORTIFY_SOURCE
#define _FORTIFY_SOURCE 0
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wconversion"
#if PerlVersion < 5220 && \
    ( !defined(__APPLE__) && !(__clang_major__ == 3 && __clang_minor__ == 4) || \
       defined(__APPLE__) && __clang_major__ != 5 )
#pragma clang diagnostic ignored "-Wpointer-bool-conversion"
#endif
#if PerlVersion < 5200
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
#endif
#if PerlVersion < 5180
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#if PerlVersion < 5220
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#endif

#if PerlVersion >= 5380
# define PERL_USE_VOLATILE_API
#endif
#include <EXTERN.h>
#include <perl.h>
START_EXTERN_C
#include <pp_proto.h>
END_EXTERN_C

#ifdef PERL_IMPLICIT_CONTEXT
// pass the interpreter via my_perl variable as far as possible
// don't use TLS, polymake never lets perl clone itself
#  define PERL_NO_GET_CONTEXT
#  define getTHX aTHX
#  ifdef dTHX
#    undef dTHX
#  endif
#  define dTHX dTHXa(PERL_GET_INTERP)
#else
#  ifdef dTHX
#    undef dTHX
#    define dTHX
#  endif
#  ifdef dTHXa
#    undef dTHXa
#    define dTHXa(a)
#  endif
#  define getTHX nullptr
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

// PerlVersion < 5180
#ifndef PadlistARRAY
# define PadlistARRAY(x) ((AV**)AvARRAY(x))
# define PadARRAY(x)     ((SV**)AvARRAY(x))
# define PadMAX(x)       AvFILLp(x)
# define PadlistMAX(x)   AvFILLp(x)
# define PadlistNAMESARRAY(x) AvARRAY(PadlistARRAY(x)[0])
# define PADNAME              SV
# define PadnameLEN(x)        SvCUR(x)
# define PadnamePV(x)         SvPVX(x)
#endif
#ifndef ReANY
# define ReANY(x) ((struct regexp *)SvANY(x))
#endif

// PerlVersion < 5220 does not provide macros for manipulating op siblings
#ifndef OpHAS_SIBLING
# define OpHAS_SIBLING(o) ((o)->op_sibling != NULL)
# define OpSIBLING(o) ((o)->op_sibling)
# define OpMORESIB_set(o, sib) ((o)->op_sibling = (sib))
# define OpLASTSIB_set(o, parent) ((o)->op_sibling = NULL)
# define OpMAYBESIB_set(o, sib, parent) OpMORESIB_set(o, sib)

# define PmOpCopySibling(to, from) ((to)->op_sibling=(from)->op_sibling)
#else
# if defined PERL_OP_PARENT
#  define PmOpCopySibling(to, from) ((to)->op_moresib=(from)->op_moresib, (to)->op_sibparent=(from)->op_sibparent)
# else
#  define PmOpCopySibling(to, from) ((to)->op_moresib=(from)->op_moresib, (to)->op_sibling=(from)->op_sibling)
# endif
#endif

#if PerlVersion >= 5200
# define PmEmptyArraySlot nullptr
#else
# define PmEmptyArraySlot &PL_sv_undef
#endif

// check whether this private flag is not used in OP_METHOD_NAMED for each new perl release
#define MethodIsCalledOnLeftSideOfArrayAssignment 1

namespace pm { namespace perl { namespace glue {

#if PerlVersion >= 5380
I32 Perl_keyword (pTHX_ const char *name, I32 len, bool all_keywords);
#else
using ::Perl_keyword;
#endif

// public export from Ext
extern bool skip_debug_cx;

inline bool skip_debug_sub(pTHX_ CV* cv)
{
   return skip_debug_cx && CvSTASH(cv)==PL_debstash;
}

inline bool skip_debug_frame(pTHX_ PERL_CONTEXT* cx)
{
   return skip_debug_cx && CopSTASH_eq(cx->blk_oldcop, PL_debstash);
}

bool is_boolean_value(pTHX_ SV* sv);
SV* get_boolean_string(SV* sv);

CV* get_cur_cv(pTHX);
SV** get_cx_curpad(pTHX_ PERL_CONTEXT* cx, PERL_CONTEXT* cx_bottom);

// public export from Poly
OP* select_method_helper_op(pTHX);
SV* name_of_ret_var(pTHX);
int canned_dup(pTHX_ MAGIC* mg, CLONE_PARAMS* param);

// result of is_lvalue
enum is_lvalue_kind { no_lvalue, magic_lvalue, pure_lvalue };

// public export from RefHash
HE* refhash_fetch_ent(pTHX_ HV* hv, SV* keysv, I32 lval);
extern Perl_ppaddr_t def_pp_CONST, def_pp_ENTERSUB;

// public export from namespaces
SV* namespace_try_lookup(pTHX_ HV* stash, SV* name, I32 type);
HV* namespace_lookup_class(pTHX_ HV* stash, const char* class_name, STRLEN class_namelen, int lex_lookup_ix, bool override_negative_cache=false);
HV* namespace_lookup_class_autoload(pTHX_ HV* stash, const char* class_name, STRLEN class_namelen, int lex_lookup_ix);
CV* namespace_lookup_sub(pTHX_ HV* stash, const char* name, STRLEN namelen, CV* lex_context_cv);
SV* namespace_create_explicit_typelist(pTHX_ int size);
using namespace_plugin_fun_ptr=void (*)(pTHX_ SV*);
void namespace_register_plugin(pTHX_ namespace_plugin_fun_ptr enabler, namespace_plugin_fun_ptr disabler, SV* data);

// public export from CPlusPlus
OP* cpp_helem(pTHX_ HV* hv, const MAGIC* mg);
OP* cpp_hslice(pTHX_ HV* hv, const MAGIC* mg);
OP* cpp_exists(pTHX_ HV* hv, const MAGIC* mg);
OP* cpp_delete_hslice(pTHX_ HV* hv, const MAGIC* mg);
OP* cpp_delete_helem(pTHX_ HV* hv, const MAGIC* mg);
OP* cpp_keycnt(pTHX_ HV* hv, const MAGIC* mg);
SSize_t cpp_hassign(pTHX_ HV* hv, MAGIC* mg, I32* firstRp, I32 lastR, bool return_size);
bool cpp_has_assoc_methods(const MAGIC* mg);

// public export from Struct
SV* retrieve_pkg(pTHX_ SV* obj);
HV* retrieve_pkg_stash(pTHX_ SV* obj);

// public export from interrupts
void set_interrupt_signal(pTHX_ int signum, bool must_reset_state = true);
void reset_interrupt_signal();

// public export from UserSettings

int monitored_dup(pTHX_ MAGIC* mg, CLONE_PARAMS* param);

// CvROOT and CvXSUB are in the same union
inline bool is_well_defined_sub(CV* x)
{
   return CvROOT(x) != nullptr;
}

#if PerlVersion >= 5380
# define op_scalar_context(o) Perl_op_contextualize(aTHX_ o, G_SCALAR)
# define op_list_context(o) Perl_op_contextualize(aTHX_ o, G_LIST)
#else
# define op_scalar_context(o) Perl_scalar(aTHX_ o)
# define op_list_context(o) Perl_list(aTHX_ o)
#endif

inline
void write_protect_on(pTHX_ SV* x)
{
   if (x != &PL_sv_undef) SvREADONLY_on(x);
}

inline
void write_protect_off(pTHX_ SV* x)
{
   if (x != &PL_sv_undef) SvREADONLY_off(x);
}

// for given OP_ENTERSUB, find the corresponding OP_METHOD_NAMED, or return NULL
inline
OP* method_named_op(OP* o)
{
   if (o->op_flags & OPf_KIDS) {
      OP* k = cUNOPo->op_first;
      if (k->op_type == OP_NULL && k->op_targ == OP_LIST) o = k;
      o = cLISTOPo->op_last;
      if (o->op_type == OP_METHOD_NAMED) return o;
   }
   return nullptr;
}

template <typename Fptr>
MAGIC* get_magic_by_dup_marker(SV* sv, Fptr dup)
{
   MAGIC* mg;
   for (mg = SvMAGIC(sv); mg && (!mg->mg_virtual || mg->mg_virtual->svt_dup != dup); mg=mg->mg_moremagic) ;
   return mg;
}

inline
MAGIC* get_cpp_magic(SV* sv)
{
   return get_magic_by_dup_marker(sv, &canned_dup);
}

inline
MAGIC* get_monitored_magic(SV* sv)
{
   return get_magic_by_dup_marker(sv, &monitored_dup);
}

inline
HV* get_cached_stash(pTHX_ SV* pkg)
{
   if (!pkg) return nullptr;
   if (!(SvFLAGS(pkg) & SVf_IVisUV)) {
      HV* stash=gv_stashsv(pkg, GV_NOADD_NOINIT);
#if PerlVersion < 5180
      // perl 5.16 destroys these flags in SvUPGRADE
      // which has catastrophic consequences for use of such constants in reentrant subs
      // (they are not propagated into PAD clones)
      const auto save_flags = SvFLAGS(pkg) & (SVf_FAKE | SVf_READONLY);
      SvFLAGS(pkg) &= ~(SVf_FAKE | SVf_READONLY);
#endif
      SvUPGRADE(pkg, SVt_PVIV);
      SvUV_set(pkg, reinterpret_cast<UV>(stash));
#if PerlVersion < 5180
      SvFLAGS(pkg) |= SVf_IVisUV | save_flags;
#else
      SvFLAGS(pkg) |= SVf_IVisUV;
#endif
   }
   return reinterpret_cast<HV*>(SvUVX(pkg));
}

// for sprintf and similar using a "%.*s" format
#define PmPrintGvNAME(gv) (int)GvNAMELEN(gv), GvNAME(gv)
#define PmPrintHvNAME(hv) (int)HvNAMELEN(hv), HvNAME(hv)

inline
SV* get_named_constant_sv(pTHX_ HV* stash, const AnyString& name)
{
   SV** gvp = hv_fetch(stash, name.ptr, I32(name.len), false);
   CV* cv;
   if (!gvp || (cv = GvCV(*gvp), !cv || !CvISXSUB(cv)))
      Perl_croak(aTHX_ "unknown constant %.*s::%.*s", PmPrintHvNAME(stash), (int)name.len, name.ptr);
   return (SV*)XSANY.any_ptr;
}

inline
int get_named_constant(pTHX_ HV* stash, const AnyString& name)
{
   return int(SvIV(get_named_constant_sv(aTHX_ stash, name)));
}

inline
HV* get_named_stash(pTHX_ const AnyString& name, int flags = 0)
{
   HV* stash = gv_stashpvn(name.ptr, I32(name.len), flags);
   if (!stash)
      Perl_croak(aTHX_ "unknown package %.*s", (int)name.len, name.ptr);
   return stash;
}

inline
GV* get_named_variable(pTHX_ const AnyString& name, svtype type, int flags = 0)
{
   GV* gv = gv_fetchpvn_flags(name.ptr, I32(name.len), flags, type);
   if (!gv)
      Perl_croak(aTHX_ "unknown variable %.*s", (int)name.len, name.ptr);
   return gv;
}

// a sort of light-weight unique_ptr for OPs under construction
template <typename OpType>
class op_keeper {
public:
   op_keeper(pTHX_ OpType* o)
      : op(o) {}

   op_keeper& operator= (OpType* o)
   {
      op = o;
      return *this;
   }

   operator OpType* () const { return op; }
   OpType* operator-> () const { return op; }
   explicit operator bool () const { return op != nullptr; }

   OpType* release()
   {
      OpType* const o = op;
      op = nullptr;
      return o;
   }

   ~op_keeper()
   {
      dTHX;
      if (op) op_free(op);
   }

private:
   op_keeper(op_keeper&&) = delete;
   void operator=(op_keeper&&) = delete;
   op_keeper(const op_keeper&) = delete;
   void operator=(const op_keeper&) = delete;

   OpType* op;
};

#if defined(NDEBUG) || PerlVersion >= 5220
# define PmNewCustomOP(type, ...) new##type(OP_CUSTOM, __VA_ARGS__)
#else
// old perls have a too restrictive assertion denying creation of custom operations of types other than BASEOP.
inline OP* set_op_type(OP* o, uint32_t type)
{
   o->op_type=type;
   return o;
}
# define PmNewCustomOP(type, ...) set_op_type(new##type((OA_##type==OA_SVOP ? OP_CONST : OP_NULL), __VA_ARGS__), OP_CUSTOM)
#endif

#define report_parse_error(...) \
   Perl_qerror(aTHX_ Perl_mess(aTHX_ __VA_ARGS__))

int parse_enhanced_local(pTHX_ OP** op_ptr);
OP* parse_expression_in_parens(pTHX);
int parse_interrupts_op(pTHX_ const bool localize, OP** op_ptr);
int parse_set_custom(pTHX_ OP** op_ptr);
int parse_reset_custom(pTHX_ OP** op_ptr);
}
namespace ops {

OP* is_boolean(pTHX);
OP* is_string(pTHX);
OP* is_integer(pTHX);
OP* is_float(pTHX);
OP* is_numeric(pTHX);
OP* is_object(pTHX);
OP* is_code(pTHX);
OP* is_constant_sub(pTHX);
OP* is_array(pTHX);
OP* is_hash(pTHX);
OP* is_scalar_ref(pTHX);
OP* is_like_array(pTHX);
OP* is_like_hash(pTHX);
OP* is_defined_and_false(pTHX);
OP* local_ref(pTHX);
void localize_scalar(pTHX_ SV* var);
void localize_scalar(pTHX_ SV* var, SV* value);
OP* make_weak(pTHX);
void init_globals(pTHX);

} } }


// Local Variables:
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
