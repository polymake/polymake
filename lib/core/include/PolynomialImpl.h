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

#include "polymake/Integer.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/SparseVector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TransformedContainer.h"
#include "polymake/numerical_functions.h"
#include "polymake/hash_map"
#include "polymake/list"
#include "polymake/vector"
#include "polymake/PolynomialVarNames.h"

#include <cassert>
#include <forward_list>

namespace pm {


template <typename Coefficient, typename Exponent>
class UniPolynomial;
template <typename Coefficient, typename Exponent>
class Polynomial;
template <typename Coefficient, typename Exponent>
class RationalFunction;

// some magic needed for operator construction
struct is_polynomial {};

// forward declarations needed for friends
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

namespace polynomial_impl {

// how to convert something to a coefficient of a Polynomial ---------------------

// primary template catches bogus input not being a polynomial at all
template <typename T, typename TBogus, typename enabled=void>
struct deeper_coefficient_impl
  : std::false_type {};

// if T can directly be converted into Polynomial's coefficient, it will be handled by the corresponding constructor
template <typename T, typename Coefficient>
struct deeper_coefficient_impl<T, Coefficient,
                               typename std::enable_if_t<std::is_convertible<T, Coefficient>::value>>
: std::false_type {};

// T can be converted to Polynomial's Coefficient's coefficient
template <typename T, typename Coefficient>
struct deeper_coefficient_impl<T, Coefficient,
                               typename std::enable_if_t<std::is_convertible<T, typename Coefficient::coefficient_type>::value>>
: std::true_type {
  using coefficient_type = Coefficient;

  static coefficient_type construct(const T& x, const Int n_vars)
  {
    return coefficient_type(x, n_vars);
  }
};

// T can be converted to some deeper coefficient in the nesting hierarchy
template <typename T, typename Coefficient>
struct deeper_coefficient_impl<T, Coefficient,
                               typename std::enable_if_t<!std::is_convertible<T, typename Coefficient::coefficient_type>::value &&
                                                         deeper_coefficient_impl<T, typename Coefficient::coefficient_type>::value>>
: std::true_type {
  using deeper = deeper_coefficient_impl<T, typename Coefficient::coefficient_type>;
  using coefficient_type = Coefficient;

  static coefficient_type construct(const T& x, const Int n_vars)
  {
    return coefficient_type(deeper::construct(x, n_vars), n_vars);
  }
};

// Sorting of monomials ----------------------------------------------
     
template <typename Exponent = Int, bool strict = true>
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
  template <typename TMatrix>
  cmp_value compare_values(const SparseVector<Exponent>& m1, const SparseVector<Exponent>& m2, const GenericMatrix<TMatrix>& order) const
  {
    cmp_value v(operations::cmp()(order * m1, order * m2));
    if (v != cmp_eq || !strict)
      return v;
    else
      return operations::cmp()(m1, m2);
  }

