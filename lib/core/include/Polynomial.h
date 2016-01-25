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

#ifndef POLYMAKE_POLYNOMIAL_H
#define POLYMAKE_POLYNOMIAL_H

#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Ring.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/SparseVector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TransformedContainer.h"
#include "polymake/numerical_functions.h"
#include "polymake/hash_map"
#include "polymake/list"
#include <cassert>

namespace pm {

template <typename Coefficient=Rational, typename Exponent=int>
class Term;

template <typename Coefficient=Rational, typename Exponent=int>
class UniTerm;

template <typename Coefficient=Rational, typename Exponent=int>
class Polynomial;

template <typename Coefficient=Rational, typename Exponent=int>
class UniPolynomial;

template <typename Coefficient=Rational, typename Exponent=int>
class RationalFunction;

template <typename Coefficient, typename Exponent>
struct matching_ring< Polynomial<Coefficient, Exponent> > : True
{
   typedef Ring<Coefficient, Exponent> type;
};

template <typename Coefficient, typename Exponent>
struct matching_ring< UniPolynomial<Coefficient, Exponent> > : True
{
   typedef Ring<Coefficient, Exponent> type;
};

template <typename Coefficient, typename Exponent>
struct matching_ring< RationalFunction<Coefficient, Exponent> > : True
{
   typedef Ring<Coefficient, Exponent> type;
};

template <typename Exponent=int>
class cmp_monomial_ordered_base {
public:
   // for multi-variate polynomials
   cmp_value operator()(const SparseVector<Exponent>& m1, const SparseVector<Exponent>& m2) const
   {
      return compare_values(m1, m2, unit_matrix<Exponent>(m1.dim()));
   }

   // for univariate polynomials
   cmp_value operator()(const Exponent& exp1, const Exponent& exp2) const
   {
      return operations::cmp()(exp1, exp2);
   }

   // for multi-variate polynomials
   template <typename _Matrix>
   cmp_value compare_values(const SparseVector<Exponent>& m1, const SparseVector<Exponent>& m2, const GenericMatrix<_Matrix>& order) const
   {
      cmp_value v(operations::cmp()(order * m1, order * m2));
      if (v != cmp_eq)
         return v;
      else
         return operations::cmp()(m1, m2);
   }

   // for univariate polynomials
   cmp_value compare_values(const Exponent& exp1, const Exponent& exp2, const Exponent& reverse) const
   {
      return operations::cmp()(reverse * exp1, reverse * exp2);
   }
};


template <typename Order, typename order_type_tag=typename object_traits<Order>::generic_tag>
class cmp_monomial_ordered
   : public cmp_monomial_ordered_base<Order>
{
public:
   typedef Order exponent_type;

   explicit cmp_monomial_ordered(const exponent_type& order_arg)
      : order(order_arg) {}

   cmp_value operator()(const exponent_type& exp1, const exponent_type& exp2) const
   {
      return this->compare_values(exp1, exp2, order);
   }

private:
   const exponent_type order;
};


template <typename Order>
class cmp_monomial_ordered<Order, is_matrix>
   : public cmp_monomial_ordered_base<typename Order::element_type>
{
public:
   explicit cmp_monomial_ordered(const Order& order_arg)
      : order(order_arg) {}

   cmp_value operator()(const SparseVector<typename Order::element_type>& m1, const SparseVector<typename Order::element_type>& m2) const
   {
      return this->compare_values(m1, m2, order);
   }

private:
   const Order& order;
};

// forward declaration in Ring.h

template <typename Coefficient, typename Exponent>
class Monomial {
public:
   typedef Ring<Coefficient, Exponent> ring_type;
   typedef Coefficient coefficient_type;
   typedef Exponent exponent_type;
   typedef SparseVector<Exponent> value_type;

protected:
   value_type the_monom;
   ring_type ring;

public:
   // undefined object - to be read later from perl::Value
   Monomial() {}

   explicit Monomial(const ring_type& r)
      : the_monom(r.n_vars())
      , ring(r) {}

   template <typename Vector>
   Monomial(const GenericVector<Vector, Exponent>& m, const ring_type& r)
      : the_monom(m)
      , ring(r)
   {
      if (POLYMAKE_DEBUG || !Unwary<Vector>::value) {
         if (m.dim() != ring.n_vars())
            throw std::runtime_error("Monomial constructor - dimension mismatch");
      }
   }

   int dim() const { return the_monom.dim(); }
   const value_type& get_value() const { return the_monom; }
   const ring_type& get_ring() const { return ring; }

   void clear()
   {
      // can't say the_monom.clear() here as it would reset the dimension to 0.
      the_monom.fill(Exponent(0));
   }

   void swap(Monomial& m)
   {
      ring.swap(m.ring);
      the_monom.swap(m.the_monom);
   }

   static value_type default_value(const ring_type& r)
   {
      return value_type(r.n_vars());
   }

   static value_type empty_value(const ring_type& r)
   {
      return same_element_vector(std::numeric_limits<Exponent>::min(),r.n_vars());
   }

   static bool equals_to_default(const value_type& x)
   {
      return x.empty();
   }

   template <typename Comparator> static
   cmp_value compare_values(const value_type& x1, const value_type& x2, const Comparator& ordered_cmp)
   {
      return ordered_cmp(x1, x2);
   }

   cmp_value compare(const Monomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("Monomials of different rings");
      return compare_values(the_monom, m.the_monom, cmp_monomial_ordered_base<Exponent>());
   }

   template <typename Matrix>
   cmp_value compare(const Monomial& m, const GenericMatrix<Matrix, Exponent>& order) const
   {
      if (ring != m.ring) throw std::runtime_error("Monomials of different rings");
      return compare_values(the_monom, m.the_monom, cmp_monomial_ordered<Matrix>(order.top()));
   }

   Exponent operator[] (const int i) const { return the_monom[i]; }

   Monomial operator* (const Monomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("Monomials of different rings");
      return Monomial(the_monom+m.the_monom, ring);
   }

   Monomial& operator*= (const Monomial& m)
   {
      if (ring != m.ring) throw std::runtime_error("Monomials of different rings");
      the_monom += m.the_monom;
      return *this;
   }

   Monomial operator^ (typename function_argument<Exponent>::type exp) const
   {
      return Monomial(exp*the_monom, ring);
   }

   Monomial& operator^= (typename function_argument<Exponent>::type exp)
   {
      the_monom*=exp;
      return *this;
   }

   bool operator== (const Monomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("Monomials of different rings");
      return the_monom == m.the_monom;
   }
   bool operator!= (const Monomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("Monomials of different rings");
      return the_monom != m.the_monom;
   }

   bool operator< (const Monomial& m) const
   {
      return operations::cmp()(*this,m)==cmp_lt;
   }
   bool operator<= (const Monomial& m) const
   {
      return operations::cmp()(*this,m)!=cmp_gt;
   }
   bool operator> (const Monomial& m) const
   {
      return operations::cmp()(*this,m)==cmp_gt;
   }
   bool operator>= (const Monomial& m) const
   {
      return operations::cmp()(*this,m)!=cmp_lt;
   }

   template <typename Output> static
   void pretty_print(GenericOutput<Output>& out, const value_type& m, const ring_type& r)
   {
      if (m.empty()) {
         out.top() << one_value<Coefficient>(); // constant monomial
         return;
      }

      bool first = true;
      for (typename value_type::const_iterator it=m.begin(); !it.at_end(); ++it) {
         if (first)
            first = false;
         else
            out.top() << '*';
         out.top() << r.names()[it.index()];
         if (!is_one(*it)) out.top() << '^' << *it;
      }
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const Monomial& me)
   {
      pretty_print(out, me.the_monom, me.ring);
      return out.top();
   }

   template <typename> friend struct spec_object_traits;
};


template <typename Coefficient, typename Exponent>
class UniMonomial {
public:
   typedef Ring<Coefficient, Exponent> ring_type;
   typedef Coefficient coefficient_type;
   typedef Exponent exponent_type;
   typedef Exponent value_type;

   static ring_type default_ring() { return ring_type(1); }

protected:
   value_type the_monom;
   ring_type ring;

public:
   UniMonomial() :
      the_monom(),
      ring(default_ring()) {}

   explicit UniMonomial(const Exponent& exp) :
      the_monom(exp),
      ring(default_ring()) {}

   explicit UniMonomial(const ring_type& r) :
      the_monom(),
      ring(r)
   {
      if (r.n_vars() != 1) throw std::runtime_error("UniMonomial constructor - invalid ring");
   }

   UniMonomial(const Exponent& exp, const ring_type& r) :
      the_monom(exp),
      ring(r)
   {
      if (r.n_vars() != 1) throw std::runtime_error("UniMonomial constructor - invalid ring");
   }

   int dim() const { return 1; }
   const value_type& get_value() const { return the_monom; }
   const ring_type& get_ring() const { return ring; }

   void clear()
   {
      the_monom=default_value();
   }

   void swap(UniMonomial& m)
   {
      ring.swap(m.ring);
      the_monom.swap(m.the_monom);
   }

   static value_type default_value(const ring_type&)
   {
      return zero_value<Exponent>();
   }

   static value_type empty_value(const ring_type&)
   {
      return std::numeric_limits<Exponent>::min();
   }

   static bool equals_to_default(const value_type& x)
   {
      return is_zero(x);
   }

   template <typename Comparator>
   static cmp_value compare_values(const value_type& x1, const value_type& x2, const Comparator& cmp)
   {
      return cmp(x1, x2);
   }

   cmp_value compare(const UniMonomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return compare_values(the_monom, m.the_monom, cmp_monomial_ordered_base<Exponent>());
   }

   cmp_value compare(const UniMonomial& m, const Exponent& order) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return compare_values(the_monom, m.the_monom, cmp_monomial_ordered<Exponent>(order));
   }

   UniMonomial operator* (const UniMonomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return UniMonomial(the_monom+m.the_monom, ring);
   }

   UniMonomial& operator*= (const UniMonomial& m)
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      the_monom += m.the_monom;
      return *this;
   }

   UniMonomial operator^ (typename function_argument<Exponent>::type exp) const
   {
      return UniMonomial(exp*the_monom, ring);
   }

   UniMonomial& operator^= (typename function_argument<Exponent>::type exp)
   {
      the_monom*=exp;
      return *this;
   }

   bool operator== (const UniMonomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return the_monom == m.the_monom;
   }
   bool operator!= (const UniMonomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return the_monom != m.the_monom;
   }
   bool operator< (const UniMonomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return the_monom < m.the_monom;
   }
   bool operator<= (const UniMonomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return the_monom <= m.the_monom;
   }
   bool operator> (const UniMonomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return the_monom > m.the_monom;
   }
   bool operator>= (const UniMonomial& m) const
   {
      if (ring != m.ring) throw std::runtime_error("UniMonomials of different rings");
      return the_monom >= m.the_monom;
   }

   template <typename Output> static
   void pretty_print(GenericOutput<Output>& out, const value_type& m, const ring_type& r)
   {
      if (equals_to_default(m)) {
         out.top() << one_value<Coefficient>();  // constant monomial
         return;
      }
      out.top() << r.names().front();
      if (!is_one(m)) out.top() << '^' << m;
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const UniMonomial& me)
   {
      pretty_print(out, me.the_monom, me.ring);
      return out.top();
   }

   template <typename> friend struct spec_object_traits;
   friend class RationalFunction<Coefficient, Exponent>;
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< Monomial<Coefficient, Exponent> > > :
   spec_object_traits<is_composite> {

   typedef Monomial<Coefficient,Exponent> masquerade_for;

   typedef cons<typename Monomial<Coefficient, Exponent>::value_type,
                typename Monomial<Coefficient, Exponent>::ring_type> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me.the_monom << me.ring;
   }
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< UniMonomial<Coefficient, Exponent> > > :
   spec_object_traits<is_composite> {

   typedef UniMonomial<Coefficient,Exponent> masquerade_for;

   typedef cons<typename UniMonomial<Coefficient, Exponent>::value_type,
                typename UniMonomial<Coefficient, Exponent>::ring_type> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me.the_monom << me.ring;
   }
};

