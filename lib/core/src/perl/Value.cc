/* Copyright (c) 1997-2018
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
   sv = newSV_type(SVt_NULL);
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

SVHolder::SVHolder(SV* sv_arg, std::true_type)
{
   dTHX;
   sv = newSVsv(sv_arg);
}

void SVHolder::set_copy(SV* sv_arg)
{
   dTHX;
   sv_setsv(sv, sv_arg);
}

SV* Scalar::undef()
{
   dTHX;
   return &PL_sv_undef;
}

SV* Scalar::const_string(const char* s, size_t l)
{
   dTHX;
   SV* sv = newSV_type(SVt_PV);
   SvFLAGS(sv) |= SVf_READONLY | SVf_POK | SVp_POK;
   SvPV_set(sv, const_cast<char*>(s));
   SvCUR_set(sv, l);
   return sv;
}

SV* Scalar::const_string_with_int(const char* s, size_t l, int i)
{
   dTHX;
   SV* sv = newSV_type(SVt_PVIV);
   SvFLAGS(sv) |= SVf_READONLY | SVf_POK | SVp_POK | SVf_IOK | SVp_IOK;
   SvPV_set(sv, const_cast<char*>(s));
   SvCUR_set(sv, l);
   SvIV_set(sv, i);
   return sv;
}

SV* Scalar::const_int(int i)
{
   dTHX;
   SV* sv=newSViv(i);
   SvREADONLY_on(sv);
   return sv;
}

int Scalar::convert_to_int(SV* sv)
{
   MAGIC* mg=SvMAGIC(SvRV(sv));
   const glue::scalar_vtbl* t = reinterpret_cast<const glue::scalar_vtbl*>(mg->mg_virtual);
   return (t->to_int)(mg->mg_ptr);
}

double Scalar::convert_to_float(SV* sv)
{
   MAGIC* mg=SvMAGIC(SvRV(sv));
   const glue::scalar_vtbl* t = reinterpret_cast<const glue::scalar_vtbl*>(mg->mg_virtual);
   return (t->to_float)(mg->mg_ptr);
}

SV* ArrayHolder::init_me(int size)
{
   dTHX;
   AV* av = newAV();
   if (size > 0) av_extend(av, size - 1);
   return newRV_noinc((SV*)av);
}

void ArrayHolder::upgrade(int size)
{
   dTHX;
   if (!SvROK(sv)) {
      AV* av = newAV();
      if (size > 0) av_extend(av, size - 1);
      (void)SvUPGRADE(sv, SVt_RV);
      SvRV(sv) = (SV*)av;
      SvROK_on(sv);
   }
}

void ArrayHolder::verify() const
{
   if (__builtin_expect(!(SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVAV), 0))
      throw std::runtime_error("input argument is not an array");
}

void ArrayHolder::set_contains_aliases()
{
   AvREAL_off((AV*)SvRV(sv));
}

int ArrayHolder::size() const
{
   dTHX;
   return AvFILL((AV*)SvRV(sv)) + 1;
}

SV* ArrayHolder::operator[] (int i) const
{
   dTHX;
   return *av_fetch((AV*)SvRV(sv), i, TRUE);
}

bool SVHolder::is_tuple() const
{
   return SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVAV && !SvOBJECT(SvRV(sv));
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

void ArrayHolder::unshift(SV* x)
{
   dTHX;
   AV* const ary=(AV*)SvRV(sv);
   av_unshift(ary, 1);
   av_store(ary, 0, x);
}

void ArrayHolder::push(SV* x)
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

bool HashHolder::exists(const AnyString& key) const
{
   dTHX;
   return hv_exists((HV*)SvRV(sv), key.ptr, key.len);
}

SV* HashHolder::fetch(const AnyString& key, bool create) const
{
   dTHX;
   SV** valp = hv_fetch((HV*)SvRV(sv), key.ptr, key.len, create);
   return valp ? *valp : &PL_sv_undef;
}

ListValueInputBase::ListValueInputBase(SV* sv)
   : dim_sv(nullptr)
   , i(0)
   , cols_(-1)
   , dim_(-1)
   , sparse_(false)
{
   dTHX;
   if (SvROK(sv)) {
      arr_or_hash = SvRV(sv);
      const bool is_magic = SvMAGICAL(arr_or_hash) != 0;
      if (SvTYPE(arr_or_hash) == SVt_PVAV) {
         AV* av = (AV*)arr_or_hash;
         size_ = (is_magic ? av_len(av) : AvFILLp(av)) + 1;
         // TODO: remove this when XML parser vanishes
         if (MAGIC* mg = glue::array_flags_magic(aTHX_ arr_or_hash)) {
            if (mg->mg_len >= 0 && mg->mg_obj && SvPOKp(mg->mg_obj)) {
               if (SvCUR(mg->mg_obj) == 3 && !strncmp(SvPVX(mg->mg_obj), "dim", 3)) {
                  dim_ = mg->mg_len;
                  sparse_ = true;
               } else if (SvCUR(mg->mg_obj) == 4 && !strncmp(SvPVX(mg->mg_obj), "cols", 4)) {
                  cols_ = mg->mg_len;
               }
            }
         } else if (!is_magic) {
            if (size_ > 0) {
               SV* last = AvARRAY(av)[size_ - 1];
               if (SvOBJECT(av)) {
                  if (SvSTASH(av) == glue::Serializer_Sparse_stash) {
                     sparse_ = true;
                     if (size_ % 2 == 1 && SvIOK(last)) {
                        dim_ = SvUVX(last);
                        --size_;
                     }
                  }
               } else if (SvROK(last)) {
                  last = SvRV(last);
                  if (SvTYPE(last) == SVt_PVHV && !SvOBJECT(last) && !SvMAGICAL(last)) {
                     HV* attrs = (HV*)last;
                     SV** colsp;
                     if (HvUSEDKEYS(attrs) == 1 && (colsp = hv_fetch(attrs, "cols", 4, FALSE))) {
                        cols_ = SvIV(*colsp);
                        --size_;
                     }
                  }
               }
            } else {
               cols_ = 0;  // in case, it was supposed to be an empty matrix
            }
         }
         return;
      }
      if (!is_magic && SvTYPE(arr_or_hash) == SVt_PVHV) {
         HV* hv = (HV*)arr_or_hash;
         sparse_ = true;
         UV dim_val;
         if ((dim_sv = hv_delete_ent(hv, glue::Serializer_Sparse_dim_key, 0, SvSHARED_HASH(glue::Serializer_Sparse_dim_key)))) {
            SvREFCNT_inc_simple_void_NN(dim_sv);  // overcome sv_2mortal
            if (SvIOK(dim_sv)) {
               dim_ = SvIVX(dim_sv);
            } else if (SvPOK(dim_sv) && SvCUR(dim_sv) > 0 && grok_number(SvPVX(dim_sv), SvCUR(dim_sv), &dim_val) == IS_NUMBER_IN_UV) {
               dim_ = dim_val;
            } else {
               throw std::runtime_error("wrong " + AnyString(SvPVX(glue::Serializer_Sparse_dim_key), SvCUR(glue::Serializer_Sparse_dim_key)) + " element in sparse input");
            }
         }
         size_ = HvUSEDKEYS(hv);
         hv_iterinit(hv);
         if (!hv_iternext(hv)) i = size_;
         return;
      }
   }
   throw std::runtime_error("invalid list input: must be an array or hash");
}

bool ListValueInputBase::is_ordered() const
{
   return SvTYPE(arr_or_hash) == SVt_PVAV;
}

SV* ListValueInputBase::get_first() const
{
   dTHX;
   if (is_ordered()) {
      if (!sparse_representation()) {
         return SvMAGICAL(arr_or_hash) ? *av_fetch((AV*)arr_or_hash, 0, FALSE) : AvARRAY(arr_or_hash)[0];
      } else if (size_ > 2) {
         return AvARRAY(arr_or_hash)[2];
      } else {
         return nullptr;
      }
   }
   HE* entry = HvEITER((HV*)arr_or_hash);
   return HeVAL(entry);
}

void ListValueInputBase::finish()
{
   if (SvTYPE(arr_or_hash) == SVt_PVHV && dim_sv) {
      dTHX;
      HV* hv = (HV*)arr_or_hash;
      hv_iterinit(hv);
      hv_store_ent(hv, glue::Serializer_Sparse_dim_key, dim_sv, SvSHARED_HASH(glue::Serializer_Sparse_dim_key));
      dim_sv = nullptr;
   }
}

SV* ListValueInputBase::get_next()
{
   dTHX;
   if (is_ordered()) {
      SV* sv;
      if (sparse_representation()) {
         sv = AvARRAY(arr_or_hash)[i + 1];
         i += 2;
      } else {
         sv = SvMAGICAL(arr_or_hash) ? *av_fetch((AV*)arr_or_hash, i++, FALSE) : AvARRAY(arr_or_hash)[i++];
      }
      return sv;
   }
   HE* entry = HvEITER((HV*)arr_or_hash);
   if (!hv_iternext((HV*)arr_or_hash)) i = size_;
   return HeVAL(entry);
}

int ListValueInputBase::get_index() const
{
   if (!sparse_representation())
      throw std::runtime_error("dense/sparse input mismatch");
   if (is_ordered()) {
      SV* ix_sv = AvARRAY(arr_or_hash)[i];
      if (SvIOK(ix_sv))
         return SvIVX(ix_sv);
      throw std::runtime_error("sparse input - invalid index");
   }
   dTHX;
   HE* entry = HvEITER((HV*)arr_or_hash);
   I32 klen = -1;
   const char* key = hv_iterkey(entry, &klen);
   UV index;
   if (klen <= 0 || grok_number(key, klen, &index) != IS_NUMBER_IN_UV)
      throw std::runtime_error("sparse input - invalid index");
   return index;
}

void ListValueInputBase::retrieve_key(std::string& dst) const
{
   dTHX;
   HE* entry = HvEITER((HV*)arr_or_hash);
   I32 klen = -1;
   const char* key = hv_iterkey(entry, &klen);
   dst.assign(key, klen);
}

Stack::Stack() {}

Stack::Stack(SV** start)
{
   dTHX;
   PL_stack_sp = start;
}

Stack::Stack(int reserve)
{
   dTHX;
   PmStartFuncall(reserve);
   PUTBACK;
}

void Stack::push(SV* x) const
{
   dTHX;
   dSP;
   PUSHs(x);
   PUTBACK;
}

void Stack::xpush(SV* x) const
{
   dTHX;
   dSP;
   XPUSHs(x);
   PUTBACK;
}

void Stack::extend(int n)
{
   dTHX;
   dSP;
   EXTEND(SP, n);
   PUTBACK;
}

void Stack::push(const AnyString& s) const
{
   dTHX;
   dSP;
   mPUSHp(s.ptr, s.len);
   PUTBACK;
}

void Stack::cancel()
{
   dTHX;
   PmCancelFuncall;
}

void ListReturn::upgrade(int size)
{
   dTHX;
   dSP;
   EXTEND(SP, size);
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

std::false_type* Value::retrieve(std::string& x) const
{
   dTHX;
   if (__builtin_expect(SvOK(sv), 1)) {
      if (__builtin_expect(SvROK(sv) && !SvAMAGIC(sv), 0))
         throw std::runtime_error("invalid value for an input string property");
      size_t l;
      const char* p=SvPV(sv,l);
      x.assign(p,l);
   } else {
      x.clear();
   }
   return nullptr;
}

std::false_type* Value::retrieve(AnyString& x) const
{
   dTHX;
   if (__builtin_expect(SvOK(sv), 1)) {
      if (__builtin_expect(SvROK(sv) && !SvAMAGIC(sv), 0))
         throw std::runtime_error("invalid value for an input string property");
      size_t l;
      const char* p=SvPV(sv, l);
      x=AnyString(p, l);
   } else {
      x=AnyString();
   }
   return nullptr;
}

std::false_type* Value::retrieve(char& x) const
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
   return nullptr;
}

bool Value::is_defined() const noexcept
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
         if (MAGIC* mg=glue::get_cpp_magic(obj)) {
            const glue::base_vtbl* t=(const glue::base_vtbl*)mg->mg_virtual;
            if ((t->flags & ClassFlags::kind_mask) == ClassFlags::is_scalar)
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
      MAGIC* mg;
      if (!SvOBJECT(sv) && (mg=SvMAGIC(sv)) && mg->mg_type == PERL_MAGIC_arylen)
         return number_is_int;
   }
   return not_a_number;
}

std::false_type* Value::retrieve(bool& x) const
{
   dTHX;
   if (SvPOK(sv) && SvCUR(sv)==5 && !strcmp(SvPVX(sv), "false")) {
      x=false;
   } else {
      x=SvTRUE(sv);
   }
   return nullptr;
}

std::false_type* Value::retrieve(double& x) const
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
   return nullptr;
}

std::false_type* Value::retrieve(Array& x) const
{
   dTHX;
   if (__builtin_expect(SvOK(sv), 1)) {
      if (SvROK(x.sv)) sv_unref_flags(x.sv, SV_IMMEDIATE_UNREF);
      sv_setsv(x.sv, sv);
      x.verify();
   } else if (options * ValueFlags::allow_undef) {
      x.clear();
   } else {
      throw undefined();
   }
   return nullptr;
}

bool Value::is_plain_text(bool expect_numeric_scalar) const
{
   if ((SvFLAGS(sv) & (SVs_GMG | SVs_RMG | SVf_ROK | SVf_POK | (expect_numeric_scalar ? SVf_IOK | SVf_NOK : 0))) == SVf_POK) {
      return true;
   } else if (SvROK(sv) && SvOBJECT(SvRV(sv))) {
      dTHX;
      SV* type;
      if (sv_derived_from(sv, "Polymake::Core::Object")) {
         PmStartFuncall(1);
         PUSHs(sv);
         PUTBACK;
         type=glue::call_method_scalar(aTHX_ "type");
      } else if (sv_derived_from(sv, "Polymake::Core::ObjectType")) {
         type=sv;
      } else {
         return false;
      }
      PmStartFuncall(1);
      PUSHs(type);
      PUTBACK;
      type=glue::call_method_scalar(aTHX_ "full_name");
      std::string type_name(SvPVX(type));
      SvREFCNT_dec(type);
      throw std::runtime_error("tried to read a full " + type_name + " object as an input property");
   } else {
      return false;
   }
}

Value::canned_data_t Value::get_canned_data(SV* sv_arg) noexcept
{
   if (SvROK(sv_arg)) {
      MAGIC* mg;
      SV* obj=SvRV(sv_arg);
      if (SvOBJECT(obj) && (mg=glue::get_cpp_magic(obj)))
         return { reinterpret_cast<glue::base_vtbl*>(mg->mg_virtual)->type,
                  mg->mg_ptr,
                  (mg->mg_flags & uint8_t(ValueFlags::read_only)) != 0 };
   }
   return { nullptr, nullptr, false };
}

int Value::get_canned_dim(bool tell_size_if_dense) const
{
   if (SvROK(sv)) {
      MAGIC* mg;
      SV* obj=SvRV(sv);
      if (SvOBJECT(obj) && (mg=glue::get_cpp_magic(obj))) {
         const glue::container_vtbl* t=(const glue::container_vtbl*)mg->mg_virtual;
         if (((t->flags & ClassFlags::kind_mask) == ClassFlags::is_container) && t->own_dimension==1) {
            if (tell_size_if_dense || t->flags * ClassFlags::is_sparse_container)
               return (t->size)(mg->mg_ptr);
         }
      }
   }
   return -1;
}

Value::NoAnchors Value::put_val(long x, int)
{
   dTHX;
   sv_setiv(sv, x);
   return NoAnchors();
}

Value::NoAnchors Value::put_val(unsigned long x, int)
{
   dTHX;
   sv_setuv(sv, x);
   return NoAnchors();
}

Value::NoAnchors Value::put_val(bool x, int)
{
   dTHX;
   sv_setsv(sv, x ? &PL_sv_yes : &PL_sv_no);
   return NoAnchors();
}

Value::NoAnchors Value::put_val(double x, int)
{
   dTHX;
   sv_setnv(sv, x);
   return NoAnchors();
}

Value::NoAnchors Value::put_val(const undefined&, int)
{
   dTHX;
   sv_setsv(sv, &PL_sv_undef);
   return NoAnchors();
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

namespace {

inline
Value::Anchor* finalize_primitive_ref(pTHX_ const Value& v, const char* xptr, SV* descr, int n_anchors, bool take_ref)
{
   if (take_ref) {
      MAGIC* mg=glue::upgrade_to_builtin_magic_sv(aTHX_ v.get(), descr, n_anchors);
      mg->mg_ptr=(char*)xptr;
      mg->mg_flags |= uint8_t(v.get_flags() & ValueFlags::read_only);
      return n_anchors ? glue::MagicAnchors::first(mg) : nullptr;
   } else {
      MAGIC* mg=glue::upgrade_to_builtin_magic_sv(aTHX_ v.get(), descr, 0);
      mg->mg_flags |= uint8_t(ValueFlags::read_only);
      return nullptr;
   }
}

}

void Value::Anchor::store(SV* sv) noexcept
{
   if (SvROK(sv)) sv=SvRV(sv);
   stored=SvREFCNT_inc_simple_NN(sv);
}

std::pair<void*, Value::Anchor*> Value::allocate_canned(SV* descr, int n_anchors) const
{
   dTHX;
   MAGIC* mg=glue::allocate_canned_magic(aTHX_ sv, descr, options | ValueFlags::alloc_magic, n_anchors);
   mg->mg_flags |= MGf_GSKIP;    // if the following constructor call dies with an exception, destroy_canned won't try to delete the non-existent object
   return { mg->mg_ptr, n_anchors ? glue::MagicAnchors::first(mg) : nullptr };
}

void Value::mark_canned_as_initialized()
{
   MAGIC* mg=SvMAGIC(SvRV(sv));
   mg->mg_flags &= ~MGf_GSKIP;    // confirm that the canned object is fully initialized
}

SV* Value::get_constructed_canned()
{
   mark_canned_as_initialized();
   return get_temp();
}

Value::Anchor* Value::store_canned_ref_impl(void* val, SV* descr, ValueFlags flags, int n_anchors) const
{
   dTHX;
   MAGIC* mg=glue::allocate_canned_magic(aTHX_ sv, descr, flags, n_anchors);
   mg->mg_ptr=(char*)val;
   return n_anchors ? glue::MagicAnchors::first(mg) : nullptr;
}

Value::Anchor* Value::store_primitive_ref(const bool& x, SV* descr, int n_anchors, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setiv(sv, x);
   return finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, n_anchors, take_ref);
}
Value::Anchor* Value::store_primitive_ref(const int& x, SV* descr, int n_anchors, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setiv(sv, x);
   return finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, n_anchors, take_ref);
}
Value::Anchor* Value::store_primitive_ref(const unsigned int& x, SV* descr, int n_anchors, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setuv(sv, x);
   return finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, n_anchors, take_ref);
}
Value::Anchor* Value::store_primitive_ref(const long& x, SV* descr, int n_anchors, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setiv(sv, x);
   return finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, n_anchors, take_ref);
}
Value::Anchor* Value::store_primitive_ref(const unsigned long& x, SV* descr, int n_anchors, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setuv(sv, x);
   return finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, n_anchors, take_ref);
}
Value::Anchor* Value::store_primitive_ref(const double& x, SV* descr, int n_anchors, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setnv(sv, x);
   return finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, n_anchors, take_ref);
}
Value::Anchor* Value::store_primitive_ref(const std::string& x, SV* descr, int n_anchors, bool take_ref)
{
   dTHX;
   sv_upgrade(sv, SVt_PVLV);
   sv_setpvn(sv, x.c_str(), x.size());
   return finalize_primitive_ref(aTHX_ *this, reinterpret_cast<const char*>(&x), descr, n_anchors, take_ref);
}

istream::istream(SV* sv)
   : BufferHolder<istreambuf>(sv), std::istream(&my_buf)
{
   exceptions(failbit | badbit);
   if (SvCUR(sv)==0)
      setstate(eofbit);
}

std::runtime_error istream::parse_error() const
{
   return std::runtime_error(std::to_string(CharBuffer::get_ptr(rdbuf()) - CharBuffer::get_buf_start(rdbuf())) + '\t');
}

istreambuf::istreambuf(SV* sv)
{
   dTHX;
   if (__builtin_expect(SvROK(sv) && !SvAMAGIC(sv), 0))
      throw std::runtime_error("invalid value for an input property");
   size_t l;
   char* p=SvPV(sv,l);
   setg(p,p,p+l);
}

ostreambuf::ostreambuf(SV* sv)
   : val(sv)
{
   dTHX;
   sv_setpvn(sv, "", 0);
   char* p=SvGROW(sv,24);
   setp(p, p+23);
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
   char* p=SvGROW(val, l+513);
   setp(p, p+l+512);
   pbump(l);
   if (!traits_type::eq(c, traits_type::eof())) {
      *pptr()=c;
      pbump(1);
   }
   return traits_type::not_eof(c);
}

undefined::undefined()
   : std::runtime_error("unexpected undefined value of an input property") {}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
