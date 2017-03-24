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

#ifndef __POLYMAKE_POLY2LP_H__
#define __POLYMAKE_POLY2LP_H__

#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Array.h"
#include <fstream>

namespace polymake { namespace polytope {
namespace {

template <typename Vector>
void print_row(std::ostream& os,
               const std::string& tag,
               int index,
               const GenericVector<Vector>& v,
               const Array<std::string>& variable_names,
               const char* relop=0)
{
   if (v == unit_vector<typename Vector::element_type>(v.dim(),0)) // don't print the line " 0 >= -1 "
      return;
   typename Entire<Vector>::const_iterator e(entire(v.top()));
   typename Vector::element_type free_term(0);
   if (!e.at_end() && e.index()==0) {
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

template<typename Scalar=Rational>
void print_lp(perl::Object p, perl::Object lp, const bool maximize, std::ostream& os)
{
   const int is_feasible=p.give("FEASIBLE");
   const SparseMatrix<Scalar> 
      IE = p.give("FACETS | INEQUALITIES"),
      EQ = p.lookup("AFFINE_HULL | EQUATIONS");
   const SparseVector<Scalar> LO = lp.give("LINEAR_OBJECTIVE");
   const int n_variables = IE.cols()-1;

   if (!is_feasible)
      throw std::runtime_error("input is not FEASIBLE");

   Array<std::string> variable_names;
   if (lp.get_attachment("VARIABLE_NAMES") >> variable_names) {
      if (variable_names.size() != n_variables)
         throw std::runtime_error("dimension mismatch between the polytope and VARIABLE_NAMES");
   } else {
      variable_names.resize(n_variables);
      for (int j=0; j<n_variables; ++j)
         variable_names[j]='x' + std::to_string(j+1);
   }

   Array<bool> integers = lp.get_attachment("INTEGER_VARIABLES");

   os << std::setprecision(16)
      << (maximize ? "MAXIMIZE\n" : "MINIMIZE\n");
   
   print_row(os, "obj", 0, LO, variable_names);

   os << "Subject To\n";
   for (typename Entire<Rows<SparseMatrix<Scalar>>>::const_iterator ie=entire(rows(IE)); !ie.at_end(); ++ie) {
      print_row(os, "ie", ie.index(), *ie, variable_names, ">=");
   }

   for (typename Entire<Rows<SparseMatrix<Scalar>>>::const_iterator eq=entire(rows(EQ)); !eq.at_end(); ++eq) {
      print_row(os, "eq", eq.index(), *eq, variable_names, "=");
   }

   os << "BOUNDS\n";
   for (int i=0; i<n_variables; ++i)
      os << "  " << variable_names[i] << " free\n"; 

   if (!integers.empty()) {
      os << "GENERAL\n";
      for (int i=0; i<integers.size(); ++i)
         if (integers[i]) os << "  " << variable_names[i] << '\n';
   }
   os << "END" << endl;
}

} }

#endif // __POLYMAKE_POLY2LP_H__

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