template <typename Coefficient, typename Exponent>
class Ring_impl<Coefficient, Exponent>::Variables :
   public modified_container_impl<Variables,
                                  list( Container< sequence >,
                                        Operation< Monomial_constructor > ) > {
public:
   Variables(const Ring_impl& r) : ring(r) {}

   sequence get_container() const { return sequence(0, ring.n_vars()); }
   Monomial_constructor get_operation() const { return ring; }
private:
   Ring_impl ring;
};

template <typename Coefficient, typename Exponent> inline
typename Ring_impl<Coefficient, Exponent>::Variables
Ring_impl<Coefficient, Exponent>::variables() const
{
   return *this;
}

template <typename Coefficient, typename Exponent> inline
Monomial<Coefficient, Exponent>
Ring_impl<Coefficient, Exponent>::Monomial_constructor::operator() (int i) const
{
   if (POLYMAKE_DEBUG) {
      if (i<0 || i >= ring.n_vars())
         throw std::runtime_error("Ring::variables - index out of range");
   }
   return result_type(unit_vector<Exponent>(ring.n_vars(), i), Ring<Coefficient,Exponent>(ring));
}

template <typename Coefficient, typename Exponent> inline
UniMonomial<Coefficient, Exponent>
Ring_impl<Coefficient, Exponent>::variable() const
{
   assert(n_vars()==1);
   return UniMonomial<Coefficient, Exponent>(1, Ring<Coefficient, Exponent>(*this));
}

// how to reconstruct the type of the Term or Polynomial from its Monomial type

template <typename Monomial> struct Term_result {};
template <typename Monomial> struct Polynomial_result {};

template <typename Coefficient, typename Exponent>
struct Term_result< Monomial<Coefficient, Exponent> > {
   typedef Term<Coefficient, Exponent> type;
};

template <typename Coefficient, typename Exponent>
struct Term_result< UniMonomial<Coefficient, Exponent> > {
   typedef UniTerm<Coefficient, Exponent> type;
};

template <typename Coefficient, typename Exponent>
struct Polynomial_result< Monomial<Coefficient, Exponent> > {
   typedef Polynomial<Coefficient, Exponent> type;
};

template <typename Coefficient, typename Exponent>
struct Polynomial_result< UniMonomial<Coefficient, Exponent> > {
   typedef UniPolynomial<Coefficient, Exponent> type;
};

// how to convert something to a coefficient of a Term or Polynomial

// this catches the SFINAE cases, that is, when Result does not have an inner coefficient_type at all
template <typename T, typename Result, typename enabler=void>
struct deeper_coefficient_of : False {};

// if T can be directly converted into Result's coefficient, it will be handled by the corresponding constructor
template <typename T, typename Result>
struct deeper_coefficient_of<T, Result, typename enable_if<void, convertible_to<T, typename Result::coefficient_type>::value>::type> : False {};

// T can be converted to Result's coefficient's coefficient
template <typename T, typename Result>
struct deeper_coefficient_of<T, Result, typename enable_if<void, convertible_to<T, typename Result::coefficient_type::coefficient_type>::value>::type> : True
{
   typedef typename Result::coefficient_type result_type;

   static
   result_type construct(typename function_argument<T>::type x, const typename Result::ring_type& ring)
   {
      return result_type(x, ring.get_coefficient_ring());
   }
};

// T can be converted to some deeper coefficient in the nesting hierarchy
template <typename T, typename Result>
struct deeper_coefficient_of<T, Result, typename enable_if<void, (!convertible_to<T, typename Result::coefficient_type::coefficient_type>::value &&
                                                                  deeper_coefficient_of<T, typename Result::coefficient_type>::value)>::type> : True
{
   typedef deeper_coefficient_of<T, typename Result::coefficient_type> deeper;
   typedef typename Result::coefficient_type result_type;

   static
   result_type construct(typename function_argument<T>::type x, const typename Result::ring_type& ring)
   {
      return result_type(deeper::construct(x, ring.get_coefficient_ring()), ring.get_coefficient_ring());
   }
};

template <typename T, typename TermOrPolynomial, bool answer=(convertible_to<T, typename TermOrPolynomial::coefficient_type>::value ||
                                                              deeper_coefficient_of<T, TermOrPolynomial>::value)>
struct fits_as_coefficient : True {};

template <typename T, typename TermOrPolynomial>
struct fits_as_coefficient<T, TermOrPolynomial, false> : False {};

template <typename Monomial>
class Term_base {
public:
   typedef Monomial monomial_type;
   typedef typename monomial_type::ring_type ring_type;
   typedef typename monomial_type::coefficient_type coefficient_type;
   typedef typename monomial_type::exponent_type exponent_type;
   typedef std::pair<typename monomial_type::value_type, coefficient_type> value_type;
   typedef typename Term_result<Monomial>::type term_result;

   Term_base() {}

   explicit Term_base(const ring_type& r) :
      ring(r) {}

   Term_base(const coefficient_type& c, const ring_type& r) :
      the_term(monomial_type::default_value(r), c),
      ring(r) {}

   template <typename T>
   Term_base(const T& c, const typename enable_if<ring_type, deeper_coefficient_of<T, Term_base>::value>::type& r) :
      the_term(monomial_type::default_value(r), deeper_coefficient_of<T, Term_base>::construct(c, r)),
      ring(r) {}

   Term_base(const typename monomial_type::value_type& m, const coefficient_type& c, const ring_type& r) :
      the_term(m, c),
      ring(r) {}

   template <typename T>
   Term_base(const typename monomial_type::value_type& m, const T& c, const typename enable_if<ring_type, deeper_coefficient_of<T, Term_base>::value>::type& r) :
      the_term(m, deeper_coefficient_of<T, Term_base>::construct(c, r)),
      ring(r) {}

   void swap(Term_base& t)
   {
      ring.swap(t.ring);
      the_term.swap(t.the_term);
   }

   const value_type& get_value() const { return the_term; }
   const ring_type& get_ring() const { return ring; }

   monomial_type get_monomial() const { return monomial_type(the_term.first, ring); }
   const coefficient_type& get_coefficient() const { return the_term.second; }

   term_result& top() { return static_cast<term_result&>(*this); }
   const term_result& top() const { return static_cast<const term_result&>(*this); }

   template <typename T>
   typename enable_if<term_result, fits_as_coefficient<T, Term_base>::value>::type
   operator* (const T& c) const
   {
      return term_result(value_type(the_term.first, the_term.second * c), ring);
   }

   template <typename T> friend
   typename enable_if<term_result, fits_as_coefficient<T, Term_base>::value>::type
   operator* (const T& c, const Term_base& t)
   {
      return term_result(value_type(t.the_term.first, c * t.the_term.second), t.ring);
   }

   template <typename T>
   typename enable_if<term_result, fits_as_coefficient<T, Term_base>::value>::type&
   operator*= (const T& c)
   {
      the_term.second *= c;
      return top();
   }

   template <typename T>
   typename enable_if<term_result, fits_as_coefficient<T, Term_base>::value>::type
   operator/ (const T& c) const
   {
      if (is_zero(c)) throw GMP::ZeroDivide();
      return term_result(value_type(the_term.first, the_term.second / c), ring);
   }

   template <typename T>
   typename enable_if<term_result, fits_as_coefficient<T, Term_base>::value>::type&
   operator/= (const T& c)
   {
      if (is_zero(c)) throw GMP::ZeroDivide();
      the_term.second /= c;
      return top();
   }

   term_result operator* (const monomial_type& m) const
   {
      if (ring != m.get_ring()) throw std::runtime_error("Terms of different rings");
      return term_result(value_type(the_term.first + m.get_value(), the_term.second), ring);
   }

   friend
   term_result operator* (const monomial_type& m, const Term_base& t)
   {
      return t*m;
   }

   term_result& operator*= (const monomial_type& m)
   {
      if (ring != m.get_ring()) throw std::runtime_error("Terms of different rings");
      the_term.first += m.get_value();
      return top();
   }

   term_result operator* (const Term_base& t) const
   {
      if (ring != t.ring) throw std::runtime_error("Terms of different rings");
      return term_result(value_type(the_term.first + t.the_term.first, the_term.second * t.the_term.second), ring);
   }

   term_result& operator*= (const Term_base& t)
   {
      if (ring != t.ring) throw std::runtime_error("Terms of different rings");
      the_term.first += t.the_term.first;
      the_term.second *= t.the_term.second;
      return top();
   }

   term_result operator- () const
   {
      return term_result(value_type(the_term.first, -the_term.second), ring);
   }

   term_result& negate()
   {
      operations::neg<coefficient_type> neg;
      neg.assign(the_term.second);
      return top();
   }

   bool operator== (const Term_base& x2) const
   {
      if (ring != x2.ring) throw std::runtime_error("Terms of different rings");
      return the_term==x2.the_term;
   }

   bool operator!= (const Term_base& x2) const
   {
      return !operator==(x2);
   }

   bool operator== (const monomial_type& x2) const
   {
      if (ring != x2.get_ring()) throw std::runtime_error("Terms of different rings");
      return the_term.first==x2.get_value() && is_one(the_term.second);
   }

   bool operator!= (const monomial_type& x2)
   {
     return !operator==(x2);
   }

   friend
   bool operator== (const monomial_type& x1, const Term_base& x2)
   {
      return x2==x1;
   }

   friend
   bool operator!= (const monomial_type& x1, const Term_base& x2)
   {
      return !(x2==x1);
   }

   template <typename T>
   typename enable_if<bool, fits_as_coefficient<T, Term_base>::value>::type
   operator== (const T& x2) const
   {
      return is_zero(x2) && is_zero(the_term.second) || monomial_type::equals_to_default(the_term.first) && the_term.second==x2;
   }

   template <typename T>
   typename enable_if<bool, fits_as_coefficient<T, Term_base>::value>::type
   operator!= (const T& x2) const
   {
     return !operator==(x2);
   }

   template <typename T> friend
   typename enable_if<bool, fits_as_coefficient<T, Term_base>::value>::type
   operator== (const T& x1, const Term_base& x2)
   {
      return x2==x1;
   }

   template <typename T> friend
   typename enable_if<bool, fits_as_coefficient<T, Term_base>::value>::type
   operator!= (const T& x1, const Term_base& x2)
   {
      return !(x2==x1);
   }

