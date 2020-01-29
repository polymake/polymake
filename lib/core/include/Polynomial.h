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

#ifndef POLYMAKE_POLYNOMIAL_H
#define POLYMAKE_POLYNOMIAL_H

#include "polymake/PolynomialVarNames.h"
#include "polymake/PolynomialImpl.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"

#ifdef POLYMAKE_WITH_FLINT
#include "polymake/FlintPolynomial.h"

namespace pm {
namespace polynomial_impl {

using FlintImplQ = FlintPolynomial;

template <>
struct impl_chooser<polynomial_impl::UnivariateMonomial<Int>, Rational> {
  typedef FlintImplQ type;
};

} }

#endif

namespace pm {

template <typename Coefficient = Rational, typename Exponent = Int>
class UniPolynomial {
  static_assert(!std::is_same<Exponent, int>::value, "use Int instead");
  friend class RationalFunction<Coefficient, Exponent>;
  template <typename> friend struct spec_object_traits;
public:
  typedef typename polynomial_impl::impl_chooser< polynomial_impl::UnivariateMonomial<Exponent>, Coefficient>::type impl_type;
  typedef typename impl_type::monomial_type monomial_type;
  typedef Coefficient coefficient_type;
  typedef typename impl_type::term_hash term_hash;
  typedef typename impl_type::monomial_list_type monomial_list_type;
  template <typename T>
  using fits_as_coefficient = typename impl_type::template fits_as_coefficient<T>;
  template <typename T>
  using is_deeper_coefficient = typename impl_type::template is_deeper_coefficient<T>;

  ~UniPolynomial() = default;
  UniPolynomial(UniPolynomial&&) = default;
  UniPolynomial& operator=(UniPolynomial&&) = default;
  UniPolynomial(const UniPolynomial& p)
    : impl_ptr{ std::make_unique<impl_type>(*p.impl_ptr) } {}

  UniPolynomial& operator=(const UniPolynomial& p)
  {
    impl_ptr = std::make_unique<impl_type>(*p.impl_ptr);
    return *this;
  }

  /// construct a copy
  explicit UniPolynomial(const impl_type& impl)
    : impl_ptr{std::make_unique<impl_type>(impl)} {}

  /// construct a zero polynomial
  UniPolynomial()
    : impl_ptr{std::make_unique<impl_type>(1)} {}

  /// construct a polynomial of degree 0
  template <typename T, typename = std::enable_if_t<fits_as_coefficient<T>::value>>
  explicit UniPolynomial(const T& c)
    : impl_ptr{std::make_unique<impl_type>(c,1)} {}

  /// construct a polynomial with a single term
  template <typename T, typename = std::enable_if_t<fits_as_coefficient<T>::value>>
  UniPolynomial(const T& c, const Exponent& exp)
    : UniPolynomial(same_element_vector(static_cast<Coefficient>(c), 1), same_element_vector(exp, 1)) {}

  template <typename Container1, typename Container2,
            typename = std::enable_if_t<(isomorphic_to_container_of<Container1, Coefficient>::value &&
                                         isomorphic_to_container_of<Container2, Exponent>::value)>>
  UniPolynomial(const Container1& coefficients, const Container2& monomials)
    : impl_ptr{std::make_unique<impl_type>(coefficients, monomials, 1)} {}

  explicit UniPolynomial(const term_hash& terms, Int nvars = 1)
    : impl_ptr{std::make_unique<impl_type>(terms,1)} {}

  /// construct a monomial of degree 1
  static UniPolynomial monomial() { return UniPolynomial(one_value<Coefficient>(), 1); }

  // Interface forwarding
  void swap(UniPolynomial& p) { impl_ptr.swap(p.impl_ptr); }
  void clear() { impl_ptr->clear(); }

  template <typename Other>
  void croak_if_incompatible(const Other& other) const
  {
    impl_ptr->croak_if_incompatible(other);
  }

  Int n_vars() const { return 1; }
  Int n_terms() const { return impl_ptr->n_terms(); }
  const term_hash& get_terms() const { return impl_ptr->get_terms(); }
  bool trivial() const { return impl_ptr->trivial(); }
  bool is_one() const { return impl_ptr->is_one(); }
  Vector<Coefficient> coefficients_as_vector() const { return impl_ptr->coefficients_as_vector(); }

  bool operator== (const UniPolynomial& p2) const { return impl_ptr->operator==(*p2.impl_ptr); }
  bool operator!= (const UniPolynomial& p2) const { return !operator==(p2); }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator== (const T& c) const { return impl_ptr->operator==(c); }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator!= (const T& c) const { return !operator==(c); }

  template <typename T> friend
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator==(const T&c, const UniPolynomial& p) { return p == c; }

  template <typename T> friend
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator!=(const T&c, const UniPolynomial& p) { return p != c; }

  Coefficient get_coefficient(const monomial_type& m) const { return impl_ptr->get_coefficient(m); }

  bool exists(const monomial_type& m) const { return impl_ptr->exists(m); }

  Exponent deg() const { return impl_ptr->deg(); }
  Exponent lower_deg() const { return impl_ptr->lower_deg(); }
  UniPolynomial lt() const { return UniPolynomial(impl_ptr->lt()); }

  UniPolynomial lt(const Exponent& order) const
  {
    return UniPolynomial(impl_ptr->lt(order));
  }

  monomial_type lm() const { return impl_ptr->lm(); }
  monomial_type lm(const Exponent& order) const { return impl_ptr->lm(order); }
  Coefficient lc() const { return impl_ptr->lc(); }
  Coefficient lc(const Exponent& order) const { return impl_ptr->lc(order); }
  Coefficient constant_coefficient() const { return impl_ptr->get_coefficient(0); }

  UniPolynomial initial_form(const Exponent& weights) const
  {
    return UniPolynomial(impl_ptr->initial_form(weights));
  }

  // compatibility with Polynomial zero for correct n_vars
  UniPolynomial zero() const
  {
     return zero_value<UniPolynomial>();
  }

