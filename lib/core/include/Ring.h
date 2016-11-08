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

#ifndef POLYMAKE_RING_H
#define POLYMAKE_RING_H

#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/hash_map"

namespace pm {

// simplified key for identification of Rings built with auto-generated variable names
struct simplified_ring_key {
   std::string varname;
   int arity;
   const unsigned int* coeff_ring_id;

   simplified_ring_key(const std::string& varname_arg, int arity_arg, const unsigned int* coeff_ring_arg = NULL) :
      varname(varname_arg), arity(arity_arg), coeff_ring_id(coeff_ring_arg) {}

   bool operator== (const simplified_ring_key& other) const
   {
      return varname       == other.varname
          && arity         == other.arity
          && coeff_ring_id == other.coeff_ring_id;
   }
};

template <>
struct hash_func<simplified_ring_key, is_opaque> {
public:
   size_t operator() (const simplified_ring_key& a) const
   {
      return str_hash(a.varname) + a.arity + size_t(a.coeff_ring_id);
   }
protected:
   hash_func<std::string> str_hash;
};

class Ring_base {
public:
   // unique id for each Ring used during the entire session
   typedef unsigned int id_type;

   // key identifying a Ring: names of the variables and the id of the coefficient Ring (NULL for elementary numeric types)
   typedef std::pair<Array<std::string>, const id_type*> ring_key_type;

protected:
   // repository of Rings of a certain type
   typedef hash_map<ring_key_type, id_type> repo_by_key_type;

   // search directory by simplified keys 
   typedef hash_map<simplified_ring_key, const id_type*> repo_by_arity_type;

   static const id_type* find_by_key(repo_by_key_type& repo_by_key, const ring_key_type& key);
   static const id_type* find_by_arity(repo_by_key_type& repo_by_key, repo_by_arity_type& repo_by_arity, const simplified_ring_key& s_key);

   static const ring_key_type& get_repo_key(const id_type* id_ptr)
   {
      return reverse_cast(id_ptr, &repo_by_key_type::value_type::second)->first;
   }

   static id_type global_id;