   //! Compare the monomials according to the given order,
   //! use coefficients as tie breaker.
   template <typename Comparator> static
   cmp_value compare_values(const value_type& t1, const value_type& t2, const Comparator& cmp_order)
   {
      const cmp_value cmp_monom=monomial_type::compare_values(t1.first, t2.first, cmp_order);
      return cmp_monom != cmp_eq ? cmp_monom : operations::cmp()(t1.second, t2.second);
   }

protected:
   template <typename Comparator>
   cmp_value compare_ordered(const Term_base& x, const Comparator& cmp) const
   {
      if (ring != x.ring) throw std::runtime_error("Terms of different rings");
      return compare_values(the_term, x.the_term, cmp);
   }

public:
   cmp_value compare(const Term_base& x) const
   {
      return compare_ordered(x, cmp_monomial_ordered_base<exponent_type>());
   }

   template <typename Matrix>
   cmp_value compare(const Term_base& x, const GenericMatrix<Matrix, exponent_type>& order) const
   {
      return compare_ordered(x, cmp_monomial_ordered<Matrix>(order.top()));
   }

   bool operator< (const Term_base<monomial_type>& x2) const
   {
      return compare(x2) == cmp_lt;
   }

   bool operator> (const Term_base<monomial_type>& x2) const
   {
     return compare(x2) == cmp_gt;
   }

   bool operator<= (const Term_base<monomial_type>& x2) const
   {
     return compare(x2) != cmp_gt;
   }

   bool operator>= (const Term_base<monomial_type>& x2)
   {
     return compare(x2) != cmp_lt;
   }

   template <typename Output> static
   void pretty_print(GenericOutput<Output>& out,
                     const typename monomial_type::value_type& m,
                     const coefficient_type& c,
                     const ring_type& r)
   {
      if (!is_one(c)) {
         if (!(is_field<coefficient_type>::value && is_one(-c))){
            if (!identical<typename object_traits<coefficient_type>::model, is_scalar>::value)
               out.top() << '(';
            out.top() << c;
            if (!identical<typename object_traits<coefficient_type>::model, is_scalar>::value)
               out.top() << ')';
            if (monomial_type::equals_to_default(m)) return;
            out.top() << '*';
         } else {
            out.top() << "- ";
         }
      }
      monomial_type::pretty_print(out, m, r);
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const Term_base& me)
   {
      pretty_print(out, me.the_term.first, me.the_term.second, me.ring);
      return out.top();
   }

   static bool needs_plus(const coefficient_type& c)
   {
      // in particular gives false for Polynomial and TropicalNumber
      return needs_plus(c, is_field<coefficient_type>() );
   }

protected:
   static bool needs_plus(const coefficient_type& c, True) { return c>=zero_value<coefficient_type>(); }
   static bool needs_plus(const coefficient_type&, False) { return true; }

   value_type the_term;
   ring_type ring;
};

template <typename Coefficient, typename Exponent>
class Term : public Term_base< Monomial<Coefficient, Exponent> > {
public:
   typedef Monomial<Coefficient, Exponent> monomial_type;
private:
   typedef Term_base<monomial_type> super;
public:
   Term() {}

   explicit Term(const typename super::ring_type& r) :
      super(r) {}

   template <typename T>
   Term(const T& c, const typename super::ring_type& r,
        typename enable_if<void**, fits_as_coefficient<T, Term>::value>::type=0) :
      super(c,r) {}

   Term(const monomial_type& m) :
      super(m.get_value(), m.get_ring().one_coef(), m.get_ring()) {}

   template <typename T>
   Term(const monomial_type& m, const T& c,
        typename enable_if<void**, fits_as_coefficient<T, Term>::value>::type=0) :
      super(m.get_value(), c, m.get_ring()) {}

   Term(const std::pair<const typename monomial_type::value_type, Coefficient>& p, const typename super::ring_type& r) :
      super(p.first, p.second, r) {}

   template <typename Vector, typename T>
   Term(const GenericVector<Vector,Exponent>& m, const T& c, const typename super::ring_type& r,
        typename enable_if<void**, fits_as_coefficient<T, Term>::value>::type=0) :
      super(m, c, r) {}

   template <typename> friend struct spec_object_traits;
};

template <typename Coefficient, typename Exponent>
class UniTerm : public Term_base< UniMonomial<Coefficient, Exponent> > {
public:
   typedef UniMonomial<Coefficient, Exponent> monomial_type;
private:
   typedef Term_base<monomial_type> super;
public:
   UniTerm() : super(monomial_type::default_ring()) {}

   explicit UniTerm(const typename super::ring_type& r) :
      super(r)
   {
      if (r.n_vars() != 1) throw std::runtime_error("UniTerm constructor - invalid ring");
   }

   template <typename T>
   explicit UniTerm(const T& c,
                    typename enable_if<void**, fits_as_coefficient<T, UniTerm>::value>::type=0) :
      super(c, monomial_type::default_ring()) {}

   template <typename T>
   UniTerm(const T& c, const typename super::ring_type& r,
           typename enable_if<void**, fits_as_coefficient<T, UniTerm>::value>::type=0) :
      super(c,r)
   {
      if (r.n_vars() != 1) throw std::runtime_error("UniTerm constructor - invalid ring");
   }

   UniTerm(const monomial_type& m) :
      super(m.get_value(), m.get_ring().one_coef(), m.get_ring()) {}

   template <typename T>
   UniTerm(const monomial_type& m, const T& c,
           typename enable_if<void**, fits_as_coefficient<T, UniTerm>::value>::type=0) :
      super(m.get_value(), c, m.get_ring()) {}

   UniTerm(const std::pair<const typename monomial_type::value_type, Coefficient>& p, const typename super::ring_type& r) :
      super(p.first, p.second, r) {}

   template <typename> friend struct spec_object_traits;
};


template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< Term<Coefficient,Exponent> > > :
   spec_object_traits<is_composite> {

   typedef Term<Coefficient,Exponent> masquerade_for;

   typedef cons<typename Term<Coefficient,Exponent>::value_type,
                typename Term<Coefficient,Exponent>::ring_type> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me.the_term << me.ring;
   }
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< UniTerm<Coefficient,Exponent> > > :
   spec_object_traits<is_composite> {

   typedef UniTerm<Coefficient,Exponent> masquerade_for;

   typedef cons<typename UniTerm<Coefficient,Exponent>::value_type,
                typename UniTerm<Coefficient,Exponent>::ring_type> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me.the_term << me.ring;
   }
};


