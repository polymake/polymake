/* Copyright (c) 1997-2022
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
#include "polymake/topaz/complex_tools.h"
#include "polymake/Set.h"
#include "polymake/list"

namespace polymake { namespace topaz {
namespace {

struct LabeledFacet {
   Set<Int> facet;
   Int label;
   Int index;
        
   bool operator== (const LabeledFacet& otherLF) const 
   {
      return facet == otherLF.facet && index == otherLF.index;
   }
        
   LabeledFacet(const Set<Int>& F, const Int lab, const Int ind)
      : facet(F)
      , label(lab)
      , index(ind) {}
};

typedef std::list<LabeledFacet> LabeledFacetList;
typedef std::vector<Set<Int>> facet_vector;
typedef std::pair<Int, Int> pair_int;

bool is_boundary_ridge(const facet_vector& C, const Set<Int>& F)
{
   Int num_inc = 0;
   for (facet_vector::const_iterator i=C.begin(); i!=C.end(); ++i) {            
      if(!i->empty() && (F - *i).empty()) {
         if (num_inc == 1) return false;
         ++num_inc;
      }
   }
   return (num_inc == 1) ? true : false;
}

Int num_boundary_ridges(const facet_vector& C, const Set<Int>& facet)
{
   Int num = 0;
   for (auto i = entire(all_subsets_less_1(facet));  !i.at_end(); ++i) {
      if (is_boundary_ridge(C,*i)) ++num;
   }
   return num;
}

bool next_candidate(const facet_vector& fta, const Array<Int>& hvec, const Int smallest_index, Int* cur_index)
{
   const Int dim = hvec.size()-2;
   for (Int candidate = smallest_index; candidate < Int(fta.size()); ++candidate) {
      if (!fta[candidate].empty() && hvec[dim+1-num_boundary_ridges(fta, fta[candidate]) ] > 0) {
         *cur_index = candidate;
         return true;
      }
   }
   return false;
}

bool larger_facet_exists(const LabeledFacetList& assigned_facets, const facet_vector& facets_to_assign, const Int cur_index)
{
   Int k = num_boundary_ridges(facets_to_assign, facets_to_assign[cur_index]);           
   facet_vector next_fta = facets_to_assign;
   next_fta[cur_index] = Set<Int>();
   for (const auto& del : assigned_facets) {
      if (del.label == k && (facets_to_assign[cur_index] - del.facet).empty() 
          && num_boundary_ridges(next_fta, del.facet) == k) {
         return true;
      }
   }
   return false;
}

Array<Int> binomial_expansion(const Int n, const Int k)
{
   Int tmp_n = n;
   Int tmp_k = k;
   Array<Int> bexp(k);
   while (0 < tmp_n && tmp_k != 0) {
      Integer tmp;
      for (tmp = tmp_k-1; Integer::binom(tmp+1, tmp_k) <= tmp_n; ++tmp);
      bexp[k - tmp_k] = Int(tmp);
      tmp_n -= Int(Integer::binom(tmp, tmp_k));
      --tmp_k;
   }
   for (; tmp_k > 0; --tmp_k)
      bexp[k-tmp_k]=0;
   return bexp;
}

Int binomial_delta(const Array<Int>& bexp)
{
   Int bdelta = 0;
   const Int k = bexp.size();
   if (bexp[0] == 0) return bdelta;
   for (Int i = 0; i < k-1; ++i) {
      if (bexp[i] == 0) break;
      bdelta += Int(Integer::binom(bexp[i]-1, k-i-1));
   }
   return bdelta;
}

bool is_M_sequence(const Array<Int>& vec, hash_map<pair_int, Array<Int>>& expansion, const Int ci)
{
   pair_int cur_index(ci,vec[ci]);
   if (ci==0) {
      if (vec[0] != 1) return false;
   } else {
      if (expansion.find(cur_index)==expansion.end()) expansion[cur_index] = binomial_expansion(vec[ci],ci);
      if ( vec[ci-1] < binomial_delta(expansion[cur_index]) ) return false;             
   }    
   if( ci < vec.size()-1 ) {
      ++cur_index.first; cur_index.second = vec[ci+1];
      if (expansion.find(cur_index)==expansion.end()) expansion[cur_index] = binomial_expansion(vec[ci+1],ci+1);
      if ( vec[ci] < binomial_delta(expansion[cur_index]) ) return false;
   }
   return true;
}
} // end anonymous namespace

// returns an ordered list of facets consituting a shelling if p_in is shellable,
// returns a list containing only the empty set otherwise
// note: the simplicial complex has to be pure
Array<Set<Int>> shelling(BigObject p_in)
{       
   facet_vector facets_to_assign= p_in.give("FACETS");
   Array<Int> h_vector = p_in.give("H_VECTOR");
   const Int nf = facets_to_assign.size();
   const Int dim = h_vector.size()-2;
   std::list<Set<Int>> shell;
        
   Int cur_index = 0;
   bool found = next_candidate(facets_to_assign, h_vector, 0, &cur_index);
   LabeledFacetList assigned_facets;
        
   // compute the binomial expansions of the h-vector coordinates
   // at the same time check if any of them is negative
   if (h_vector[0] < 0)
      return Array<Set<Int>>(shell); 

   hash_map<pair_int, Array<Int>> binom_exp;
   for (Int i = 1; i < h_vector.size(); ++i) {
      if (h_vector[0] < 0)
         return Array<Set<Int>>(shell); 
      if (binom_exp.find(pair_int(i,h_vector[i]))==binom_exp.end())
         binom_exp[pair_int(i,h_vector[i])] = binomial_expansion(h_vector[i],i);
   }

   while (Int(assigned_facets.size()) < nf) {
      if (found) {                    
         Int k = num_boundary_ridges(facets_to_assign,facets_to_assign[cur_index]);                     
         if (!larger_facet_exists(assigned_facets, facets_to_assign, cur_index) ) {
            assigned_facets.push_back(LabeledFacet(facets_to_assign[cur_index],dim+1-k,cur_index));     
            facets_to_assign[cur_index] = Set<Int>();
            --h_vector[dim+1-k];
            found = next_candidate(facets_to_assign, h_vector, 0, &cur_index) && is_M_sequence(h_vector,binom_exp,dim+1-k);                     
         }              
      } else if (assigned_facets.empty()) {
         shell.push_back(Set<Int>());
         return Array<Set<Int>>(shell);
      }
      else {
         LabeledFacet to_remove = assigned_facets.back();
         Int new_index = to_remove.index+1;
         ++h_vector[to_remove.label];
         to_remove.label = -1;
         facets_to_assign[to_remove.index] = to_remove.facet;
         assigned_facets.pop_back();
         found = next_candidate(facets_to_assign, h_vector, new_index, &cur_index);
      }
   }
   for (const auto& af : assigned_facets)
      shell.push_front(af.facet);
   return Array<Set<Int>>(shell);
}

Function4perl(&shelling, "shelling");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
