/* Copyright (c) 1997-2021
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


// polymake includes
#include "polymake/client.h"
#include "polymake/Map.h"
#include "polymake/Polynomial.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"

namespace polymake { 
namespace ideal {

class SingularIdeal;


class SingularIdeal_wrap {
public:
   virtual ~SingularIdeal_wrap() { }

   virtual SingularIdeal_wrap* copy() const = 0;
   
   virtual void groebner() = 0;
   
   virtual int dim() = 0;

   virtual Polynomial<> contains_monomial() const = 0;

   virtual Array<Polynomial<>> reduce(const Array<Polynomial<>>& ideal) const = 0;

   virtual Polynomial<> reduce(const Polynomial<>& p) const = 0;
   
   virtual Array<Polynomial<>> division(const Polynomial<>& p, const bool is_std = 0) const = 0;

   virtual SingularIdeal_wrap* radical() const = 0;

   virtual SingularIdeal_wrap* saturation(const Array<Polynomial<>>& rhs) const = 0;

   virtual SingularIdeal_wrap* initial_ideal() const = 0;

   virtual Matrix< std::pair<double,double> > solve() const = 0;
   
   virtual Array<SingularIdeal_wrap*> primary_decomposition() const = 0;

   virtual Array<Polynomial<>> polynomials() const = 0;
   
   static SingularIdeal_wrap* create(const Array<Polynomial<>>& gens, const Vector<Int>& order);
   static SingularIdeal_wrap* create(const Array<Polynomial<>>& gens, const Matrix<Int>& order);
   static SingularIdeal_wrap* create(const Array<Polynomial<>>& gens, const std::string& order);

//   static SingularIdeal_wrap* quotient(const SingularIdeal_wrap* I, const SingularIdeal_wrap* J);
};

class SingularIdeal {
private:
   SingularIdeal_wrap* singIdeal;

public:
   template <typename Ordertype>
   SingularIdeal(const Array<Polynomial<>>& gens, const Ordertype& order) {
      singIdeal = SingularIdeal_wrap::create(gens, order);
   }

   SingularIdeal(SingularIdeal_wrap* sIw) {
      singIdeal = sIw;
   }

   SingularIdeal(const SingularIdeal& sI) {
      singIdeal = sI.singIdeal->copy();
   }

   ~SingularIdeal() {
      delete singIdeal;
   }

   int dim() const  {
      return singIdeal->dim();
   }
   
   Polynomial<> contains_monomial() const {
     return singIdeal->contains_monomial();
   }

   void groebner() const  {
      singIdeal->groebner();
   }

   Array<Polynomial<>> reduce(const Array<Polynomial<>>& ideal) const {
      return singIdeal->reduce(ideal);
   }

   Polynomial<> reduce(const Polynomial<>& p) const {
      return singIdeal->reduce(p);
   }

   Array<Polynomial<>> division( const Polynomial<>& p, const bool is_std = 0 ) const {
      return singIdeal->division(p, is_std);
   }

   SingularIdeal initial_ideal() const  {
      return SingularIdeal(singIdeal->initial_ideal());
   }

   Matrix< std::pair<double,double> > solve() const {
     return singIdeal->solve();
   }
   
   SingularIdeal radical() const  {
      return SingularIdeal(singIdeal->radical());
   }
   
   SingularIdeal saturation(const Array<Polynomial<>>& rhs) const  {
      return SingularIdeal(singIdeal->saturation(rhs));
   }

   ListReturn primary_decomposition() const {
      Array<SingularIdeal_wrap*> pd = singIdeal->primary_decomposition();
      ListReturn result;
      for (auto id = entire(pd); !id.at_end(); ++id) {
         result << SingularIdeal(*id);
      }
      return result;
   }

   Array<Polynomial<>> polynomials() const {
      return singIdeal->polynomials();
   }
   
   friend BigObject quotient(BigObject I, BigObject J);
};
/*template<>
class TermOrder<Matrix<Integer> >{
   
public:

   TermOrder(const Ring<>& r, const Matrix<Integer>& orderMatrix);

};

template<>
class TermOrder<int>{
   
public:

   TermOrder(const Ring<>& r, const int& singularOrder);

};

template<>
class TermOrder<Vector<Integer> >{
   
public:

   TermOrder(const Ring<>& r, const Vector<Integer>& weightVector);

};
*/


/*
   Ideal& operator+=(const Ideal& I) {
      if(this->get_ring() != I.get_ring()) throw std::runtime_error("Ideals of different rings.");
      append(I.size(),I.begin());
      return static_cast<Ideal&>(*this);
   }

   friend Ideal operator+(const Ideal& i1, const Ideal& i2) {
      if(i1.get_ring() != i2.get_ring()) throw std::runtime_error("Ideals of different rings.x");
      Ideal result = i1;
      result+=i2;
      return result;
   }

   Ideal groebner()
   {
      if(singObj == NULL) {
         //Ideal* writable = const_cast<Ideal*>(this);
         singObj = SingularIdeal::create(this);
      }
      SingularIdeal* basis = singObj->groebner();
      return Ideal(basis->polynomials(this->get_ring()),basis);

   }

};
*/


} // end namespace ideal
} // end namespace polymake