template <typename monomial_type, typename T> inline
typename enable_if<typename Term_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator* (const T& c, const monomial_type& m)
{
   return typename Term_result<monomial_type>::type(m, c);
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Term_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator* (const monomial_type& m, const T& c)
{
   return typename Term_result<monomial_type>::type(m, c);
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Term_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator/ (const monomial_type& m, const T& c)
{
   if (is_zero(c)) throw GMP::ZeroDivide();
   return typename Term_result<monomial_type>::type(m, m.get_ring().one_coef() / c);
}

template <typename monomial_type> inline
typename Term_result<monomial_type>::type
operator- (const monomial_type& m)
{
   return typename Term_result<monomial_type>::type(m, -m.get_ring().one_coef());
}

template <typename Monomial>
class Polynomial_base {
public:
   typedef Monomial monomial_type;
   typedef typename monomial_type::ring_type ring_type;
   typedef typename monomial_type::coefficient_type coefficient_type;
   typedef typename monomial_type::exponent_type exponent_type;
   typedef typename Term_result<monomial_type>::type term_type;
   typedef typename Polynomial_result<monomial_type>::type polynomial_result;
   typedef hash_map<typename monomial_type::value_type, coefficient_type> term_hash;
   typedef std::list<typename monomial_type::value_type> sorted_terms_type; // use forward_list when using c++11

protected:
   struct impl {
      term_hash the_terms;
      ring_type ring;

      // terms ordered by lex termorder 
      mutable sorted_terms_type the_sorted_terms;
      // true if sorted_terms has a valid value
      mutable bool the_sorted_terms_set;

      impl() : the_sorted_terms(), the_sorted_terms_set(false) {}

      explicit impl(const ring_type& r) : ring(r), the_sorted_terms(), the_sorted_terms_set(false) {}

      impl(const ring_type& r, const sorted_terms_type& sorted_terms) :
         ring(r), the_sorted_terms(sorted_terms), the_sorted_terms_set(true) {}

      void forget_sorted_terms()
      {
         if (the_sorted_terms_set) {
            the_sorted_terms.clear();
            the_sorted_terms_set=false;
         }
      }

      void clear()
      {
         forget_sorted_terms();
         the_terms.clear();
      }
   };

   struct shared_clear {
      void operator() (void* where, const impl& d) const { new(where) impl(d.ring); }
      void operator() (impl& d) const { d.clear(); }
   };

   shared_object<impl> data;

   // enforced copy
   explicit Polynomial_base(const impl& src) :
      data(src) {}

public:
   Polynomial_base() {}

   // the default polynomial in a given ring
   explicit Polynomial_base(const ring_type& r) :
      data(make_constructor(r, (impl*)0)) {}

   Polynomial_base(const coefficient_type& c, const ring_type& r) :
      data(make_constructor(r, (impl*)0))
   {
      if (__builtin_expect(!is_zero(c), 1)) {
         data.get()->the_terms.insert(monomial_type::default_value(r), c);
      }
   }

   template <typename T>
   Polynomial_base(const T& c, const ring_type& r,
                   typename enable_if<void**, deeper_coefficient_of<T, Polynomial_base>::value>::type=0) :
      data(make_constructor(r, (impl*)0))
   {
      if (__builtin_expect(!is_zero(c), 1)) {
         data.get()->the_terms.insert(monomial_type::default_value(r), deeper_coefficient_of<T, Polynomial_base>::construct(c, r));
      }
   }

   Polynomial_base(const monomial_type& m) :
      data(make_constructor(m.get_ring(), (impl*)0))
   {
      data.get()->the_terms.insert(m.get_value(), m.get_ring().one_coef());
   }

   Polynomial_base(const Term_base<monomial_type>& t) :
      data(make_constructor(t.get_ring(), (impl*)0))
   {
      if (__builtin_expect(!is_zero(t.get_coefficient()), 1)) {
         data.get()->the_terms.insert(t.get_value().first, t.get_coefficient());
      }
   }

#if POLYMAKE_DEBUG
   ~Polynomial_base() { POLYMAKE_DEBUG_METHOD(Polynomial_base,dump); }
   void dump() const { cerr << *this << std::flush; }
#endif

   void swap(Polynomial_base& p)
   {
      data.swap(p.data);
   }

   /// set this equal to a zero polynomial (in the same ring)
   void clear()
   {
      data.apply(shared_clear());
   }

   polynomial_result& top() { return static_cast<polynomial_result&>(*this); }
   const polynomial_result& top() const { return static_cast<const polynomial_result&>(*this); }

   int n_vars() const { return data->ring.n_vars(); }

   const ring_type& get_ring() const { return data->ring; }

   int n_terms() const { return data->the_terms.size(); }

   const term_hash& get_terms() const { return data->the_terms; }

   bool trivial() const { return data->the_terms.empty(); }

   bool unit() const
   {
      return data->the_terms.size()==1
          && monomial_type::equals_to_default(data->the_terms.begin()->first)
          && is_one(data->the_terms.begin()->second);
   }

   // returns the coefficient of the monomial m or 0 iff m does not exists
   const coefficient_type& get_coefficient(const monomial_type& m) const
   {
      if (m.get_ring() != get_ring()) throw std::runtime_error("Polynomial - Monomial of different ring");
      return get_coefficient(m.get_value());
   }

   const coefficient_type& get_coefficient(const typename monomial_type::value_type& m) const
   {
      typename term_hash::const_iterator find = data->the_terms.find(m);
      if (find != data->the_terms.end())
         return find->second;
      return get_ring().zero_coef();
   }

   bool exists(const monomial_type& m) const
   {
      if (m.get_ring() != get_ring()) throw std::runtime_error("Polynomial - Monomial of different ring");
      return exists(m.get_value());
   }

   bool exists(const typename monomial_type::value_type& m) const
   {
      return data->the_terms.exists(m);
   }

   Vector<coefficient_type> coefficients_as_vector() const
   {
      return Vector<coefficient_type>(n_terms(),
                                      attach_operation(data->the_terms, BuildUnary<operations::take_second>()).begin());
   }

   // leading term
   term_type lt() const
   {
      if (trivial())
         return term_type(this->lm(), data->ring.zero_coef());
      else
         return term_type(*find_lex_lm(), data->ring);
   }

   template <typename Matrix> inline
   term_type lt(const GenericMatrix<Matrix, exponent_type>& order) const
   {
      if (trivial())
         return term_type(this->lm(order), data->ring.zero_coef());
      else
         return term_type(*find_lm(cmp_monomial_ordered<Matrix>(order.top())), data->ring);
   }

   //! Return the leading monomial.
   monomial_type lm() const
   {
      if (trivial())
         return monomial_type(monomial_type::empty_value(data->ring), data->ring);
      else
         return monomial_type(find_lex_lm()->first, data->ring);
   }

   template <typename Matrix>
   monomial_type lm(const GenericMatrix<Matrix, exponent_type>& order) const
   {
      if (trivial())
         return monomial_type(monomial_type::empty_value(data->ring),data->ring);
      else
         return monomial_type(find_lm(cmp_monomial_ordered<Matrix>(order.top()))->first, data->ring);
   }

   //! Return the leading monomial's exponents.
   typename monomial_type::value_type lm_exp() const
   {
      if (trivial())
         return monomial_type::empty_value(data->ring);
      else
         return find_lex_lm()->first;
   }

   template <typename Matrix>
   typename monomial_type::value_type lm_exp(const GenericMatrix<Matrix, exponent_type>& order) const
   {
      if (trivial())
         return monomial_type::empty_value(data->ring);
      else
         return find_lm(cmp_monomial_ordered<Matrix>(order.top()))->first;
   }

   //! Return the leading coefficient.
   const coefficient_type& lc() const
   {
      if (trivial())
         return data->ring.zero_coef();
      else
         return find_lex_lm()->second;
   }

   template <typename Matrix>
   const coefficient_type& lc(const GenericMatrix<Matrix, exponent_type>& order) const
   {
      if (trivial())
         return data->ring.zero_coef();
      else
         return find_lm(cmp_monomial_ordered<Matrix>(order.top()))->second;
   }

   template <typename T>
   typename enable_if<polynomial_result, fits_as_coefficient<T, Polynomial_base>::value>::type
   operator* (const T& c) const
   {
      if (__builtin_expect(is_zero(c), 0))
         return polynomial_result(get_ring());
      Polynomial_base prod(*data);
      return prod *= c;
   }

   template <typename T>
   typename enable_if<polynomial_result, fits_as_coefficient<T, Polynomial_base>::value>::type
   mult_from_right(const T& c) const
   {
      if (__builtin_expect(is_zero(c), 0))
         return polynomial_result(get_ring());
      Polynomial_base prod(*data);
      for (typename Entire<term_hash>::iterator it=entire(prod.data->the_terms); !it.at_end(); ++it)
         it->second = c * it->second;
      return prod.top();
   }

   template <typename T>
   typename enable_if<polynomial_result, fits_as_coefficient<T, Polynomial_base>::value>::type&
   operator*= (const T& c)
   {
      if (__builtin_expect(is_zero(c), 0)) {
         clear();
      } else {
         for (typename Entire<term_hash>::iterator it=entire(data->the_terms); !it.at_end(); ++it)
            it->second *= c;
      }
      return top();
   }

   template <typename T>
   typename enable_if<polynomial_result, fits_as_coefficient<T, Polynomial_base>::value>::type
   operator/ (const T& c) const
   {
      if (__builtin_expect(is_zero(c), 0)) throw GMP::ZeroDivide();
      Polynomial_base prod(*data);
      return prod /= c;
   }

   template <typename T>
   typename enable_if<polynomial_result, fits_as_coefficient<T, Polynomial_base>::value>::type&
   operator/= (const T& c)
   {
      if (__builtin_expect(is_zero(c), 0)) throw GMP::ZeroDivide();

      for (typename Entire<term_hash>::iterator it=entire(data->the_terms); !it.at_end(); ++it)
         it->second /= c;
      return top();
   }

   //! Divide by the coefficient of the leading monomial
   polynomial_result& normalize()
   {
      if (!trivial()) {
         const coefficient_type lead=lc();
         *this /= lead;
      }

      return top();
   }

   polynomial_result operator* (const monomial_type& m) const
   {
      if (get_ring() != m.get_ring()) throw std::runtime_error("Polynomials of different rings");

      polynomial_result prod(get_ring());
      for (typename Entire<term_hash>::const_iterator it=entire(data->the_terms); !it.at_end(); ++it)
         prod.add_term(it->first + m.get_value(), it->second, True(), True());

      return prod;
   }

   polynomial_result& operator*= (const monomial_type& m)
   {
      *this = (*this) * m;
      return top();
   }

   template <typename _from_right>
   polynomial_result mult(const Term_base<monomial_type>& t, _from_right) const
   {
      if (get_ring() != t.get_ring()) throw std::runtime_error("Polynomials of different rings");

      polynomial_result prod(get_ring());
      if (__builtin_expect(!is_zero(t.get_coefficient()), 1)) {
         for (typename Entire<term_hash>::const_iterator it=entire(data->the_terms); !it.at_end(); ++it)
            prod.add_term(it->first + t.get_value().first,
                          _from_right::value ? t.get_value().second * it->second
                                             : it->second * t.get_value().second,
                          True(), True());

      }
      return prod;
   }

   polynomial_result operator* (const term_type& t) const
   {
      return mult(t, False());
   }

   polynomial_result& operator*= (const term_type& t)
   {
      *this = (*this) * t;
      return top();
   }

   polynomial_result operator* (const Polynomial_base& p2) const
   {
      if (get_ring() != p2.get_ring()) throw std::runtime_error("Polynomials of different rings");

      polynomial_result prod(get_ring());
      for (typename Entire<term_hash>::const_iterator it1=entire(data->the_terms); !it1.at_end(); ++it1)
         for (typename Entire<term_hash>::const_iterator it2=entire(p2.data->the_terms); !it2.at_end(); ++it2)
            prod.add_term(it1->first + it2->first, it1->second * it2->second, True(), True());

      return prod;
   }

   polynomial_result& operator*= (const polynomial_result& p)
   {
      *this = (*this) * p;
      return top();
   }

   polynomial_result operator- () const
   {
      Polynomial_base result(*data);
      return result.negate();
   }

   polynomial_result& negate()
   {
      for (typename Entire<term_hash>::iterator it=entire(data->the_terms); !it.at_end(); ++it)
         pm::negate(it->second);
      return top();
   }

   template <typename T>
   typename enable_if<polynomial_result, fits_as_coefficient<T, Polynomial_base>::value>::type
   operator+ (const T& c) const
   {
      Polynomial_base sum(*data);
      return sum += c;
   }

   polynomial_result& operator+= (const coefficient_type& c)
   {
      if (__builtin_expect(!is_zero(c), 1))
         add_term(monomial_type::default_value(get_ring()), c, True(), True());
      return top();
   }

   template <typename T>
   typename enable_if<polynomial_result, deeper_coefficient_of<T, Polynomial_base>::value>::type&
   operator+= (const T& c)
   {
      return operator+= (deeper_coefficient_of<T, Polynomial_base>::construct(c, get_ring()));
   }

   template <typename T>
   typename enable_if<polynomial_result, fits_as_coefficient<T, Polynomial_base>::value>::type
   operator- (const T& c) const
   {
      Polynomial_base diff(*data);
      return diff -= c;
   }

   polynomial_result& operator-= (const coefficient_type& c)
   {
      if (__builtin_expect(!is_zero(c), 1))
         add_term(monomial_type::default_value(get_ring()), c, True(), False());
      return top();
   }

   template <typename T>
   typename enable_if<polynomial_result, deeper_coefficient_of<T, Polynomial_base>::value>::type&
   operator-= (const T& c)
   {
      return operator-= (deeper_coefficient_of<T, Polynomial_base>::construct(c, get_ring()));
   }

   polynomial_result operator+ (const monomial_type& m) const
   {
      Polynomial_base sum(*data);
      return sum+=m;
   }

   polynomial_result& operator+= (const monomial_type& m)
   {
      if (get_ring() != m.get_ring()) throw std::runtime_error("Polynomials of different rings");
      add_term(m.get_value(), m.get_ring().one_coef(), True(), True());
      return top();
   }

   polynomial_result operator- (const monomial_type& m) const
   {
      Polynomial_base diff(*data);
      return diff-=m;
   }

   polynomial_result& operator-= (const monomial_type& m)
   {
      if (get_ring() != m.get_ring()) throw std::runtime_error("Polynomials of different rings");
      add_term(m.get_value(), m.get_ring().one_coef(), True(), False());
      return top();
   }

   polynomial_result operator+ (const term_type& t) const
   {
      Polynomial_base sum(*data);
      return sum+=t;
   }

   polynomial_result& operator+= (const term_type& t)
   {
      if (get_ring() != t.get_ring()) throw std::runtime_error("Polynomials of different rings");
      add_term(t.get_value().first, t.get_value().second, False(), True());
      return top();
   }

   polynomial_result operator- (const term_type& t) const
   {
      Polynomial_base diff(*data);
      return diff-=t;
   }

   polynomial_result& operator-= (const term_type& t)
   {
      if (get_ring() != t.get_ring()) throw std::runtime_error("Polynomials of different rings");
      add_term(t.get_value().first, t.get_value().second, False(), False());
      return top();
   }

   polynomial_result operator+ (const Polynomial_base& p) const
   {
      Polynomial_base sum(*data);
      return sum+=p;
   }

   polynomial_result& operator+= (const Polynomial_base& p)
   {
      if (get_ring() != p.get_ring()) throw std::runtime_error("Polynomials of different rings");
      for (typename Entire<term_hash>::const_iterator t=entire(p.data->the_terms); !t.at_end(); ++t)
         add_term(t->first, t->second, True(), True());
      return top();
   }

   polynomial_result operator- (const Polynomial_base& p) const
   {
     Polynomial_base diff(*data);
      return diff-=p;
   }

   polynomial_result& operator-= (const Polynomial_base& p)
   {
      if (get_ring() != p.get_ring()) throw std::runtime_error("Polynomials of different rings");
      for (typename Entire<term_hash>::const_iterator t=entire(p.data->the_terms); !t.at_end(); ++t)
         add_term(t->first, t->second, True(), False());
      return top();
   }

   template <typename Output, typename Order>
   void pretty_print(GenericOutput<Output>& out, const Order& order) const
   {
      // this list will carry the sorted terms except in lex
      sorted_terms_type temp;
      const sorted_terms_type& sorted_terms = pm::identical<Order, cmp_monomial_ordered_base<exponent_type> >::value ? get_sorted_terms() : get_sorted_terms(temp, order);
      bool first = true;
      for (typename sorted_terms_type::const_iterator tp_it = sorted_terms.begin(), tp_end = sorted_terms.end(); tp_it != tp_end; ++tp_it) {
        typename term_hash::const_iterator term = data->the_terms.find(*tp_it); 
         if (first)
            first = false;
         else if (term_type::needs_plus(term->second))
            out.top() << " + ";
         else
            out.top() << ' ';

         term_type::pretty_print(out, term->first, term->second, get_ring());
      }
      if (first) out.top() << zero_value<coefficient_type>();
   }

protected:
   // replace by lambda function when using c++11
   template <typename Order>
   struct ordered_gt
   {
      typedef const typename monomial_type::value_type first_argument_type;
      typedef const typename monomial_type::value_type second_argument_type;
      typedef bool result_type;

      explicit ordered_gt(const Order& order_arg)
         : order(order_arg) {}

      template< typename argument_type >
      result_type operator() (argument_type t1, argument_type t2) const
      {
         return order(t1, t2) == cmp_gt;
      }
     private:
      const Order& order;
   };
   // returns a list containing the exponents ordered by lex
   const sorted_terms_type& get_sorted_terms() const
   {
      if(data->the_sorted_terms_set) return data->the_sorted_terms;
      for(typename term_hash::const_iterator it = data->the_terms.begin(); it != data->the_terms.end(); ++it) {
         data->the_sorted_terms.push_back(it->first);
      }
      data->the_sorted_terms.sort(ordered_gt< cmp_monomial_ordered_base<exponent_type> >(cmp_monomial_ordered_base<exponent_type>())); // TODO: check if this is sorted by lex
      data->the_sorted_terms_set = true;
      return data->the_sorted_terms;
   }

   // returns a list containing the exponents ordered by cmp_order 
   template<typename Order>
   const sorted_terms_type& get_sorted_terms(sorted_terms_type& sort, const Order& cmp_order) const
   {
      for(typename term_hash::const_iterator it = data->the_terms.begin(); it != data->the_terms.end(); ++it) {
         sort.push_back(it->first);
      }
      sort.sort(ordered_gt< Order >(cmp_order));
      return sort;
   }

   bool terms_sorted() const
   {
      return data->the_sorted_terms_set;
   }

   // find the leading term with respect of the lexicographic order
   // Constant time, if terms have be sorted, else linear
   typename term_hash::const_iterator find_lex_lm() const
   {
      if (!trivial()) {
         typename term_hash::const_iterator lt_it;
         if(terms_sorted()) lt_it = data->the_terms.find(*(get_sorted_terms().begin()));
         else lt_it = find_lm(cmp_monomial_ordered_base<exponent_type>());
         return lt_it;
      } else {
         return data->the_terms.end();
      }
   }

   template <typename Comparator>
   typename term_hash::const_iterator find_lm(const Comparator& cmp_order) const
   {
      typename term_hash::const_iterator it=data->the_terms.begin(), lt_it=it, end=data->the_terms.end();
      if (it != end) {
         while (++it != end)
            if (monomial_type::compare_values(it->first, lt_it->first, cmp_order) == cmp_gt)
               lt_it=it;
      }
      return lt_it;
   }

   template <bool trusted, bool addition>
   void add_term(const typename monomial_type::value_type& m, const coefficient_type& c, bool2type<trusted>, bool2type<addition>)
   {
      if (!trusted && __builtin_expect(is_zero(c), 0)) return;

      data->forget_sorted_terms();
      std::pair<typename term_hash::iterator, bool> it = data->the_terms.find_or_insert(m);
      if (it.second) {
         if (addition)
            it.first->second=c;
         else
            it.first->second=-c;
      } else if (is_zero(addition ? (it.first->second += c) : (it.first->second -= c))) {
         data->the_terms.erase(it.first);
      }
   }

public:
   bool operator== (const Polynomial_base& p2) const
   {
      if (get_ring() != p2.get_ring()) throw std::runtime_error("Polynomials of different rings");
      return data->the_terms == p2.data->the_terms;
   }

   bool operator!= (const Polynomial_base& p2) const
   {
      return !operator==(p2);
   }

   bool operator== (const term_type& t) const
   {
      if (get_ring() != t.get_ring()) throw std::runtime_error("Polynomials of different rings");
      return data->the_terms.size()==1
          && *data->the_terms.begin() == t.get_value();
   }

   bool operator!= (const term_type& t) const
   {
      return !operator==(t);
   }

   friend
   bool operator== (const term_type& t, const Polynomial_base& p)
   {
      return p==t;
   }

   friend
   bool operator!= (const term_type& t, const Polynomial_base& p)
   {
     return !(p==t);
   }

   bool operator== (const monomial_type& m) const
   {
      if (get_ring() != m.get_ring()) throw std::runtime_error("Polynomials of different rings");
      return data->the_terms.size()==1
          && data->the_terms.begin()->first == m.get_value()
          && is_one(data->the_terms.begin()->second);
   }

   bool operator!= (const monomial_type& m) const
   {
      return !operator==(m);
   }

   friend
   bool operator== (const monomial_type& m, const Polynomial_base& p)
   {
      return p==m;
   }

   friend
   bool operator!= (const monomial_type& m, const Polynomial_base& p)
   {
     return !(p==m);
   }

   template <typename T>
   typename enable_if<bool, fits_as_coefficient<T, Polynomial_base>::value>::type
   operator== (const T& c) const
   {
      return trivial() && is_zero(c)
         || data->the_terms.size()==1
          && monomial_type::equals_to_default(data->the_terms.begin()->first)
          && data->the_terms.begin()->second==c;
   }

   template <typename T>
   typename enable_if<bool, fits_as_coefficient<T, Polynomial_base>::value>::type
   operator!= (const T& c) const
   {
      return !operator==(c);
   }

   template <typename T> friend
   typename enable_if<bool, fits_as_coefficient<T, Polynomial_base>::value>::type
   operator== (const T& c, const Polynomial_base& p)
   {
      return p==c;
   }

   template <typename T> friend
   typename enable_if<bool, fits_as_coefficient<T, Polynomial_base>::value>::type
   operator!= (const T& c, const Polynomial_base& p)
   {
     return !(p==c);
   }

   //! compare term-wise with respect of the given ordering
   template <typename Comparator>
   cmp_value compare_ordered(const Polynomial_base& p, const Comparator& cmp_order) const
   {
      if (get_ring() != p.get_ring()) throw std::runtime_error("Polynomials of different rings");

      if (trivial()) return p.trivial() ? cmp_eq : cmp_lt;
      if (p.trivial()) return cmp_gt;

      // this list will carry the sorted terms except in lex
      sorted_terms_type t1, t2;

      const sorted_terms_type& fst = pm::identical<Comparator, cmp_monomial_ordered_base<exponent_type> >::value ?   get_sorted_terms() :   get_sorted_terms(t1, cmp_order);
      const sorted_terms_type& snd = pm::identical<Comparator, cmp_monomial_ordered_base<exponent_type> >::value ? p.get_sorted_terms() : p.get_sorted_terms(t2, cmp_order);
        
      typename sorted_terms_type::const_iterator it1 = fst.begin(), 
                                                 it2 = snd.begin();

      while (it1 != fst.end() && it2 != snd.end()) {
         typename term_hash::const_iterator it_term1=data->the_terms.find(*it1),
                                            it_term2=p.data->the_terms.find(*it2);
         if (POLYMAKE_DEBUG) {
            if (it_term1 == data->the_terms.end()) {
               cerr << "Polynomial:\n" << data->the_terms << "\nSorted terms:\n" << fst << "\n";
               throw std::runtime_error("wrong 1st sorted term sequence");
            }
            if (it_term2 == p.data->the_terms.end()) {
               cerr << "Polynomial:\n" << p.data->the_terms << "\nSorted terms:\n" << snd << "\n";
               throw std::runtime_error("wrong 2nd sorted term sequence");
            }
         }

         cmp_value cmp_terms = term_type::compare_values(*it_term1, *it_term2,
                                                         cmp_monomial_ordered_base<exponent_type>()); 
         if(cmp_terms != cmp_eq) return cmp_terms;
         else {
           ++it1;
           ++it2;
         }
      }
      if(it1 == fst.end()) {
        if(it2 == snd.end()) return cmp_eq;
        else return cmp_lt;
      } else return cmp_gt;
   }

   //! compare lexicographically
   cmp_value compare(const Polynomial_base& p) const
   {
     return compare_ordered(p, cmp_monomial_ordered_base<exponent_type>());
   }

   friend
   bool operator< (const Polynomial_base& p1, const Polynomial_base& p2)
   {
      return p1.compare(p2) == cmp_lt;
   }
   friend
   bool operator> (const Polynomial_base& p1, const Polynomial_base& p2)
   {
      return p1.compare(p2) == cmp_gt;
   }
   friend
   bool operator<= (const Polynomial_base& p1, const Polynomial_base& p2)
   {
      return p1.compare(p2) != cmp_gt;
   }
   friend
   bool operator>= (const Polynomial_base& p1, const Polynomial_base& p2)
   {
      return p1.compare(p2) != cmp_lt;
   }
};


template <typename Coefficient, typename Exponent>
class Polynomial :
   public Polynomial_base< Monomial<Coefficient, Exponent> > {

   template <typename> friend struct spec_object_traits;
public:
   typedef Monomial<Coefficient, Exponent> monomial_type;
protected:
   typedef Polynomial_base<monomial_type> super;
   explicit Polynomial(const typename super::impl& d) : super(d) {}
public:
   // undefined object - to be read from perl::Value
   Polynomial() {}

   // the default polynomial in a given ring
   explicit Polynomial(const typename super::ring_type& r) :
      super(r) {}

   // a constant polynomial
   template <typename T>
   Polynomial(const T& c, const typename super::ring_type& r,
              typename enable_if<void**, fits_as_coefficient<T, Polynomial>::value>::type=0) :
      super(c, r) {}

   Polynomial(const monomial_type& m) :
      super(m) {}

   Polynomial(const typename super::term_type& t) :
      super(t) {}

   template <typename Matrix, typename Container>
   Polynomial(const GenericMatrix<Matrix,Exponent>& monoms,
              const Container& coeffs,
              const typename super::ring_type& r,
              typename enable_if<void**, isomorphic_to_container_of<Container, Coefficient>::value>::type=0) :
      super(r)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (monoms.rows() != coeffs.size() || monoms.cols() != this->n_vars())
            throw std::runtime_error("Polynomial constructor - dimension mismatch");
      }
      typename Container::const_iterator c=coeffs.begin();
      for (typename Entire<Rows<Matrix> >::const_iterator m=entire(rows(monoms)); !m.at_end(); ++m, ++c)
         this->add_term(*m, *c, False(), True());
   }

   template <typename Matrix>
   Matrix monomials_as_matrix() const
   {
      return Matrix(this->n_terms(), this->n_vars(),
                    entire(attach_operation(this->get_terms(), BuildUnary<operations::take_first>())));
   }

   SparseMatrix<Exponent> monomials_as_matrix() const
   {
      return monomials_as_matrix< SparseMatrix<Exponent> >();
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const Polynomial& me)
   {
      me.pretty_print(out, cmp_monomial_ordered_base<Exponent>());
      return out.top();
   }

   template <typename Matrix>
   void print_ordered(const GenericMatrix<Matrix, Exponent>& order) const
   {
      this->pretty_print(cout, cmp_monomial_ordered<Matrix>(order.top()));
      cout << std::flush;
   }
};

