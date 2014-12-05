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
#include "polymake/perl/calls.h"
#include "polymake/perl/Object.h"
#include "polymake/Array.h"

namespace pm { namespace perl {

static glue::cached_cv
   give_cv={ "Polymake::Core::Object::give", 0 },
   give_nm_cv={ "Polymake::Core::Object::give_with_name", 0 },
   take_cv={ "Polymake::Core::Object::take", 0 },
   lookup_cv={ "Polymake::Core::Object::lookup_pv", 0 },
   add_cv={ "Polymake::Core::Object::add", 0 },
   remove_cv={ "Polymake::Core::Object::remove", 0 },
   attach_cv={ "Polymake::Core::Object::attach", 0 },
   remove_attachment_cv={ "Polymake::Core::Object::remove_attachment", 0 },
   set_name_cv={ "Polymake::Core::Object::set_name", 0 },
   object_isa_cv={ "Polymake::Core::Object::isa", 0 },
   object_type_isa_cv={ "Polymake::Core::ObjectType::isa", 0 },
   commit_cv={ "Polymake::Core::Object::commit", 0 },
   new_cv={ "Polymake::Core::Object::new_named", 0 },
   construct_cv={ "Polymake::Core::Object::construct", 0 },
   copy_cv={ "Polymake::Core::Object::copy", 0 },
   cast_cv={ "Polymake::Core::Object::cast", 0 },
   load_cv={ "Polymake::User::load", 0 },
   save_cv={ "Polymake::User::save", 0 };

namespace {

inline
void copy_ref(SV* &dst, SV* const src)
{
   dTHX;
   if (SvROK(dst)) {
      if (SvROK(src)) {
         if (SvRV(src) != SvRV(dst)) {
            sv_unref_flags(dst, SV_IMMEDIATE_UNREF);
            sv_setsv(dst, src);
         }
      } else {
         sv_unref_flags(dst, SV_IMMEDIATE_UNREF);
      }
   } else {
      if (SvROK(src))
         sv_setsv(dst, src);
   }
}

inline
SV* init_copy_ref(SV* src)
{
   dTHX;
   if (SvROK(src)) {
      return newSVsv(src);
   }
   return newSV_type(SVt_IV);
}

inline
bool equal_refs(SV* r1, SV* r2)
{
   return SvROK(r1) ? SvRV(r1)==SvRV(r2) : !SvROK(r2);
}

}

ObjectType::ObjectType()
{
   dTHX;
   obj_ref=newSV_type(SVt_IV);
}

ObjectType::ObjectType(const ObjectType& x)
   : obj_ref(init_copy_ref(x.obj_ref)) {}

ObjectType& ObjectType::operator= (const ObjectType& x)
{
   copy_ref(obj_ref, x.obj_ref);
   return *this;
}

ObjectType::~ObjectType()
{
   dTHX;
   SvREFCNT_dec(obj_ref);
}

Object::Object()
   : needs_commit(false)
{
   dTHX;
   obj_ref=newSV_type(SVt_IV);
}

Object::Object(const Object& x)
   : obj_ref(init_copy_ref(x.obj_ref))
   , needs_commit(x.needs_commit)
{
   x.needs_commit=false;
}

Object::Object(SV* ref_arg, const Array_access<Object>&)
   : obj_ref(SvREFCNT_inc_simple(ref_arg))
   , needs_commit(false) {}

void Object::create_new(const ObjectType& type)
{
   _create(type);
}

Object& Object::operator= (const Object& x)
{
   copy_ref(obj_ref, x.obj_ref);
   needs_commit=x.needs_commit;
   x.needs_commit=false;
   return *this;
}

Object::~Object()
{
   dTHX;
   SvREFCNT_dec(obj_ref);
}

std::string Object::name() const
{
   dTHX;
   size_t l=0;
   const char* namep=SvPV(PmArray(obj_ref)[glue::Object_name_index], l);
   return std::string(namep,l);
}

std::string Object::description() const
{
   dTHX;
   size_t l=0;
   const char* descrp=SvPV(PmArray(obj_ref)[glue::Object_description_index], l);
   return std::string(descrp,l);
}

void Object::set_name(const std::string& name)
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name.c_str(), name.size());
   PUTBACK;
   glue::call_func_void(aTHX_ set_name_cv);
}

