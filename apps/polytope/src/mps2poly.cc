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

#include "polymake/client.h"

#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/hash_map"
#include "polymake/linalg.h"
#include <fstream>
#include <cmath> /* isnan */


namespace polymake { namespace polytope {

namespace {

// &variable_bound
// value is not used for FR, MI, PL and BV
template <typename Scalar>
bool set_bound(Array<Scalar> &variable_bound, const std::string &bound_type, Scalar value = zero_value<Scalar>()){
  if(variable_bound.empty()){
    variable_bound = Array<Scalar>(2);
    variable_bound[0] = 0;
    variable_bound[1] = std::numeric_limits<Scalar>::infinity();
  }
  if(bound_type == "LO"){
    variable_bound[0] = value; // lower bound
  }else if(bound_type == "UP"){
    variable_bound[1] = value; // upper bound
  }else if(bound_type == "FX"){  
    // fixed variable
    variable_bound[0] = value;
    variable_bound[1] = value;
  }else if(bound_type == "FR"){  
    // free variable 
    variable_bound[0] = -std::numeric_limits<Scalar>::infinity();
    variable_bound[1] = std::numeric_limits<Scalar>::infinity();
  }else if(bound_type == "MI"){ 
    variable_bound[0] = -std::numeric_limits<Scalar>::infinity(); // no lower bound
  }else if(bound_type == "PL"){ 
    variable_bound[1] = std::numeric_limits<Scalar>::infinity(); // no upper bound
  }else if(bound_type == "BV"){  
    // binary variable
    variable_bound[0] = 0;
    variable_bound[1] = 1;
    return true;
  }else if(bound_type == "LI"){
    // lower bound for an integer
    variable_bound[0] = value;
    return true;
  }else if(bound_type == "UI"){
    // upper bound for an integer
    variable_bound[1] = value;
    return true;
  }else{
    throw std::runtime_error("Unknown variable bound type '" + bound_type + "'.");
  }
  
  return false;
} 

std::string string_to_lower(const std::string& str) {
   std::string res;
   std::transform(str.begin(), str.end(), std::back_inserter(res), ::tolower);
   return res;
}

template <typename T>
void parse_scalar(std::istream& is, T& d) {
   is >> d;
}

template <>
__attribute__((used))
void parse_scalar(std::istream& is, Rational& r) {
   std::string str;
   is >> str;
   r.set(str.c_str());
}

}

template<typename Scalar>
BigObject mps2poly(std::string file, std::string prefix, bool use_lazy){
  // use_lazy decides if lazyconstrains are used as rows or ignored.
  std::ifstream input(file);
  std::string WHITESPACE = " \n\r\t\f\v";
  /*Set<std::string> WHITESPACE_SET(std::string(" "));
  WHITESPACE_SET+= "\n"; 
  WHITESPACE_SET+= "\r";
  WHITESPACE_SET+= "\t";
  WHITESPACE_SET+= "\f";
  WHITESPACE_SET+= "\v";
  */
  if(!input){
    throw std::runtime_error("Can't open the file " + file);
  }
  
  std::string line;
  // go over comments
  while (std::getline(input, line)){
    if(line[0] != '*'){ break;}
  }
  
  std::string poly_name;
  if((string_to_lower(line.substr(0,4))=="name")) {
    if (line.find_last_not_of(WHITESPACE)>=14)
      poly_name = line.substr(14, line.find_last_not_of(WHITESPACE)-13);
    std::getline(input, line);
  }
  
  // go over comments
  do {
    if(line[0] != '*'){ break;}
  } while (std::getline(input, line));
  
  //get rows
  if(string_to_lower(line.substr(0,4)) != "rows"){
    throw std::runtime_error("Can't find the rows");
  }
  
  // define a map from the names of the rows to the index and type
  hash_map<std::string,std::pair<Int,std::string>> Rows;
  
  // tools to deal with the different types of "rows"
  Array<Int> counter(3);
  counter[0] = 0;
  counter[1] = 0;
  counter[2] = 0;
  hash_map<std::string,Int> letter2index;
  letter2index[std::string("N")] = 0;
  letter2index[std::string("G")] = 1;
  letter2index[std::string("L")] = 1;
  letter2index[std::string("E")] = 2;
  
  // fill the map with rows and lazyconstrains
  while(true){
    while(std::getline(input,line)){
      if(line[0] == '*'){
        continue;
      }else if(!isspace(line[0])){
        break;
      }
      
      std::stringstream stream_line;
      stream_line << line;
      
      std::string type;
      stream_line >> type;
      std::string row_name;
      stream_line >> row_name;
      Int index = counter[letter2index[type]];
      counter[letter2index[type]] += 1;
      
      // check if the row name already exists
      if( Rows.find(row_name) != Rows.end() ){
        //throw error name is used two times
        throw std::runtime_error("The row name '"+row_name+"' was defined twice.");
      }
      // and if not create it
      Rows[row_name] = std::make_pair(index, type);
      
    }
    // if wanted check lazy constrains
    if(use_lazy && (line.substr(0,8) == "LAZYCONS")){
      continue;
    }else{
      break;
    }
  }
  
  // ignore lazy constrains if not used
  while(!use_lazy && (line.substr(0,8) == "LAZYCONS")){
    while(std::getline(input,line)){
      if(line[0] == '*'){
        continue;
      }else if(!isspace(line[0])){
        break;
      }else{
        continue;
      }
    }
  }
  if(line.substr(0,7) != "COLUMNS"){
    throw std::runtime_error("Column declaretion not in the right place. The declaration of the columns has to come after the declaration of the rows.");
  }
  
  // define a map from the names of the cols to the index
  hash_map<std::string,size_t> Cols;
  Set<Int> integer_variables;
  
  // Build constrains and costfunction from COLUMNS
  // since the number of columns are not known right now
  // each of them is represented als a (vector) entry in a c++
  // vector 
  Array<std::vector<SparseVector<Scalar>>> cols_Matrices(3);
  bool int_mode = false;
  while(std::getline(input,line)){
    if(line[0] == '*'){
      continue;
    }else if(!isspace(line[0])){
      break;
    }
    std::stringstream stream_line;
    stream_line << line;
    std::string col;
    std::string row;
    Scalar value;
    stream_line >> col;
    stream_line >> row;
    
    // check if it is a marker
    if((row.size()>3) && (row.substr(1,row.size()-2) == "MARKER")){
      // TODO make bullet proof against craise structures
      int_mode = !int_mode;
      continue;
    }
    
    // check if variable name is known and if not add it to the map
    if(Cols.find(col) == Cols.end()){
      Int size = Cols.size() + 1;
      Cols[col] = size;
      if(int_mode){
        integer_variables += size;
      }
    }
    
    // get the entries
    do{
      if (row[0] == '$')
         break;
      parse_scalar(stream_line, value);
      Int pos = Rows[row].first;
      
      std::string letter = Rows[row].second;
      Int index = letter2index[letter];
      
      // add columns until we reach the needed number for this entry
      // the first column with index 0 is for the right hand side
      while (cols_Matrices[index].size() <= Cols[col]) {
        SparseVector<Scalar> v(counter[index]);
        cols_Matrices[index].push_back(v);
      }
      cols_Matrices[index][Cols[col]][pos] = (letter == "L") ? - value : value;
      
    }while(stream_line >> row);
  }

  
  if(line.substr(0,3) != "RHS"){
    throw std::runtime_error("The definition of the right hand sight (RHS) is not in the right place. The definition has to come after the declaration of the columns.");
  }
  
  // get right hand side and objective of set
  while(std::getline(input,line)){
    if(line[0] == '*'){
      continue;
    }else if(!isspace(line[0])){
      break;
    }
    std::stringstream stream_line;
    stream_line << line;
    std::string name;
    std::string row;
    Scalar value;
    stream_line >> name; // can be ignored
    stream_line >> row;
    do{
      parse_scalar(stream_line, value);
      Int pos = Rows[row].first;
      
      std::string letter = Rows[row].second;
      Int index = letter2index[letter];
      
      cols_Matrices[index][0][pos] = (letter == "L") ? value : - value;
      
    }while(stream_line >> row); 
  }
  
  // now we get the variable bounds
  if(line.substr(0,6) != "BOUNDS"){
    throw std::runtime_error("The definition of the variable bounds is not in the right place. The definition has to come after the definition of the right hand sight (RHS).");
  }
  
  //first read
  Array<Array<Scalar>> variable_bounds(Cols.size());
  // get right hand side and objective of set
  while(std::getline(input,line)){
    if(line[0] == '*'){
      continue;
    }else if(!isspace(line[0])){
      break;
    }
    
    std::stringstream stream_line;
    stream_line << line;
    
    std::string bound_type;
    std::string name;
    std::string col;
    Scalar value;
    
    stream_line >> bound_type;
    stream_line >> name; // will be ignored
    stream_line >> col;    // check if variable name is known and if not add it to the map
    // we might have variables that only appear in the bounds
    if(Cols.find(col) == Cols.end()){
      Int size = Cols.size() + 1;
      Cols[col] = size;
      variable_bounds.resize(size);
    }
    if((bound_type == "FR")||(bound_type == "MI")||(bound_type == "PL")||(bound_type == "BV")){
      if(set_bound(variable_bounds[Cols[col]-1], bound_type)){
        integer_variables += Int(Cols[col]);
      }
    }else{
      parse_scalar(stream_line, value);
      if(set_bound(variable_bounds[Cols[col]-1], bound_type, value)){
        integer_variables += Int(Cols[col]);
      }
    }
  }
  
  // makes shure the number of columns is for all the same if it exist
  // adjusting for new variables from the bounds section now
  for (Int index=0; index<3; ++index){
    if(counter[index]>0){
      while (cols_Matrices[index].size() <= Cols.size()) {
        SparseVector<Scalar> v(counter[index]);
        cols_Matrices[index].push_back(v);
      }
    }
  }
  
  // build matrix out of bounds
  // here the entries of the c++ vector are the rows of the matrices
  Array<ListMatrix<SparseVector<Scalar>>> bound_Matrices(2);
  for(Int i = 0; i<variable_bounds.size(); ++i){
    SparseVector<Scalar> v(Cols.size()+1);
    if(variable_bounds[i].empty()){
      // 0 <= x <= inf
      v[i+1] = 1;
      bound_Matrices[0] /= v;
    }else if(variable_bounds[i][0] == variable_bounds[i][1]){
      // x = a
      v[i+1] = -1;
      v[0] = variable_bounds[i][0];
      bound_Matrices[1] /= v;
    }else{
      // a <= x <= b
      if(!isinf(variable_bounds[i][0])){
        // set lower bound
        v[i+1] = 1;
        v[0] = - variable_bounds[i][0];
        bound_Matrices[0] /= v;
      }
      if(!isinf(variable_bounds[i][1])){
        // set upper bound
        v[i+1] = -1;
        v[0] =  variable_bounds[i][1];
        bound_Matrices[0] /= v;
      }
    }
  }
  
  BigObject p("Polytope", mlist<Scalar>());
  if (!poly_name.empty())
    p.set_name(poly_name);
  
  // compose inequality matrix
  ListMatrix<SparseVector<Scalar>> ineq(0, Cols.size()+1);
  if (cols_Matrices[1].size() > 0) {
    ineq /= T(SparseMatrix<Scalar>(cols_Matrices[1]));
  }
  if (bound_Matrices[0].rows() > 0) {
    ineq /= bound_Matrices[0];
  }
  p.take("INEQUALITIES") << remove_zero_rows(ineq);
  
  // compose equation matrix
  SparseMatrix<Scalar> eq(0, Cols.size()+1);
  if (cols_Matrices[2].size() > 0) {
    eq /= remove_zero_rows(T(SparseMatrix<Scalar>(cols_Matrices[2])));
  }
  if(bound_Matrices[1].rows() > 0){
    eq /= remove_zero_rows(bound_Matrices[1]);
  }
  if (eq.rows() > 0) {
    p.take("EQUATIONS") << eq;
  }
  
  if (counter[0] == 1){
    SparseVector<Scalar> obj(SparseMatrix<Scalar>(cols_Matrices[0]).col(0));
    if (integer_variables.size() > 0){
      BigObject milp("MixedIntegerLinearProgram", mlist<Scalar>());
      milp.take("LINEAR_OBJECTIVE") << obj;
      milp.take("INTEGER_VARIABLES") << integer_variables;
      p.take("MILP") << milp;
    } else {
      BigObject lp("LinearProgram", mlist<Scalar>());
      lp.take("LINEAR_OBJECTIVE") << obj;
      p.take("LP") << lp;
    }
  } else if (counter[0] > 1) {
    SparseMatrix<Scalar> objs(cols_Matrices[0]);
    if (integer_variables.size() > 0) {
      for (auto v = entire(cols(objs)); !v.at_end(); ++v){
        BigObject milp("MixedIntegerLinearProgram", mlist<Scalar>());
        milp.take("LINEAR_OBJECTIVE") << *v;
        milp.take("INTEGER_VARIABLES") << integer_variables;
        p.add("MILP", milp);
      }
    } else {
      for (auto v = entire(cols(objs)); !v.at_end(); ++v) {
        BigObject lp("LinearProgram", mlist<Scalar>());
        lp.take("LINEAR_OBJECTIVE") << *v;
        p.add("LP", lp);
      }
    }
  
  }
  
  return p;
}

UserFunctionTemplate4perl("# @category Optimization"
                          "# Read a linear problem or mixed integer problem  from in MPS-Format"
                          "# (as used by Gurobi and other linear problem solvers) and convert it to"
                          "# a [[Polytope<Scalar>]] object with one or multiple added LP property" 
                          "# or MILP property."
                          "# This interface has some limitations since the MPS-Format offer a wide"
                          "# range of functionalities, which are not all compatible with polymake" 
                          "# right now."
                          "# @tparam Scalar coordinate type of the resulting polytope; default is rational"
                          "# @param [complete file] String file filename of a linear programming problem in MPS-Format"
                          "# @param String prefix If prefix is present, all variables in the LP file are assumed to have the form $prefix$i"
                          "# @param Bool use_lazy Also use the lazy constrains if they are given to build the polytope.",
                          "mps2poly<Scalar=Rational>(String; String='x', Bool=false)");


}}