template <typename Coefficient, typename Exponent>
struct algebraic_traits< UniPolynomial<Coefficient, Exponent> > {
   typedef RationalFunction<typename algebraic_traits<Coefficient>::field_type, Exponent> field_type;
};

// forward declarations needed for friends

template <typename Coefficient, typename Exponent>
Div< UniPolynomial<Coefficient, Exponent> >
div(const UniPolynomial<Coefficient, Exponent>& num, const UniMonomial<Coefficient, Exponent>& den);

template <typename Coefficient, typename Exponent>
Div< UniPolynomial<Coefficient, Exponent> >
div(const UniPolynomial<Coefficient, Exponent>& num, const UniTerm<Coefficient, Exponent>& den);

template <typename Coefficient, typename Exponent>
Div< UniPolynomial<Coefficient, Exponent> >
div(const UniPolynomial<Coefficient, Exponent>& num, const UniPolynomial<Coefficient, Exponent>& den);

template <typename Coefficient, typename Exponent>
UniPolynomial<Coefficient, Exponent>
gcd(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b);

template <typename Coefficient, typename Exponent>
ExtGCD< UniPolynomial<Coefficient, Exponent> >
ext_gcd(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b,
        bool normalize_gcd=true);

template <typename Coefficient, typename Exponent>
UniPolynomial<Coefficient, Exponent>
lcm(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b);


