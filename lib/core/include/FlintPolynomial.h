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

#ifndef POLYMAKE_FLINTPOLY_H
#define POLYMAKE_FLINTPOLY_H

#ifndef POLYMAKE_WITH_FLINT

#error "this should not be included when flint is disabled!"

#else

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <flint/fmpq_poly.h>
#pragma GCC diagnostic pop

#include "polymake/Rational.h"
#include "polymake/Polynomial.h"
#include "polymake/hash_map"

namespace pm {

class FlintPolynomial;
// bool abs_equal(const FlintPolynomial& a, const FlintPolynomial& b);

class FlintPolynomial{
   template <typename,typename> friend class UniPolynomial;

   private:
      fmpq_poly_t flintPolynomial;
      int shift;

   public:

      using monomial_type = int;
      using coefficient_type = Rational;

      using term_hash = hash_map<monomial_type, coefficient_type>;
      using sorted_terms_type = typename std::forward_list<monomial_type>;
      using monomial_list_type = Vector<monomial_type>;

      using generic_impl = polynomial_impl::GenericImpl<polynomial_impl::UnivariateMonomial<monomial_type>,coefficient_type>;
      template <typename T>
      using fits_as_coefficient = can_upgrade<T, coefficient_type>;
      template <typename T>
      using is_deeper_coefficient = typename generic_impl::template is_deeper_coefficient<T>;

     static PolynomialVarNames& var_names()
     {
         return generic_impl::var_names();
     }
     static void reset_var_names()
     {
         return generic_impl::reset_var_names();
     }


   protected:
      mutable std::unique_ptr<generic_impl> generic_impl_cache;

      int convertUniPolynomial2Flint(fmpq_poly_t& out, const typename FlintPolynomial::generic_impl& in){
         fmpq_poly_init(out);
         int expshift = std::min(in.lower_deg(),0);
         for (auto t = entire(in.get_terms()); !t.at_end(); ++t){
            fmpq_poly_set_coeff_mpq(out, t->first-expshift, t->second.get_rep());
         }
         return expshift;
      }

      const hash_map<int, Rational> to_terms() const
      {
         hash_map<int, Rational> result;
         for(int i=lower_deg(); i<=deg(); i++){
            if (exists(i)) {
               result[i] = get_coefficient(i);
            }
         }
         return result;
      }

      const generic_impl& to_generic() const
      {
         if (!generic_impl_cache) {
            generic_impl_cache.reset(new FlintPolynomial::generic_impl(1,to_terms()));
         }
         return *generic_impl_cache;
      }

      void set_terms (const term_hash& src)
      {
         shift = 0;
         for (auto t = entire(src); !t.at_end(); ++t){
            if(t->first < shift){
               shift = t->first;
            }
         }
         for (auto t = entire(src); !t.at_end(); ++t){
            fmpq_poly_set_coeff_mpq(flintPolynomial, t->first-shift, convert_to<Rational>(t->second).get_rep());
         }
      }

   public:
      const hash_map<int, Rational>& get_terms() const
      {
         return to_generic().get_terms();
      }
      
      const sorted_terms_type get_sorted_terms() const
      {
         auto mons = monomials();
         return sorted_terms_type(mons.rbegin(),mons.rend());
      }

      // Constructors
      FlintPolynomial(){
         shift = 0;
         fmpq_poly_init(flintPolynomial);
      }

      explicit FlintPolynomial(int n_vars){
         if (n_vars != 1) throw std::runtime_error("FlintPolynomial: univariate only");
         fmpq_poly_init(flintPolynomial);
         shift = 0;
      }

      FlintPolynomial(int c, int n_vars) {
         if (n_vars != 1) throw std::runtime_error("FlintPolynomial: univariate only");
         fmpq_poly_init(flintPolynomial);
         fmpq_poly_set_si(flintPolynomial,c);
         shift = 0;
      }

      FlintPolynomial(const FlintPolynomial& p){
         fmpq_poly_init(flintPolynomial);
         fmpq_poly_set(flintPolynomial, p.flintPolynomial);
         shift = p.shift;
      }

      FlintPolynomial(const Rational& c, int n_vars) {
         if (n_vars != 1) throw std::runtime_error("FlintPolynomial: univariate only");
         fmpq_poly_init(flintPolynomial);
         fmpq_poly_set_mpq(flintPolynomial,c.get_rep());
         shift = 0;
      }