void Object::set_description(const std::string& value, bool append)
{
   dTHX;
   SV* const sv=PmArray(obj_ref)[glue::Object_description_index];
   if (append) {
      sv_catpvn(sv, value.c_str(), value.size());
   } else {
      sv_setpvn(sv, value.c_str(), value.size());
      SvUTF8_on(sv);
   }
}

std::string ObjectType::name() const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   PUTBACK;
   return PropertyValue(glue::call_method_scalar(aTHX_ "full_name"));
}

SV* Object::_give(const char* name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   PUTBACK;
   return glue::call_func_scalar(aTHX_ give_cv);
}

SV* Object::_lookup(const char* name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   PUTBACK;
   return glue::call_method_scalar(aTHX_ "lookup");
}

SV* Object::_give_with_property_name(const char* name, size_t nl, std::string& given) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   PUTBACK;
   if (glue::call_func_list(aTHX_ give_nm_cv)==2) {
      SPAGAIN;
      Value(POPs) >> given;
      PmPopLastResult(pval);
      return pval;
   } else {
      throw std::runtime_error(std::string("property ") + name + " does not exist");
   }
}

SV* Object::_lookup_with_property_name(const char* name, size_t nl, std::string& given) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   PUTBACK;
   if (glue::call_method_list(aTHX_ "lookup_with_name")==2) {
      SPAGAIN;
      Value(POPs) >> given;
      PmPopLastResult(pval);
      return pval;
   } else {
      return &PL_sv_undef;
   }
}

SV* Object::_lookup(const char* name, size_t nl, const std::string& subobj_name) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   mXPUSHp(subobj_name.c_str(), subobj_name.size());
   PUTBACK;
   return glue::call_method_scalar(aTHX_ "give");
}

SV* Object::_give(const char* name, size_t nl, SV* props, property_type t) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   XPUSHs(props);
   if (t==temporary) XPUSHs(&PL_sv_yes);
   PUTBACK;
   return glue::call_func_scalar(aTHX_ give_cv);
}

void Object::save(const std::string& filename) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(filename.c_str(), filename.size());
   PUTBACK;
   glue::call_func_void(aTHX_ save_cv);
}

Object Object::load(const std::string& filename)
{
   dTHX;
   PmStartFuncall;
   mXPUSHp(filename.c_str(), filename.size());
   PUTBACK;
   return Object(glue::call_func_scalar(aTHX_ load_cv));
}

SV* Object::_lookup(const char* name, size_t nl, SV* props) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   XPUSHs(props);
   PUTBACK;
   return glue::call_method_scalar(aTHX_ "lookup");
}

SV* Object::_give_all(const char* name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   mXPUSHp("*",1);
   PUTBACK;
   return glue::call_method_scalar(aTHX_ "lookup");
}

SV* Object::_get_attachment(const char* name, size_t nl) const
{
   dTHX;
   SV** const valp=hv_fetch((HV*)SvRV(PmArray(obj_ref)[glue::Object_attachments_index]), name, nl, false);
   return valp ? SvREFCNT_inc(PmArray(*valp)[0]) : &PL_sv_undef;
}

bool Object::_exists(const char* name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   PUTBACK;
   return glue::call_func_bool(aTHX_ lookup_cv, false);
}

SV* Object::_add(const char* name, size_t nl, SV* sub_obj, property_type t) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   if (sub_obj) XPUSHs(sub_obj);
   if (t==temporary) XPUSHs(&PL_sv_yes);
   PUTBACK;
   return glue::call_func_scalar(aTHX_ add_cv);
}

void Object::_remove(const char* name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   PUTBACK;
   glue::call_func_void(aTHX_ remove_cv);
}

void Object::_remove_attachment(const char* name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name,nl);
   PUTBACK;
   glue::call_func_void(aTHX_ remove_attachment_cv);
}

void Object::remove(const Object& sub_obj)
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   XPUSHs(sub_obj.obj_ref);
   PUTBACK;
   glue::call_func_void(aTHX_ remove_cv);
}

