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

#include "polymake/client.h"

#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include <fstream>
#include <iomanip>
#include <cmath>

namespace polymake { namespace polytope {

namespace {
Int char_length(Int value, Int base){
  Int len = 1;
  while(value>pow(base,len)){
    len++;
  }
  return len;
}


template<typename Scalar>
auto start_loop(const GenericMatrix<Scalar> &M){
  auto v = entire<indexed>(cols(M));
  if(!v.at_end()){
    return ++v;
  }else{
    return v; 
  }
}

template<typename Vector>
void print_col(std::ostream& os,
               const std::string& variable_name,
               const GenericVector<Vector>& v,
               const Array<std::string>& row_names)
{
   // This function is designt to manage the printing of a single column of the polytope
   Matrix<typename Vector::element_type> help_matrix = call_function("remove_zero_rows",vector2col(v));
   if (is_zero(help_matrix.rows())){ // break if varaibles is not used
      return;
   }
   
   bool second = false; // holds if the entry is the first or second in his output line
   for(auto e = entire<indexed>(v.top()); !e.at_end(); ++e){
      double value = convert_to<double>(*e);
      // ignore zero entries or rows without names
      if(is_zero(value) or row_names[e.index()].empty()){
         continue;
      }
      if(!second){
      	 // write variable name, if the line is empty
         os << std::string(4,' ') << variable_name 
            << std::string(std::max(Int(10-variable_name.size()),Int(2)), ' ');
      }
      
      os << row_names[e.index()] << std::string(2,' ');
      
      // convert value to string to calculate the needed white space
      std::stringstream value_stream;
      value_stream << std::setprecision(16) <<value;
      std::string value_string = value_stream.str(); 
      
      os << value_string;
      
      if(!second){
         // write space to the next entry
         os << std::string(std::max(Int(25-value_string.size()),Int(2)),' ');
      }else{
         // or end line
         os << "\n";
      }
      second = !second;
   }
   
   if(second){
     os << "\n";
   }
}
} // end anonymous namespace

template<typename Scalar, bool is_lp>
void print_lp(BigObject p, BigObject lp, Set<Int> br, std::ostream& os)
{
   // This function write the whole lp discribed by p and lp to
   // the output way os. The inequalities in br where treated as veriable bounds
   const Int is_feasible=p.give("FEASIBLE");
   const SparseMatrix<Scalar> 
      IE = p.give("FACETS | INEQUALITIES"), // TODO find a way to remove 1>=0
      EQ = p.lookup("AFFINE_HULL | EQUATIONS");
   const SparseVector<Scalar> LO = lp.give("LINEAR_OBJECTIVE");
   const Int n_variables = IE.cols()-1;
   
   // generate the inverse set to br
   Set<Int> br_inv;
   for(Int i=0; i<IE.rows(); ++i){
      if(!br.contains(i)){
        br_inv += i;
      }
   }
   
   // merge all non boundary rows to one file
   const SparseMatrix<Scalar> BIE = IE.minor(br, All);
   const SparseMatrix<Scalar> AM = is_zero(EQ.cols()) ? SparseMatrix<Scalar>(LO/(IE.minor(br_inv,All))) : SparseMatrix<Scalar>(LO/(IE.minor(br_inv,All))/EQ);
   std::string name = lp.name();

   if (!is_feasible)
      throw std::runtime_error("input is not FEASIBLE");
   
   // since the length of the names are fixed we have to check
   // if we can do it with decimal or hex numbers
   bool row_name_dec = true;
   Int rows_coding_len = 7;
   if (AM.rows() > Int(pow(10.0,7.0))){
      if (AM.rows() > Int(pow(16.0,7.0))){
         // if the numbers are to longe we take more space
         rows_coding_len = char_length(AM.rows(),16); 
         
      }
      row_name_dec = false;
   }

   // define the variable names for the output
   Array<std::string> variable_names;
   if (lp.get_attachment("COORDINATE_LABELS") >> variable_names) {
      if (variable_names.size() != n_variables)
         throw std::runtime_error("dimension mismatch between the polytope and COORDINATE_LABELS");
   } else {
      variable_names.resize(n_variables);
      for (Int j=0; j < n_variables; ++j)
         variable_names[j]='x' + std::to_string(j+1);
   }

   // colleced the integer variables
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

   // output some comments
   os << std::setprecision(16) << "* Class:\t"
      << (is_lp ? "LP\n" : "MIP\n");
   os << "* Rows:\t\t" << AM.rows() << "\n";
   os << "* Columns:\t" << n_variables << "\n";
   os << "* Format:\tMPS\n*\n";
   
   // output the name
   os << "Name" << std::string(10, ' ') << name << "\n";
   
   // define a name fo each row and the cost function
   // the write it to the output
   // each name is 8 characters long
   Array<std::string> row_names(AM.rows());
   row_names[0] = "C"+std::string(rows_coding_len,'0');
   os << "ROWS\n" << " N  " << row_names[0] << "\n";
   
   for(Int i=0; i<AM.rows()-1; i++){
      // check if any variable if used in the row
      SparseVector<Scalar> test_v(AM.cols());
      test_v[0] = AM(i+1,0);
      if(SparseVector<Scalar>(AM.row(i+1)) == test_v){
        continue;
      }
      std::stringstream stream;
      stream << "R";
      if(row_name_dec){
        Int len = char_length(i, 10);
        stream << std::string(rows_coding_len-len, '0') << i;
      }else{
        Int len = char_length(i, 16);
        stream << std::string(rows_coding_len-len, '0') << std::hex << i << std::dec;
      }
      row_names[1+i] = stream.str();
      // write the name to the output
      if(i < IE.rows()-BIE.rows()){
         os << " G  ";
      }else{ 
         os << " E  ";
      }
      os << row_names[1+i] << "\n";
   }
   
   
   // write down the non zero entries for each variable
   os << "COLUMNS\n";
   bool intmode_on = false;
   Int marker_nr = 0;
   Int marker_coding_len = std::max(Int(7), char_length(AM.cols()-1,16));
   
   for(auto v =start_loop(AM); !v.at_end(); ++v){
      // add integer marker
      if(!integers.empty() && (integers[v.index()-1] != intmode_on)){ //integers[i-1] XOR intmode_on
         //Int len = ceil(log10(marker_nr)/log10(16));
         Int len = char_length(marker_nr, 16);
         os <<  std::string(4, ' ') << "M" << std::string(marker_coding_len-len, '0'); 
         os << std::hex << marker_nr << std::dec << std::string(2,' ');
         os << "'MARKER'" << std::string(17, ' '); 
         os << ((integers[v.index()-1] && !intmode_on)? "'INTORG'\n" : "'INTEND'\n");
         if(intmode_on){
            marker_nr += 1;
         }
         intmode_on = !intmode_on;
      }
      
      print_col(os, variable_names[v.index()-1], *v, row_names);//AM.col(i), row_names);
      
   }
   
   // close the last Int marker if it is open
      if(intmode_on){
         Int len = char_length(marker_nr, 16);
         os <<  std::string(4, ' ') << "M" << std::string(marker_coding_len-len, '0') << std::hex << marker_nr << std::dec << std::string(2,' ');
         os << "'MARKER'" << std::string(17, ' ') << "'INTEND'\n";
      }
   
   
   // define right hand side
   os << "RHS\n";
   print_col(
      os, 
      "B", 
      -AM.col(0),
      row_names
   );
   
   os << "BOUNDS\n";
   
   // colleged the given variable bounds and save the strongest
   // upper and lower bound for each
   Array<Array<double>> boundary (variable_names.size());
   for(auto v = entire<indexed>(rows(BIE)); !v.at_end(); ++v){
      Scalar rhs = 0;
      for(auto e = entire<indexed>((*v).top()); !e.at_end() ; ++e){
          if(!is_zero(*e)){
             if(is_zero(e.index())){
                rhs = *e;
             }else{
                Int lower_bound = (*e>0) ? 0 : 1;
                Int var_index = e.index()-1;
                double bound = convert_to<double>(- rhs / *e);
                // update boundary entry or create it
                if(boundary[var_index].empty()){
                   boundary[var_index] = Array<double>(2);
                   boundary[var_index][0] = NAN;
                   boundary[var_index][1] = NAN;
                }
                if(std::isnan(boundary[var_index][lower_bound])){
                   // create entry
                   boundary[var_index][lower_bound] = bound;
                }else{
                   //update entry
                   boundary[var_index][lower_bound] = (*e>0) ? std::max(boundary[var_index][lower_bound], bound) : std::min(boundary[var_index][lower_bound], bound);
                }
             }
          }
      } 
   }   
   
   // output the variable bounds or unbounded if non are given
   for(Int i =0; i < variable_names.size(); ++i){
      if(boundary[i].empty()){
         os << " FR BND" << std::string(7, ' ');
         os << variable_names[i] << std::string(2, ' ') << "\n";
      }else{// TODO write nicer
         if(std::isnan(boundary[i][0])){
            os << " MI BND" << std::string(7, ' ');
            os << variable_names[i] << std::string(2, ' ') << "\n";
         }else{
            os << " LO BND" << std::string(7, ' ');
            os << variable_names[i] << std::string(10-variable_names[i].size(), ' ') << boundary[i][0] << "\n";
         }
         if(std::isnan(boundary[i][1])){
            os << " PL BND" << std::string(7, ' ');
            os << variable_names[i] << std::string(2, ' ') << "\n";
         }else{
            os << " UP BND" << std::string(7, ' ');
            os << variable_names[i] << std::string(10-variable_names[i].size(), ' ') << boundary[i][1] << "\n";
         }
      }
   }
   
   os << "ENDATA" << endl;
}

template<typename Scalar>
Int poly2mps(BigObject p, BigObject lp, Set<Int> br, const std::string& file)
{
   if(!(lp.isa("LinearProgram") || lp.isa("MixedIntegerLinearProgram"))){
      throw std::runtime_error("Second argument must be a (MixedInteger)LinearProgram");
   }

   bool is_lp = lp.isa("LinearProgram");
   
   if (file.empty() || file=="-") {
      if(is_lp){
         print_lp<Scalar, true>(p, lp, br, perl::cout);
      } else {
         print_lp<Scalar, false>(p, lp, br, perl::cout);
      }
      return 1;
   } else {
      std::ofstream os(file.c_str());
      os.exceptions(std::ofstream::failbit | std::ofstream::badbit);
      if(is_lp){
         print_lp<Scalar, true>(p, lp, br, os);
      } else {
         print_lp<Scalar, false>(p, lp, br, os);
      }
      return 1;
   }
}

UserFunctionTemplate4perl("# @category Optimization"
                          "# Convert a polymake description of a polyhedron to MPS format (as used by Gurobi and"
                          "# other linear problem solvers) and write it to standard output or to a //file//."
                          "# If //LP// comes with an attachment 'INTEGER_VARIABLES' (of type Array<Bool>),"
                          "# the output will contain markers for them."
                          "# You can give the indices rows, which are just variable bounds (x_i>=b_i or x_i<=b_i)," 
                          "# as a Set. If you do so, the will be in this way."
                          "# If the polytope is not FEASIBLE, the function will throw a runtime error."
                          "# Alternatively one can also provide a //MILP//, instead of a //LP// with 'INTEGER_VARIABLES' attachment."
                          "# @param Polytope P"
                          "# @param LinearProgram LP default value: //P//->LP"
                          "# @param Set<Int> br the possible empty set of inequalities of the form x_i <=/>= b_i, that should be handelt as variable bounds" 
                          "# @param String file default value: standard output",
                          "poly2mps<Scalar>(Polytope<Scalar>; $=$_[0]->LP, Set<Int>=new Set<Int>(), $='')");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
