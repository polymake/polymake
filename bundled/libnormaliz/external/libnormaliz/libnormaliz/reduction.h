/*
 * Normaliz
 * Copyright (C) 2007-2014  Winfried Bruns, Bogdan Ichim, Christof Soeger
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

//---------------------------------------------------------------------------
#ifndef REDUCTION_HPP
#define REDUCTION_HPP
//---------------------------------------------------------------------------
#include <vector>
#include <list>
#include <iostream>
#include <string>

#include "libnormaliz/full_cone.h"
#include "libnormaliz/list_operations.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using std::list;
using std::vector;
using std::string;

template<typename Integer> class CandidateList;
template<typename Integer> class CandidateTable;
template<typename Integer> class Full_Cone;
template<typename Integer> class Cone_Dual_Mode;

template<typename Integer>
class Candidate { // for reduction

  friend class CandidateList<Integer>;
  friend class CandidateTable<Integer>;

  public:

  vector<Integer> cand;  // the vector
  vector<Integer> values; // values under support forms
  long sort_deg;  // the sorting degree
  bool reducible;    // indicates reducibility
  bool original_generator; // marks the original generators in the primal algorithm

  Integer mother;   // for the dual algorithm
  size_t old_tot_deg;


  // construct candidate from given components
  Candidate(const vector<Integer>& v, const vector<Integer>& val, long sd);

  // construct candidate from a vector and the support forms of C
  Candidate(const vector<Integer>& v, const Full_Cone<Integer>& C);

  // construct candidate for dual algorithm
  Candidate(const vector<Integer>& v, size_t max_size);
  Candidate(size_t cand_size, size_t val_size);

  // compute values and sort deg, cand must be set beforehand
  void compute_values_deg(const Full_Cone<Integer>& C);
}; //end class

template<typename Integer>
bool deg_compare(const Candidate<Integer>& a, const Candidate<Integer>& b);

template<typename Integer>
bool val_compare(const Candidate<Integer>& a, const Candidate<Integer>& b);

template <typename Integer>
ostream& operator<< (ostream& out, const Candidate<Integer>& c) {
    out << "cand=" << c.cand;
    out << ", values=" << c.values;
    out << ", sort_deg=" << c.sort_deg;
    out << ", reducible=" << c.reducible;
    out << ", original_generator=" << c.original_generator;
//    out << ", generation=" << c.generation;
    out << ", mother=" << c.mother;
    out << ", old_tot_deg=" << c.old_tot_deg;
    // out << ", in_HB=" << c.in_HB;
    return out;
}

template<typename Integer>
class CandidateList {

  friend class Full_Cone<Integer>;
  friend class Cone_Dual_Mode<Integer>;

  public:
      
  bool verbose;

  list <Candidate<Integer> > Candidates;
  bool dual;
  size_t last_hyp;
  Candidate<Integer> tmp_candidate; // buffer to avoid reallocation

  CandidateList(); 
  CandidateList(bool dual_algorithm); 


  // Checks for irreducibility
  bool is_reducible(const vector<Integer>& values, const long sort_deg) const; // basic function
  // bool is_reducible_last_hyp(const vector<Integer>& values, const long sort_deg) const;
  // including construction of candidate
  bool is_reducible(Candidate<Integer>& c) const;
  // bool is_reducible_last_hyp(Candidate<Integer>& c) const;
  bool is_reducible(vector<Integer> v,Candidate<Integer>& cand, const Full_Cone<Integer>& C) const;

  // reduction against Reducers and insertion into *this. returns true if inserted
  bool reduce_by_and_insert(Candidate<Integer>& cand, const CandidateList<Integer>& Reducers);
  // including construction of candidate
  bool reduce_by_and_insert(const vector<Integer>& v, Full_Cone<Integer>& C, CandidateList<Integer>& Reducers); 

  // reduce *this against Reducers
  void reduce_by(CandidateList<Integer>& Reducers);
  // reduce *this against itself
  void auto_reduce();
  void auto_reduce_sorted();
  // void unique_auto_reduce(bool no_pos_in_level0);

  // erases dupicate elements in Candidates
  void unique_vectors();



  void sort_by_deg();
  void sort_by_val();
  void divide_sortdeg_by2();
  void merge(CandidateList<Integer>& NewCand);
  void merge_by_val(CandidateList<Integer>& NewCand);
  void merge_by_val(CandidateList<Integer>& NewCand,list<Candidate<Integer>* >& New_Elements);
  void merge_by_val_inner(CandidateList<Integer>& NewCand, bool collect_new_elements, 
                list<Candidate<Integer>* >& New_Elements);
  void splice(CandidateList<Integer>& NewCand);
  void extract(list<vector<Integer> >& V_List);
  void push_back(const Candidate<Integer>& c);
  void clear();
  size_t size();
  bool empty();

}; // end class

template <typename Integer>
ostream& operator<< (ostream& out, const CandidateList<Integer>& cl) {
    out << "CandidateList with " << cl.Candidates.size() << " candidates." << endl;
    out << "dual = " << cl.dual << "    last_hyp = " << cl.last_hyp << endl;
    out << cl.Candidates;
    return out;
}

template<typename Integer>
class CandidateTable {  // for parallelized reduction with moving of reducer to the front

  friend class CandidateList<Integer>;

  public:

  list < pair< size_t, vector<Integer>* > > ValPointers;
  bool dual;
  size_t last_hyp;

  CandidateTable(CandidateList<Integer>& CandList);
  CandidateTable(bool dual, size_t last_hyp);

  bool is_reducible(Candidate<Integer>& c);
  bool is_reducible(const vector<Integer>& values, const long sort_deg);
  bool is_reducible_unordered(Candidate<Integer>& c);
  bool is_reducible_unordered(const vector<Integer>& values, const long sort_deg);

}; // end class


} // namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