SV* ObjectType::construct_parameterized_type(const char* type_name, size_t nl)
{
   dTHX;
   // type arguments are already pushed on the stack
   return glue::call_func_scalar(aTHX_ glue::fetch_typeof_gv(aTHX_ type_name, nl));
}

bool ObjectType::isa(const ObjectType& other) const
{
   if (equal_refs(obj_ref,other.obj_ref)) return true;
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   XPUSHs(other.obj_ref);
   PUTBACK;
   return glue::call_func_bool(aTHX_ object_type_isa_cv, true);
}

bool Object::isa(const ObjectType& type) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   XPUSHs(type.obj_ref);
   PUTBACK;
   return glue::call_func_bool(aTHX_ object_isa_cv, true);
}

bool ObjectType::_isa(const char* type_name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(type_name, nl);
   PUTBACK;
   return glue::call_func_bool(aTHX_ object_type_isa_cv, true);
}

SV* ObjectType::find_type(const char* type_name, size_t tl)
{
   dTHX;
   PmStartFuncall;
   SP=glue::push_current_application(aTHX_ SP);
   mXPUSHp(type_name, tl);
   PUTBACK;
   return glue::call_method_scalar(aTHX_ "eval_type_throw");
}

bool Object::_isa(const char* type_name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(type_name, nl);
   PUTBACK;
   return glue::call_func_bool(aTHX_ object_isa_cv, true);
}

ObjectType Object::type() const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   PUTBACK;
   return ObjectType(glue::call_method_scalar(aTHX_ "type"));
}

PerlInterpreter* Object::_take(const char* name, size_t nl) const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   mXPUSHp(name, nl);
   PUTBACK;
   return getTHX;
}

void PropertyOut::finish()
{
   dTHXa(pi);
   dSP;
   XPUSHs(val.get_temp());
   if (t==attachment) {
      t=_done;
      PUTBACK;
      glue::call_func_void(aTHX_ attach_cv);
   } else {
      if (t==temporary) XPUSHs(&PL_sv_yes);
      t=_done;
      PUTBACK;
      glue::call_func_void(aTHX_ take_cv);
   }
}

void PropertyOut::cancel()
{
   dTHXa(pi);
   PmCancelFuncall;
}

void Object::_create(const ObjectType& type, const char* name, size_t nl)
{
   dTHX;
   PmStartFuncall;
   XPUSHs(type.obj_ref);
   if (name) mXPUSHp(name, nl);
   PUTBACK;
   glue::call_func_scalar(aTHX_ new_cv, &obj_ref);
   needs_commit=true;
}

void Object::_create_copy(const ObjectType& type, const Object& src)
{
   dTHX;
   PmStartFuncall;
   XPUSHs(type.obj_ref);
   XPUSHs(src.obj_ref);
   PUTBACK;
   glue::call_func_scalar(aTHX_ construct_cv, &obj_ref);
   needs_commit=false;
}

Object Object::copy() const
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   PUTBACK;
   return Object(glue::call_func_scalar(aTHX_ copy_cv));
}

Object& Object::cast(const ObjectType& type)
{
   dTHX;
   PmStartFuncall;
   XPUSHs(obj_ref);
   XPUSHs(type.obj_ref);
   PUTBACK;
   glue::call_func_void(aTHX_ cast_cv);
   return *this;
}

Object Object::parent() const
{
   dTHX;
   if (obj_ref != NULL) {
      SV* parent_ref=PmArray(obj_ref)[glue::Object_parent_index];
      if (SvROK(parent_ref))
         return Object(newSVsv(parent_ref));
   }
   return Object();
}

False* Value::retrieve(Object& x) const
{
   dTHX;
   if (!(options & value_not_trusted) ||
       __builtin_expect(SvROK(sv) && sv_derived_from(sv, "Polymake::Core::Object"), 1)) {
      if (SvROK(x.obj_ref))
         sv_unref_flags(x.obj_ref, SV_IMMEDIATE_UNREF);
      sv_setsv(x.obj_ref, sv);
   } else {
      throw exception("input value is not an Object");
   }
   return NULL;
}

