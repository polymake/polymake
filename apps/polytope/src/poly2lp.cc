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

#include "polymake/client.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Array.h"
#include <fstream>

namespace polymake { namespace polytope {
namespace {

template <typename Vector>
void print_row(std::ostream& os, const GenericVector<Vector>& v, const Array<std::string>& variable_names, const char* relop=0)
{
   typename Entire<Vector>::const_iterator e=entire(v.top());
   typename Vector::element_type free_term=0;
   if (!e.at_end() && e.index()==0) {
      free_term=*e;  ++e;
   }
   while (!e.at_end()) {
      os << ' ' << std::setiosflags(std::ios::showpos) << *e << std::resetiosflags(std::ios::showpos)
         << ' ' << variable_names[e.index()-1];
      ++e;
   }
   if (relop) {
      os << ' ' << relop << ' ' << -free_term;
   } else if (free_term) {
      os << ' ' << std::setiosflags(std::ios::showpos) << free_term << std::resetiosflags(std::ios::showpos);
   }
   os << '\n';
}
} // end anonymous namespace

void print_lp(perl::Object p, perl::Object lp, const bool maximize, std::ostream& os)
{
   const int is_feasible=p.give("FEASIBLE");
   const SparseMatrix<double> IE=p.give("FACETS | INEQUALITIES"),
      EQ=p.lookup("AFFINE_HULL | EQUATIONS");
   const SparseVector<double> LO=lp.give("LINEAR_OBJECTIVE");
   const int n_variables=IE.cols()-1;

   if (!is_feasible)
      throw std::runtime_error("input is not FEASIBLE");

   Array<std::string> variable_names;
   if (lp.get_attachment("VARIABLE_NAMES") >> variable_names) {
      if (variable_names.size() != n_variables)
         throw std::runtime_error("dimension mismatch between the polytope and VARIABLE_NAMES");
   } else {
      variable_names.resize(n_variables);
      for (int j=0; j<n_variables; ++j) {
         std::ostringstream name;
         name << 'x' << j+1;
         variable_names[j]=name.str();
      }
   }

   Array<bool> integers = lp.get_attachment("INTEGER_VARIABLES");

   if(maximize){
      os << std::setprecision(16)
         << "MAXIMIZE\n"
         << "  obj:";
   } else {
      os << std::setprecision(16)
         << "MINIMIZE\n"
         << "  obj:";
   }
   print_row(os, LO, variable_names);

   os << "Subject To\n";
   for (Entire< Rows< SparseMatrix<double> > >::const_iterator ie=entire(rows(IE)); !ie.at_end(); ++ie) {
      os << "  ie" << ie.index() << ':';      // cplex enumerates from 1, ...
      print_row(os, *ie, variable_names, ">=");
   }

   for (Entire< Rows< SparseMatrix<double> > >::const_iterator eq=entire(rows(EQ)); !eq.at_end(); ++eq) {
      os << "  eq" << eq.index() << ':';      // cplex enumerates from 1, ...
      print_row(os, *eq, variable_names, "=");
   }

   os << "BOUNDS\n";
   for (int i=0; i<n_variables; ++i)
      os << "  " << variable_names[i] << " free\n"; 

   if (!integers.empty()) {
      os << "GENERAL\n";
      for (int i=0; i<n_variables; ++i)
         if (integers[i]) os << "  " << variable_names[i] << '\n';
   }
   os << "END" << endl;
}

void poly2lp(perl::Object p, perl::Object lp, const bool maximize, const std::string& file)
{
   if (file.empty() || file=="-") {
      print_lp(p,lp,maximize,std::cout);
   } else {
      std::ofstream os(file.c_str());
      print_lp(p,lp,maximize,os);
   }
}

UserFunction4perl("# @category Optimization"
                  "# Convert a polymake description of a polyhedron to LP format (as used by CPLEX and"
                  "# other linear problem solvers) and write it to standard output or to a //file//."
                  "# If //LP// comes with an attachment 'INTEGER_VARIABLES' (of type Array<Bool>),"
                  "# the output will contain an additional section 'GENERAL',"
                  "# allowing for IP computations in CPLEX."
                  "# If the polytope is not FEASIBLE, the function will throw a runtime error."
                  "# @param Polytope P"
                  "# @param LinearProgram LP default value: //P//->LP"
                  "# @param Bool maximize produces a maximization problem; default value: 0 (minimize)"
                  "# @param String file default value: standard output",
                  &poly2lp, "poly2lp(Polytope; LinearProgram=$_[0]->LP, $=0, $='')");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