template <typename Coefficient, typename Exponent>
class UniPolynomial :
   public Polynomial_base< UniMonomial<Coefficient, Exponent> > {

   friend class RationalFunction<Coefficient, Exponent>;
   template <typename> friend struct spec_object_traits;
public:
   typedef UniMonomial<Coefficient, Exponent> monomial_type;
protected:
   typedef Polynomial_base<monomial_type> super;
   explicit UniPolynomial(const typename super::impl& d) : super(d) {}
public:
   UniPolynomial() :
      super(monomial_type::default_ring()) {}

   explicit UniPolynomial(const typename super::ring_type& r) :
      super(r)
   {
      if (r.n_vars() != 1) throw std::runtime_error("UniPolynomial constructor - invalid ring");
   }

   template <typename T>
   explicit UniPolynomial(const T& c,
                          typename enable_if<void**, fits_as_coefficient<T, UniPolynomial>::value>::type=0) :
      super(c, monomial_type::default_ring()) {}

   template <typename T>
   UniPolynomial(const T& c, const typename super::ring_type& r,
                 typename enable_if<void**, fits_as_coefficient<T, UniPolynomial>::value>::type=0) :
      super(c,r)
   {
      if (r.n_vars() != 1) throw std::runtime_error("UniPolynomial constructor - invalid ring");
   }

   UniPolynomial(const monomial_type& m) :
      super(m) {}

   UniPolynomial(const typename super::term_type& t) :
      super(t) {}

   template <typename Container1, typename Container2>
   UniPolynomial(const Container1& coeffs,
                 const Container2& monoms,
                 const typename super::ring_type& r,
                 typename enable_if<void**, (isomorphic_to_container_of<Container1, Coefficient>::value &&
                                             isomorphic_to_container_of<Container2, Exponent>::value)>::type=0) :
      super(r)
   {
      if (r.n_vars() != 1) throw std::runtime_error("UniPolynomial constructor - invalid ring");
      if (POLYMAKE_DEBUG) {
         if (monoms.size() != coeffs.size())
            throw std::runtime_error("UniPolynomial constructor - dimension mismatch");
      }

      typename Container1::const_iterator c=coeffs.begin();
      for (typename Entire<Container2>::const_iterator m=entire(monoms); !m.at_end(); ++m, ++c)
         this->add_term(*m, *c, False(), True());
   }

   Vector<Exponent> monomials_as_vector()  const
   {
      return Vector<Exponent>(this->n_terms(),
                              attach_operation(this->get_terms(), BuildUnary<operations::take_first>()).begin());
   }

   //! the degree of the polynomial
   Exponent deg() const { return this->lm_exp(); }

   //! the lowest degree of a monomial occuring here
   Exponent lower_deg() const
   {
      Exponent low=std::numeric_limits<Exponent>::max();
      for (typename Entire<typename super::term_hash>::const_iterator it=entire(this->get_terms()); !it.at_end(); ++it)
         assign_min(low, it->first);
      return low;
   }

   //! Return the leading coefficient.
   const Coefficient& lc() const
   {
      if (this->trivial())
         return this->data->ring.zero_coef();
      else
         return this->find_lex_lm()->second;
   }

   // find leading coefficent (after multiplicating exponents with order, e.g. +/- 1)
   const Coefficient& lc(const Exponent& order) const
   {
      if (this->trivial())
         return this->data->ring.zero_coef();
      else
         return this->find_lm(cmp_monomial_ordered<Exponent>(order))->second;
   }

   template <typename T>
   typename enable_if<typename algebraic_traits<T>::field_type, (is_field_of_fractions<Exponent>::value && fits_as_coefficient<T, UniPolynomial>::value)>::type
   evaluate(const T& t, const long exp_lcm=1) const
   {
      typedef typename algebraic_traits<T>::field_type field;
      field res;
      for (typename Entire<typename super::term_hash>::const_iterator it=entire(this->get_terms()); !it.at_end(); ++it)
      {
         const Exponent exp = exp_lcm * it->first;
         if (denominator(exp) != 1)
            throw std::runtime_error("Exponents non-integral, larger exp_lcm needed.");
         res += it->second * field::pow(t, convert_to<long>(exp));
      }
      return res;
   }

   template <typename T>
   typename enable_if<typename algebraic_traits<T>::field_type, (std::numeric_limits<Exponent>::is_integer && fits_as_coefficient<T, UniPolynomial>::value)>::type
   evaluate(const T& t, const long exp_lcm=1) const
   {
      typedef typename algebraic_traits<T>::field_type field;
      field res;
      for (typename Entire<typename super::term_hash>::const_iterator it=entire(this->get_terms()); !it.at_end(); ++it)
      {
         res += it->second * field::pow(t, convert_to<long>(exp_lcm * it->first));
      }
      return res;
   }



   double evaluate_float(const double a) const
   {
      double res = 0;
      for (typename Entire<typename super::term_hash>::const_iterator it=entire(this->get_terms()); !it.at_end(); ++it)
      {
         res += convert_to<double>(it->second) * std::pow(a,convert_to<double>(it->first));
      }
      return res;
   }

   /*! Perform the polynomial division and assign the quotient to *this.
    *  Like in the free function of the same name, you can pass an arbitrary polynomial to this method,
    *  not only a factor of *this.
    */
   UniPolynomial& div_exact(const UniPolynomial& b)
   {
      if (this->get_ring() != b.get_ring()) throw std::runtime_error("Polynomials of different rings");
      if (b.trivial()) throw GMP::ZeroDivide();

      UniPolynomial quot(this->get_ring());
      this->data.enforce_unshared();
      remainder(b, quot.data.get()->the_terms.make_filler());
      this->swap(quot);
      return *this;
   }

   UniPolynomial& div_exact(const monomial_type& b)
   {
      if (this->get_ring() != b.get_ring()) throw std::runtime_error("Polynomials of different rings");

      UniPolynomial quot(this->get_ring());
      this->data.enforce_unshared();
      remainder(b.get_value(), quot.data.get()->the_terms.make_filler());
      this->swap(quot);
      return *this;
   }

   UniPolynomial& div_exact(const UniTerm<Coefficient, Exponent>& b)
   {
      if (this->get_ring() != b.get_ring()) throw std::runtime_error("Polynomials of different rings");
      if (is_zero(b.get_value().second)) throw GMP::ZeroDivide();

      UniPolynomial quot(this->get_ring());
      this->data.enforce_unshared();
      remainder(b.get_value().first, quot.data.get()->the_terms.make_filler());
      quot /= b.get_value().second;
      this->swap(quot);
      return *this;
   }

   UniPolynomial& operator%= (const UniPolynomial& b)
   {
      if (this->get_ring() != b.get_ring()) throw std::runtime_error("Polynomials of different rings");
      if (b.trivial()) throw GMP::ZeroDivide();
      if (!this->trivial()) {
         this->data.enforce_unshared();
         remainder(b, quot_black_hole());
      }
      return *this;
   }

   UniPolynomial operator% (const UniPolynomial& b) const
   {
      UniPolynomial tmp(*this->data);
      return tmp %= b;
   }

   UniPolynomial& operator%= (const monomial_type& b)
   {
      if (this->get_ring() != b.get_ring()) throw std::runtime_error("Polynomials of different rings");

      this->data.enforce_unshared();
      remainder(b.get_value(), quot_black_hole());
      return *this;
   }

   UniPolynomial operator% (const monomial_type& b) const
   {
      UniPolynomial tmp(*this->data);
      return tmp %= b;
   }

   UniPolynomial& operator%= (const UniTerm<Coefficient, Exponent>& b)
   {
      if (this->get_ring() != b.get_ring()) throw std::runtime_error("Polynomials of different rings");
      if (is_zero(b.get_value().second)) throw GMP::ZeroDivide();

      this->data.enforce_unshared();
      remainder(b.get_value().first, quot_black_hole());
      return *this;
   }

   UniPolynomial operator% (const UniTerm<Coefficient, Exponent>& b) const
   {
      UniPolynomial tmp(*this->data);
      return tmp %= b;
   }

   friend
   Div<UniPolynomial> div<>(const UniPolynomial& a, const UniPolynomial& b);

   friend
   Div<UniPolynomial> div<>(const UniPolynomial& a, const monomial_type& b);

   friend
   Div<UniPolynomial> div<>(const UniPolynomial& a, const UniTerm<Coefficient, Exponent>& b);

   friend
   UniPolynomial gcd<>(const UniPolynomial& a, const UniPolynomial& b);

   friend
   ExtGCD<UniPolynomial> ext_gcd<>(const UniPolynomial& a, const UniPolynomial& b,
                                   bool normalize_gcd);

   friend
   UniPolynomial lcm<>(const UniPolynomial& a, const UniPolynomial& b);

   RationalFunction<Coefficient, Exponent> to_rational_function() const {
      return RationalFunction<Coefficient, Exponent>(*this);
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const UniPolynomial& me)
   {
      me.pretty_print(out, cmp_monomial_ordered<Exponent>(Exponent(1)));
      return out.top();
   }

   void print_ordered(const Exponent& order) const
   {
      this->pretty_print(cout, cmp_monomial_ordered<Exponent>(order));
      cout << std::flush;
   }

private:
   struct quot_black_hole
   {
      void operator() (const Exponent&, const Coefficient&) const {}
   };

   // replace this with a remainder of division by b, consume the quotient
   // data must be brought in exclusive posession before calling this method.
   template <typename QuotConsumer>
   void remainder(const UniPolynomial& b, const QuotConsumer& quot_consumer)
   {
      const typename super::term_hash::const_iterator
         this_end=this->data.get()->the_terms.end(),
           b_lead=b.find_lex_lm(),
            b_end=b.data->the_terms.end();

      typename super::term_hash::const_iterator this_lead;

      while ((this_lead=this->find_lex_lm()) != this_end && this_lead->first >= b_lead->first) {
         const Coefficient k = this_lead->second / b_lead->second;
         const Exponent d = this_lead->first - b_lead->first;
         quot_consumer(d, k);
         this->data.get()->forget_sorted_terms();

         for (typename super::term_hash::const_iterator b_it=b.data->the_terms.begin();  b_it != b_end;  ++b_it) {
            std::pair<typename super::term_hash::iterator, bool> it = this->data.get()->the_terms.find_or_insert(b_it->first + d);
            if (it.second) {
               it.first->second= -k * b_it->second;
            } else if (is_zero(it.first->second -= k * b_it->second)) {
               this->data.get()->the_terms.erase(it.first);
            }
         }
      }
   }

   // replace this with a remainder of division by b, consume the quotient
   // data must be brought in exclusive posession before calling this method.
   template <typename QuotConsumer>
   void remainder(typename function_argument<Exponent>::type b, const QuotConsumer& quot_consumer)
   {
      for (typename super::term_hash::const_iterator it=this->data.get()->the_terms.begin(), end=this->data.get()->the_terms.end();  it != end;  ) {
         if (it->first < b) {
            ++it;
         } else {
            if (!identical<QuotConsumer, quot_black_hole>::value)
               quot_consumer(it->first - b, it->second);
            this->data.get()->the_terms.erase(it++);
         }
      }
      this->data.get()->forget_sorted_terms();
   }
};

