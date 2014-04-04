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

#include "polymake/perl/glue.h"
#include <sstream>
#include <cstdlib>

namespace pm { namespace perl {

SVHolder::SVHolder()
{
   dTHX;
   sv=newSV(0);
}

void SVHolder::forget()
{
   dTHX;
   SvREFCNT_dec(sv);
}

SV* SVHolder::get_temp()
{
   dTHX;
   return sv_2mortal(sv);
}

SVHolder::SVHolder(SV *sv_arg, True)
{
   dTHX;
   sv=newSVsv(sv_arg);
}

void SVHolder::set_copy(SV *sv_arg)
{
   dTHX;
   sv_setsv(sv, sv_arg);
}

SV* Scalar::undef()
{
   dTHX;
   return &PL_sv_undef;
}

SV* Scalar::const_string(const char *s, size_t l)
{
   dTHX;
   SV *sv=newSV(0);
   sv_upgrade(sv, SVt_PV);
   SvFLAGS(sv) |= SVf_READONLY | SVf_POK | SVp_POK;
   SvPV_set(sv,(char*)s);
   SvCUR_set(sv,l);
   return sv;
}

SV* Scalar::const_string_with_int(const char *s, size_t l, int i)
{
   dTHX;
   SV *sv=newSV(0);
   sv_upgrade(sv, SVt_PVIV);
   SvFLAGS(sv) |= SVf_READONLY | SVf_POK | SVp_POK | SVf_IOK | SVp_IOK;
   SvPV_set(sv,(char*)s);
   SvCUR_set(sv,l);
   SvIV_set(sv,i);
   return sv;
}

int Scalar::convert_to_int(SV *sv)
{
   MAGIC *mg=SvMAGIC(SvRV(sv));
   const glue::scalar_vtbl *t=(const glue::scalar_vtbl*)mg->mg_virtual;
   return (t->to_int)(mg->mg_ptr);
}

double Scalar::convert_to_float(SV *sv)
{
   MAGIC *mg=SvMAGIC(SvRV(sv));
   const glue::scalar_vtbl *t=(const glue::scalar_vtbl*)mg->mg_virtual;
   return (t->to_double)(mg->mg_ptr);
}

SV* ArrayHolder::init_me(int size)
{
   dTHX;
   AV *av=newAV();
   if (size) av_extend(av, size);
   return newRV_noinc((SV*)av);
}

void ArrayHolder::upgrade(int size)
{
   dTHX;
   AV *av=newAV();
   if (size) av_extend(av, size);
   (void)SvUPGRADE(sv, SVt_RV);
   SvRV(sv)=(SV*)av;
   SvROK_on(sv);
}

void ArrayHolder::verify() const
{
   if (__builtin_expect(!(SvROK(sv) && SvTYPE(SvRV(sv))==SVt_PVAV), 0))
      throw std::runtime_error("input argument is not an array");
}

void ArrayHolder::set_contains_aliases()
{
   AvREAL_off((AV*)SvRV(sv));
}

int ArrayHolder::size() const
{
   dTHX;
   return AvFILL((AV*)SvRV(sv))+1;
}

SV* ArrayHolder::operator[] (int i) const
{
   dTHX;
   return *av_fetch((AV*)SvRV(sv), i, TRUE);
}

int ArrayHolder::dim(bool& has_sparse_representation) const
{
   dTHX;
   MAGIC* mg=mg_find(SvRV(sv), PERL_MAGIC_ext);
   if (mg && mg->mg_virtual==&pm_perl_array_flags_vtbl && mg->mg_len>=0) {
      has_sparse_representation=true;
      return mg->mg_len;
   } else {
      has_sparse_representation=false;
      return -1;
   }
}

bool SVHolder::is_tuple() const
{
   dTHX;
   if (SvROK(sv)) {
      MAGIC* mg=mg_find(SvRV(sv), PERL_MAGIC_ext);
      return mg && mg->mg_virtual==&pm_perl_array_flags_vtbl && mg->mg_len<0;
   } else {
      return false;
   }
}

SV* ArrayHolder::shift()
{
   dTHX;
   return av_shift((AV*)SvRV(sv));
}

SV* ArrayHolder::pop()
{
   dTHX;
   return av_pop((AV*)SvRV(sv));
}

void ArrayHolder::unshift(SV *x)
{
   dTHX;
   AV* const ary=(AV*)SvRV(sv);
   av_unshift(ary, 1);
   av_store(ary, 0, x);
}

void ArrayHolder::push(SV *x)
{
   dTHX;
   av_push((AV*)SvRV(sv), x);
}

void ArrayHolder::resize(int size)
{
   dTHX;
   av_fill((AV*)SvRV(sv), size-1);
}

SV* HashHolder::init_me()
{
   dTHX;
   return newRV_noinc((SV*)newHV());
}

void HashHolder::verify()
{
   if (__builtin_expect(!(SvROK(sv) && SvTYPE(SvRV(sv))==SVt_PVHV), 0))
      throw std::runtime_error("input argument is not a hash");
}

bool HashHolder::exists(const char *key, size_t klen) const
{
   dTHX;
   return hv_exists((HV*)SvRV(sv), key, klen);
}

Value HashHolder::_access(const char* key, size_t klen, bool create) const
{
   dTHX;
   SV **valp=hv_fetch((HV*)SvRV(sv), key, klen, create);
   return Value(valp ? *valp : &PL_sv_undef, value_flags(value_not_trusted | value_allow_undef));
}

Stack::Stack()
{
   dTHX;
   pi=getTHX;
}

Stack::Stack(SV **start)
{
   dTHX;
   pi=getTHX;
   PL_stack_sp=start;
}

Stack::Stack(bool room_for_object, int reserve)
{
   dTHX;
   pi=getTHX;
   PmStartFuncall;
   EXTEND(SP,reserve);
   if (room_for_object) ++SP;
   PUTBACK;
}

void Stack::push(SV* x) const
{
   dTHXa(pi);
   dSP;
   XPUSHs(x);
   PUTBACK;
}

void Stack::cancel()
{
   dTHXa(pi);
   PmCancelFuncall;
}

void ListReturn::upgrade(int size)
{
   dTHX; dSP;
   EXTEND(SP, size);
}

const char* Value::frame_lower_bound()
{
   char c=0;
   const char *flo=&c;
   return flo;
}

long Value::int_value() const
{
   dTHX;
   return SvIV(sv);
}

long Value::enum_value() const
{
   dTHX;
   return SvIV(SvRV(sv));
}

double Value::float_value() const
{
   dTHX;
   return SvNV(sv);
}

False* Value::retrieve(std::string& x) const
{
   dTHX;
   if (__builtin_expect(SvOK(sv), 1)) {
      if (__builtin_expect(SvROK(sv) && !SvAMAGIC(sv), 0))
         throw std::runtime_error("invalid value for an input string property");
      size_t l;
      const char *p=SvPV(sv,l);
      x.assign(p,l);
   } else {
      x.clear();
   }
   return NULL;
}

False* Value::retrieve(const char* &x) const
{
   dTHX;
   if (__builtin_expect(SvOK(sv), 1)) {
      if (__builtin_expect(SvROK(sv) && !SvAMAGIC(sv), 0))
         throw std::runtime_error("invalid value for an input string property");
      x=SvPV_nolen(sv);
   } else {
      x=NULL;
   }
   return NULL;
}

False* Value::retrieve(char& x) const
{
   dTHX;
   if (SvPOK(sv)) {
      x=*SvPVX(sv);
   } else if (SvOK(sv)) {
      switch (classify_number()) {
      case number_is_int: {
         const int ival=SvIV(sv);
         if (ival>=0 && ival<=9)
            x=ival+'0';
         else
            throw std::runtime_error("invalid value for an input character property");
         break;
      }
      case number_is_float: {
         const double dval=SvNV(sv);
         if (dval>=0 && dval<=9)
            x=static_cast<int>(dval)+'0';
         else
            throw std::runtime_error("invalid value for an input character property");
         break;
      }
      default:
         if (SvROK(sv) && !SvAMAGIC(sv))
            throw std::runtime_error("invalid value for an input character property");
         x=*SvPV_nolen(sv);
      }
   } else {
      x=0;
   }
   return NULL;
}

bool Value::is_defined() const
{
   return (SvFLAGS(sv) & (SVf_IOK|SVf_NOK|SVf_POK|SVf_ROK|SVp_IOK|SVp_NOK|SVp_POK|SVs_GMG)) != 0;
}

bool Value::is_TRUE() const
{
   dTHX;
   return SvTRUE(sv);
}

Value::number_flags Value::classify_number() const
{
   dTHX;
   I32 flags=SvFLAGS(sv);
   if (flags & SVf_IOK)
      return number_is_int;
   if (flags & SVf_NOK)
      return number_is_float;
   if (flags & SVf_POK) {
      if (SvCUR(sv) > 0) {
         flags=looks_like_number(sv);
         if (flags & (IS_NUMBER_NOT_INT | IS_NUMBER_GREATER_THAN_UV_MAX))
            return number_is_float;
         if (flags & IS_NUMBER_IN_UV)
            return number_is_int;
      } else {
         return number_is_zero;
      }
   }
   if (flags & SVf_ROK) {
      SV* const obj=SvRV(sv);
      if (SvOBJECT(obj)) {
         if (MAGIC* mg=pm_perl_get_cpp_magic(obj)) {
            const glue::base_vtbl *t=(const glue::base_vtbl*)mg->mg_virtual;
            if ((t->flags & class_is_kind_mask) == class_is_scalar)
               return number_is_object;
         }
      }
      return not_a_number;
   }
   if (flags & SVp_IOK) {
      // a copy of an array length can look like this
      if (!SvOBJECT(sv) && !SvMAGIC(sv))
         return number_is_int;
   }
   if (flags & SVs_GMG) {
      // an array length magic variable
      MAGIC *mg;
      if (!SvOBJECT(sv) && (mg=SvMAGIC(sv)) && mg->mg_type == PERL_MAGIC_arylen)
         return number_is_int;
   }
   return not_a_number;
}

False* Value::retrieve(bool& x) const
{
   dTHX;
   if (SvPOK(sv) && SvCUR(sv)==5 && !strcmp(SvPVX(sv), "false")) {
      x=false;
   } else {
      x=SvTRUE(sv);
   }
   return NULL;
}

False* Value::retrieve(double& x) const
{
   dTHX;
   switch (classify_number()) {
   case number_is_int:
      x=double(SvIV(sv));
      break;
   case number_is_float:
      x=SvNV(sv);
      break;
   case number_is_object:
      x=Scalar::convert_to_float(sv);
      break;
   case number_is_zero:
      x=0;
      break;
   default:
      throw std::runtime_error("invalid value for an input floating-point property");
   }
   return NULL;
}

False* Value::retrieve(Array& x) const
{
   dTHX;
   if (__builtin_expect(SvOK(sv), 1)) {
      if (SvROK(x.sv)) sv_unref_flags(x.sv, SV_IMMEDIATE_UNREF);
      sv_setsv(x.sv, sv);
      x.verify();
   } else if (options & value_allow_undef) {
      x.clear();
   } else {
      throw undefined();
   }
   return NULL;
}

bool Value::is_plain_text(bool expect_numeric_scalar) const
{
   if ((SvFLAGS(sv) & (SVs_GMG | SVs_RMG | SVf_ROK | SVf_POK | (expect_numeric_scalar ? SVf_IOK | SVf_NOK : 0))) == SVf_POK) {
      return true;
   } else if (SvROK(sv) && SvOBJECT(SvRV(sv))) {
      dTHX;
      SV *type;
      if (sv_derived_from(sv, "Polymake::Core::Object")) {
         PmStartFuncall;
         XPUSHs(sv);
         PUTBACK;
         type=glue::call_method_scalar(aTHX_ "type");
      } else if (sv_derived_from(sv, "Polymake::Core::ObjectType")) {
         type=sv;
      } else {
         return false;
      }
      PmStartFuncall;
      XPUSHs(type);
      PUTBACK;
      type=glue::call_method_scalar(aTHX_ "full_name");
      std::string type_name(SvPVX(type));
      SvREFCNT_dec(type);
      throw std::runtime_error("tried to read a full " + type_name + " object as an input property");
   } else {
      return false;
   }
}

const std::type_info* Value::get_canned_typeinfo() const
{
   if (SvROK(sv)) {
      MAGIC *mg;
      SV *obj=SvRV(sv);
      if (SvOBJECT(obj) && (mg=pm_perl_get_cpp_magic(obj)))
         return ((glue::base_vtbl*)mg->mg_virtual)->type;
   }
   return NULL;
}

char* Value::get_canned_value(SV* sv_arg)
{
   return pm_perl_get_cpp_magic(SvRV(sv_arg))->mg_ptr;
}

int Value::get_canned_dim(bool tell_size_if_dense) const
{
   if (SvROK(sv)) {
      MAGIC *mg;
      SV *obj=SvRV(sv);
      if (SvOBJECT(obj) && (mg=pm_perl_get_cpp_magic(obj))) {
         const glue::container_vtbl *t=(const glue::container_vtbl*)mg->mg_virtual;
         if (((t->flags & class_is_kind_mask) == class_is_container) && t->own_dimension==1) {
            if (tell_size_if_dense || (t->flags & class_is_sparse_container))
               return (t->size)(mg->mg_ptr);
         }
      }
   }
   return -1;
}

Value::AnchorChain Value::put(long x, const char*, int)
{
   dTHX;
   sv_setiv(sv, x);
   return AnchorChain();
}

Value::AnchorChain Value::put(unsigned long x, const char*, int)
{
   dTHX;
   sv_setuv(sv, x);
   return AnchorChain();
}

Value::AnchorChain Value::put(bool x, const char*, int)
{
   dTHX;
   sv_setsv(sv, x ? &PL_sv_yes : &PL_sv_no);
   return AnchorChain();
}

Value::AnchorChain Value::put(double x, const char*, int)
{
   dTHX;
   sv_setnv(sv, x);
   return AnchorChain();
}

Value::AnchorChain Value::put(const undefined&, const char*, int)
{
   dTHX;
   sv_setsv(sv, &PL_sv_undef);
   return AnchorChain();
}

void Value::set_copy(const SVHolder& x)
{
   dTHX;
   sv_setsv(sv, x.get());
}

void Value::set_string_value(const char* x)
{
   dTHX;
   sv_setpv(sv, x);
}

void Value::set_string_value(const char* x, size_t l)
{
   dTHX;
   sv_setpvn(sv, x, l);
}

void Value::set_perl_type(SV *proto)
{
   dTHX;
   if (SvROK(sv) && proto)
      sv_bless(sv, gv_stashsv(PmArray(proto)[glue::PropertyType_pkg_index], TRUE));
}

namespace {

inline
MAGIC* allocate_magic(SV* sv, SV* descr, int flags)
{
   dTHX;
   const glue::base_vtbl *t=(const glue::base_vtbl*)SvPVX(PmArray(descr)[glue::TypeDescr_vtbl_index]);
   (t->sv_maker)(aTHX_ sv, descr, flags);
   return SvMAGIC(SvRV(sv));
}

inline
void finalize_primitive_ref(pTHX_ const Value& v, const char *xptr, SV *descr, bool take_ref)
{
   MAGIC *mg=glue::upgrade_to_builtin_magic_sv(aTHX_ v.get(), descr);
   if (take_ref) {
      mg->mg_ptr=(char*)xptr;
      mg->mg_flags |= v.get_flags() & value_read_only;
   } else {
      mg->mg_flags |= value_read_only;
   }
}

}

void* Value::allocate_canned(SV* descr) const
{
   return allocate_magic(sv, descr, options | value_alloc_magic)->mg_ptr;
}

void Value::store_canned_ref(SV* descr, void* val, value_flags flags) const
{
   allocate_magic(sv, descr, flags)->mg_ptr=(char*)val;
}

Value::AnchorChain& Value::AnchorChain::operator() (int n_anchors)
{
   if (cur) {
      SV* sv=reinterpret_cast<Value*>(cur)->get();
      if (SvROK(sv)) sv=SvRV(sv);
      MAGIC* mg=SvMAGIC(sv);
      mg->mg_private=n_anchors;
      if (n_anchors==1) {
         cur=&mg->mg_obj;
      } else {
         Newxz(cur, n_anchors, SV*);
         mg->mg_obj=(SV*)cur;
      }
   }
   return *this;
}

Value::AnchorChain& Value::AnchorChain::operator() (const Value& anchor)
{
   if (cur) {
      SV* sv=anchor.get();
      if (SvROK(sv)) sv=SvRV(sv);
      *cur++ = SvREFCNT_inc_simple_NN(sv);
   }
   return *this;
}

void Value::store_primitive_ref(const bool& x, SV *descr, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setiv(sv, x);
   finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, take_ref);
}
void Value::store_primitive_ref(const int& x, SV *descr, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setiv(sv, x);
   finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, take_ref);
}
void Value::store_primitive_ref(const unsigned int& x, SV *descr, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setuv(sv, x);
   finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, take_ref);
}
void Value::store_primitive_ref(const long& x, SV *descr, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setiv(sv, x);
   finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, take_ref);
}
void Value::store_primitive_ref(const unsigned long& x, SV *descr, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setuv(sv, x);
   finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, take_ref);
}
void Value::store_primitive_ref(const double& x, SV *descr, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setnv(sv, x);
   finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, take_ref);
}
void Value::store_primitive_ref(const std::string& x, SV *descr, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setpvn(sv, x.c_str(), x.size());
   finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, take_ref);
}