      explicit FlintPolynomial(const typename FlintPolynomial::generic_impl& in)
      {
         shift = convertUniPolynomial2Flint(flintPolynomial, in);
      }


      template <typename Container1, typename Container2>
      FlintPolynomial(const Container1& coefficients, const Container2& monomials, const int n_vars)
      {  
         if (n_vars != 1) throw std::runtime_error("FlintPolynomial: univariate only");
         fmpq_poly_init(flintPolynomial);
         shift = 0;
         for (auto m = entire(monomials); !m.at_end(); ++m){
            if(*m < shift){
               shift = *m;
            }
         }
         auto c = entire(coefficients);
         for (auto e = entire(monomials); !e.at_end(); ++e, ++c){
            fmpq_poly_set_coeff_mpq(flintPolynomial, *e-shift, convert_to<Rational>(*c).get_rep());
         }
      }

      explicit FlintPolynomial(const term_hash& terms, int nvars=1)
      {
         if (nvars != 1) throw std::runtime_error("FlintPolynomial: univariate only");
         fmpq_poly_init(flintPolynomial);
         this->set_terms(terms);
      }

      // destruction
      ~FlintPolynomial(){fmpq_poly_clear(flintPolynomial);}

      void clear()
      {
         fmpq_poly_zero(flintPolynomial);
         generic_impl_cache.reset(nullptr);
      }

      // assignment
      FlintPolynomial& operator = (const FlintPolynomial &p){
         fmpq_poly_init(flintPolynomial);
         fmpq_poly_set(flintPolynomial, p.flintPolynomial);
         shift = p.shift;
         generic_impl_cache.reset(nullptr);
         return *this;
      }


      // Operators
      FlintPolynomial operator+ (const FlintPolynomial& p) const{
         FlintPolynomial result(*this);
         result += p;
         return result;
      }

