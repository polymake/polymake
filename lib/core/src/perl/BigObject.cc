/* Copyright (c) 1997-2020
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

#include "polymake/perl/glue.h"
#include "polymake/perl/calls.h"
#include "polymake/perl/BigObject.h"
#include "polymake/Array.h"

namespace pm { namespace perl {

namespace {

glue::cached_cv give_cv{ "Polymake::Core::BigObject::give" },
             give_nm_cv{ "Polymake::Core::BigObject::give_with_name" },
                take_cv{ "Polymake::Core::BigObject::take" },
              lookup_cv{ "Polymake::Core::BigObject::lookup_pv" },
                 add_cv{ "Polymake::Core::BigObject::add" },
              remove_cv{ "Polymake::Core::BigObject::remove" },
              attach_cv{ "Polymake::Core::BigObject::attach" },
  remove_attachment_cv={ "Polymake::Core::BigObject::remove_attachment" },
            set_name_cv{ "Polymake::Core::BigObject::set_name" },
         set_changed_cv{ "Polymake::Core::BigObject::set_changed" },
          object_isa_cv{ "Polymake::Core::BigObject::isa" },
     object_type_isa_cv{ "Polymake::Core::BigObjectType::isa" },
              commit_cv{ "Polymake::Core::BigObject::commit" },
           new_named_cv{ "Polymake::Core::BigObject::new_named" },
          new_filled_cv{ "Polymake::Core::BigObject::new_filled" },
            new_copy_cv{ "Polymake::Core::BigObject::new_copy" },
 construct_with_size_cv{ "Polymake::Core::BigObjectArray::construct_with_size" },
                copy_cv{ "Polymake::Core::BigObject::copy" },
                cast_cv{ "Polymake::Core::BigObject::cast_me" },
                load_cv{ "Polymake::User::load" },
                save_cv{ "Polymake::User::save" };

std::pair<SV*, SV*> get_Array_pkg_and_typeof_impl(pTHX)
{
   HV* builtin_pkgs=(HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_builtins_index]);
   SV** svp=hv_fetch(builtin_pkgs, "array", 5, FALSE);
   if (!svp) throw std::runtime_error("Array PropertyType not declared in the rules");
   HV* stash=gv_stashsv(*svp, FALSE);
   if (!stash) throw std::runtime_error("Array generic package not found");
   SV** typeof_gvp=hv_fetch(stash, "typeof", 6, FALSE);
   if (!typeof_gvp) throw std::runtime_error("Array typeof method not found");
   return { *svp, *typeof_gvp };
}

SV* get_Array_type(pTHX_ SV* el_type)
{
   static std::pair<SV*, SV*> pkg_and_typeof=get_Array_pkg_and_typeof_impl(aTHX);
   PmStartFuncall(2);
   PUSHs(pkg_and_typeof.first);
   PUSHs(el_type);
   PUTBACK;
   return perl::glue::call_func_scalar(aTHX_ pkg_and_typeof.second, true);
}

void set_Array_type(SV* ar_ref, SV* el_type)
{
   dTHX;
   SV* array_type=get_Array_type(aTHX_ el_type);
   if (!array_type)
      throw std::runtime_error("can't construct the full type for a big object array");
   sv_bless(ar_ref, gv_stashsv(PmArray(array_type)[glue::PropertyType_pkg_index], TRUE));
}

void copy_ref(SV* &dst, SV* const src)
{
   dTHX;
   if (dst) {
      if (src) {
         if (SvROK(dst)) {
            if (SvRV(src)==SvRV(dst)) return;
            sv_unref_flags(dst, SV_IMMEDIATE_UNREF);
         }
         sv_setsv(dst, src);
      } else {
         SvREFCNT_dec(dst);
         dst=nullptr;
      }
   } else if (src) {
      dst=newSVsv(src);
   }
}

SV* init_copy_ref(SV* src)
{
   if (src) {
      dTHX;
      return newSVsv(src);
   }
   return src;
}

bool has_init_transaction(SV* obj_ref)
{
   SV* trans_sv=PmArray(obj_ref)[glue::Object_transaction_index];
   return SvROK(trans_sv) && SvSTASH(SvRV(trans_sv))==glue::Object_InitTransaction_stash;
}

void check_ref(SV* obj_ref)
{
   if (__builtin_expect(obj_ref != nullptr, 1))
      assert(SvROK(obj_ref));
   else
      throw std::runtime_error("invalid object");
}

}

BigObjectType::BigObjectType(const BigObjectType& x)
   : obj_ref(init_copy_ref(x.obj_ref)) {}

BigObjectType& BigObjectType::operator= (const BigObjectType& x)
{
   copy_ref(obj_ref, x.obj_ref);
   return *this;
}

BigObjectType::~BigObjectType()
{
   dTHX;
   SvREFCNT_dec(obj_ref);
}

BigObject::BigObject(const BigObject& x)
   : obj_ref(init_copy_ref(x.obj_ref))
{}

BigObject& BigObject::operator= (const BigObject& x)
{
   copy_ref(obj_ref, x.obj_ref);
   return *this;
}

BigObject::~BigObject()
{
   dTHX;
   SvREFCNT_dec(obj_ref);
}

std::string BigObject::name() const
{
   check_ref(obj_ref);
   dTHX;
   size_t l=0;
   const char* namep=SvPV(PmArray(obj_ref)[glue::Object_name_index], l);
   return std::string(namep, l);
}

std::string BigObject::description() const
{
   check_ref(obj_ref);
   dTHX;
   size_t l=0;
   const char* descrp=SvPV(PmArray(obj_ref)[glue::Object_description_index], l);
   return std::string(descrp,l);
}

void BigObject::set_name(const std::string& name)
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.c_str(), name.size());
   PUTBACK;
   glue::call_func_void(aTHX_ set_name_cv);
}

void BigObject::set_description(const std::string& value, bool append)
{
   check_ref(obj_ref);
   dTHX;
   SV* const sv=PmArray(obj_ref)[glue::Object_description_index];
   if (append) {
      sv_catpvn(sv, value.c_str(), value.size());
   } else {
      sv_setpvn(sv, value.c_str(), value.size());
      SvUTF8_on(sv);
   }
   PmStartFuncall(1);
   PUSHs(obj_ref);
   PUTBACK;
   glue::call_func_void(aTHX_ set_changed_cv);
}

std::string BigObjectType::name() const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(1);
   PUSHs(obj_ref);
   PUTBACK;
   return PropertyValue(glue::call_method_scalar(aTHX_ "full_name"));
}

SV* BigObject::give_impl(const AnyString& name) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUTBACK;
   return glue::call_func_scalar(aTHX_ give_cv);
}

SV* BigObject::lookup_impl(const AnyString& name) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUTBACK;
   return glue::call_method_scalar(aTHX_ "lookup");
}

SV* BigObject::give_with_property_name_impl(const AnyString& name, std::string& given) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUTBACK;
   if (glue::call_func_list(aTHX_ give_nm_cv) == 2) {
      SPAGAIN;
      Value(POPs) >> given;
      PmPopLastResult(pval);
      return pval;
   } else {
      throw std::runtime_error("property " + name + " does not exist");
   }
}

SV* BigObject::lookup_with_property_name_impl(const AnyString& name, std::string& given) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUTBACK;
   if (glue::call_method_list(aTHX_ "lookup_with_name") == 2) {
      SPAGAIN;
      Value(POPs) >> given;
      PmPopLastResult(pval);
      return pval;
   } else {
      return &PL_sv_undef;
   }
}

BigObject BigObject::lookup_multi(const AnyString& name, const std::string& subobj_name) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(3);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   mPUSHp(subobj_name.c_str(), subobj_name.size());
   PUTBACK;
   return BigObject(glue::call_method_scalar(aTHX_ "lookup", true));
}

BigObject BigObject::lookup_multi(const AnyString& name, const OptionSet& props) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(3);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUSHs(props.get());
   PUTBACK;
   return BigObject(glue::call_method_scalar(aTHX_ "lookup", true));
}

pm::Array<BigObject> BigObject::lookup_multi(const AnyString& name, all_selector) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(3);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   mPUSHp("*", 1);
   PUTBACK;
   return pm::Array<BigObject>(glue::call_method_scalar(aTHX_ "lookup"), ValueFlags::allow_undef);
}

BigObject BigObject::give_multi(const AnyString& name, const OptionSet& props, property_kind t) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(4);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUSHs(props.get());
   if (t == property_kind::temporary) PUSHs(&PL_sv_yes);
   PUTBACK;
   return BigObject(glue::call_func_scalar(aTHX_ give_cv, true));
}

void BigObject::save(const std::string& filename) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(filename.c_str(), filename.size());
   PUTBACK;
   glue::call_func_void(aTHX_ save_cv);
}

BigObject BigObject::load(const std::string& filename)
{
   dTHX;
   PmStartFuncall(1);
   mPUSHp(filename.c_str(), filename.size());
   PUTBACK;
   return BigObject(glue::call_func_scalar(aTHX_ load_cv, true));
}

PropertyValue BigObject::get_attachment(const AnyString& name) const
{
   check_ref(obj_ref);
   dTHX;
   SV** const valp = hv_fetch((HV*)SvRV(PmArray(obj_ref)[glue::Object_attachments_index]), name.ptr, I32(name.len), FALSE);
   return PropertyValue(valp ? SvREFCNT_inc(PmArray(*valp)[0]) : &PL_sv_undef, ValueFlags::allow_undef);
}

bool BigObject::exists(const AnyString& name) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUTBACK;
   return glue::call_func_bool(aTHX_ lookup_cv);
}

void BigObject::start_add(const AnyString& prop_name, property_kind t, const AnyString& sub_name, SV* sub_obj, size_t add_args) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(ssize_t(add_args)+4);
   PUSHs(obj_ref);
   mPUSHp(prop_name.ptr, prop_name.len);
   if (t == property_kind::temporary)
      PUSHs(glue::temporary_value_flag);
   if (sub_obj)
      PUSHs(sub_obj);
   else if (sub_name)
      mPUSHp(sub_name.ptr, sub_name.len);
   PUTBACK;
}

SV* BigObject::finish_add()
{
   dTHX;
   return glue::call_func_scalar(aTHX_ add_cv, true);
}

void BigObject::remove(const AnyString& name)
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUTBACK;
   glue::call_func_void(aTHX_ remove_cv);
}

void BigObject::remove_attachment(const AnyString& name)
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUTBACK;
   glue::call_func_void(aTHX_ remove_attachment_cv);
}

void BigObject::remove(const BigObject& sub_obj)
{
   check_ref(obj_ref);
   check_ref(sub_obj.obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   PUSHs(sub_obj.obj_ref);
   PUTBACK;
   glue::call_func_void(aTHX_ remove_cv);
}

bool BigObjectType::isa(const BigObjectType& other) const
{
   check_ref(obj_ref);
   check_ref(other.obj_ref);
   if (SvRV(obj_ref)==SvRV(other.obj_ref)) return true;
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   PUSHs(other.obj_ref);
   PUTBACK;
   return glue::call_func_bool(aTHX_ object_type_isa_cv);
}

bool BigObjectType::operator== (const BigObjectType& o) const
{
   return obj_ref && o.obj_ref ? SvROK(obj_ref)==SvROK(o.obj_ref) : obj_ref==o.obj_ref;
}

bool BigObject::isa(const BigObjectType& type) const
{
   check_ref(obj_ref);
   check_ref(type.obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   PUSHs(type.obj_ref);
   PUTBACK;
   return glue::call_func_bool(aTHX_ object_isa_cv);
}

bool BigObjectType::isa(const AnyString& type_name) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(type_name.ptr, type_name.len);
   PUTBACK;
   return glue::call_func_bool(aTHX_ object_type_isa_cv);
}

AnyString BigObjectType::TypeBuilder::app_method_name()
{
   // must be aligned with Application.pm
   return Str("construct_type");
}

bool BigObject::isa(const AnyString& type_name) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(type_name.ptr, type_name.len);
   PUTBACK;
   return glue::call_func_bool(aTHX_ object_isa_cv);
}

BigObjectType BigObject::type() const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(1);
   PUSHs(obj_ref);
   PUTBACK;
   return BigObjectType(glue::call_method_scalar(aTHX_ "type", true));
}

void BigObject::take_impl(const AnyString& name) const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   mPUSHp(name.ptr, name.len);
   PUTBACK;
}

void PropertyOut::finish()
{
   dTHX;
   dSP;
   XPUSHs(val.get_temp());
   if (t == property_kind::attachment) {
      if (construct_paths)
         mPUSHp(construct_paths.ptr, construct_paths.len);
      t = property_kind::none;
      PUTBACK;
      glue::call_func_void(aTHX_ attach_cv);
   } else {
      if (t == property_kind::temporary) XPUSHs(&PL_sv_yes);
      t = property_kind::none;
      PUTBACK;
      glue::call_func_void(aTHX_ take_cv);
   }
}

void PropertyOut::cancel()
{
   dTHX;
   PmCancelFuncall;
}

void BigObject::start_construction(const BigObjectType& type, const AnyString& name, size_t add_args)
{
   check_ref(type.obj_ref);
   dTHX;
   PmStartFuncall(ssize_t(add_args)+2);
   PUSHs(type.obj_ref);
   if (name) mPUSHp(name.ptr, name.len);
   PUTBACK;
}

SV* BigObject::finish_construction(bool with_props)
{
   dTHX;
   return glue::call_func_scalar(aTHX_ with_props ? new_filled_cv : new_named_cv, true);
}

void BigObject::pass_property(const AnyString& prop_name, Value& val)
{
   dTHX;
   dSP;
   mPUSHp(prop_name.ptr, prop_name.len);
   PUSHs(val.get_temp());
   PUTBACK;
}

BigObject::BigObject(const BigObjectType& type, const BigObject& src)
{
   check_ref(src.obj_ref);
   start_construction(type, AnyString());
   dTHX;
   dSP;
   PUSHs(src.obj_ref);
   PUTBACK;
   obj_ref = glue::call_func_scalar(aTHX_ new_copy_cv, true);
}

BigObject BigObject::copy() const
{
   check_ref(obj_ref);
   dTHX;
   PmStartFuncall(1);
   PUSHs(obj_ref);
   PUTBACK;
   return BigObject(glue::call_func_scalar(aTHX_ copy_cv, true));
}

BigObject& BigObject::cast(const BigObjectType& type)
{
   check_ref(obj_ref);
   check_ref(type.obj_ref);
   dTHX;
   PmStartFuncall(2);
   PUSHs(obj_ref);
   PUSHs(type.obj_ref);
   PUTBACK;
   glue::call_func_void(aTHX_ cast_cv);
   return *this;
}

BigObject BigObject::parent() const
{
   dTHX;
   if (obj_ref) {
      SV* parent_ref=PmArray(obj_ref)[glue::Object_parent_index];
      if (parent_ref && SvROK(parent_ref))
         return BigObject(newSVsv(parent_ref));
   }
   return BigObject();
}

std::false_type* Value::retrieve(BigObject& x) const
{
   dTHX;
   if (!(options * ValueFlags::not_trusted) ||
       __builtin_expect(SvROK(sv) && sv_derived_from(sv, "Polymake::Core::BigObject"), 1)) {
      copy_ref(x.obj_ref, sv);
   } else if (SvOK(sv)) {
      throw exception("input value is not an BigObject");
   } else {
      copy_ref(x.obj_ref, nullptr);
   }
   return nullptr;
}

std::false_type* Value::retrieve(BigObjectType& x) const
{
   dTHX;
   if (!(options * ValueFlags::not_trusted) ||
       __builtin_expect(SvROK(sv) && sv_derived_from(sv, "Polymake::Core::BigObjectType"), 1)) {
      copy_ref(x.obj_ref, sv);
   } else if (SvOK(sv)) {
      throw exception("input value is not a valid BigObjectType");
   } else {
      copy_ref(x.obj_ref, nullptr);
   }
   return nullptr;
}

Value::NoAnchors Value::put_val(const BigObjectType& x, int)
{
   check_ref(x.obj_ref);
   copy_ref(sv, x.obj_ref);
   return NoAnchors();
}


Value::NoAnchors Value::put_val(const BigObject& x, int)
{
   check_ref(x.obj_ref);
   dTHX;

   // If the read_only flag is set, then this call is part of the preparation for parent_object.take();
   // in this case the child's transaction will be hung into the parent's one.
   // TODO: this is going to be changed: init transaction is going to be closed prior to adding the subobject!
   if ((options & (ValueFlags::read_only | ValueFlags::expect_lval)) != ValueFlags::read_only &&
       has_init_transaction(x.obj_ref)) {
      PmStartFuncall(1);
      PUSHs(x.obj_ref);
      PUTBACK;
      glue::call_func_void(aTHX_ commit_cv);
   }
   copy_ref(sv, x.obj_ref);

   if ((options & (ValueFlags::read_only | ValueFlags::allow_non_persistent | ValueFlags::allow_store_any_ref)) ==
       (ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref)) {
      // returning a new BigObject thru the glueing layer
      SV* const name=PmArray(x.obj_ref)[glue::Object_name_index];
      if (!SvOK(name)) {
         if (SV* const var_name=glue::name_of_ret_var(aTHX))
            sv_setsv(name, var_name);
      }
   }

   return NoAnchors();
}


Value::NoAnchors Value::put_val(const pm::Array<BigObject>& ar, int)
{
   dTHX;
   // If the read_only flag is set, then this call is part of the preparation for parent_object.take();
   // in this case the children's transactions will be hung into the parent's one.
   if ((options & (ValueFlags::read_only | ValueFlags::expect_lval)) == ValueFlags::read_only) {
      if (!ar.empty() && !ar.element_type().valid())
         throw std::runtime_error("can't create a property from an a big object array with incompatible element types");
   } else if (!SvREADONLY(SvRV(ar.get()))) {
      ssize_t last = AvFILLp(SvRV(ar.get()));
      if (last >= 0) {
         for (SV **objp = PmArray(ar.get()), **lastp = objp + last; objp <= lastp; ++objp) {
            SV* const objref = *objp;
            if (objref && SvROK(objref)) {
               if (has_init_transaction(objref)) {
                  PmStartFuncall(1);
                  PUSHs(objref);
                  PUTBACK;
                  glue::call_func_void(aTHX_ commit_cv);
               }
            } else {
               throw std::runtime_error("invalid void element in a big object array");
            }
         }
      }
   }
   if (SvROK(sv)) sv_unref_flags(sv, SV_IMMEDIATE_UNREF);
   sv_setsv(sv, ar.get());
   return NoAnchors();
}

template <bool is_readonly>
BigObject::Array_element<is_readonly>::Array_element(SV* ref_arg, const BigObjectType& type)
   : BigObject(SvREFCNT_inc_simple(ref_arg))
   , allowed_type(type)
{}

template <bool is_readonly>
BigObject::Array_element<is_readonly>& BigObject::Array_element<is_readonly>::operator= (const BigObject& o)
{
   if (is_readonly || SvREADONLY(obj_ref))
      throw std::runtime_error("attempt to modify an immutable array of big objects");
   if (allowed_type.valid() && !o.isa(allowed_type))
      throw std::runtime_error("object does not match the prescribed element type");
   copy_ref(obj_ref, o.obj_ref);
   return *this;
}

template class BigObject::Array_element<true>;
template class BigObject::Array_element<false>;

BigObject::Schedule::~Schedule()
{
   dTHX;
   SvREFCNT_dec(obj_ref);
}

BigObject::Schedule::Schedule(const Schedule& x)
   : obj_ref(init_copy_ref(x.obj_ref)) {}

BigObject::Schedule::Schedule(FunCall&& fc)
   : obj_ref(nullptr)
{
   PropertyValue v(fc);
   if (SvROK(v.sv)) {
      obj_ref=v.sv;
      SvREFCNT_inc_simple_void(obj_ref);
   }
}

BigObject::Schedule& BigObject::Schedule::operator= (const Schedule& x)
{
   copy_ref(obj_ref, x.obj_ref);
   return *this;
}

BigObject::Schedule& BigObject::Schedule::operator= (FunCall&& fc)
{
   dTHX;
   PropertyValue v(fc);
   SvREFCNT_dec(obj_ref);
   if (SvROK(v.sv)) {
      obj_ref=v.sv;
      SvREFCNT_inc_simple_void(obj_ref);
   } else {
      obj_ref=nullptr;
   }
   return *this;
}

ListResult BigObject::Schedule::list_new_properties() const
{
   check_ref(obj_ref);
   return FunCall::call_method("list_new_properties", obj_ref);
}

void BigObject::Schedule::apply(BigObject& o) const
{
   check_ref(obj_ref);
   check_ref(o.obj_ref);
   (void)FunCall::call_method("apply", obj_ref, o.obj_ref);
}

}

using namespace perl;

pm::Array<BigObject>::Array(std::initializer_list<BigObject> objects)
   : base_t(objects.size())
{
   dTHX;
   Int i = 0;
   for (const BigObject& o : objects) {
      sv_setsv(ArrayHolder::operator[](i), o.obj_ref);
      ++i;
   }
}

pm::Array<BigObject>::Array(const BigObjectType& type, std::initializer_list<BigObject> objects)
   : Array(objects)
{
   el_type=type;
   for (const BigObject& o : objects) {
      if (!o.isa(el_type))
         throw std::runtime_error("object does not match the prescribed element type");
   }
   set_Array_type(sv, el_type.obj_ref);
}

pm::Array<BigObject>::Array(const BigObjectType& type, Int n)
   : base_t(nullptr)
   , el_type(type)
{
   check_ref(type.obj_ref);
   dTHX;
   SV* array_type = get_Array_type(aTHX_ el_type.obj_ref);
   PmStartFuncall(2);
   PUSHs(array_type);
   mPUSHi(n);
   PUTBACK;
   sv=glue::call_func_scalar(aTHX_ construct_with_size_cv);
}

void pm::Array<BigObject>::resize(Int n)
{
   if (SvREADONLY(SvRV(sv)))
       throw std::runtime_error("attempt to modify an immutable array of big objects");
   if (SvOBJECT(SvRV(sv)))
      (void)FunCall::call_method("resize", sv, n);
   else
      ArrayHolder::resize(n);
}

const BigObjectType& pm::Array<BigObject>::element_type() const
{
   if (!el_type.valid()) {
      if (SvOBJECT(SvRV(sv))) {
         // the underlying perl array has a type
         dTHX;
         PmStartFuncall(1);
         PUSHs(sv);
         PUTBACK;
         SV* arr_type = glue::call_method_scalar(aTHX_ "type", true);
         if (!arr_type)
            throw std::runtime_error("can't retrieve the full type of a big object array");
         el_type.obj_ref=newSVsv(PmArray(PmArray(arr_type)[glue::PropertyType_params_index])[0]);
      } else if (!empty()) {
         // try to deduce from elements
         el_type = (*this)[0].type();
         const Int n = size();
         for (Int i = 1; i < n; ++i) {
            BigObjectType t = (*this)[i].type();
            if (t != el_type) {
               if (el_type.isa(t)) {
                  el_type = t;
               } else if (!t.isa(el_type)) {
                  // incompatible: reset to invalid and give up
                  el_type = BigObjectType();
                  break;
               }
            }
         }
         if (el_type.valid())
            set_Array_type(sv, el_type.obj_ref);
      }
   }
   return el_type;
}

pm::Array<BigObject> pm::Array<BigObject>::copy() const
{
   if (el_type.valid()) {
      dTHX;
      PmStartFuncall(1);
      PUSHs(sv);
      PUTBACK;
      Array result(glue::call_method_scalar(aTHX_ "copy"));
      result.el_type = el_type;
      return result;
   } else {
      const Int n = size();
      Array result(n);
      for (Int i = 0; i < n; ++i) {
         result[i] = (*this)[i].copy();
      }
      result.el_type = el_type;
      return result;
   }
}

void pm::Array<BigObject>::push_back(BigObject&& o)
{
   if (SvREADONLY(SvRV(sv)))
      throw std::runtime_error("attempt to modify an immutable array of big objects");
   if (el_type.valid() && !o.isa(el_type))
      throw std::runtime_error("object does not match the prescribed element type");
   ArrayHolder::push(o.obj_ref);
   o.obj_ref=nullptr;
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
