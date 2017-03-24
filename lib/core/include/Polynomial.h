/* Copyright (c) 1997-2017
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

#include "polymake/PolynomialVarNames.h"
#include "polymake/PolynomialImpl.h"
#include "polymake/Rational.h"

namespace pm {

template <typename Coefficient, typename Exponent>
class UniPolynomial;
template <typename Coefficient, typename Exponent>
class Polynomial;
template <typename Coefficient, typename Exponent>
class RationalFunction;

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

template <typename Coefficient = Rational, typename Exponent = int>
class UniPolynomial {

  friend class RationalFunction<Coefficient, Exponent>;
  template <typename> friend struct spec_object_traits;
public:
  typedef polynomial_impl::GenericImpl< polynomial_impl::UnivariateMonomial<Exponent>, Coefficient> impl_type;
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
  template <typename T, typename enabled=typename std::enable_if<fits_as_coefficient<T>::value>::type>
  explicit UniPolynomial(const T& c)
    : impl_ptr{std::make_unique<impl_type>(c,1)} {}

  /// construct a polynomial with a single term
  template <typename T, typename enabled=typename std::enable_if<fits_as_coefficient<T>::value>::type>
  UniPolynomial(const T& c, const Exponent& exp)
    : UniPolynomial(same_element_vector(static_cast<Coefficient>(c), 1), same_element_vector(exp, 1)) {}

  template <typename Container1, typename Container2,
            typename enabled=typename std::enable_if<isomorphic_to_container_of<Container1, Coefficient>::value &&
                                                     isomorphic_to_container_of<Container2, Exponent>::value>::type>
  UniPolynomial(const Container1& coefficients, const Container2& monomials)
    : impl_ptr{std::make_unique<impl_type>(coefficients, monomials, 1)} {}

  /// construct a monomial of degree 1
  static UniPolynomial monomial() { return UniPolynomial(1, 1); }

  // Interface forwarding
  void swap(UniPolynomial& p) { impl_ptr.swap(p.impl_ptr); }
  void clear() { impl_ptr->clear(); }

  template <typename Other>
  void croak_if_incompatible(const Other& other) const
  {
    impl_ptr->croak_if_incompatible(other);
  }

  int n_vars() const { return impl_ptr->n_vars(); }
  int n_terms() const { return impl_ptr->n_terms(); }
  const term_hash& get_terms() const { return impl_ptr->get_terms(); }
  bool trivial() const { return impl_ptr->trivial(); }
  bool unit() const { return impl_ptr->unit(); }
  Vector<Coefficient> coefficients_as_vector() const { return impl_ptr->coefficients_as_vector(); }

  bool operator== (const UniPolynomial& p2) const { return impl_ptr->operator==(*p2.impl_ptr); }
  bool operator!= (const UniPolynomial& p2) const { return !operator==(p2); }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
  operator== (const T& c) const { return impl_ptr->operator==(c); }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
  operator!= (const T& c) const { return !operator==(c); }

  template <typename T> friend
  typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
  operator==(const T&c, const UniPolynomial& p) { return p == c; }

  template <typename T> friend
  typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
  operator!=(const T&c, const UniPolynomial& p) { return p != c; }

  const Coefficient& get_coefficient(const monomial_type& m) const { return impl_ptr->get_coefficient(m); }

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
  const Coefficient& lc() const { return impl_ptr->lc(); }
  const Coefficient& lc(const Exponent& order) const { return impl_ptr->lc(order); }

  UniPolynomial initial_form(const Exponent& weights) const
  {
    return UniPolynomial(impl_ptr->initial_form(weights));
  }

  template <typename T>
  typename std::enable_if<is_field_of_fractions<Exponent>::value && fits_as_coefficient<T>::value,
                          typename algebraic_traits<T>::field_type>::type
  evaluate(const T& t, const long exp_lcm=1) const
  {
    typedef typename algebraic_traits<T>::field_type field;
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
  typename std::enable_if<std::numeric_limits<Exponent>::is_integer && fits_as_coefficient<T>::value,
                          typename algebraic_traits<T>::field_type>::type
  evaluate(const T& t, const long exp_lcm=1) const
  {
    typedef typename algebraic_traits<T>::field_type field;
    field res;
    for (const auto& term : get_terms())
    {
      res += term.second * field::pow(t, static_cast<long>(exp_lcm * term.first));
    }
    return res;
  }

  double evaluate_float(const double a) const
  {
    double res = 0;
    for (const auto& term : get_terms())
    {
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

    UniPolynomial quot;
    remainder(b, quot.get_mutable_terms().make_filler());
    swap(quot);
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
      remainder(b, quot_black_hole());
    }
    return *this;
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type
  operator*(const T& c) const
  {
    return UniPolynomial(impl_ptr->operator*(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type
  mult_from_right(const T& c) const
  {
    return UniPolynomial(impl_ptr->mult_from_right(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type&
  operator*= (const T& c)
  {
    impl_ptr->operator*=(c);
    return *this;
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type
  operator/ (const T& c) const
  {
    return UniPolynomial(impl_ptr->operator/(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type&
  operator/= (const T& c)
  {
    impl_ptr->operator/=(c);
    return *this;
  }

  Polynomial<Coefficient, Exponent> homogenize(int new_variable_index = 0) const
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
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type
  operator+ (const T& c) const
  {
    return UniPolynomial(impl_ptr->operator+(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type&
  operator+= (const T& c)
  {
    impl_ptr->operator+=(c);
    return *this;
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type
  operator- (const T& c) const
  {
    return UniPolynomial(impl_ptr->operator-(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, UniPolynomial>::type&
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

  typename term_hash::const_iterator find_lex_lm() const { return impl_ptr->find_lex_lm(); }

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
    const auto b_lead=b.find_lex_lm();
    typename term_hash::const_iterator this_lead;

    while ((this_lead=find_lex_lm()) != get_mutable_terms().cend() && this_lead->first >= b_lead->first) {
      const Coefficient k = this_lead->second / b_lead->second;
      const Exponent d = this_lead->first - b_lead->first;
      quot_consumer(d, k);
      impl_ptr->forget_sorted_terms();

      for (const auto& b_term : b.get_terms()) {
        auto it = get_mutable_terms().find_or_insert(b_term.first + d);
        if (it.second) {
          it.first->second= -k * b_term.second;
        } else if (is_zero(it.first->second -= k * b_term.second)) {
          get_mutable_terms().erase(it.first);
        }
      }
    }
  }

  // replace this with a remainder of division by b, consume the quotient
  // data must be brought in exclusive posession before calling this method.
  template <typename QuotConsumer>
  void remainder(const Exponent& b, const QuotConsumer& quot_consumer)
  {
    for (auto it=impl_ptr->the_terms.begin(), end=impl_ptr->the_terms.end();  it != end;  ) {
      if (it->first < b) {
        ++it;
      } else {
        if (!std::is_same<QuotConsumer, quot_black_hole>::value)
          quot_consumer(it->first - b, it->second);
        impl_ptr->the_terms.erase(it++);
      }
    }
    impl_ptr->forget_sorted_terms();
  }
};

template <typename Coefficient, typename Exponent> inline
Div< UniPolynomial<Coefficient, Exponent> >
div(const UniPolynomial<Coefficient, Exponent>& num, const UniPolynomial<Coefficient, Exponent>& den)
{
  typedef typename UniPolynomial<Coefficient,Exponent>::impl_type impl_type;
  num.croak_if_incompatible(den);
  if (den.trivial()) throw GMP::ZeroDivide();
  Div< UniPolynomial<Coefficient, Exponent> > res;
  res.rem.impl_ptr=std::make_unique<impl_type>(*num.impl_ptr);
  res.rem.remainder(den, res.quot.impl_ptr->get_mutable_terms().make_filler());
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

  while (!p2.trivial() && !is_zero(p2.lm())) {
    p1.remainder(p2, typename UniPolynomial<Coefficient, Exponent>::quot_black_hole());
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
      p1.remainder(p2, k.get_mutable_terms().make_filler());
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
      p2.remainder(p1, k.get_mutable_terms().make_filler());
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


template <typename Coefficient = Rational, typename Exponent = int>
class Polynomial {

  template <typename> friend struct spec_object_traits;
public:
  typedef polynomial_impl::GenericImpl< polynomial_impl::MultivariateMonomial<Exponent>, Coefficient> impl_type;
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
  explicit Polynomial(const int n_vars)
    : impl_ptr{std::make_unique<impl_type>(n_vars)} {}

  /// construct a polynomial of degree 0 with the given number of variables
  template <typename T, typename enabled=typename std::enable_if<fits_as_coefficient<T>::value>::type>
  Polynomial(const T& c, const int n_vars)
    : impl_ptr{std::make_unique<impl_type>(c, n_vars)} {}

  /// construct a polynomial with a single term
  template <typename T, typename TVector, typename enabled=typename std::enable_if<fits_as_coefficient<T>::value>::type>
  Polynomial(const T& c, const GenericVector<TVector>& monomial)
    : Polynomial(same_element_vector(c, 1), vector2row(monomial)) {}

  template <typename Container, typename TMatrix, typename enabled=typename std::enable_if<isomorphic_to_container_of<Container, Coefficient>::value>::type>
  Polynomial(const Container& coefficients, const GenericMatrix<TMatrix, Exponent>& monomials)
    : impl_ptr{std::make_unique<impl_type>(coefficients, rows(monomials), monomials.cols())} {}

  /// construct a monomial of the given variable
  static Polynomial monomial(int var_index, int n_vars)
  {
    return Polynomial(1, unit_vector<Exponent>(n_vars, var_index));
  }

  // Interface forwarding
  void swap(Polynomial& p) { impl_ptr.swap(p.impl_ptr); }
  void clear() { impl_ptr->clear(); }

  template <typename Other>
  void croak_if_incompatible(const Other& other) const
  {
    impl_ptr->croak_if_incompatible(other);
  }

  int n_vars() const { return impl_ptr->n_vars(); }
  int n_terms() const { return impl_ptr->n_terms(); }
  const term_hash& get_terms() const { return impl_ptr->get_terms(); }
  bool trivial() const { return impl_ptr->trivial(); }
  bool unit() const { return impl_ptr->unit(); }

  Vector<Coefficient> coefficients_as_vector() const { return impl_ptr->coefficients_as_vector(); }

  bool operator== (const Polynomial& p2) const { return impl_ptr->operator==(*p2.impl_ptr); }
  bool operator!= (const Polynomial& p2) const { return !operator==(p2); }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
  operator== (const T& c) const { return impl_ptr->operator==(c); }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
  operator!= (const T& c) const { return !operator==(c); }

  template <typename T> friend
  typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
  operator==(const T&c, const Polynomial& p) { return p == c; }

  template <typename T> friend
  typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
  operator!=(const T&c, const Polynomial& p) { return p != c; }

  const Coefficient& get_coefficient(const monomial_type& m) const { return impl_ptr->get_coefficient(m); }
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

  const Coefficient& lc() const { return impl_ptr->lc(); }

  template <typename TMatrix>
  const Coefficient& lc(const GenericMatrix<TMatrix, Exponent>& order) const
  {
    return impl_ptr->lc(order);
  }

  template <typename TVector>
  Polynomial initial_form(const GenericVector<TVector, Exponent>& weights) const
  {
    return Polynomial(impl_ptr->initial_form(weights));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type
  operator*(const T& c) const
  {
    return Polynomial(impl_ptr->operator*(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type
  mult_from_right(const T& c) const
  {
    return Polynomial(impl_ptr->mult_from_right(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type&
  operator*=(const T& c)
  {
    impl_ptr->operator*=(c); return *this;
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type
  operator/(const T& c) const
  {
    return Polynomial(impl_ptr->operator/(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type&
  operator/=(const T& c)
  {
    impl_ptr->operator/=(c); return *this;
  }

  Polynomial<Coefficient, Exponent> homogenize(int new_variable_index = 0) const
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
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type
  operator+(const T& c) const
  {
    return Polynomial(impl_ptr->operator+(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type&
  operator+= (const T& c)
  {
    impl_ptr->operator+=(c); return *this;
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type
  operator- (const T& c) const
  {
    return Polynomial(impl_ptr->operator-(c));
  }

  template <typename T>
  typename std::enable_if<fits_as_coefficient<T>::value, Polynomial>::type&
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

  friend bool operator< (const Polynomial& p1, const Polynomial& p2)
  {
    return *p1.impl_ptr < *p2.impl_ptr;
  }
  friend bool operator> (const Polynomial& p1, const Polynomial& p2)
  {
    return *p1.impl_ptr > *p2.impl_ptr;
  }
  friend bool operator<= (const Polynomial& p1, const Polynomial& p2)
  {
    return *p1.impl_ptr <= *p2.impl_ptr;
  }
  friend bool operator>= (const Polynomial& p1, const Polynomial& p2)
  {
    return *p1.impl_ptr >= *p2.impl_ptr;
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
};

template <typename Coefficient, typename Exponent>
struct is_gcd_domain< UniPolynomial<Coefficient, Exponent> >
  : is_field<Coefficient> {};

template <typename Coefficient, typename Exponent>
struct algebraic_traits< UniPolynomial<Coefficient, Exponent> > {
  typedef RationalFunction<typename algebraic_traits<Coefficient>::field_type, Exponent> field_type;
};

// some magic needed for operator construction
struct is_polynomial {};


template <typename Coefficient, typename Exponent, typename T>
struct compatible_with_polynomial {
  static const bool value= isomorphic_types<Coefficient, T>::value ||
                           Polynomial<Coefficient, Exponent>::template is_deeper_coefficient<T>::value;
};

template <typename Coefficient, typename Exponent, typename T>
struct compatible_with_unipolynomial {
  static const bool value= isomorphic_types<Coefficient, T>::value ||
                           UniPolynomial<Coefficient, Exponent>::template is_deeper_coefficient<T>::value;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<Polynomial<Coefficient, Exponent>, T,
                             typename std::enable_if<compatible_with_polynomial<Coefficient, Exponent, T>::value, is_polynomial>::type,
                             TModel>
: std::false_type {
  typedef cons<is_polynomial, is_scalar> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<T, Polynomial<Coefficient, Exponent>, TModel,
                             typename std::enable_if<compatible_with_polynomial<Coefficient, Exponent, T>::value, is_polynomial>::type>
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
                             typename std::enable_if<compatible_with_unipolynomial<Coefficient, Exponent, T>::value, is_polynomial>::type,
                             TModel>
: std::false_type {
  typedef cons<is_polynomial, is_scalar> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<T, UniPolynomial<Coefficient, Exponent>, TModel,
                             typename std::enable_if<compatible_with_unipolynomial<Coefficient, Exponent, T>::value, is_polynomial>::type>
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
    return p.unit();
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

  static
  const persistent_type& variable() {
    static const persistent_type var(same_element_vector<Coefficient>(1,1), same_element_vector<Exponent>(1,1));
    return var;
  }
};

namespace polynomial_impl {

template <typename Coefficient, typename Exponent>
struct nesting_level< UniPolynomial<Coefficient, Exponent> >
  : int_constant<nesting_level<Coefficient>::value+1> {};

template <typename Coefficient, typename Exponent>
struct nesting_level< Polynomial<Coefficient, Exponent> >
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

  typedef typename UniPolynomial<Coefficient,Exponent>::impl_type impl_type;

  typedef spec_object_traits< Serialized<impl_type> > impl_spec;

  typedef UniPolynomial<Coefficient, Exponent> masquerade_for;

  typedef typename impl_spec::elements elements;

  template <typename Visitor>
  static void visit_elements(Serialized<masquerade_for> & me, Visitor& v)
  {
    me.impl_ptr = std::make_unique<impl_type>();
    impl_spec::visit_elements(*me.impl_ptr,v);
  }

  template <typename Visitor>
  static void visit_elements(const Serialized<masquerade_for>& me, Visitor& v)
  {
    impl_spec::visit_elements(*me.impl_ptr,v);
  }
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< Polynomial<Coefficient,Exponent> > >
  : spec_object_traits<is_composite> {

  typedef typename Polynomial<Coefficient,Exponent>::impl_type impl_type;

  typedef spec_object_traits< Serialized<impl_type> > impl_spec;

  typedef Polynomial<Coefficient, Exponent> masquerade_for;

  typedef typename impl_spec::elements elements;

  template <typename Visitor>
  static void visit_elements(Serialized<masquerade_for> & me, Visitor& v)
  {
    me.impl_ptr = std::make_unique<impl_type>();
    impl_spec::visit_elements(*me.impl_ptr,v);
  }

  template <typename Visitor>
  static void visit_elements(const Serialized<masquerade_for>& me, Visitor& v)
  {
    impl_spec::visit_elements(*me.impl_ptr,v);
  }
};


template <typename PolynomialType>
struct hash_func<PolynomialType, is_polynomial> {
  size_t operator() (const PolynomialType& p) const noexcept
  {
    return p.get_hash();
  }
};

template <typename C, typename E, typename T> inline
typename std::enable_if<UniPolynomial<C,E>:: template fits_as_coefficient<T>::value, UniPolynomial<C,E>>::type
operator+ (const T& c, const UniPolynomial<C,E>& p)
{
  return p+c;
}

template <typename C, typename E, typename T> inline
typename std::enable_if<Polynomial<C,E>:: template fits_as_coefficient<T>::value, Polynomial<C,E>>::type
operator+ (const T& c, const Polynomial<C,E>& p)
{
  return p+c;
}

template <typename C, typename E, typename T> inline
typename std::enable_if<UniPolynomial<C,E>:: template fits_as_coefficient<T>::value, UniPolynomial<C,E>>::type
operator- (const T& c, const UniPolynomial<C,E>& p)
{
  return (-p)+=c;
}

template <typename C, typename E, typename T> inline
typename std::enable_if<Polynomial<C,E>:: template fits_as_coefficient<T>::value, Polynomial<C,E>>::type
operator- (const T& c, const Polynomial<C,E>& p)
{
  return (-p)+=c;
}

template <typename C, typename E, typename T> inline
typename std::enable_if<UniPolynomial<C,E>:: template fits_as_coefficient<T>::value, UniPolynomial<C,E>>::type
operator* (const T& c, const UniPolynomial<C,E>& p)
{
  return p.mult_from_right(c);
}

template <typename C, typename E, typename T> inline
typename std::enable_if<Polynomial<C,E>:: template fits_as_coefficient<T>::value, Polynomial<C,E>>::type
operator* (const T& c, const Polynomial<C,E>& p)
{
  return p.mult_from_right(c);
}

} // end namespace pm

namespace polymake {
   using pm::Polynomial;
   using pm::UniPolynomial;
}

namespace std {

template <typename Coefficient, typename Exponent>
void swap(pm::UniPolynomial<Coefficient, Exponent>& x1, pm::UniPolynomial<Coefficient, Exponent> & x2) { x1.swap(x2); }

template <typename Coefficient, typename Exponent>
void swap(pm::Polynomial<Coefficient, Exponent>& x1, pm::Polynomial<Coefficient, Exponent> & x2) { x1.swap(x2); }

}

#endif // POLYMAKE_POLYNOMIAL_H
