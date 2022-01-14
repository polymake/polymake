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

#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include <fstream>
#include "polymake/linalg.h"

namespace polymake { namespace polytope {
namespace {

template<typename Scalar, std::enable_if_t<!std::is_same<Rational, Scalar>::value, int> = 42>
void multiply_by_lcm_denom(SparseVector<Scalar>& v){
}

template<typename Scalar, std::enable_if_t<std::is_same<Rational, Scalar>::value, int> = 42>
void multiply_by_lcm_denom(SparseVector<Scalar>& v){
   Integer s = lcm(denominators(v));
   if(s > 10000) return; // If the lcm is higher than 10000, it might be better to just stay with the fractions
   auto e = entire(v.top());
   while (!e.at_end()){
      *e = *e*s;
      e++;
   }
}

template <typename VectorType>
void print_row(std::ostream& os,
               const std::string& tag,
               Int index,
               const GenericVector<VectorType>& v,
               const Array<std::string>& variable_names,
               const char* relop = nullptr)
{
   if (v == unit_vector<typename VectorType::element_type>(v.dim(),0)) // don't print the line " 0 >= -1 "
      return;
      
   SparseVector<typename VectorType::element_type> tmp(v);
   // multiply inequality/equation by lcm
   if (tag=="ie" || tag=="eq") {
      multiply_by_lcm_denom(tmp);
   }
   
   auto e = entire(tmp.top());
   typename VectorType::element_type free_term(0);

   if (!e.at_end() && e.index() == 0) {
      free_term=*e;  ++e;
   }
   os << "  " << tag;
   if (tag != "obj") os << index;
   os << ":";

   while (!e.at_end()) {
         os << ' ' << std::setiosflags(std::ios::showpos) << convert_to<double>(*e) << std::resetiosflags(std::ios::showpos)
            << ' ' << variable_names[e.index()-1];
      ++e;
   }
   if (relop) {
      os << ' ' << relop << ' ' << convert_to<double>(-free_term);
   } else if (!is_zero(free_term)) {
      os << ' ' << std::setiosflags(std::ios::showpos) << convert_to<double>(free_term) << std::resetiosflags(std::ios::showpos);
   }
   os << '\n';
}
} // end anonymous namespace

template<typename Scalar, bool is_lp>
void print_lp(BigObject p, BigObject lp, const bool maximize, std::ostream& os)
{
   const Int is_feasible=p.give("FEASIBLE");
   const SparseMatrix<Scalar> 
      IE = p.give("FACETS | INEQUALITIES"),
      EQ = p.lookup("AFFINE_HULL | EQUATIONS");
   const SparseVector<Scalar> LO = lp.give("LINEAR_OBJECTIVE");
   const Int n_variables = IE.cols()-1;

   if (!is_feasible)
      throw std::runtime_error("input is not FEASIBLE");

   Array<std::string> variable_names;
   if (lp.get_attachment("COORDINATE_LABELS") >> variable_names) {
      if (variable_names.size() != n_variables)
         throw std::runtime_error("dimension mismatch between the polytope and COORDINATE_LABELS");
   } else {
      variable_names.resize(n_variables);
      for (Int j=0; j < n_variables; ++j)
         variable_names[j]='x' + std::to_string(j+1);
   }

   Array<bool> integers(LO.dim());
   if(is_lp){
      Array<bool> tmp = lp.get_attachment("INTEGER_VARIABLES");
      integers = tmp;
   } else {
      Set<Int> tmp = lp.give("INTEGER_VARIABLES");
      for(const auto& e : tmp){
         integers[e] = true;
      }
   }

   os << std::setprecision(16)
      << (maximize ? "MAXIMIZE\n" : "MINIMIZE\n");
   
   print_row(os, "obj", 0, LO, variable_names);

   os << "Subject To\n";
   for (auto ie=entire(rows(IE)); !ie.at_end(); ++ie) {
      print_row(os, "ie", ie.index(), *ie, variable_names, ">=");
   }

   for (auto eq=entire(rows(EQ)); !eq.at_end(); ++eq) {
      print_row(os, "eq", eq.index(), *eq, variable_names, "=");
   }

   os << "BOUNDS\n";
   for (Int i = 0; i < n_variables; ++i)
      os << "  " << variable_names[i] << " free\n"; 

   if (!integers.empty()) {
      os << "GENERAL\n";
      for (Int i = 0; i < integers.size(); ++i)
         if (integers[i]) os << "  " << variable_names[i] << '\n';
   }
   os << "END" << endl;
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