template <typename Coefficient, typename Exponent> inline
Div< UniPolynomial<Coefficient, Exponent> >
div(const UniPolynomial<Coefficient, Exponent>& num, const UniPolynomial<Coefficient, Exponent>& den)
{
   if (num.get_ring() != den.get_ring()) throw std::runtime_error("Polynomials of different rings");
   if (den.trivial()) throw GMP::ZeroDivide();
   Div< UniPolynomial<Coefficient, Exponent> > res;
   res.quot.data.get()->ring=num.get_ring();
   res.rem.data.assign_copy(num.data);
   res.rem.remainder(den, res.quot.data.get()->the_terms.make_filler());
   return res;
}

template <typename Coefficient, typename Exponent> inline
Div< UniPolynomial<Coefficient, Exponent> >
div(const UniPolynomial<Coefficient, Exponent>& num, const UniMonomial<Coefficient, Exponent>& den)
{
   if (num.get_ring() != den.get_ring()) throw std::runtime_error("Polynomials of different rings");
   Div< UniPolynomial<Coefficient, Exponent> > res;
   res.quot.data.get()->ring=num.get_ring();
   res.rem.data.assign_copy(num.data);
   res.rem.remainder(den.get_value(), res.quot.data.get()->the_terms.make_filler());
   return res;
}

template <typename Coefficient, typename Exponent> inline
Div< UniPolynomial<Coefficient, Exponent> >
div(const UniPolynomial<Coefficient, Exponent>& num, const UniTerm<Coefficient, Exponent>& den)
{
   if (num.get_ring() != den.get_ring()) throw std::runtime_error("Polynomials of different rings");
   if (is_zero(den.get_value().second)) throw GMP::ZeroDivide();
   Div< UniPolynomial<Coefficient, Exponent> > res;
   res.quot.data.get()->ring=num.get_ring();
   res.rem.data.assign_copy(num.data);
   res.rem.remainder(den.get_value().first, res.quot.data.get()->the_terms.make_filler());
   res.quot /= den.get_value().second;
   return res;
}

template <typename Coefficient, typename Exponent>
UniPolynomial<Coefficient, Exponent>
gcd(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b)
{
   if (a.get_ring() != b.get_ring()) throw std::runtime_error("Polynomials of different rings");
   if (a.trivial()) return b;
   if (b.trivial()) return a;

   const bool sw = a.lm_exp() < b.lm_exp();
   UniPolynomial<Coefficient, Exponent> p1(*(sw ? b : a).data),
                                        p2(*(sw ? a : b).data);

   while (!p2.trivial() && !is_zero(p2.lm_exp())) {
      p1.remainder(p2, typename UniPolynomial<Coefficient, Exponent>::quot_black_hole());
      p1.swap(p2);
   }
   if (p2.trivial())
      return p1.normalize();
   else
      return UniPolynomial<Coefficient, Exponent>(a.get_ring().one_coef(), a.get_ring());  // =1
}

template <typename Coefficient, typename Exponent>
ExtGCD< UniPolynomial<Coefficient, Exponent> >
ext_gcd(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b,
        bool normalize_gcd)
{
   typedef UniPolynomial<Coefficient, Exponent> Polynomial;
   const typename Polynomial::ring_type& ring=a.get_ring();
   if (ring != b.get_ring()) throw std::runtime_error("Polynomials of different rings");

   ExtGCD<Polynomial> res;
   if (a.trivial()) {
      res.g=b;
      res.p=res.q=res.k2=Polynomial(ring.one_coef(), ring);
      res.k1.data.get()->ring=ring;

   } else if (b.trivial()) {
      res.g=a;
      res.p=res.q=res.k1=Polynomial(ring.one_coef(), ring);
      res.k2.data.get()->ring=ring;

   } else {
      Polynomial U[2][2]={ { Polynomial(ring.one_coef(), ring),  Polynomial(ring) },
                           { Polynomial(ring),  Polynomial(ring.one_coef(), ring) } };

      const bool sw = a.lm_exp() < b.lm_exp();
      Polynomial p1(*(sw ? b : a).data),
                 p2(*(sw ? a : b).data),
                 k(ring);

      for (;;) {
         k.clear();
         p1.remainder(p2, k.data.get()->the_terms.make_filler());
         // multiply U from left with { {1, -k}, {0, 1} }
         U[0][0] -= k * U[1][0];
         U[0][1] -= k * U[1][1];
         if (p1.trivial()) {
            res.g.swap(p2);
            res.p.swap(U[1][sw]);  res.q.swap(U[1][1-sw]);
            res.k2.swap(U[0][sw]);  res.k1.swap(U[0][1-sw]);
            (sw ? res.k2 : res.k1).negate();
            break;
         }

         k.clear();
         p2.remainder(p1, k.data.get()->the_terms.make_filler());
         // multiply U from left with { {1, 0}, {-k, 1} }
         U[1][0] -= k * U[0][0];
         U[1][1] -= k * U[0][1];
         if (p2.trivial()) {
            res.g.swap(p1);
            res.p.swap(U[0][sw]);  res.q.swap(U[0][1-sw]);
            res.k2.swap(U[1][sw]);  res.k1.swap(U[1][1-sw]);
            (sw ? res.k1 : res.k2).negate();
            break;
         }
      }

      if (normalize_gcd) {
         const Coefficient lead=res.g.lc();
         if (!is_one(lead)) {
            res.g /= lead;
            res.p /= lead;
            res.q /= lead;
            res.k1 *= lead;
            res.k2 *= lead;
         }
      }
   }

   return res;
}

template <typename Coefficient, typename Exponent> inline
UniPolynomial<Coefficient, Exponent>
lcm(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b)
{
   const ExtGCD< UniPolynomial<Coefficient, Exponent> > x = ext_gcd(a, b);
   return a * x.k2;
}

/*! Perform the polynomial division, discarding the remainder.
 *  Although the name suggests that the divisor must be a factor of the divident,
 *  you can put arbitrary polynomials here.
 *  The name is rather chosen for compatibility with the Integer class.
 */
template <typename Coefficient, typename Exponent> inline
UniPolynomial<Coefficient, Exponent> div_exact(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b)
{
   UniPolynomial<Coefficient, Exponent> tmp(a);
   return tmp.div_exact(b);
}

template <typename Coefficient, typename Exponent> inline
UniPolynomial<Coefficient, Exponent> div_exact(const UniPolynomial<Coefficient, Exponent>& a, const UniMonomial<Coefficient, Exponent>& b)
{
   UniPolynomial<Coefficient, Exponent> tmp(a);
   return tmp.div_exact(b);
}

template <typename Coefficient, typename Exponent> inline
UniPolynomial<Coefficient, Exponent> div_exact(const UniPolynomial<Coefficient, Exponent>& a, const UniTerm<Coefficient, Exponent>& b)
{
   UniPolynomial<Coefficient, Exponent> tmp(a);
   return tmp.div_exact(b);
}

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< Polynomial<Coefficient,Exponent> > > :
   spec_object_traits<is_composite> {

   typedef Polynomial<Coefficient,Exponent> masquerade_for;

   typedef cons<typename Polynomial<Coefficient,Exponent>::term_hash,
                typename Polynomial<Coefficient,Exponent>::ring_type> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      // here we read a serialized polynomial and should clear the sorted terms first
      me.data->forget_sorted_terms();
      v << me.data->the_terms << me.data->ring;
   }

   template <typename Me, typename Visitor>
   static void visit_elements(const Me& me, Visitor& v)
   {
      v << me.data->the_terms << me.data->ring;
   }
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< UniPolynomial<Coefficient,Exponent> > > :
   spec_object_traits<is_composite> {

   typedef UniPolynomial<Coefficient,Exponent> masquerade_for;

   typedef cons<typename UniPolynomial<Coefficient,Exponent>::term_hash,
                typename UniPolynomial<Coefficient,Exponent>::ring_type> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      // here we read a serialized polynomial and should clear the sorted terms first
      me.data->forget_sorted_terms();
      v << me.data->the_terms << me.data->ring;
   }

   template <typename Me, typename Visitor>
   static void visit_elements(const Me& me, Visitor& v)
   {
      v << me.data->the_terms << me.data->ring;
   }
};

// some magic needed for operator construction
struct is_polynomial {};

template <typename Coefficient, typename Exponent>
struct choose_generic_object_traits< Polynomial<Coefficient, Exponent>, false, false > :
   spec_object_traits< Polynomial<Coefficient, Exponent> > {
   typedef void generic_type;
   typedef is_polynomial generic_tag;
   typedef Polynomial<Coefficient, Exponent> persistent_type;

   static
   bool is_zero(const persistent_type& p)
   {
      return p.trivial();
   }

   static
   bool is_one(const persistent_type& p)
   {
      return p.unit();
   }
};

template <typename Coefficient, typename Exponent>
struct choose_generic_object_traits< UniPolynomial<Coefficient, Exponent>, false, false > :
   spec_object_traits< UniPolynomial<Coefficient, Exponent> > {
   typedef void generic_type;
   typedef is_polynomial generic_tag;
   typedef UniPolynomial<Coefficient, Exponent> persistent_type;

   static
   bool is_zero(const persistent_type& p)
   {
      return p.trivial();
   }

   static
   bool is_one(const persistent_type& p)
   {
      return p.unit();
   }

   static
   const persistent_type& zero()
   {
      static const persistent_type x=persistent_type();
      return x;
   }

   static
   const persistent_type& one()
   {
      static const persistent_type x(one_value<Coefficient>());
      return x;
   }
};

template <typename Coefficient, typename Exponent> inline
bool is_zero(const Term<Coefficient, Exponent>& t)
{
   return is_zero(t.get_coefficient());
}

template <typename Coefficient, typename Exponent> inline
bool is_zero(const UniTerm<Coefficient, Exponent>& t)
{
   return is_zero(t.get_coefficient());
}