  // for multi-variate polynomials
  template <typename TVector>
  cmp_value compare_values(const SparseVector<Exponent>& m1, const SparseVector<Exponent>& m2, const GenericVector<TVector>& order) const
  {
    cmp_value v(operations::cmp()(order * m1, order * m2));
    if (v != cmp_eq || !strict)
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


template <typename Order, bool strict=true, typename order_type_tag=typename object_traits<Order>::generic_tag>
class cmp_monomial_ordered
  : public cmp_monomial_ordered_base<Order, strict>
{
public:
  using exponent_type = Order;

  explicit cmp_monomial_ordered(const exponent_type& order_arg)
    : order(order_arg) {}

  cmp_value operator()(const exponent_type& exp1, const exponent_type& exp2) const
  {
    return this->compare_values(exp1, exp2, order);
  }

private:
  const exponent_type order;
};


template <typename Order, bool strict>
class cmp_monomial_ordered<Order, strict, is_matrix>
  : public cmp_monomial_ordered_base<typename Order::element_type, strict>
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

template <typename Order, bool strict>
class cmp_monomial_ordered<Order, strict, is_vector>
  : public cmp_monomial_ordered_base<typename Order::element_type, strict>
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


// Univariate and multivariate monomials
// these don't actually contain any data, they just encode the basic functionality.
template <typename Exponent>
struct UnivariateMonomial;
template <typename Exponent>
struct MultivariateMonomial;

template <typename Coefficient>
struct nesting_level : int_constant<0> {};

template <typename Exponent>
struct UnivariateMonomial {
  template <typename exp>
  using new_type = UnivariateMonomial<exp>;
  using exponent_type = Exponent;
  using monomial_type = Exponent;
  using monomial_list_type = Vector<Exponent>;
  using homogenized_type = MultivariateMonomial<Exponent>;
  static Exponent deg(const monomial_type& m) { return m; }
  static monomial_type default_value(const Int n_vars) { return zero_value<Exponent>(); }
  static bool equals_to_default(const monomial_type& m) { return is_zero(m); }

  static monomial_type empty_value(const Int n_vars, Int sign = 1)
  {
    if (std::numeric_limits<Exponent>::has_infinity)
      return -sign * std::numeric_limits<Exponent>::infinity();
    else if (sign > 0)
      return std::numeric_limits<Exponent>::min();
    else
      return std::numeric_limits<Exponent>::max();
  }

  static void croak_if_incompatible(const monomial_type& m, const Int n_vars)
  {
    if (n_vars != 1)
      throw std::runtime_error("Monomial has different number of variables");
  }

  template <typename coefficient_type>
  static monomial_list_type monomials(const Int n_vars, const Int n_terms,
                                      const hash_map<monomial_type, coefficient_type>& h)
  {
    return monomial_list_type(n_terms, attach_operation(h, BuildUnary<operations::take_first>()).begin());
  }

  template <typename Coefficient, typename Output>
  static void pretty_print(Output& out, const monomial_type& m, 
                           const Coefficient& default_coefficient, const PolynomialVarNames& names)
  {
    if (equals_to_default(m)) {
      out << default_coefficient;  // constant monomial
      return;
    }
    out << names(0,1);
    if (!is_one(m)) out << '^' << m;
  }

  static typename homogenized_type::monomial_type homogenize(const monomial_type& m, Int new_variable_index, const Exponent& to_degree)
  {
    typename homogenized_type::monomial_type result(2);
    result[new_variable_index != 0] = to_degree - m;
    result[new_variable_index == 0] = m;
    return result;
  }
};

template <typename Exponent>
struct MultivariateMonomial {
  template <typename exp>
  using new_type = MultivariateMonomial<exp>;
  using exponent_type = Exponent;
  using monomial_type = SparseVector<Exponent>;
  using monomial_list_type = Matrix<Exponent>;
  using homogenized_type = MultivariateMonomial<Exponent>;
  static Exponent deg(const monomial_type& m) { return accumulate(m, operations::add<Exponent, Exponent>()); }
  static monomial_type default_value(const Int n_vars) { return monomial_type(n_vars); }
  static bool equals_to_default(const monomial_type& m) { return m.empty(); }

  static monomial_type empty_value(const Int n_vars, Int sign = 1)
  {
    return same_element_vector<Exponent>(UnivariateMonomial<Exponent>::empty_value(n_vars, sign), n_vars);
  }

  static void croak_if_incompatible(const monomial_type& m, const Int n_vars)
  {
    if (n_vars != m.dim())
      throw std::runtime_error("Monomial has different number of variables");
  }

  template <typename Coefficient>
  static monomial_list_type monomials(const Int n_vars, const Int n_terms,
                                      const hash_map<monomial_type, Coefficient>& h)
  {
    return monomial_list_type(n_terms, n_vars, 
                              attach_operation(h, BuildUnary<operations::take_first>()).begin());
  }

  template <typename Output, typename Coefficient>
  static void pretty_print(Output& out, const monomial_type& m, 
                           const Coefficient& default_coefficient, const PolynomialVarNames& names)
  {
    if (m.empty()) {
      out << default_coefficient; // constant monomial
      return;
    }

    bool first = true;
    for (auto it=m.begin(); !it.at_end(); ++it) {
      if (first)
        first = false;
      else
        out << '*';
      out << names(it.index(), m.dim());
      if (!is_one(*it)) out << '^' << *it;
    }
  }

  static typename homogenized_type::monomial_type
  homogenize(const monomial_type& m, Int new_variable_index, const Exponent& to_degree)
  {
    monomial_type result(m.dim()+1);
    result[new_variable_index] = to_degree - deg(m);
    result.slice(~scalar2set(new_variable_index)) = m;
    return result;
  }
};

template <typename T>
std::enable_if_t<is_field<T>::value, bool>
is_minus_one(const T& x)
{
  return is_one(-x);
}

template <typename T>
std::enable_if_t<!is_field<T>::value, bool>
is_minus_one(const T& x)
{
  return false;
}

// The generic implementation of a polynomial.

template <typename Monomial, typename Coefficient>
class GenericImpl {

  template <typename> friend struct pm::spec_object_traits;
  template <typename, typename> friend class GenericImpl;

public:
  using coefficient_type = Coefficient;
  using exponent_type = typename Monomial::exponent_type;
  using monomial_type = typename Monomial::monomial_type;
  using sorted_terms_type = typename std::forward_list<monomial_type>;
  using term_hash = hash_map<monomial_type, coefficient_type>;
  using monomial_list_type = typename Monomial::monomial_list_type;


  friend
     Div< UniPolynomial<Coefficient, exponent_type> >
     div<>(const UniPolynomial<Coefficient, exponent_type>& num, const UniPolynomial<Coefficient, exponent_type>& den);
  friend
     UniPolynomial<Coefficient, exponent_type>
     gcd<>(const UniPolynomial<Coefficient, exponent_type>& a, const UniPolynomial<Coefficient, exponent_type>& b);
  friend
     ExtGCD< UniPolynomial<Coefficient, exponent_type> >
     ext_gcd<>(const UniPolynomial<Coefficient, exponent_type>& a, const UniPolynomial<Coefficient, exponent_type>& b,
           bool normalize_gcd);
  friend
     UniPolynomial<Coefficient, exponent_type>
     lcm<>(const UniPolynomial<Coefficient, exponent_type>& a, const UniPolynomial<Coefficient, exponent_type>& b);

  static constexpr int coefficient_nesting_level = nesting_level<coefficient_type>::value;

  template <typename T>
  struct is_deeper_coefficient
    : deeper_coefficient_impl<T, coefficient_type> {};

  template <typename T>
  struct fits_as_coefficient
    : bool_constant<can_upgrade<T, coefficient_type>::value || is_deeper_coefficient<T>::value> {};

  explicit GenericImpl(const Int n_vars = 0)
    : n_variables(n_vars)
    , the_sorted_terms_set(false) {}

  template <typename T, typename = std::enable_if_t<fits_as_coefficient<T>::value>>
  GenericImpl(const T& c, const Int n_vars)
    : n_variables(n_vars)
    , the_sorted_terms_set(false)
  {
    if (__builtin_expect(!is_zero(c), 1)) {
      the_terms.emplace(Monomial::default_value(n_variables), static_cast<coefficient_type>(c));
    }
  }

  template <typename Container1, typename Container2>
  GenericImpl(const Container1& coefficients, const Container2& monomials, const Int n_vars)
    : n_variables(n_vars)
    , the_sorted_terms_set(false)
  {
    if (POLYMAKE_DEBUG) {
      if (static_cast<size_t>(monomials.size()) != static_cast<size_t>(coefficients.size()))
        throw std::runtime_error("Polynomial constructor: Numbers of monomials and coefficients don't match");
    }
    auto c = coefficients.begin();
    for (auto m = entire(monomials); !m.at_end(); ++m, ++c)
      add_term(*m, *c, std::false_type());
  }

  GenericImpl(const term_hash& src, const Int n_vars)
    : n_variables(n_vars)
    , the_terms(src)
    , the_sorted_terms_set(false) {}
               
  GenericImpl(const Int n_vars, const term_hash& src)
    : GenericImpl(src,n_vars) {}

  // Can't have this as constructor, might be ambiguous if coefficient_type = Int
  static GenericImpl fromMonomial(const monomial_type&m, const coefficient_type& c, const Int n_vars)
  {
    GenericImpl result(n_vars);
    result.the_terms.insert(m, c);
    return result;
  }

  static GenericImpl fromMonomial(const typename term_hash::const_iterator it, const Int n_vars)
  {
    GenericImpl result(n_vars);
    result.the_terms.insert(it->first, it->second);
    return result;
  }

  void clear()
  {
    the_terms.clear();
    forget_sorted_terms();
  }

  template <typename Other>
  void croak_if_incompatible(const Other& other) const
  {
    if (n_vars() != other.n_vars())
      throw std::runtime_error("Polynomials of different rings");
  }

  const Int& n_vars() const { return n_variables; }

  Int n_terms() const { return the_terms.size(); }

  const term_hash& get_terms() const { return the_terms; }
  term_hash& get_mutable_terms() { return the_terms; }
					
  bool trivial() const { return the_terms.empty(); }

  bool is_one() const
  {
    return the_terms.size()==1
        && Monomial::equals_to_default(the_terms.begin()->first)
        && pm::is_one(the_terms.begin()->second);
  }

  Vector<coefficient_type> coefficients_as_vector() const
  {
    return Vector<coefficient_type>(n_terms(),
                                    attach_operation(the_terms, BuildUnary<operations::take_second>()).begin());
  }

  monomial_list_type monomials() const
  {
    return Monomial::monomials(n_variables, n_terms(), the_terms);
  }

  bool operator== (const GenericImpl& p2) const
  {
    croak_if_incompatible(p2);
    return the_terms == p2.the_terms;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator== (const T& c) const
  {
    return trivial() && is_zero(c)
       || (the_terms.size()==1
           && Monomial::equals_to_default(the_terms.begin()->first)
           && the_terms.begin()->second==c);
  }

  // returns the coefficient of the monomial m or 0 iff m does not exists
  const coefficient_type& get_coefficient(const monomial_type& m) const
  {
    Monomial::croak_if_incompatible(m, n_variables);
    typename term_hash::const_iterator find = the_terms.find(m);
    if (find != the_terms.end())
      return find->second;
    return zero_value<coefficient_type>(); 
  }

  bool exists(const monomial_type& m) const
  {
    Monomial::croak_if_incompatible(m);
    return the_terms.exists(m);
  }

  typename Monomial::exponent_type deg() const
  {
    return Monomial::deg(lm());
  }

  typename Monomial::exponent_type lower_deg() const
  {
    typename Monomial::exponent_type low = Monomial::deg(Monomial::empty_value(n_variables, -1));
    for (auto it = entire(get_terms()); !it.at_end(); ++it)
      assign_min(low, Monomial::deg(it->first));
    return low;
  }

  // leading term
  GenericImpl lt() const
  {
    if (trivial())
      return fromMonomial(this->lm(), zero_value<coefficient_type>(), n_variables);
    else
      return fromMonomial(find_lex_lm(), n_variables);
  }

  template <typename TMatrix> inline
  GenericImpl lt(const GenericMatrix<TMatrix, exponent_type>& order) const
  {
    if (trivial())
      return fromMonomial(this->lm(order), zero_value<coefficient_type>(), n_variables);
    else
      return fromMonomial(find_lm(cmp_monomial_ordered<TMatrix>(order.top())), n_variables);
  }

  GenericImpl lt(const exponent_type& order) const
  {
    if (trivial())
      return fromMonomial(this->lm(order), zero_value<coefficient_type>(), n_variables);
    else
      return fromMonomial(find_lm(cmp_monomial_ordered<exponent_type>(order)), n_variables);
  }

  //! Return the leading monomial.
  monomial_type lm() const
  {
    if (trivial())
      return Monomial::empty_value(n_variables);
    else
      return find_lex_lm()->first;
  }

  template <typename TMatrix>
  monomial_type lm(const GenericMatrix<TMatrix, exponent_type>& order) const
  {
    if (trivial())
      return monomial_type::empty_value(n_variables);
    else
      return find_lm(cmp_monomial_ordered<TMatrix>(order.top()))->first;
  }

  monomial_type lm(const exponent_type& order) const
  {
    if (trivial())
      return monomial_type::empty_value(n_variables);
    else
      return find_lm(cmp_monomial_ordered<exponent_type>(order))->first;
  }

  //! Return the leading coefficient.
  const coefficient_type& lc() const
  {
    if (trivial())
      return zero_value<coefficient_type>();
    else
      return find_lex_lm()->second;
  }

  template <typename TMatrix>
  const coefficient_type& lc(const GenericMatrix<TMatrix, exponent_type>& order) const
  {
    if (trivial())
      return zero_value<coefficient_type>();
    else
      return find_lm(cmp_monomial_ordered<TMatrix>(order.top()))->second;
  }

  const coefficient_type& lc(const exponent_type& order) const
  {
    if (trivial())
      return zero_value<coefficient_type>();
    else
      return find_lm(cmp_monomial_ordered<exponent_type>(order))->second;
  }

  template <typename TVector>
  static const auto cmp_ordered_function(const GenericVector<TVector, exponent_type>& weights)
  {
    return cmp_monomial_ordered<TVector,false>(weights.top());
  }

  static const auto cmp_ordered_function(const exponent_type& weights)
  {
    return cmp_monomial_ordered<exponent_type>(weights);
  }

  template <typename TWeights>
  GenericImpl initial_form(const TWeights& weights) const
  {
    const auto cmp_ordered = cmp_ordered_function(weights);
    typename term_hash::const_iterator it=the_terms.begin(), max_term=it, end=the_terms.end();
    std::list<typename term_hash::const_iterator> if_list;
    if (it != end) {
      if_list.push_back(max_term);
      while (++it != end) {
        cmp_value c = cmp_ordered(it->first, max_term->first);
        if (c == cmp_gt) {
          max_term = it;
          if_list.clear();
          if_list.push_back(it);
        } else if (c == cmp_eq) {
          if_list.push_back(it);
        }
      }
    }
    GenericImpl in_form(n_variables);
    // this is an iterator over iterators
    for (auto&& termit : if_list)
      in_form.add_term(termit->first, termit->second, std::true_type());
    return in_form;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, GenericImpl>
  operator* (const T& c) const
  {
    if (__builtin_expect(is_zero(c), 0))
      return GenericImpl(n_variables);
    GenericImpl prod(n_variables, the_terms);
    return prod *= c;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, GenericImpl>
  mult_from_right(const T& c) const
  {
    if (__builtin_expect(is_zero(c), 0))
      return GenericImpl(n_variables);
    GenericImpl prod(n_variables, the_terms);
    for (auto& term : prod.the_terms)
      term.second = c*term.second;
    return prod;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, GenericImpl&>
  operator*= (const T& c)
  {
    if (__builtin_expect(is_zero(c), 0)) {
      clear();
    } else {
      for (auto& term : the_terms)
        term.second *= c;
    }
    return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, GenericImpl>
  operator/ (const T& c) const
  {
    if (__builtin_expect(is_zero(c), 0)) throw GMP::ZeroDivide();
    GenericImpl prod(n_variables, the_terms);
    return prod /= c;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, GenericImpl&>
  operator/= (const T& c)
  {
    if (__builtin_expect(is_zero(c), 0)) throw GMP::ZeroDivide();
    for (auto& term : the_terms)
      term.second /= c;
    return *this;
  }

  GenericImpl div_exact(const GenericImpl& b)
  {
    GenericImpl quot;
    remainder(b, quot);
    return quot;
  }

  GenericImpl& operator%= (const GenericImpl& b)
  {
      quot_black_hole black_hole;
      remainder(b, black_hole);
      return *this;
  }

  //! Divide by the coefficient of the leading monomial
  GenericImpl& normalize()
  {
    if (!trivial()) {
      const coefficient_type lead=lc();
      *this /= lead;
    }
    return *this; 
  }

  GenericImpl<typename Monomial::homogenized_type, Coefficient> homogenize(Int new_variable_index = 0) const
  {
    const typename Monomial::exponent_type degree = deg();
    hash_map<typename Monomial::homogenized_type::monomial_type, Coefficient> result_hash;
    for (const auto& term : the_terms) {
      result_hash.insert( Monomial::homogenize( term.first, new_variable_index, degree), term.second);
    }
    return GenericImpl<typename Monomial::homogenized_type, Coefficient>(n_variables+1, result_hash);
  }

  GenericImpl operator* (const GenericImpl& p2) const
  {
    croak_if_incompatible(p2);
    GenericImpl prod(n_variables);
    for (const auto& term1 : the_terms)
      for (const auto& term2 : p2.the_terms)
        prod.add_term(term1.first + term2.first, term1.second * term2.second, std::true_type());

    return prod;
  }

  GenericImpl& operator*= (const GenericImpl& p)
  {
    *this = (*this) * p;
    return *this;
  }

  template <typename E>
  std::enable_if_t<std::numeric_limits<E>::is_integer, GenericImpl>
  exponentiate_monomial(const E& exp) const
  {
    if (the_terms.size() != 1)
      throw std::runtime_error("exponentiate_monomial: invalid term number");
    const auto& t = *(the_terms.begin());
    GenericImpl result(n_variables);
    result.the_terms.emplace(monomial_type(t.first * exp), pm::pow(t.second,exp));
    return result;
  }

  template <typename E>
  std::enable_if_t<!std::numeric_limits<E>::is_integer, GenericImpl>
  exponentiate_monomial(const E& exp) const
  {
    if (the_terms.size() != 1)
      throw std::runtime_error("exponentiate_monomial: invalid term number");
    const auto& t = *(the_terms.begin());
    if (t.second != one_value<coefficient_type>())
      throw std::runtime_error("Except for integers, Exponentiation is only implemented for normalized monomials");
    GenericImpl result(n_variables);
    result.the_terms.emplace(monomial_type(t.first * exp), t.second);
    return result;
  }

  template <typename E>
  std::enable_if_t<!std::numeric_limits<E>::is_integer, GenericImpl>
  pow(const E& exp) const
  {
    return exponentiate_monomial(exp);
  }

  template <typename E>
  std::enable_if_t<std::numeric_limits<E>::is_integer, GenericImpl>
  pow(const E& exp) const
  {
    if (exp < 0)
      return exponentiate_monomial(exp);
    if (exp == 1)
      return GenericImpl(*this);

    GenericImpl result(one_value<coefficient_type>(), n_variables);
    if (exp != 0) {
      long e = exp;
      GenericImpl pow2(*this);
      for (;;) {
        if (e & 1) {
          result *= pow2;
        }
        if (e /= 2) {
          pow2 *= pow2;
        } else {
          break;
        }
      }
    }
    return result;
  }

  template <typename Exp=exponent_type, typename T>
  auto substitute_monomial(const T& exponent) const
  {
    typedef typename Monomial::template new_type<Exp> mon;
    GenericImpl<mon,Coefficient> tmp(n_variables);
    for (const auto& term : the_terms)
      tmp.the_terms.emplace(typename mon::monomial_type(convert_to<Exp>(term.first * exponent)), term.second);
    return tmp;
  }

  GenericImpl operator- () const
  {
    GenericImpl result(n_variables, the_terms);
    return result.negate();
  }

  GenericImpl& negate()
  {
    for (auto& term : the_terms)
      pm::negate(term.second);
    return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, GenericImpl>
  operator+ (const T& c) const
  {
    GenericImpl sum(n_variables, the_terms);
    return sum += c;
  }

  GenericImpl& operator+= (const coefficient_type& c)
  {
    if (__builtin_expect(!is_zero(c), 1))
      add_term(Monomial::default_value(n_variables), c, std::true_type());
    return *this;
  }

  template <typename T>
  std::enable_if_t<is_deeper_coefficient<T>::value, GenericImpl&>
  operator+= (const T& c)
  {
    return operator+=(is_deeper_coefficient<T>::construct(c, n_variables));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, GenericImpl>
  operator- (const T& c) const
  {
    GenericImpl diff(n_variables, the_terms);
    return diff -= c;
  }

  GenericImpl& operator-= (const coefficient_type& c)
  {
    if (__builtin_expect(!is_zero(c), 1))
      sub_term(Monomial::default_value(n_variables), c, std::true_type());
    return *this;
  }

  template <typename T>
  std::enable_if_t<is_deeper_coefficient<T>::value, GenericImpl&>
  operator-= (const T& c)
  {
    return operator-=(is_deeper_coefficient<T>::construct(c, n_variables));
  }

  GenericImpl operator+ (const GenericImpl& p) const
  {
    GenericImpl sum(n_variables, the_terms);
    return sum+=p;
  }

  GenericImpl& operator+= (const GenericImpl& p)
  {
    croak_if_incompatible(p);
    for (const auto& term : p.the_terms)
      add_term(term.first, term.second, std::true_type());
    return *this;
  }

  GenericImpl operator- (const GenericImpl& p) const
  {
    GenericImpl diff(n_variables, the_terms);
    return diff-=p;
  }

  GenericImpl& operator-= (const GenericImpl& p)
  {
    croak_if_incompatible(p);
    for (const auto& t : p.the_terms)
      sub_term(t.first, t.second, std::true_type());
    return *this;
  }

  static PolynomialVarNames& var_names()
  {
    // at the beginning, the default naming scheme is established
    static PolynomialVarNames names(nesting_level<coefficient_type>::value);
    return names;
  }

  static void reset_var_names()
  {
    var_names()=PolynomialVarNames(nesting_level<coefficient_type>::value);
  }

  // Printing the polynomial
  template <typename Output, typename Order>
  void pretty_print(Output& out, const Order& order) const
  {
    // this list will carry the sorted terms except in lex
    sorted_terms_type temp;
    const sorted_terms_type& sorted_terms = std::is_same<Order, cmp_monomial_ordered_base<exponent_type>>::value ? get_sorted_terms() : get_sorted_terms(temp, order);
    bool first = true;
    for (const auto& tp : sorted_terms) {
      auto term = the_terms.find(tp); 
      if (first)
        first = false;
      else if (needs_plus(term->second))
        out << " + ";
      else
        out << ' ';

      pretty_print_term(out, term->first, term->second);
    }
    if (first) out << zero_value<coefficient_type>();
  }

  // Printing a term
  template <typename Output> static
  void pretty_print_term(Output& out, const monomial_type& m, const coefficient_type& c)
  {
    if (!pm::is_one(c)) {
      if (is_minus_one(c)) {
        out << "- ";
      } else {
        pretty_print_coefficient(out, c, bool_constant<coefficient_nesting_level==0>());
        if (Monomial::equals_to_default(m)) return;
        out << '*';
      }
    }
    Monomial::pretty_print(out, m, one_value<coefficient_type>(), var_names());
  }

  template <typename Output> static
  void pretty_print_coefficient(Output& out, const coefficient_type& c, bool_constant<true>)
  {
    out << c;
  }

  template <typename Output> static
  void pretty_print_coefficient(Output& out, const coefficient_type& c, bool_constant<false>)
  {
    out << '(' << c << ')';
  }

  //! compare term-wise with respect of the given ordering
  template <typename Comparator>
  cmp_value compare_ordered(const GenericImpl& p, const Comparator& cmp_order) const
  {
    croak_if_incompatible(p);
    if (trivial()) return p.trivial() ? cmp_eq : cmp_lt;
    if (p.trivial()) return cmp_gt;

    // this list will carry the sorted terms except in lex
    sorted_terms_type t1, t2;

    const sorted_terms_type& fst = std::is_same<Comparator, cmp_monomial_ordered_base<exponent_type> >::value ?   get_sorted_terms() :   get_sorted_terms(t1, cmp_order);
    const sorted_terms_type& snd = std::is_same<Comparator, cmp_monomial_ordered_base<exponent_type> >::value ? p.get_sorted_terms() : p.get_sorted_terms(t2, cmp_order);

    auto it1 = fst.begin(), 
         it2 = snd.begin();

    while (it1 != fst.end() && it2 != snd.end()) {
      auto it_term1=the_terms.find(*it1),
           it_term2=p.the_terms.find(*it2);
      if (POLYMAKE_DEBUG) {
        if (it_term1 == the_terms.end()) {
          cerr << "Polynomial:\n" << the_terms << "\nSorted terms:\n" << fst << "\n";
          throw std::runtime_error("wrong 1st sorted term sequence");
        }
        if (it_term2 == p.the_terms.end()) {
          cerr << "Polynomial:\n" << p.the_terms << "\nSorted terms:\n" << snd << "\n";
          throw std::runtime_error("wrong 2nd sorted term sequence");
        }
      }

      cmp_value cmp_terms = compare_terms(*it_term1, *it_term2, cmp_monomial_ordered_base<exponent_type>()); 
      if (cmp_terms != cmp_eq) return cmp_terms;
      ++it1;
      ++it2;
    }

    if (it1 == fst.end()) {
      return it2 == snd.end() ? cmp_eq : cmp_lt;
    }
    return cmp_gt;
  }

  template <typename Comparator, typename TermType> static
  cmp_value compare_terms(const TermType& t1, const TermType& t2, const Comparator& cmp_order)
  {
    const cmp_value cmp_monom = cmp_order(t1.first, t2.first);
    return cmp_monom != cmp_eq ? cmp_monom :
           operations::cmp()(t1.second, t2.second);
  }

  static bool needs_plus(const coefficient_type& c) 
  {
    // in particular gives false for Polynomial and TropicalNumber
    return needs_plus(c, is_field<coefficient_type>() );
  }

  //! compare lexicographically
  cmp_value compare(const GenericImpl& p) const
  {
    return compare_ordered(p, cmp_monomial_ordered_base<exponent_type>());
  }

  size_t get_hash() const noexcept
  {
    return hash_func<Int>()(n_variables) * hash_func<term_hash>()(the_terms);
  }

  template <typename Order> static 
  auto get_sorting_lambda(const Order& cmp_order)
  {
    return [cmp_order] (monomial_type a, monomial_type b) { return cmp_order(a,b) == cmp_gt; };
  }

  // returns a list containing the exponents ordered by lex
  const sorted_terms_type& get_sorted_terms() const
  {
    if (the_sorted_terms_set) return the_sorted_terms;
    for (const auto& term : the_terms) {
      the_sorted_terms.push_front(term.first);
    }
    the_sorted_terms.sort(get_sorting_lambda(cmp_monomial_ordered_base<exponent_type>())); 
    the_sorted_terms_set = true;
    return the_sorted_terms;
  }

  // returns a list containing the exponents ordered by cmp_order 
  template<typename Order>
  const sorted_terms_type& get_sorted_terms(sorted_terms_type& sort, const Order& cmp_order) const
  {
    for (const auto& term : the_terms) {
      sort.push_front(term.first);
    }
    sort.sort(get_sorting_lambda(cmp_order));
    return sort;
  }

  bool terms_sorted() const
  {
    return the_sorted_terms_set;
  }

  // find the leading term with respect of the lexicographic order
  // Constant time, if terms have be sorted, else linear
  typename term_hash::const_iterator find_lex_lm() const
  {
    if (!trivial()) {
      return terms_sorted() ? the_terms.find(*(get_sorted_terms().begin())) : find_lm(cmp_monomial_ordered_base<exponent_type>());
    } else {
      return the_terms.end();
    }
  }

  template <typename Comparator>
  typename term_hash::const_iterator find_lm(const Comparator& cmp_order) const
  {
    auto it=the_terms.begin(), lt_it=it, end=the_terms.end();
    if (it != end) {
      while (++it != end)
        if (cmp_order(it->first, lt_it->first) == cmp_gt)
          lt_it=it;
    }
    return lt_it;
  }

private:
  struct quot_black_hole
  {
    template <bool trusted>
    void add_term(const monomial_type&, const Coefficient&, bool_constant<trusted>) const {}
  };

    // replace this with a remainder of division by b, consume the quotient
  // data must be brought in exclusive posession before calling this method.
  template <typename QuotConsumer>
  void remainder(const GenericImpl& b, QuotConsumer& quot_consumer)
  {
    const auto b_lead=b.find_lex_lm();
    typename term_hash::const_iterator this_lead;

    while ((this_lead=find_lex_lm()) != get_mutable_terms().cend() && this_lead->first >= b_lead->first) {
      const Coefficient k = this_lead->second / b_lead->second;
      const monomial_type d = this_lead->first - b_lead->first;
      quot_consumer.add_term(d, k, std::false_type());
      forget_sorted_terms();

      for (const auto& b_term : b.get_terms()) {
        auto it = get_mutable_terms().find_or_insert(b_term.first + d);
        if (it.second) {
          it.first->second= -k*b_term.second;
        } else if (is_zero(it.first->second -= k*b_term.second)) {
          get_mutable_terms().erase(it.first);
        }
      }
    }
  }

  // replace this with a remainder of division by b, consume the quotient
  // data must be brought in exclusive posession before calling this method.
  template <typename QuotConsumer>
  void remainder(const monomial_type& b, QuotConsumer& quot_consumer)
  {
    for (auto it=the_terms.begin(), end=the_terms.end();  it != end;  ) {
      if (it->first < b) {
        ++it;
      } else {
        if (!std::is_same<QuotConsumer, quot_black_hole>::value)
          quot_consumer.add_term(it->first - b, it->second, std::false_type());
        the_terms.erase(it++);
      }
    }
    forget_sorted_terms();
  }

private:

  template <typename T, bool trusted>
  void add_term(const monomial_type& m, T&& c, bool_constant<trusted>)
  {
    if (!trusted && __builtin_expect(is_zero(c), 0)) return;

    forget_sorted_terms();
    auto it = the_terms.find_or_insert(m);
    if (it.second)
      it.first->second=std::forward<T>(c);
    else if (is_zero(it.first->second += c))
      the_terms.erase(it.first);
  }

  template <typename T, bool trusted>
  void sub_term(const monomial_type& m, T&& c, bool_constant<trusted>)
  {
    if (!trusted && __builtin_expect(is_zero(c), 0)) return;

    forget_sorted_terms();
    auto it = the_terms.find_or_insert(m);
    if (it.second)
      it.first->second=-std::forward<T>(c);
    else if (is_zero(it.first->second -= c))
      the_terms.erase(it.first);
  }

  static bool needs_plus(const coefficient_type& c, std::true_type) { return c >= zero_value<coefficient_type>(); }
  static bool needs_plus(const coefficient_type&, std::false_type) { return true; }

  void forget_sorted_terms()
  {
    if (the_sorted_terms_set) {
      the_sorted_terms.clear();
      the_sorted_terms_set=false;
    }
  }

protected:
  Int n_variables;
  term_hash the_terms;
  // terms ordered by lex termorder 
  mutable sorted_terms_type the_sorted_terms;
  // true if sorted_terms has a valid value
  mutable bool the_sorted_terms_set;
};


template <typename Monomial, typename Coefficient>
struct impl_chooser {
  typedef GenericImpl< Monomial, Coefficient> type;
};

} //end namespace polynomial_impl 

} //end namespace pm