   static hash_map<id_type,Array<std::string>>& pretty_names_map()
   {
      static hash_map<id_type,Array<std::string>> override_names;
      return override_names;
   }

public:
   template <typename P>
   static void override_varnames(const Array<std::string>& n);

};

template <typename Coefficient=Rational, typename Exponent=int>
class Monomial;
template <typename Coefficient=Rational, typename Exponent=int>
class UniMonomial;

template <typename Coefficient, typename Exponent>
class Ring_impl :
   public Ring_base {
protected:
   const id_type* id_ptr;
   const id_type* coeff_id_ptr;

   static repo_by_key_type& repo_by_key()
   {
      static repo_by_key_type repo;
      return repo;
   }

   static repo_by_arity_type& repo_by_arity()
   {
      static repo_by_arity_type repo;
      return repo;
   }

   class Monomial_constructor {
   public:
      typedef int argument_type;
      typedef Monomial<Coefficient, Exponent> result_type;
      result_type operator() (argument_type i) const;

      Monomial_constructor(const Ring_impl& r) : ring(r) {}
   private:
      Ring_impl ring;
   };

   Ring_impl(const id_type* id_ptr_arg=NULL) :
      id_ptr(id_ptr_arg), coeff_id_ptr(NULL) {}

   Ring_impl(const ring_key_type& key) :
      id_ptr(find_by_key(repo_by_key(), key)), coeff_id_ptr(key.second) {}

   Ring_impl(const simplified_ring_key& s_key) :
      id_ptr(find_by_arity(repo_by_key(), repo_by_arity(), s_key)),
      coeff_id_ptr(s_key.coeff_ring_id) {}

   Ring_impl(int n, const std::string& name, const id_type* coeff_ring_id) :
      id_ptr(n>1 ? find_by_arity(repo_by_key(), repo_by_arity(), simplified_ring_key(name, n, coeff_ring_id))
                 : find_by_key(repo_by_key(), ring_key_type(Array<std::string>(1, name), coeff_ring_id))),
      coeff_id_ptr(coeff_ring_id) {}
public:
   class Variables;

   typedef Coefficient coefficient_type;
   typedef Exponent exponent_type;

   void swap(Ring_impl& other)
   {
      std::swap(id_ptr, other.id_ptr);
      std::swap(coeff_id_ptr, other.coeff_id_ptr);
   }

   int n_vars() const
   {
      return names().size();
   }

   const Array<std::string>& names() const
   {
      return get_repo_key(id_ptr).first;
   }

   const Array<std::string>& pretty_names() const
   {
      const auto& name_map_it = pretty_names_map().find(*id_ptr);
      if (name_map_it == pretty_names_map().end())
         return names();
      else
         return name_map_it->second;
   }

   void set_pretty_names(const Array<std::string>& n) const
   {
      if (n.size() != n_vars())
         throw std::runtime_error("Ring->set_pretty_names: wrong number of variables");
      pretty_names_map()[*id_ptr] = n;
   }

   id_type id() const { return *id_ptr; }

   Variables variables() const;

   UniMonomial<Coefficient, Exponent> variable() const;

   bool operator== (const Ring_impl& other) const
   {
      // uninitialized rings are never equal to each other
      return id_ptr != NULL && id_ptr == other.id_ptr;
   }

   bool operator!= (const Ring_impl& other) const
   {
      return !operator==(other);
   }
};

template <typename Something>
struct matching_ring : std::false_type {};

template <typename ring_type, bool requires_coeff_ring=matching_ring<typename ring_type::coefficient_type>::value>
struct ring_var_name;

template <typename ring_type>
struct ring_var_name<ring_type, false> {
   static const int depth = 0;
   static const char name = 'x';
};

template <typename ring_type>
struct ring_var_name<ring_type, true> {
   static const int depth = ring_var_name<typename ring_type::coefficient_ring_type>::depth + 1;
   // x y z u v w t
   static const char name = depth < 3 ? 'x' + depth : depth < 6 ? 'u' + depth - 3 : 't';
};

template <typename P>
void Ring_base::override_varnames(const Array<std::string>& n)
{
   typename matching_ring<P>::type(1).set_pretty_names(n);
}

template <typename Coefficient=Rational, typename Exponent=int,
          bool requires_coeff_ring=matching_ring<Coefficient>::value>
class Ring
   : public Ring_impl<Coefficient, Exponent> {
   typedef Ring_impl<Coefficient, Exponent> super;
   template <typename, typename, bool> friend class Ring;
   template <typename, typename> friend class Ring_impl;
   template <typename> friend struct spec_object_traits;

   explicit Ring(const typename super::id_type* id_ptr_arg)
      : super(id_ptr_arg) {}

   explicit Ring(const super ring_impl)
      : super(ring_impl) {}
public:
   typedef Ring scalar_ring_type;

   // undefined object - to be read later from perl::Value
   Ring() {}

   // construct with given variable names, passed in a container
   template <typename Container, typename enabled=typename std::enable_if<isomorphic_to_container_of<Container, std::string>::value>::type>
   explicit Ring(const Container& names)
      : super(typename super::ring_key_type(names, NULL)) {}

   // construct with given variable names, passed in a static array
   template <size_t n>
   explicit Ring(const char* (&names)[n])
      : super(typename super::ring_key_type(Array<std::string>(n, names), NULL)) {}

   // construct with standard variable names x1, x2, ...
   explicit Ring(int n, const std::string& name=std::string(1,ring_var_name<Ring>::name))
      : super(n, name, NULL) {}

   // construct a univariate ring
   explicit Ring(const std::string& name)
      : super(1, name, NULL) {}

   const Coefficient& zero_coef() const
   {
      return zero_value<Coefficient>();
   }

   const Coefficient& one_coef() const
   {
      return one_value<Coefficient>();
   }

   const scalar_ring_type& get_scalar_ring() const { return *this; }
};

template <typename Coefficient, typename Exponent>
class Ring<Coefficient, Exponent, true>
   : public Ring_impl<Coefficient, Exponent> {
   typedef Ring_impl<Coefficient, Exponent> super;
   template <typename, typename, bool> friend class Ring;
   template <typename, typename> friend class Ring_impl;
   template <typename> friend struct spec_object_traits;

   explicit Ring(const typename super::id_type* id_ptr_arg)
      : super(id_ptr_arg) {}

   explicit Ring(const super& ring_impl)
      : super(ring_impl) {}
public:
   typedef typename matching_ring<Coefficient>::type coefficient_ring_type;
   typedef typename coefficient_ring_type::scalar_ring_type scalar_ring_type;

private:
   // cache a copy of the coefficient ring, also makes it live long enough for serialization
   mutable coefficient_ring_type coeff_ring_cache;

public:
   // undefined object - to be read later from perl::Value
   Ring() {}

   // construct with a ring of coefficients given explicitly

   template <typename Container, typename enabled=typename std::enable_if<isomorphic_to_container_of<Container, std::string>::value>::type>
   Ring(const coefficient_ring_type& coeff_ring, const Container& names)
      : super(typename super::ring_key_type(names, coeff_ring.id_ptr)) {}

   template <size_t n>
   Ring(const coefficient_ring_type& coeff_ring, const char* (&names)[n])
      : super(typename super::ring_key_type(Array<std::string>(n, names), coeff_ring.id_ptr)) {}

   Ring(const coefficient_ring_type& coeff_ring, int n, const std::string& name=std::string(1,ring_var_name<Ring>::name))
      : super(n, name, coeff_ring.id_ptr) {}

   Ring(const coefficient_ring_type& coeff_ring, const std::string& name)
      : super(1, name, coeff_ring.id_ptr) {}

   // construct with a default (unvariate) ring of coefficients

   template <typename Container, typename enabled=typename std::enable_if<isomorphic_to_container_of<Container, std::string>::value>::type>
   explicit Ring(const Container& names)
      : super(typename super::ring_key_type(names, default_coefficient_ring().id_ptr)) {}

   template <size_t n>
   explicit Ring(const char* (&names)[n])
      : super(typename super::ring_key_type(Array<std::string>(n, names), default_coefficient_ring().id_ptr)) {}

   explicit Ring(int n, const std::string& name=std::string(1,ring_var_name<Ring>::name)) :
      super(n, name, default_coefficient_ring().id_ptr) {}

   explicit Ring(const std::string& name)
      : super(1, name, default_coefficient_ring().id_ptr) {}

   const coefficient_ring_type& get_coefficient_ring() const
   {
      if (this->coeff_id_ptr == NULL)
         throw std::runtime_error("internal Ring error: invalid coefficient ring id");
      if (coeff_ring_cache.id_ptr == NULL)
         coeff_ring_cache = coefficient_ring_type(this->coeff_id_ptr);
      return coeff_ring_cache;
   }

   const Coefficient& zero_coef() const
   {
      static const Coefficient zero_c(get_coefficient_ring());
      return zero_c;
   }

   const Coefficient& one_coef() const
   {
      static const Coefficient one_c(get_coefficient_ring().one_coef(), get_coefficient_ring());
      return one_c;
   }

   // assuming per default univariate polynomials
   static
   coefficient_ring_type default_coefficient_ring()
   {
      return coefficient_ring_type(1);
   }

   const scalar_ring_type& get_scalar_ring() const
   {
      return get_coefficient_ring().get_scalar_ring();
   }
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< Ring<Coefficient, Exponent, false> > >
   : spec_object_traits<is_composite> {

   typedef Ring<Coefficient, Exponent, false> masquerade_for;

   typedef Array<std::string> elements;

   template <typename Visitor>
   static void visit_elements(Serialized<masquerade_for>& me, Visitor& v)
   {
      Array<std::string> names;
      v << names;
      me.id_ptr=me.find_by_key(me.repo_by_key(), typename masquerade_for::ring_key_type(names, NULL));
   }

   template <typename Visitor>
   static void visit_elements(const Serialized<masquerade_for>& me, Visitor& v)
   {
      v << me.names();
   }
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< Ring<Coefficient, Exponent, true> > >
   : spec_object_traits<is_composite> {

   typedef Ring<Coefficient, Exponent, true> masquerade_for;

   typedef cons<typename masquerade_for::coefficient_ring_type,
                Array<std::string> > elements;

   template <typename Visitor>
   static void visit_elements(Serialized<masquerade_for>& me, Visitor& v)
   {
      typename masquerade_for::coefficient_ring_type coeff_ring;
      Array<std::string> names;
      v << coeff_ring << names;
      me.id_ptr=me.find_by_key(me.repo_by_key(), typename masquerade_for::ring_key_type(names, coeff_ring.id_ptr));
      me.coeff_id_ptr = coeff_ring.id_ptr;
   }

   template <typename Visitor>
   static void visit_elements(const Serialized<masquerade_for>& me, Visitor& v)
   {
      v << me.get_coefficient_ring() << me.names();
   }
};

} // end namespace pm

namespace polymake {
   using pm::Ring;
}

#endif // POLYMAKE_RING_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