template <typename Coefficient, typename Exponent> inline
bool is_zero(const Monomial<Coefficient, Exponent>&)
{
   return false;
}

template <typename Coefficient, typename Exponent> inline
bool is_zero(const UniMonomial<Coefficient, Exponent>&)
{
   return false;
}

template <typename Coefficient, typename Exponent>
struct is_gcd_domain< UniPolynomial<Coefficient, Exponent> > : is_field<Coefficient> {};

template <typename Coefficient, typename Exponent, typename T>
struct compatible_with_polynomial {
   static const bool value= isomorphic_types<Coefficient, T>::value ||
                            deeper_coefficient_of<T, Term<Coefficient, Exponent> >::value ||
                            derived_from<T, Monomial<Coefficient, Exponent> >::value ||
                            derived_from<T, Term<Coefficient, Exponent> >::value;
};

template <typename Coefficient, typename Exponent, typename T>
struct compatible_with_unipolynomial {
   static const bool value= isomorphic_types<Coefficient, T>::value ||
                            deeper_coefficient_of<T, UniTerm<Coefficient, Exponent> >::value ||
                            derived_from<T, UniMonomial<Coefficient, Exponent> >::value ||
                            derived_from<T, UniTerm<Coefficient, Exponent> >::value;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<Polynomial<Coefficient,Exponent>, T,
                             typename enable_if<is_polynomial, compatible_with_polynomial<Coefficient,Exponent,T>::value>::type,
                             TModel>
   : False {
   typedef cons<is_polynomial, is_scalar> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<T, Polynomial<Coefficient,Exponent>, TModel,
                             typename enable_if<is_polynomial, compatible_with_polynomial<Coefficient,Exponent,T>::value>::type>
   : False {
   typedef cons<is_scalar, is_polynomial> discriminant;
};

template <typename Coefficient, typename Exponent>
struct isomorphic_types_impl<Polynomial<Coefficient,Exponent>, Polynomial<Coefficient,Exponent>, is_polynomial, is_polynomial>
   : True {
   typedef cons<is_polynomial, is_polynomial> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<UniPolynomial<Coefficient,Exponent>, T,
                             typename enable_if<is_polynomial, compatible_with_unipolynomial<Coefficient,Exponent,T>::value>::type,
                             TModel>
   : False {
   typedef cons<is_polynomial, is_scalar> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<T, UniPolynomial<Coefficient,Exponent>, TModel,
                             typename enable_if<is_polynomial, compatible_with_unipolynomial<Coefficient,Exponent,T>::value>::type>
   : False {
   typedef cons<is_scalar, is_polynomial> discriminant;
};

template <typename Coefficient, typename Exponent>
struct isomorphic_types_impl<UniPolynomial<Coefficient,Exponent>, UniPolynomial<Coefficient,Exponent>, is_polynomial, is_polynomial>
   : True {
   typedef cons<is_polynomial, is_polynomial> discriminant;
};


template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator+ (const monomial_type& m, const T& c)
{
   return Polynomial_base<monomial_type>(m)+=c;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator+ (const T& c, const monomial_type& m)
{
   return m+c;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator- (const monomial_type& m, const T& c)
{
   return Polynomial_base<monomial_type>(m)-=c;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator- (const T& c, const monomial_type& m)
{
   return Polynomial_base<monomial_type>(c, m.get_ring())-=m;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type
operator+ (const monomial_type& m1, const monomial_type& m2)
{
   return Polynomial_base<monomial_type>(m1)+=m2;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type
operator- (const monomial_type& m1, const monomial_type& m2)
{
   return Polynomial_base<monomial_type>(m1)-=m2;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator+ (const Term_base<monomial_type>& t, const T& c)
{
   return Polynomial_base<monomial_type>(t)+=c;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator+ (const T& c, const Term_base<monomial_type>& t)
{
   return t+c;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator- (const Term_base<monomial_type>& t, const T& c)
{
   return Polynomial_base<monomial_type>(t)-=c;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator- (const T& c, const Term_base<monomial_type>& t)
{
   return Polynomial_base<monomial_type>(c, t.get_ring())-=t;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type
operator+ (const Term_base<monomial_type>& t, const monomial_type& m)
{
   return Polynomial_base<monomial_type>(t)+=m;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type
operator+ (const monomial_type& m, const Term_base<monomial_type>& t)
{
   return t+m;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type
operator- (const Term_base<monomial_type>& t, const monomial_type& m)
{
   return Polynomial_base<monomial_type>(t)-=m;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type
operator- (const monomial_type& m, const Term_base<monomial_type>& t)
{
   return Polynomial_base<monomial_type>(m)-=t;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type
operator+ (const Term_base<monomial_type>& t1, const Term_base<monomial_type>& t2)
{
   return Polynomial_base<monomial_type>(t1)+=t2;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type
operator- (const Term_base<monomial_type>& t1, const Term_base<monomial_type>& t2)
{
   return Polynomial_base<monomial_type>(t1)-=t2;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator+ (const T& c, const Polynomial_base<monomial_type>& p)
{
   return p+c;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator- (const T& c, const Polynomial_base<monomial_type>& p)
{
   return (-p)+=c;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type operator+ (const monomial_type& m, const Polynomial_base<monomial_type>& p)
{
   return p+m;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type operator- (const monomial_type& m, const Polynomial_base<monomial_type>& p)
{
   return (-p)+=m;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type operator+ (const Term_base<monomial_type>& t, const Polynomial_base<monomial_type>& p)
{
   return p+t;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type operator- (const Term_base<monomial_type>& t, const Polynomial_base<monomial_type>& p)
{
   return (-p)+=t;
}

template <typename monomial_type, typename T> inline
typename enable_if<typename Polynomial_result<monomial_type>::type, fits_as_coefficient<T, monomial_type>::value>::type
operator* (const T& c, const Polynomial_base<monomial_type>& p)
{
   return p.mult_from_right(c);
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type operator* (const monomial_type& m, const Polynomial_base<monomial_type>& p)
{
   return p*m;
}

template <typename monomial_type> inline
typename Polynomial_result<monomial_type>::type operator* (const Term_base<monomial_type>& t, const Polynomial_base<monomial_type>& p)
{
   return p.mult(t, True());
}

namespace operations {

// these operations will be required e.g. for Vector<Polynomial> or Matrix<Polynomial>

template <typename OpRef>
struct neg_impl<OpRef, is_polynomial> {
   typedef OpRef argument_type;
   typedef typename deref<OpRef>::type result_type;

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return -x;
   }

   void assign(typename lvalue_arg<OpRef>::type x) const
   {
      x.negate();
   }
};

template <typename LeftRef, class RightRef>
struct add_impl<LeftRef, RightRef, cons<is_polynomial, is_polynomial> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l+r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l+=r;
   }
};

template <typename LeftRef, class RightRef>
struct add_impl<LeftRef, RightRef, cons<is_polynomial, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l+r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l+=r;
   }
};

template <typename LeftRef, class RightRef>
struct add_impl<LeftRef, RightRef, cons<is_scalar, is_polynomial> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l+r;
   }
};

template <typename LeftRef, class RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_polynomial, is_polynomial> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l-r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l-=r;
   }
};

template <typename LeftRef, class RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_polynomial, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l-r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l-=r;
   }
};

template <typename LeftRef, class RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_scalar, is_polynomial> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l-r;
   }
};

template <typename LeftRef, class RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_polynomial, is_polynomial> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l*r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l*=r;
   }
};

template <typename LeftRef, class RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_polynomial, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l*r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l*=r;
   }
};

template <typename LeftRef, class RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_scalar, is_polynomial> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l*r;
   }
};

template <typename LeftRef, class RightRef>
struct div_impl<LeftRef, RightRef, cons<is_polynomial, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l/r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l/=r;
   }
};

template <typename LeftRef, class RightRef>
struct mod_impl<LeftRef, RightRef, cons<is_polynomial, is_polynomial> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l%r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l%=r;
   }
};

template <typename T>
struct cmp_common_polynomial : cmp_extremal, cmp_partial_opaque<T> {

   typedef T first_argument_type;
   typedef T second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal::operator();
   using cmp_partial_opaque<T>::operator();

   template <typename Left, typename Right>
   cmp_value operator() (const Left& p1, const Right& p2) const
   {
      return p1.compare(p2);
   }
};

template <typename Coefficient, typename Exponent>
struct cmp_opaque< Monomial<Coefficient,Exponent>, Monomial<Coefficient,Exponent> > :
   cmp_common_polynomial< Monomial<Coefficient,Exponent> > {};

template <typename Coefficient, typename Exponent>
struct cmp_opaque< UniMonomial<Coefficient,Exponent>, UniMonomial<Coefficient,Exponent> > :
   cmp_common_polynomial< UniMonomial<Coefficient,Exponent> > {};

template <typename Coefficient, typename Exponent>
struct cmp_opaque< Term<Coefficient, Exponent>, Term<Coefficient, Exponent> > :
   cmp_common_polynomial< Term<Coefficient, Exponent> > {};

template <typename Coefficient, typename Exponent>
struct cmp_opaque< UniTerm<Coefficient, Exponent>, UniTerm<Coefficient, Exponent> > :
   cmp_common_polynomial< UniTerm<Coefficient, Exponent> > {};

template <typename Coefficient, typename Exponent>
struct cmp_opaque< Polynomial<Coefficient, Exponent>, Polynomial<Coefficient, Exponent> > :
   cmp_common_polynomial< Polynomial<Coefficient, Exponent> > {};

template <typename Coefficient, typename Exponent>
struct cmp_opaque< UniPolynomial<Coefficient, Exponent>, UniPolynomial<Coefficient, Exponent> > :
   cmp_common_polynomial< UniPolynomial<Coefficient, Exponent> > {};

} // end namespace operations

} // end namespace pm

namespace polymake {
   using pm::Monomial;
   using pm::UniMonomial;
   using pm::Term;
   using pm::UniTerm;
   using pm::Polynomial;
   using pm::UniPolynomial;
}

namespace std {
   template <typename Coefficient, typename Exponent>
   void swap(pm::Monomial<Coefficient,Exponent>& x1, pm::Monomial<Coefficient,Exponent>& x2) { x1.swap(x2); }

   template <typename Coefficient, typename Exponent>
   void swap(pm::UniMonomial<Coefficient,Exponent>& x1, pm::UniMonomial<Coefficient,Exponent>& x2) { x1.swap(x2); }

   template <typename Coefficient, typename Exponent>
   void swap(pm::Term<Coefficient,Exponent>& x1, pm::Term<Coefficient,Exponent>& x2) { x1.swap(x2); }

   template <typename Coefficient, typename Exponent>
   void swap(pm::UniTerm<Coefficient,Exponent>& x1, pm::UniTerm<Coefficient,Exponent>& x2) { x1.swap(x2); }

   template <typename Coefficient, typename Exponent>
   void swap(pm::Polynomial<Coefficient,Exponent>& x1, pm::Polynomial<Coefficient,Exponent>& x2) { x1.swap(x2); }

   template <typename Coefficient, typename Exponent>
   void swap(pm::UniPolynomial<Coefficient,Exponent>& x1, pm::UniPolynomial<Coefficient,Exponent>& x2) { x1.swap(x2); }
}

#endif // POLYMAKE_POLYNOMIAL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