False* Value::retrieve(ObjectType& x) const
{
   dTHX;
   if (!(options & value_not_trusted) ||
       __builtin_expect(SvROK(sv) && sv_derived_from(sv, "Polymake::Core::ObjectType"), 1)) {
      if (SvROK(x.obj_ref))
         sv_unref_flags(x.obj_ref, SV_IMMEDIATE_UNREF);
      sv_setsv(x.obj_ref, sv);
   } else {
      throw exception("input value is not a valid ObjectType");
   }
   return NULL;
}

Value::NoAnchor* Value::put(const ObjectType& x, const char*, int)
{
   dTHX;
   if (__builtin_expect(SvROK(x.obj_ref), 1)) {
      sv_setsv(sv, x.obj_ref);
   } else {
      throw std::runtime_error("invalid assignment of a void object type");
   }
   return NULL;
}


Value::NoAnchor* Value::put(const Object& x, const char* fup, int)
{
   dTHX;
   if (__builtin_expect(SvROK(x.obj_ref), 1)) {
      if (x.needs_commit) {
         x.needs_commit=false;
         // If the read_only flag is set, then this call is part of the preparation for parent_object.take();
         // in this case the child's transaction will be hung into the parent's one.
         if ((options & (value_read_only|value_expect_lval)) != value_read_only &&
             SvOK(PmArray(x.obj_ref)[glue::Object_transaction_index])) {
            PmStartFuncall;
            XPUSHs(x.obj_ref);
            PUTBACK;
            glue::call_func_void(aTHX_ commit_cv);
         }
      }
      if (SvROK(sv)) sv_unref_flags(sv, SV_IMMEDIATE_UNREF);
      sv_setsv(sv, x.obj_ref);
      if (fup != NULL) {
         // returning a new Object to perl
         SV* const name=PmArray(x.obj_ref)[glue::Object_name_index];
         if (!SvOK(name)) {
            if (SV* const var_name=pm_perl_name_of_ret_var(aTHX))
               sv_setsv(name, var_name);
         }
      }
   } else {
      throw std::runtime_error("invalid assignment of a void object");
   }
   return NULL;
}


Value::NoAnchor* Value::put(const pm::Array<Object>& ar, const char* fup, int)
{
   dTHX;
   if (ar.needs_commit) {
      ar.needs_commit=false;
      // If the read_only flag is set, then this call is part of the preparation for parent_object.take();
      // in this case the children's transactions will be hung into the parent's one.
      if ((options & (value_read_only|value_expect_lval)) != value_read_only) {
         int last=AvFILLp(SvRV(ar.get()));
         if (last >= 0) {
            for (SV **objp=PmArray(ar.get()), **lastp=objp+last; objp<=lastp; ++objp) {
               SV* const objref=*objp;
               if (SvROK(objref)) {
                  if (SvOK(PmArray(objref)[glue::Object_transaction_index])) {
                     PmStartFuncall;
                     XPUSHs(objref);
                     PUTBACK;
                     glue::call_func_void(aTHX_ commit_cv);
                  }
               } else {
                  throw std::runtime_error("invalid void object in an Array<Object>");
               }
            }
         }
      }
   }
   if (SvROK(sv)) sv_unref_flags(sv, SV_IMMEDIATE_UNREF);
   sv_setsv(sv, ar.get());
   return NULL;
}


Object::Schedule::Schedule()
{
   dTHX;
   obj_ref=newSV_type(SVt_IV);
}

Object::Schedule::Schedule(const Schedule& x) :
   obj_ref(init_copy_ref(x.obj_ref)) {}

Object::Schedule::Schedule(const PropertyValue& v) :
   obj_ref(v.sv)
{
   SvREFCNT_inc_simple_void(obj_ref);
}

Object::Schedule& Object::Schedule::operator= (const Schedule& x)
{
   copy_ref(obj_ref, x.obj_ref);
   return *this;
}

Object::Schedule& Object::Schedule::operator= (const PropertyValue& v)
{
   dTHX;
   SvREFCNT_dec(obj_ref);
   obj_ref=v.sv;
   SvREFCNT_inc_simple_void(obj_ref);
   return *this;
}

bool Object::valid() const { return SvROK(obj_ref); }

bool Object::Schedule::valid() const { return SvROK(obj_ref); }

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