      FlintPolynomial& operator+= (const FlintPolynomial& p){
         if(shift == p.shift){
            fmpq_poly_add(flintPolynomial, flintPolynomial, p.flintPolynomial);
         } else {
            if(shift > p.shift){
               set_shift(p.shift);
               *this += p;
            } else {
               FlintPolynomial tmpp(p);
               tmpp.set_shift(shift);
               *this += tmpp;
            }
         }
         reduce_shift();
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      void reduce_shift(){
         if(shift < 0 && lower_deg() > shift){
            set_shift(lower_deg());
         }
      }

      void set_shift(int desired){
         if(shift == desired){
            return;
         } else if(desired < shift){
            fmpq_poly_shift_left(flintPolynomial, flintPolynomial, shift-desired);
            shift = desired;
         } else {
            if(desired > lower_deg()) throw std::runtime_error("Shifting would change polynomial");
            fmpq_poly_shift_right(flintPolynomial, flintPolynomial, desired-shift);
            shift = desired;
         }
      }

      // substitute a monomial with given exponent into a polynomial
      template <typename Exp=int, typename T>
      auto
      substitute_monomial(const T& exponent, std::enable_if_t<!std::is_same<Exp,int>::value,std::nullptr_t> = nullptr) const {
         return this->to_generic().substitute_monomial<Exp>(exponent);
      }

      template <typename Exp=int, typename T>
      auto
      substitute_monomial(const T& exponent, std::enable_if_t<std::is_same<Exp,int>::value,std::nullptr_t> = nullptr) const {
         FlintPolynomial tmp;
         if (__builtin_expect(pm::is_zero(exponent),0)) {
            mpq_t res;
            mpq_init(res);
            fmpq_poly_evaluate_mpz(res, flintPolynomial, Integer(1).get_rep());
            fmpq_poly_set_mpq(tmp.flintPolynomial,res);
            mpq_clear(res);
         } else if(exponent < 0) {
            tmp.shift = std::min(int(deg() * exponent),0);
            for (int i = 0; i <= deg()-shift; i++)
               if (exists(i+shift))
                  fmpq_poly_set_coeff_mpq(tmp.flintPolynomial, int((deg()-shift-i)*abs(exponent)), get_coefficient(i+shift).get_rep());
         } else {
            tmp.shift = int(shift * exponent);
            for (int i = 0; i <= deg()-shift; i++)
               if (exists(i+shift))
                  fmpq_poly_set_coeff_mpq(tmp.flintPolynomial, int(i*exponent), get_coefficient(i+shift).get_rep());
         }
         return tmp;
      }

      
      FlintPolynomial& operator+= (const Rational& c) {
         if(shift == 0){
            fmpq_t tmp;
            fmpq_init(tmp);
            fmpq_set_mpq(tmp,c.get_rep());
            fmpq_poly_add_fmpq(flintPolynomial,flintPolynomial,tmp);
            fmpq_clear(tmp);
         } else {
            FlintPolynomial C(c,1);
            *this += C;
         }
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      FlintPolynomial& operator+= (const int& c) {
         if(shift == 0){
            fmpq_poly_add_si(flintPolynomial,flintPolynomial,c);
         } else {
            FlintPolynomial C(c,1);
            *this += C;
         }
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      template <typename T>
      typename std::enable_if<fits_as_coefficient<T>::value, FlintPolynomial>::type
      operator+ (const T& c) const {
         FlintPolynomial tmp(*this);
         tmp += c;
         return tmp;
      }

      FlintPolynomial operator- (const FlintPolynomial& p) const{
         FlintPolynomial result(*this);
         result -= p;
         return result;
      }

      FlintPolynomial& operator-= (const FlintPolynomial& p){
         if(shift == p.shift){
            fmpq_poly_sub(flintPolynomial, flintPolynomial, p.flintPolynomial);
         } else {
            if(shift > p.shift){
               set_shift(p.shift);
               *this -= p;
            } else {
               FlintPolynomial tmpp(p);
               tmpp.set_shift(shift);
               *this -= tmpp;
            }
         }
         reduce_shift();
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      template <typename T>
      typename std::enable_if<fits_as_coefficient<T>::value, FlintPolynomial&>::type
      operator -= (const T& c) {
         *this += -c;
         return *this;
      }

      template <typename T>
      typename std::enable_if<fits_as_coefficient<T>::value, FlintPolynomial>::type
      operator- (const T& c) const {
         FlintPolynomial tmp(*this);
         tmp -= c;
         return tmp;
      }
      
      FlintPolynomial operator* (const FlintPolynomial& p) const{
         FlintPolynomial result(*this);
         result *= p;
         return result;
      }

      FlintPolynomial& operator*= (const FlintPolynomial& p){
         fmpq_poly_mul(flintPolynomial, flintPolynomial, p.flintPolynomial);
         shift += p.shift;
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      FlintPolynomial& operator*= (const Rational& c) {
         if (__builtin_expect(is_zero(c), 0))
            fmpq_poly_zero(flintPolynomial);
         else
            fmpq_poly_scalar_mul_mpq(flintPolynomial,flintPolynomial,c.get_rep());
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      FlintPolynomial& operator*= (const int& c) {
         if (__builtin_expect(is_zero(c), 0))
            fmpq_poly_zero(flintPolynomial);
         else
            fmpq_poly_scalar_mul_si(flintPolynomial,flintPolynomial,c);
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      template <typename T>
      typename std::enable_if<fits_as_coefficient<T>::value, FlintPolynomial>::type
      operator* (const T& c) const {
         FlintPolynomial tmp(*this);
         tmp *= c;
         return tmp;
      }

      template <typename T>
      typename std::enable_if<fits_as_coefficient<T>::value, FlintPolynomial>::type
      mult_from_right (const T& c) const {
         FlintPolynomial tmp(*this);
         tmp *= c;
         return tmp;
      }

      FlintPolynomial pow(long e) const
      {
         FlintPolynomial tmp;
         if (__builtin_expect(trivial(), 0))
            return tmp;
         if(e < 0){
            int d = deg();
            if(d != lower_deg())
               throw std::runtime_error("Exponentiation with negative exponent is only implemented for monomials");
            tmp.shift = (d-shift) * e;
            Rational c(get_coefficient(d));
            c = pm::Rational::pow(c,e);
            fmpq_poly_set_coeff_mpq(tmp.flintPolynomial, shift*e, c.get_rep());
         } else {
            fmpq_poly_pow(tmp.flintPolynomial,flintPolynomial,e);
            tmp.shift = shift*e;
         }
         return tmp;
      }
      
      FlintPolynomial& div_exact(const FlintPolynomial& p)
      {  
         if(shift == p.shift){
            FlintPolynomial tmp;
            fmpq_poly_div(tmp.flintPolynomial, flintPolynomial, p.flintPolynomial);
            fmpq_poly_set(flintPolynomial,tmp.flintPolynomial);
            // Set shift to zero since we receive a polynomial.
            shift = 0;
         } else {
            if(shift > p.shift){
               set_shift(p.shift);
               div_exact(p);
            } else {
               FlintPolynomial tmpp(p);
               tmpp.set_shift(shift);
               div_exact(tmpp);
            }
         }
         reduce_shift();
         generic_impl_cache.reset(nullptr);
         return *this;
      }
      
      FlintPolynomial& operator%= (const FlintPolynomial& p)
      {
         FlintPolynomial tmp;
         fmpq_poly_rem(tmp.flintPolynomial, flintPolynomial, p.flintPolynomial);
         fmpq_poly_set(flintPolynomial,tmp.flintPolynomial);
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      FlintPolynomial operator% (const FlintPolynomial& p)
      {
         FlintPolynomial tmp(*this);
         return tmp %= p;
      }
      
      FlintPolynomial& operator/= (const Rational& c) {
         if (__builtin_expect(is_zero(c), 0))
            throw GMP::ZeroDivide();
         fmpq_poly_scalar_div_mpq(flintPolynomial,flintPolynomial,c.get_rep());
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      FlintPolynomial& operator/= (const int& c) {
         if (__builtin_expect(is_zero(c), 0))
            throw GMP::ZeroDivide();
         fmpq_poly_scalar_div_si(flintPolynomial,flintPolynomial,c);
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      template <typename T>
      typename std::enable_if<fits_as_coefficient<T>::value, FlintPolynomial>::type
      operator/ (const T& c) const {
         FlintPolynomial tmp(*this);
         tmp /= c;
         return tmp;
      }

      FlintPolynomial& negate()
      {
         fmpq_poly_neg(flintPolynomial, flintPolynomial);
         generic_impl_cache.reset(nullptr);
         return *this;
      }

      FlintPolynomial operator- () const
      {
         FlintPolynomial result(*this);
         return result.negate();
      }

      int n_vars() const
      {
         return 1;
      }

      template <typename Other>
      void croak_if_incompatible(const Other& other) const
      {
         if (other.n_vars() != 1) throw std::runtime_error("Polynomials of different rings");
      }

      int deg() const
      {
         if (__builtin_expect(trivial(),0))
            return std::numeric_limits<int>::min();
         return int(fmpq_poly_degree(flintPolynomial)) + shift;
      }

      bool exists(int i) const
      {
         return !trivial() && i >= shift && i <= deg() && !fmpz_is_zero(fmpq_poly_numref(flintPolynomial) + (i - shift) );
      }

      int lower_deg() const
      {
         if (__builtin_expect(trivial(),0))
            return std::numeric_limits<int>::max();
         int i = 0;
         while ( i <= deg() - shift && fmpz_is_zero(fmpq_poly_numref(flintPolynomial) + i) ) {
            i++;
         };
         return i + shift;
      }

      int lm() const
      {
         return deg();
      }

      Rational get_coefficient(int i) const
      {
         if (__builtin_expect(trivial(),0) || i < shift || i > deg())
            return zero_value<Rational>();
         mpq_t tmp;
         mpq_init(tmp);
         fmpq_poly_get_coeff_mpq(tmp,flintPolynomial,i-shift);
         Rational rat(std::move(tmp));
         return rat;
      }

      Rational lc() const
      {
         if (__builtin_expect(trivial(),0))
            return zero_value<Rational>();
         return get_coefficient(deg());
      }

      Rational lc(const int o) const
      {
         if (__builtin_expect(trivial(),0))
            return zero_value<Rational>();
         return o > 0 ? get_coefficient(deg()) : get_coefficient(lower_deg());
      }

      monomial_list_type monomials() const
      {
         if (__builtin_expect(trivial(),0))
            return monomial_list_type();
         return monomial_list_type(range(lower_deg(),deg()));
      }

      Vector<Rational> coefficients_as_vector() const
      {
         if (__builtin_expect(trivial(),0))
            return Vector<Rational>();
         Vector<Rational> coeffs(deg()-lower_deg()+1);
         int i = lower_deg();
         for (auto c = entire(coeffs); !c.at_end(); ++c, ++i)
            *c = get_coefficient(i);
         return coeffs;
      }

      // comparison
      bool operator== (const FlintPolynomial& p2) const
      {  
         return (shift == p2.shift) && fmpq_poly_equal(flintPolynomial,p2.flintPolynomial);
      }

      template <typename T>
      typename std::enable_if<fits_as_coefficient<T>::value, bool>::type
      operator== (const T& p2) const
      {
         return deg() == 0 && lc() == p2;
      }

      cmp_value compare(const FlintPolynomial& p2) const
      {  
         if(shift == p2.shift)
            return cmp_value(fmpq_poly_cmp(flintPolynomial,p2.flintPolynomial));
         else return cmp_value(false);
      }

      FlintPolynomial& normalize()
      {
         return *this /= lc();
      }

      bool trivial() const
      {
         return fmpq_poly_is_zero(flintPolynomial);
      }

      bool is_one() const
      {
         return shift == 0 && fmpq_poly_is_one(flintPolynomial);
      }

      template <typename Output, typename Order>
      void pretty_print(Output& out, const Order& order) const
      {
         this->to_generic().pretty_print(out,order);
      }

      template <typename Output>
      friend
      Output& operator<<(GenericOutput<Output>& out, const FlintPolynomial & p){
         p.pretty_print(out.top(),polynomial_impl::cmp_monomial_ordered_base<monomial_type>());
         return out.top();
      }

      template <typename QuotConsumer>
      typename std::enable_if<std::is_same<QuotConsumer,FlintPolynomial>::value, void>::type
      remainder(const FlintPolynomial& den, QuotConsumer& quot)
      {
         FlintPolynomial rem;
         fmpq_poly_divrem(quot.flintPolynomial,rem.flintPolynomial,flintPolynomial,den.flintPolynomial);
         fmpq_poly_set(flintPolynomial,rem.flintPolynomial);
      }

      template <typename QuotConsumer>
      typename std::enable_if<!std::is_same<QuotConsumer,FlintPolynomial>::value, void>::type
      remainder(const FlintPolynomial& den, QuotConsumer& quot)
      {
         FlintPolynomial rem;
         fmpq_poly_rem(rem.flintPolynomial,flintPolynomial,den.flintPolynomial);
         fmpq_poly_set(flintPolynomial,rem.flintPolynomial);
      }

      static void xgcd(FlintPolynomial& g, FlintPolynomial& s, FlintPolynomial& t, 
                       const FlintPolynomial& p1, const FlintPolynomial& p2)
      {
         if(p1.shift == p2.shift){
            fmpq_poly_xgcd(g.flintPolynomial, s.flintPolynomial, t.flintPolynomial, 
                           p1.flintPolynomial, p2.flintPolynomial);
            g.shift = p1.shift;
            s.shift = p1.shift;
            t.shift = p1.shift;
            g.reduce_shift();
            s.reduce_shift();
            t.reduce_shift();
         } else {
            if(p1.shift < p2.shift){
               FlintPolynomial tmpp2(p2);
               tmpp2.set_shift(p1.shift);
               xgcd(g, s, t, p1, tmpp2);
            } else {
               FlintPolynomial tmpp1(p1);
               tmpp1.set_shift(p2.shift);
               xgcd(g, s, t, tmpp1, p2);
            }
         }
      }

      static FlintPolynomial gcd(const FlintPolynomial& p1, const FlintPolynomial& p2)
      {
         if(p1.shift == p2.shift){
            FlintPolynomial tmp;
            fmpq_poly_gcd(tmp.flintPolynomial,p1.flintPolynomial,p2.flintPolynomial);
            tmp.shift = p1.shift;
            tmp.reduce_shift();
            return tmp;
         } else {
            if(p1.shift < p2.shift){
               FlintPolynomial tmpp2(p2);
               tmpp2.set_shift(p1.shift);
               return gcd(p1, tmpp2);
            } else {
               return gcd(p2, p1);
            }
         }
      }

      size_t get_hash() const noexcept
      {
         size_t h = std::hash<int>{}(shift);
         if (__builtin_expect(trivial(),0))
            return h;
         hash_func<Rational> rathash;
         int i = lower_deg();
         while ( i <= deg()) {
            if (exists(i)) {
               hash_combine(h, std::hash<int>{}(i));
               hash_combine(h, rathash(get_coefficient(i)));
            }
            i++;
         };
         return h;
      }
};


}

namespace polymake {
   using pm::FlintPolynomial;
}

#endif

#endif