  template <typename T>
  auto substitute(const T& t, std::enable_if_t<(std::is_same<Exponent, Int>::value &&
                                                std::is_same<typename object_traits<T>::generic_tag, is_scalar>::value),
                              std::nullptr_t> = nullptr) const
  {
    typename impl_type::sorted_terms_type temp = impl_ptr->get_sorted_terms();
    // using the return type of a product allows upgrades in both directions:
    // e.g. T=Int upgraded to coefficient type,
    //      or Coeff=Rational to T=QuadraticExtension
    using ret_type = pure_type_t<decltype(std::declval<T>() * std::declval<Coefficient>())>;
    ret_type result;
    Exponent previous_exp = this->deg();
    for (const auto& exp : temp) {
      while (previous_exp > exp) {
        result *= t;
        previous_exp--;
      }
      result += this->get_coefficient(exp);
    }
    result *= pm::pow(convert_to<ret_type>(t), previous_exp);
    return result;
  }

  template <typename T>
  auto substitute(const T& t, std::enable_if_t<(std::is_same<Exponent, Int>::value &&
                                                std::is_same<typename object_traits<T>::generic_tag, is_matrix>::value),
                              std::nullptr_t> = nullptr) const
  {
    if (POLYMAKE_DEBUG || is_wary<T>()) {
      if (t.rows() != t.cols())
        throw std::runtime_error("polynomial substitute: matrix must be square");
    }
    const auto& temp = impl_ptr->get_sorted_terms();
    Exponent previous_exp = this->deg();
    using ret_type = typename T::persistent_nonsymmetric_type;
    ret_type result(t.rows(), t.rows());
    for (const auto& exp : temp) {
      while (previous_exp > exp) {
        result = result * t;
        --previous_exp;
      }
      result += this->get_coefficient(exp) * unit_matrix<typename T::value_type>(t.rows());
    }
    result = result * pm::pow<ret_type>(t, previous_exp);
    return result;
  }

  template <template <typename, typename> class T, typename TCoeff, typename TExp,
            typename = std::enable_if_t<
               std::is_same<Exponent, Int>::value &&
               std::is_same<typename object_traits<T<TCoeff, TExp>>::generic_tag, is_polynomial>::value>>
  auto
  substitute(const T<TCoeff, TExp>& t) const
  {
    using ret_coeff = pure_type_t<decltype(std::declval<TCoeff>() * std::declval<Coefficient>())>;
    using ret_type = T<ret_coeff, TExp>;
    const auto& temp = impl_ptr->get_sorted_terms();
    Exponent previous_exp = this->deg();
    ret_type result = convert_to<ret_coeff>(t.zero());
    for (const auto& exp : temp) {
      while (previous_exp > exp) {
        result *= convert_to<ret_coeff>(t);
        --previous_exp;
      }
      result += ret_coeff(this->get_coefficient(exp));
    }
    result *= convert_to<ret_coeff>(t).pow(previous_exp);
    return result;
  }

  // more efficient variant of the above for substituting x^exp
  template <typename Exp, typename T>
  UniPolynomial<Coefficient, Exp> substitute_monomial(const T& exponent) const
  {
    return UniPolynomial<Coefficient, Exp>(impl_ptr->template substitute_monomial<Exp, T>(exponent));
  }

  template <typename T>
  std::enable_if_t<is_field_of_fractions<Exponent>::value && fits_as_coefficient<T>::value,
                   typename algebraic_traits<T>::field_type>
  evaluate(const T& t, const Int exp_lcm = 1) const
  {
    using field = typename algebraic_traits<T>::field_type;
    field res;
    for (const auto& term : get_terms())
    {
      const Exponent exp = exp_lcm * term.first;
      if (denominator(exp) != 1)
        throw std::runtime_error("Exponents non-integral, larger exp_lcm needed.");
      res += term.second * field::pow(t, static_cast<long>(exp));
    }
    return res;
  }

  template <typename T>
  std::enable_if_t<std::numeric_limits<Exponent>::is_integer && fits_as_coefficient<T>::value,
                   typename algebraic_traits<T>::field_type>
  evaluate(const T& t, const Int exp_lcm = 1) const
  {
    using field = typename algebraic_traits<T>::field_type;
    if (exp_lcm == 1)
       return substitute<field>(field::pow(t, exp_lcm));
    else
       return substitute<field>(t);
  }

  double evaluate_float(const double a) const
  {
    double res = 0;
    for (const auto& term : get_terms())
    {
      // we do the terms separately here to keep it working for rational exponents
      res += convert_to<double>(term.second) * std::pow(a, convert_to<double>(term.first));
    }
    return res;
  }

  /*! Perform the polynomial division and assign the quotient to *this.
   *  Like in the free function of the same name, you can pass an arbitrary polynomial to this method,
   *  not only a factor of *this.
   */
  UniPolynomial& div_exact(const UniPolynomial& b)
  {
    croak_if_incompatible(b);
    if (b.trivial()) throw GMP::ZeroDivide();

    impl_ptr = std::make_unique<impl_type>(impl_ptr->div_exact(*b.impl_ptr));
    return *this;
  }

  UniPolynomial operator% (const UniPolynomial& b) const
  {
    UniPolynomial tmp(*this);
    return tmp %= b;
  }