istream::istream(SV *sv)
   : BufferHolder<istreambuf>(sv), std::istream(&my_buf)
{
   exceptions(failbit | badbit);
   if (SvCUR(sv)==0)
      setstate(eofbit);
}

std::runtime_error istream::parse_error() const
{
   std::ostringstream err_pos;
   err_pos << CharBuffer::get_ptr(rdbuf()) - CharBuffer::get_buf_start(rdbuf()) << '\t';
   return std::runtime_error(err_pos.str());
}

istreambuf::istreambuf(SV *sv)
{
   dTHX;
   if (__builtin_expect(SvROK(sv) && !SvAMAGIC(sv), 0))
      throw std::runtime_error("invalid value for an input property");
   size_t l;
   char* p=SvPV(sv,l);
   setg(p,p,p+l);
}

ostreambuf::ostreambuf(SV *sv)
   : val(sv)
{
   dTHX;
   sv_setpvn(sv, "", 0);
   char *p=SvGROW(sv,24);
   setp(p,p+23);
}

ostreambuf::~ostreambuf()
{
   *pptr()='\0';
   SvCUR_set(val, pptr()-pbase());
}

ostreambuf::int_type ostreambuf::overflow(int_type c)
{
   dTHX;
   size_t l=pptr()-pbase();
   SvCUR_set(val,l);
   char *p=SvGROW(val, l+513);
   setp(p, p+l+512);
   pbump(l);
   if (!traits_type::eq(c, traits_type::eof())) {
      *pptr()=c;
      pbump(1);
   }
   return traits_type::not_eof(c);
}

namespace {
const std::string undefined_what("unexpected undefined value of an input property");
}

undefined::undefined() :
   std::runtime_error(undefined_what) {}

SV* complain_obsolete_wrapper(const char* file, int line, const char* expr)
{
   dTHX;
   sv_setpvf(ERRSV,
             "Obsolete automatically generated code in file \"%s\", line %d: %s\nPlease remove or edit manually.\n",
             file, line, expr);
   throw exception();
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