  UniPolynomial& operator%= (const UniPolynomial& b)
  {
    croak_if_incompatible(b);
    if (b.trivial()) throw GMP::ZeroDivide();
    if (!trivial()) {
       *impl_ptr %= *b.impl_ptr;
    }
    return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial>
  operator*(const T& c) const
  {
    return UniPolynomial(impl_ptr->operator*(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial>
  mult_from_right(const T& c) const
  {
    return UniPolynomial(impl_ptr->mult_from_right(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial&>
  operator*= (const T& c)
  {
    impl_ptr->operator*=(c);
    return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial>
  operator/ (const T& c) const
  {
    return UniPolynomial(impl_ptr->operator/(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial&>
  operator/= (const T& c)
  {
    impl_ptr->operator/=(c);
    return *this;
  }

  Polynomial<Coefficient, Exponent> homogenize(Int new_variable_index = 0) const
  {
    return Polynomial<Coefficient, Exponent>(impl_ptr->homogenize(new_variable_index));
  }

  UniPolynomial& normalize()
  {
    impl_ptr->normalize();
    return *this;
  }

  UniPolynomial operator* (const UniPolynomial& p2) const
  {
    return UniPolynomial(impl_ptr->operator*(*p2.impl_ptr));
  }

  template <typename E>
  UniPolynomial pow(const E& exp) const
  {
    return UniPolynomial(impl_ptr->pow(exp));
  }

  template <typename E>
  UniPolynomial operator^(const E& exp) const
  {
    return pow(exp);
  }

  template <typename E>
  UniPolynomial& operator^= (const E& exp)
  {
    *this = pow(exp);
    return *this;
  }

  UniPolynomial& operator*= (const UniPolynomial& p2)
  {
    impl_ptr->operator*=(*p2.impl_ptr); return *this;
  }

  UniPolynomial operator-() const
  {
    return UniPolynomial(impl_ptr->operator-());
  }

  UniPolynomial& negate()
  {
    impl_ptr->negate();
    return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial>
  operator+ (const T& c) const
  {
    return UniPolynomial(impl_ptr->operator+(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial&>
  operator+= (const T& c)
  {
    impl_ptr->operator+=(c);
    return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial>
  operator- (const T& c) const
  {
    return UniPolynomial(impl_ptr->operator-(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, UniPolynomial&>
  operator-= (const T& c)
  {
    impl_ptr->operator-=(c);
    return *this;
  }

  UniPolynomial operator+ (const UniPolynomial& p) const
  {
    return UniPolynomial(impl_ptr->operator+(*p.impl_ptr));
  }

  UniPolynomial& operator+= (const UniPolynomial& p)
  {
    impl_ptr->operator+=(*p.impl_ptr); return *this;
  }

  UniPolynomial operator- (const UniPolynomial& p) const
  {
    return UniPolynomial(impl_ptr->operator-(*p.impl_ptr));
  }

  UniPolynomial& operator-=(const UniPolynomial& p)
  {
    impl_ptr->operator-=(*p.impl_ptr);
    return *this;
  }

  template <typename Comparator>
  cmp_value compare_ordered(const UniPolynomial& p, const Comparator& cmp_order) const
  {
    return impl_ptr->compare_ordered(*p.impl_ptr, cmp_order);
  }

  cmp_value compare(const UniPolynomial& p) const
  {
    return impl_ptr->compare(*p.impl_ptr);
  }

  friend bool operator< (const UniPolynomial& p1, const UniPolynomial& p2)
  {
    return *p1.impl_ptr < *p2.impl_ptr;
  }

  friend bool operator> (const UniPolynomial& p1, const UniPolynomial& p2)
  {
    return *p1.impl_ptr > *p2.impl_ptr;
  }

  friend bool operator<= (const UniPolynomial& p1, const UniPolynomial& p2)
  {
    return *p1.impl_ptr <= *p2.impl_ptr;
  }

  friend bool operator>= (const UniPolynomial& p1, const UniPolynomial& p2)
  {
    return *p1.impl_ptr >= *p2.impl_ptr;
  }

#if POLYMAKE_DEBUG
  void dump() const __attribute__((used)) { cerr << *this << std::flush; }
#endif

  monomial_list_type monomials_as_vector() const { return impl_ptr->monomials(); }

  static
  const Array<std::string>& get_var_names()
  {
    return impl_type::var_names().get_names();
  }

  static
  void set_var_names(const Array<std::string>& names)
  {
    impl_type::var_names().set_names(names);
  }

  static
  void reset_var_names()
  {
    impl_type::reset_var_names();
  }

  static
  void swap_var_names(PolynomialVarNames& other_names)
  {
    impl_type::var_names().swap(other_names);
  }

  template <typename Output> friend
  Output& operator<< (GenericOutput<Output>& out, const UniPolynomial& me)
  {
    me.impl_ptr->pretty_print(out.top(), polynomial_impl::cmp_monomial_ordered_base<Exponent>());
    return out.top();
  }

  template <typename Output>
  void print_ordered(GenericOutput<Output>& out, const Exponent& order) const
  {
    impl_ptr->pretty_print(out.top(), polynomial_impl::cmp_monomial_ordered<Exponent>(order));
  }

  void print_ordered(const Exponent& order) const
  {
    print_ordered(cout, order);
    cout << std::flush;
  }

  size_t get_hash() const noexcept { return impl_ptr->get_hash(); }

  friend
  Div<UniPolynomial> div<>(const UniPolynomial& a, const UniPolynomial& b);

  friend
  UniPolynomial gcd<>(const UniPolynomial& a, const UniPolynomial& b);

  friend
  ExtGCD<UniPolynomial> ext_gcd<>(const UniPolynomial& a, const UniPolynomial& b, bool normalize_gcd);

  friend
  UniPolynomial lcm<>(const UniPolynomial& a, const UniPolynomial& b);

protected:
  std::unique_ptr<impl_type> impl_ptr;

  term_hash& get_mutable_terms() const { return impl_ptr->get_mutable_terms(); }

  impl_type& get_impl() {
      return *impl_ptr;
  }

  const impl_type& get_impl() const {
      return *impl_ptr;
  }

};

template <typename Coefficient, typename Exponent>
Div< UniPolynomial<Coefficient, Exponent> >
div(const UniPolynomial<Coefficient, Exponent>& num, const UniPolynomial<Coefficient, Exponent>& den)
{
  typedef typename UniPolynomial<Coefficient,Exponent>::impl_type impl_type;
  num.croak_if_incompatible(den);
  if (den.trivial()) throw GMP::ZeroDivide();
  Div< UniPolynomial<Coefficient, Exponent> > res;
  res.rem.impl_ptr=std::make_unique<impl_type>(*num.impl_ptr);
  res.rem.impl_ptr->remainder(*den.impl_ptr, *res.quot.impl_ptr);
  return res;
}

template <typename Coefficient, typename Exponent>
UniPolynomial<Coefficient, Exponent>
gcd(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b)
{
  a.croak_if_incompatible(b);
  if (a.trivial()) return b;
  if (b.trivial()) return a;

  const bool sw = a.lm() < b.lm();
  UniPolynomial<Coefficient, Exponent> p1(*(sw ? b : a).impl_ptr),
                                       p2(*(sw ? a : b).impl_ptr);

  typename polynomial_impl::GenericImpl<polynomial_impl::UnivariateMonomial<Exponent>,Coefficient>::quot_black_hole black_hole;
  while (!p2.trivial() && !is_zero(p2.lm())) {
    p1.impl_ptr->remainder(*p2.impl_ptr, black_hole);
    p1.swap(p2);
  }
  if (p2.trivial())
    return p1.normalize();
  else
    return UniPolynomial<Coefficient, Exponent>(one_value<Coefficient>());  // =1
}

template <typename Coefficient, typename Exponent>
ExtGCD< UniPolynomial<Coefficient, Exponent> >
ext_gcd(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b,
        bool normalize_gcd)
{
  a.croak_if_incompatible(b);

  typedef UniPolynomial<Coefficient, Exponent> XUPolynomial;

  ExtGCD<XUPolynomial> res;
  if (a.trivial()) {
    res.g=b;
    res.p=res.q=res.k2=XUPolynomial(one_value<Coefficient>());
    res.k1 = XUPolynomial();

  } else if (b.trivial()) {
    res.g=a;
    res.p=res.q=res.k1=XUPolynomial(one_value<Coefficient>());
    res.k2 = XUPolynomial();

  } else {
    XUPolynomial U[2][2]={ { XUPolynomial(one_value<Coefficient>()),  XUPolynomial() },
                           { XUPolynomial(),  XUPolynomial(one_value<Coefficient>()) } };

    const bool sw = a.lm() < b.lm();
    XUPolynomial p1(*(sw ? b : a).impl_ptr),
                 p2(*(sw ? a : b).impl_ptr),
                 k;

    for (;;) {
      k.clear();
      p1.impl_ptr->remainder(*p2.impl_ptr, *k.impl_ptr);
      // multiply U from left with { {1, -k}, {0, 1} }
      U[0][0] -= k*U[1][0];
      U[0][1] -= k*U[1][1];
      if (p1.trivial()) {
        res.g.swap(p2);
        res.p.swap(U[1][sw]);  res.q.swap(U[1][1-sw]);
        res.k2.swap(U[0][sw]);  res.k1.swap(U[0][1-sw]);
        (sw ? res.k2 : res.k1).negate();
        break;
      }

      k.clear();
      p2.impl_ptr->remainder(*p1.impl_ptr, *k.impl_ptr);
      // multiply U from left with { {1, 0}, {-k, 1} }
      U[1][0] -= k*U[0][0];
      U[1][1] -= k*U[0][1];
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


template <typename Coefficient, typename Exponent>
UniPolynomial<Coefficient, Exponent>
lcm(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b)
{
  const ExtGCD< UniPolynomial<Coefficient, Exponent> > x = ext_gcd(a, b);
  return a*x.k2;
}

/*! Perform the polynomial division, discarding the remainder.
 *  Although the name suggests that the divisor must be a factor of the divident,
 *  you can put arbitrary polynomials here.
 *  The name is rather chosen for compatibility with the Integer class.
 */
template <typename Coefficient, typename Exponent>
UniPolynomial<Coefficient, Exponent> div_exact(const UniPolynomial<Coefficient, Exponent>& a, const UniPolynomial<Coefficient, Exponent>& b)
{
  UniPolynomial<Coefficient, Exponent> tmp(a);
  return tmp.div_exact(b);
}


template <typename Coefficient = Rational, typename Exponent = Int>
class Polynomial {
  static_assert(!std::is_same<Exponent, int>::value, "use Int instead");
  template <typename> friend struct spec_object_traits;
public:
  typedef typename polynomial_impl::impl_chooser< polynomial_impl::MultivariateMonomial<Exponent>, Coefficient>::type impl_type;
  typedef typename impl_type::monomial_type monomial_type;
  typedef Coefficient coefficient_type;
  typedef typename impl_type::term_hash term_hash;
  typedef typename impl_type::monomial_list_type monomial_list_type;

  template <typename T>
  using fits_as_coefficient = typename impl_type::template fits_as_coefficient<T>;
  template <typename T>
  using is_deeper_coefficient = typename impl_type::template is_deeper_coefficient<T>;

  // for reading from perl::Value
  Polynomial() {}

  ~Polynomial() = default;
  Polynomial(Polynomial&&) = default;
  Polynomial& operator=(Polynomial&&) = default;
  Polynomial(const Polynomial& p)
    : impl_ptr{ std::make_unique<impl_type>(*p.impl_ptr)} {}

  Polynomial& operator=(const Polynomial& p)
  {
    impl_ptr = std::make_unique<impl_type>(*p.impl_ptr);
    return *this;
  }

  /// construct a copy
  explicit Polynomial(const impl_type& impl)
    : impl_ptr{std::make_unique<impl_type>(impl)} {}

  /// construct a zero polynomial with the given number of variables
  explicit Polynomial(const Int n_vars)
    : impl_ptr{std::make_unique<impl_type>(n_vars)} {}

  /// construct a polynomial of degree 0 with the given number of variables
  template <typename T, typename = std::enable_if_t<fits_as_coefficient<T>::value>>
  Polynomial(const T& c, const Int n_vars)
    : impl_ptr{std::make_unique<impl_type>(c, n_vars)} {}

  /// construct a polynomial with a single term
  template <typename T, typename TVector, typename = std::enable_if_t<fits_as_coefficient<T>::value>>
  Polynomial(const T& c, const GenericVector<TVector>& monomial)
    : Polynomial(same_element_vector(c, 1), vector2row(monomial)) {}

  template <typename Container, typename TMatrix, typename = std::enable_if_t<isomorphic_to_container_of<Container, Coefficient>::value>>
  Polynomial(const Container& coefficients, const GenericMatrix<TMatrix, Exponent>& monomials)
    : impl_ptr{std::make_unique<impl_type>(coefficients, rows(monomials), monomials.cols())} {}

  Polynomial(const term_hash& terms, const Int nvars)
    : impl_ptr{std::make_unique<impl_type>(terms,nvars)} {}

  /// construct a monomial of the given variable
  static Polynomial monomial(Int var_index, Int n_vars)
  {
    return Polynomial(one_value<Coefficient>(), unit_vector<Exponent>(n_vars, var_index));
  }

  // non-static zero with correct n_vars
  Polynomial zero() const
  {
     return Polynomial(this->n_vars());
  }

  // Interface forwarding
  void swap(Polynomial& p) { impl_ptr.swap(p.impl_ptr); }
  void clear() { impl_ptr->clear(); }

  template <typename Other>
  void croak_if_incompatible(const Other& other) const
  {
    impl_ptr->croak_if_incompatible(other);
  }

  const Int& n_vars() const { return impl_ptr->n_vars(); }
  Int n_terms() const { return impl_ptr->n_terms(); }
  const term_hash& get_terms() const { return impl_ptr->get_terms(); }
  bool trivial() const { return impl_ptr->trivial(); }
  bool is_one() const { return impl_ptr->is_one(); }

  Vector<Coefficient> coefficients_as_vector() const { return impl_ptr->coefficients_as_vector(); }

  bool operator== (const Polynomial& p2) const { return impl_ptr->operator==(*p2.impl_ptr); }
  bool operator!= (const Polynomial& p2) const { return !operator==(p2); }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator== (const T& c) const { return impl_ptr->operator==(c); }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator!= (const T& c) const { return !operator==(c); }

  template <typename T> friend
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator==(const T&c, const Polynomial& p) { return p == c; }

  template <typename T> friend
  std::enable_if_t<fits_as_coefficient<T>::value, bool>
  operator!=(const T&c, const Polynomial& p) { return p != c; }

  Coefficient get_coefficient(const monomial_type& m) const { return impl_ptr->get_coefficient(m); }
  bool exists(const monomial_type& m) const { return impl_ptr->exists(m); }

  Exponent deg() const { return impl_ptr->deg(); }
  Exponent lower_deg() const { return impl_ptr->lower_deg(); }

  Polynomial lt() const { return Polynomial(impl_ptr->lt()); }

  template <typename TMatrix>
  Polynomial lt(const GenericMatrix<TMatrix, Exponent>& order) const
  {
    return Polynomial(impl_ptr->lt(order));
  }

  monomial_type lm() const { return impl_ptr->lm(); }

  template <typename TMatrix>
  monomial_type lm(const GenericMatrix<TMatrix, Exponent>& order) const
  {
    return impl_ptr->lm(order);
  }

  Coefficient lc() const { return impl_ptr->lc(); }

  template <typename TMatrix>
  Coefficient lc(const GenericMatrix<TMatrix, Exponent>& order) const
  {
    return impl_ptr->lc(order);
  }

  template <typename TVector>
  Polynomial initial_form(const GenericVector<TVector, Exponent>& weights) const
  {
    return Polynomial(impl_ptr->initial_form(weights));
  }

  Coefficient constant_coefficient() const { return get_coefficient(monomial_type(n_vars())); }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial>
  operator*(const T& c) const
  {
    return Polynomial(impl_ptr->operator*(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial>
  mult_from_right(const T& c) const
  {
    return Polynomial(impl_ptr->mult_from_right(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial&>
  operator*=(const T& c)
  {
    impl_ptr->operator*=(c); return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial>
  operator/(const T& c) const
  {
    return Polynomial(impl_ptr->operator/(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial&>
  operator/=(const T& c)
  {
    impl_ptr->operator/=(c); return *this;
  }

  Polynomial<Coefficient, Exponent> homogenize(Int new_variable_index = 0) const
  {
    return Polynomial<Coefficient, Exponent>(impl_ptr->homogenize(new_variable_index));
  }

  Polynomial& normalize() { impl_ptr->normalize(); return *this; }

  Polynomial operator* (const Polynomial& p2) const
  {
    return Polynomial(impl_ptr->operator*(*p2.impl_ptr));
  }

  Polynomial& operator*= (const Polynomial& p2)
  {
    impl_ptr->operator*=(*p2.impl_ptr); return *this;
  }

  template <typename E>
  Polynomial pow(const E& exp) const
  {
    return Polynomial(impl_ptr->pow(exp));
  }

  template <typename E>
  Polynomial operator^(const E& exp) const
  {
    return pow(exp);
  }

  template <typename E>
  Polynomial& operator^=(const E& exp)
  {
    *this = pow(exp);
    return *this;
  }

  Polynomial operator-() const
  {
    return Polynomial(impl_ptr->operator-());
  }

  Polynomial& negate()
  {
    impl_ptr->negate();
    return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial>
  operator+(const T& c) const
  {
    return Polynomial(impl_ptr->operator+(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial&>
  operator+= (const T& c)
  {
    impl_ptr->operator+=(c); return *this;
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial>
  operator- (const T& c) const
  {
    return Polynomial(impl_ptr->operator-(c));
  }

  template <typename T>
  std::enable_if_t<fits_as_coefficient<T>::value, Polynomial&>
  operator-= (const T& c)
  {
    impl_ptr->operator-=(c); return *this;
  }

  Polynomial operator+ (const Polynomial& p) const
  {
    return Polynomial(impl_ptr->operator+(*p.impl_ptr));
  }

  Polynomial& operator+= (const Polynomial& p)
  {
    impl_ptr->operator+=(*p.impl_ptr);
    return *this;
  }

  Polynomial operator-(const Polynomial& p) const
  {
    return Polynomial(impl_ptr->operator-(*p.impl_ptr));
  }

  Polynomial& operator-=(const Polynomial& p)
  {
    impl_ptr->operator-=(*p.impl_ptr);
    return *this;
  }

  template <typename Comparator>
  cmp_value compare_ordered(const Polynomial& p, const Comparator& cmp_order) const
  {
    return impl_ptr->compare_ordered(*p.impl_ptr, cmp_order);
  }

  cmp_value compare(const Polynomial& p) const
  {
    return impl_ptr->compare(*p.impl_ptr);
  }

  friend
  bool operator< (const Polynomial& p1, const Polynomial& p2)
  {
    return p1.impl_ptr->compare(*p2.impl_ptr) == cmp_lt;
  }
  friend
  bool operator> (const Polynomial& p1, const Polynomial& p2)
  {
    return p1.impl_ptr->compare(*p2.impl_ptr) == cmp_gt;
  }
  friend
  bool operator<= (const Polynomial& p1, const Polynomial& p2)
  {
    return p1.impl_ptr->compare(*p2.impl_ptr) != cmp_gt;
  }
  friend
  bool operator>= (const Polynomial& p1, const Polynomial& p2)
  {
    return p1.impl_ptr->compare(*p2.impl_ptr) != cmp_lt;
  }

  template <typename Container>
  auto substitute(const Container& values,
                  std::enable_if_t<(std::is_same<Exponent, Int>::value &&
                                    std::is_same<typename object_traits<typename Container::value_type>::generic_tag,is_scalar>::value),
                                   std::nullptr_t> = nullptr) const
  {
    if (values.size() != n_vars())
       throw std::runtime_error("substitute polynomial: number of values does not match variables");
    using T = typename Container::value_type;
    // using the return type of a product allows upgrades in both directions:
    // e.g. T=Int upgraded to coefficient type,
    //      or Coeff=Rational to T=QuadraticExtension
    using ret_type = pure_type_t<decltype(std::declval<T>() * std::declval<Coefficient>())>;
    ret_type result;
    for (auto t = entire(this->get_terms()); !t.at_end(); ++t)
    {
      ret_type term(t->second);
      accumulate_in(entire(attach_operation(values,t->first, operations::pow<T, long>())), BuildBinary<operations::mul>(),term);
      result += term;
    }
    return result;
  }

  template <template <typename, typename...> class Container,
            template <typename, typename> class Poly,
            typename Coeff,
            typename Exp,
            typename... Args>
  auto
  substitute(const Container<Poly<Coeff, Exp>, Args...>& values,
             std::enable_if_t<(std::is_same<Exponent, Int>::value &&
                               std::is_same<typename object_traits<Poly<Coeff, Exp>>::generic_tag, is_polynomial>::value),
                              std::nullptr_t> = nullptr) const
  {
    if (values.size() != n_vars())
       throw std::runtime_error("substitute polynomial: number of values does not match variables");
    using ret_coeff = pure_type_t<decltype(std::declval<Coeff>() * std::declval<Coefficient>())>;
    using ret_type = Poly<ret_coeff, Exp>;
    ret_type result = convert_to<ret_coeff>(values.begin()->zero());
    for (auto t = entire(this->get_terms()); !t.at_end(); ++t)
    {
      ret_type term(t->second);
      accumulate_in(entire(attach_operation(values,t->first,operations::pow<ret_type, long>())), BuildBinary<operations::mul>(),term);
      result += term;
    }
    return result;
  }

  // more efficient variant of the above for substituting x^exp
  template <typename Exp = Exponent, typename T>
  Polynomial<Coefficient, Exp> substitute_monomial(const T& exponent) const
  {
    return Polynomial<Coefficient, Exp>(impl_ptr->template substitute_monomial<Exp, T>(exponent));
  }


  template <typename MapType>
  auto substitute(const MapType& values,
                  std::enable_if_t<(std::is_same<Exponent, Int>::value &&
                                    std::is_same<typename object_traits<MapType>::generic_tag, is_map>::value &&
                                    std::is_same<typename MapType::key_type, Int>::value &&
                                    std::is_same<typename object_traits<typename MapType::mapped_type>::generic_tag, is_scalar>::value),
                                   std::nullptr_t> = nullptr) const
  {
    using ret_coeff = pure_type_t<decltype(std::declval<typename MapType::mapped_type>() * std::declval<Coefficient>())>;
    Polynomial<ret_coeff, Int> result(this->n_vars());
    Set<Int> indices(keys(values));
    for (auto t = entire(this->get_terms()); !t.at_end(); ++t)
    {
      ret_coeff coeff(t->second);
      for (auto v = entire(values); !v.at_end(); ++v) {
         coeff *= pm::pow<ret_coeff>(v->second, t->first[v->first]);
      }
      SparseVector<Int> exps(t->first);
      exps.slice(indices) = zero_vector<Int>(indices.size());
      result += Polynomial<ret_coeff, Int>(coeff,exps);
    }
    return result;
  }

  template <typename Container,
            typename = std::enable_if_t<isomorphic_to_container_of<Container, Int>::value>>
  auto
  project(const Container& indices) const
  {
    return Polynomial<Coefficient, Exponent>(
        this->coefficients_as_vector(),
        this->monomials_as_matrix().minor(All, indices)
      );
  }

  template <typename Container,
            typename = std::enable_if_t<isomorphic_to_container_of<Container, Int>::value>>
  auto
  mapvars(const Container& indices, Int vars = -1) const
  {
    if (indices.size() != this->n_vars())
       throw std::runtime_error("polynomial mapvars: number of indices does not match variables");
    Int maxind = 0;
    for (auto i : indices)
       assign_max(maxind, i);
    if (vars != -1) {
       if (maxind+1 > vars)
          throw std::runtime_error("polynomial mapvars: indices exceed given number of variables");
    } else {
      vars = maxind+1;
    }
    SparseMatrix<Exponent> oldexps = this->monomials_as_matrix();
    SparseMatrix<Exponent> exps(this->n_terms(),vars);
    Int j = 0;
    for (auto i = entire(indices); !i.at_end(); ++i, ++j)
       exps.col(*i) += oldexps.col(j);
    return Polynomial<Coefficient, Exponent>(this->coefficients_as_vector(), exps);
  }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { cerr << *this << std::flush; }
#endif

  template <typename TMatrix = SparseMatrix<Exponent>>
  TMatrix monomials_as_matrix() const
  {
    return TMatrix(this->n_terms(), this->n_vars(),
                   entire(attach_operation(this->get_terms(), BuildUnary<operations::take_first>())));
  }

  static
  const Array<std::string>& get_var_names()
  {
    return impl_type::var_names().get_names();
  }

  static
  void set_var_names(const Array<std::string>& names)
  {
    impl_type::var_names().set_names(names);
  }

  static
  void reset_var_names()
  {
    impl_type::reset_var_names();
  }

  static
  void swap_var_names(PolynomialVarNames& other_names)
  {
    impl_type::var_names().swap(other_names);
  }

  template <typename Output> friend
  Output& operator<< (GenericOutput<Output>& out, const Polynomial& p)
  {
    p.impl_ptr->pretty_print(out.top(), polynomial_impl::cmp_monomial_ordered_base<Exponent>());
    return out.top();
  }

  template <typename Output, typename TMatrix>
  void print_ordered(GenericOutput<Output>& out, const GenericMatrix<TMatrix, Exponent>& order)
  {
    impl_ptr->pretty_print(out.top(), polynomial_impl::cmp_monomial_ordered<TMatrix>(order.top()));
  }

  template <typename TMatrix>
  void print_ordered(const GenericMatrix<TMatrix, Exponent>& order)
  {
    print_ordered(cout, order);
    cout << std::flush;
  }

  size_t get_hash() const { return impl_ptr->get_hash(); }

protected:
  std::unique_ptr<impl_type> impl_ptr;

  impl_type& get_impl() {
      return *impl_ptr;
  }

  const impl_type& get_impl() const {
      return *impl_ptr;
  }

};

template <typename Coefficient, typename Exponent>
struct is_gcd_domain< UniPolynomial<Coefficient, Exponent> >
  : is_field<Coefficient> {};

template <typename Coefficient, typename Exponent, bool has_gcd = is_gcd_domain<UniPolynomial<Coefficient, Exponent>>::value>
struct algebraic_traits_for_UniPolynomial {};

template <typename Coefficient, typename Exponent>
struct algebraic_traits_for_UniPolynomial<Coefficient, Exponent, true> {
  using field_type = RationalFunction<typename algebraic_traits<Coefficient>::field_type, Exponent>;
};

template <typename Coefficient, typename Exponent>
struct algebraic_traits<UniPolynomial<Coefficient, Exponent>> : algebraic_traits_for_UniPolynomial<Coefficient, Exponent> {};

template <typename Coefficient, typename Exponent, typename T>
using compatible_with_polynomial = bool_constant<(isomorphic_types<Coefficient, T>::value ||
                                                  Polynomial<Coefficient, Exponent>::template is_deeper_coefficient<T>::value)>;

template <typename Coefficient, typename Exponent, typename T>
using compatible_with_unipolynomial = bool_constant<(isomorphic_types<Coefficient, T>::value ||
                                                     UniPolynomial<Coefficient, Exponent>::template is_deeper_coefficient<T>::value)>;

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<Polynomial<Coefficient, Exponent>, T,
                             std::enable_if_t<compatible_with_polynomial<Coefficient, Exponent, T>::value, is_polynomial>,
                             TModel>
: std::false_type {
  typedef cons<is_polynomial, is_scalar> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<T, Polynomial<Coefficient, Exponent>, TModel,
                             std::enable_if_t<compatible_with_polynomial<Coefficient, Exponent, T>::value, is_polynomial>>
: std::false_type {
  typedef cons<is_scalar, is_polynomial> discriminant;
};

template <typename Coefficient, typename Exponent>
struct isomorphic_types_impl<Polynomial<Coefficient,Exponent>, Polynomial<Coefficient,Exponent>, is_polynomial, is_polynomial>
  : std::true_type {
  typedef cons<is_polynomial, is_polynomial> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<UniPolynomial<Coefficient, Exponent>, T,
                             std::enable_if_t<compatible_with_unipolynomial<Coefficient, Exponent, T>::value, is_polynomial>,
                             TModel>
: std::false_type {
  typedef cons<is_polynomial, is_scalar> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<T, UniPolynomial<Coefficient, Exponent>, TModel,
                             std::enable_if_t<compatible_with_unipolynomial<Coefficient, Exponent, T>::value, is_polynomial>>
: std::false_type {
  typedef cons<is_scalar, is_polynomial> discriminant;
};

template <typename Coefficient, typename Exponent>
struct isomorphic_types_impl<UniPolynomial<Coefficient,Exponent>, UniPolynomial<Coefficient,Exponent>, is_polynomial, is_polynomial>
  : std::true_type {
  typedef cons<is_polynomial, is_polynomial> discriminant;
};


template <typename Coefficient, typename Exponent>
struct choose_generic_object_traits< Polynomial<Coefficient, Exponent>, false, false >
  : spec_object_traits< Polynomial<Coefficient, Exponent> > {

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
    return p.is_one();
  }

  // FIXME Dirty hack to allow printing of zero and one values of multivariate polynomials
  // (for which the number of variables is entirely irrelevant)

  static
  const persistent_type& zero()
  {
    static const persistent_type x = persistent_type(1);
    return x;
  }

  static
  const persistent_type& one()
  {
    static const persistent_type x = persistent_type(one_value<Coefficient>(),1);
    return x;
  }
};

template <typename Coefficient, typename Exponent>
struct choose_generic_object_traits< UniPolynomial<Coefficient, Exponent>, false, false >
  : spec_object_traits< UniPolynomial<Coefficient, Exponent> > {
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
    return p.is_one();
  }

  static
  const persistent_type& zero()
  {
    static const persistent_type x = persistent_type();
    return x;
  }

  static
  const persistent_type& one()
  {
    static const persistent_type x(one_value<Coefficient>());
    return x;
  }

  static
  const persistent_type& variable() {
    static const persistent_type var(same_element_vector<Coefficient>(1,1), same_element_vector<Exponent>(1,1));
    return var;
  }
};

template <typename Coefficient, typename Exponent>
struct has_zero_value<Polynomial<Coefficient, Exponent>> : std::true_type {};

template <typename Coefficient, typename Exponent>
struct has_zero_value<UniPolynomial<Coefficient, Exponent>> : std::true_type {};

namespace polynomial_impl {

template <typename Coefficient, typename Exponent>
struct nesting_level<UniPolynomial<Coefficient, Exponent>>
  : int_constant<nesting_level<Coefficient>::value+1> {};

template <typename Coefficient, typename Exponent>
struct nesting_level<Polynomial<Coefficient, Exponent>>
  : int_constant<nesting_level<Coefficient>::value+1> {};

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

template <typename T>
struct cmp_polynomial {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef cmp_value result_type;

  template <typename Left, typename Right>
  cmp_value operator() (const Left& p1, const Right& p2) const
  {
    return p1.compare(p2);
  }
};

template <typename Coefficient, typename Exponent>
struct cmp_basic<Polynomial<Coefficient, Exponent>, Polynomial<Coefficient, Exponent>>
  : cmp_polynomial<Polynomial<Coefficient, Exponent>> {};

template <typename Coefficient, typename Exponent>
struct cmp_basic<UniPolynomial<Coefficient, Exponent>, UniPolynomial<Coefficient, Exponent>>
  : cmp_polynomial<UniPolynomial<Coefficient, Exponent>> {};

} // end namespace operations

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< UniPolynomial<Coefficient,Exponent> > >
  : spec_object_traits<is_composite> {

  using impl_type = typename UniPolynomial<Coefficient,Exponent>::impl_type;

  using masquerade_for = UniPolynomial<Coefficient, Exponent>;

  using terms_type = typename polynomial_impl::GenericImpl<polynomial_impl::UnivariateMonomial<Exponent>,Coefficient>::term_hash;

  using elements = terms_type;

  template <typename Visitor>
  static void visit_elements(Serialized<masquerade_for> & me, Visitor& v)
  {
    terms_type terms;
    v << terms;
    me.impl_ptr = std::make_unique<impl_type>(terms,1);
  }

  template <typename Visitor>
  static void visit_elements(const Serialized<masquerade_for>& me, Visitor& v)
  {
    v << me.get_impl().get_terms();
  }
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< Polynomial<Coefficient,Exponent> > >
  : spec_object_traits<is_composite> {

  using impl_type = typename Polynomial<Coefficient,Exponent>::impl_type;

  using masquerade_for = Polynomial<Coefficient, Exponent>;

  using terms_type = typename polynomial_impl::GenericImpl<polynomial_impl::MultivariateMonomial<Exponent>,Coefficient>::term_hash;

  using elements = cons<terms_type, Int>;

  template <typename Visitor>
  static void visit_elements(Serialized<masquerade_for>& me, Visitor& v)
  {
     terms_type terms;
     Int nvars = 0;
     v << terms << nvars;
     me.impl_ptr = std::make_unique<impl_type>(terms, nvars);
  }

  template <typename Visitor>
  static void visit_elements(const Serialized<masquerade_for>& me, Visitor& v)
  {
     v << me.get_terms() << me.n_vars();
  }
};


template <typename PolynomialType>
struct hash_func<PolynomialType, is_polynomial> {
  size_t operator() (const PolynomialType& p) const noexcept
  {
    return p.get_hash();
  }
};

template <typename C, typename E, typename T>
std::enable_if_t<UniPolynomial<C,E>::template fits_as_coefficient<T>::value, UniPolynomial<C, E>>
operator+ (const T& c, const UniPolynomial<C, E>& p)
{
  return p+c;
}

template <typename C, typename E, typename T>
std::enable_if_t<Polynomial<C,E>::template fits_as_coefficient<T>::value, Polynomial<C, E>>
operator+ (const T& c, const Polynomial<C, E>& p)
{
  return p+c;
}

template <typename C, typename E, typename T>
std::enable_if_t<UniPolynomial<C, E>::template fits_as_coefficient<T>::value, UniPolynomial<C, E>>
operator- (const T& c, const UniPolynomial<C, E>& p)
{
  return (-p)+=c;
}

template <typename C, typename E, typename T>
std::enable_if_t<Polynomial<C, E>::template fits_as_coefficient<T>::value, Polynomial<C, E>>
operator- (const T& c, const Polynomial<C, E>& p)
{
  return (-p)+=c;
}

template <typename C, typename E, typename T>
std::enable_if_t<UniPolynomial<C, E>::template fits_as_coefficient<T>::value, UniPolynomial<C, E>>
operator* (const T& c, const UniPolynomial<C, E>& p)
{
  return p.mult_from_right(c);
}

template <typename C, typename E, typename T>
std::enable_if_t<Polynomial<C, E>::template fits_as_coefficient<T>::value, Polynomial<C, E>>
operator* (const T& c, const Polynomial<C, E>& p)
{
  return p.mult_from_right(c);
}

template <typename C, typename E>
UniPolynomial<C, E> pow(const UniPolynomial<C, E> & base, long exp)
{
  return base.pow(exp);
}

template <typename C, typename E>
Polynomial<C, E> pow(const Polynomial<C, E> & base, long exp)
{
  return base.pow(exp);
}


/// explicit conversion of polynomial coefficients to another type
template <typename TargetType, typename Exponent>
const Polynomial<TargetType, Exponent>&
convert_to(const Polynomial<TargetType, Exponent>& p)
{
   return p;
}

template <typename TargetType, typename Exponent>
const UniPolynomial<TargetType,Exponent>&
convert_to(const UniPolynomial<TargetType, Exponent>& p)
{
   return p;
}

template <typename TargetType, typename Coefficient, typename Exponent,
          typename = std::enable_if_t<can_initialize<Coefficient, TargetType>::value && !std::is_same<Coefficient, TargetType>::value>>
Polynomial<TargetType, Exponent>
convert_to(const Polynomial<Coefficient, Exponent>& p)
{
   return Polynomial<TargetType,Exponent>(convert_to<TargetType>(p.coefficients_as_vector()),p.monomials_as_matrix());
}

template <typename TargetType, typename Coefficient, typename Exponent,
          typename = std::enable_if_t<can_initialize<Coefficient, TargetType>::value && !std::is_same<Coefficient, TargetType>::value>>
UniPolynomial<TargetType, Exponent>
convert_to(const UniPolynomial<Coefficient, Exponent>& p)
{
   return UniPolynomial<TargetType,Exponent>(convert_to<TargetType>(p.coefficients_as_vector()),p.monomials_as_vector());
}

// flint specializations 
#ifdef POLYMAKE_WITH_FLINT

template <>
UniPolynomial<Rational, Int>
gcd(const UniPolynomial<Rational, Int>& a, const UniPolynomial<Rational, Int>& b);

template <>
ExtGCD< UniPolynomial<Rational, Int> >
ext_gcd(const UniPolynomial<Rational, Int>& a, const UniPolynomial<Rational, Int>& b, bool normalize_gcd);

#endif

} // end namespace pm

namespace polymake {
   using pm::Polynomial;
   using pm::UniPolynomial;
   using pm::convert_to;
}

namespace std {

template <typename Coefficient, typename Exponent>
void swap(pm::UniPolynomial<Coefficient, Exponent>& x1, pm::UniPolynomial<Coefficient, Exponent> & x2) { x1.swap(x2); }

template <typename Coefficient, typename Exponent>
void swap(pm::Polynomial<Coefficient, Exponent>& x1, pm::Polynomial<Coefficient, Exponent> & x2) { x1.swap(x2); }

}

#endif // POLYMAKE_POLYNOMIAL_H
