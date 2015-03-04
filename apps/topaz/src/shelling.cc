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

#include "polymake/client.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/Set.h"
#include "polymake/list"

namespace polymake { namespace topaz {
namespace {

struct LabeledFacet {
   Set<int> facet;
   int label;
   unsigned int index;
        
   bool operator==(const LabeledFacet& otherLF) const 
   {
      return (facet==otherLF.facet && index==otherLF.index) ? true : false;
   }
        
   LabeledFacet(const Set<int>& F, const int lab, const unsigned int ind)
   { 
      facet = F;
      label = lab;
      index = ind;
   }
};

typedef std::list<LabeledFacet> LabeledFacetList;
typedef std::vector<Set<int> > facet_vector;
typedef std::pair<int,int> pair_int;

bool is_boundary_ridge(const facet_vector& C, const Set<int>& F)
{
   int num_inc = 0;
   for (facet_vector::const_iterator i=C.begin(); i!=C.end(); ++i) {            
      if(!i->empty() && (F - *i).empty()) {
         if (num_inc == 1) return false;
         ++num_inc;
      }
   }
   return (num_inc == 1) ? true : false;
}

int num_boundary_ridges(const facet_vector& C, const Set<int>& facet)
{
   int num = 0;
   for(Subsets_less_1<const Set<int>&>::const_iterator i=all_subsets_less_1(facet).begin();  !i.at_end(); ++i) {
      if (is_boundary_ridge(C,*i)) ++num;
   }
   return num;
}

bool next_candidate(const facet_vector& fta, const Array<int>& hvec, const unsigned int smallest_index, unsigned int* cur_index)
{
   const int dim = hvec.size()-2;
   for (unsigned int candidate=smallest_index; candidate < fta.size(); ++candidate) {
      if( !fta[candidate].empty() && hvec[dim + 1 - num_boundary_ridges(fta, fta[candidate]) ] > 0 ) {
         *cur_index = candidate;
         return true;
      }
   }
   return false;
}

bool larger_facet_exists(const LabeledFacetList& assigned_facets, const facet_vector& facets_to_assign, const unsigned int cur_index)
{
   int k = num_boundary_ridges(facets_to_assign,facets_to_assign[cur_index]);           
   facet_vector next_fta = facets_to_assign;
   next_fta[cur_index] = Set<int>();
   for (LabeledFacetList::const_iterator delIt = assigned_facets.begin(); delIt != assigned_facets.end();  ++delIt) {
      if(delIt->label == k && (facets_to_assign[cur_index] - delIt->facet).empty() 
         && num_boundary_ridges(next_fta,delIt->facet) == k ) {
         return true;
      }
   }
   return false;
}

Array<int> binomial_expansion(const int n, const int k)
{
   int tmp_n = n;
   int tmp_k = k;
   Array<int> bexp(k);
   while (0 < tmp_n && tmp_k != 0) {
      Integer tmp;
      for (tmp=tmp_k-1; Integer::binom(tmp+1,tmp_k) <= tmp_n; ++tmp);
      bexp[k - tmp_k] = tmp.to_int();
      tmp_n -= Integer::binom(tmp, tmp_k).to_int();
      --tmp_k;
   }
   for (; tmp_k > 0; --tmp_k)
      bexp[k-tmp_k]=0;
   return bexp;
}

int binomial_delta(const Array<int>& bexp)
{
   int bdelta = 0;
   const int k = bexp.size();
   if (bexp[0] == 0) return bdelta;
   for(int i=0; i < k-1; ++i) {
      if (bexp[i] == 0) break;
      bdelta += Integer::binom(bexp[i]-1,k-i-1).to_int();
   }
   return bdelta;
}

bool is_M_sequence(const Array<int>& vec, hash_map<pair_int,Array<int> >& expansion, const int ci)
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
Array< Set<int> > shelling(perl::Object p_in)
{       
   facet_vector facets_to_assign= p_in.give("FACETS");
   Array<int > h_vector = p_in.give("H_VECTOR");
   const unsigned int nf = facets_to_assign.size();
   const int dim = h_vector.size()-2;
   std::list<Set<int> > shell;
        
   unsigned int cur_index = 0;
   bool found = next_candidate(facets_to_assign, h_vector, 0, &cur_index);
   LabeledFacetList assigned_facets;
        
   // compute the binomial expansions of the h-vector coordinates
   // at the same time check if any of them is negative
   if (h_vector[0]<0)   return as_array(shell); 
   hash_map<pair_int,Array<int> > binom_exp;
   for ( int i=1; i < h_vector.size(); ++i ) {
      if (h_vector[0]<0) return as_array(shell); 
      if (binom_exp.find(pair_int(i,h_vector[i]))==binom_exp.end()) binom_exp[pair_int(i,h_vector[i])] = binomial_expansion(h_vector[i],i);
   }

   while ( assigned_facets.size() < nf ) {
      if ( found ) {                    
         int k = num_boundary_ridges(facets_to_assign,facets_to_assign[cur_index]);                     
         if (!larger_facet_exists(assigned_facets, facets_to_assign, cur_index) ) {
            assigned_facets.push_back(LabeledFacet(facets_to_assign[cur_index],dim+1-k,cur_index));     
            facets_to_assign[cur_index] = Set<int>();
            --h_vector[dim+1-k];
            found = next_candidate(facets_to_assign, h_vector, 0, &cur_index) && is_M_sequence(h_vector,binom_exp,dim+1-k);                     
         }              
      } else if ( assigned_facets.size() == 0){
         shell.push_back(Set<int>());
         return shell;
      }
      else {
         LabeledFacet to_remove = assigned_facets.back();
         int new_index = to_remove.index + 1;
         ++h_vector[to_remove.label];
         to_remove.label = -1;
         facets_to_assign[to_remove.index] = to_remove.facet;
         assigned_facets.pop_back();
         found = next_candidate(facets_to_assign, h_vector, new_index, &cur_index);
      }
   }
   for (LabeledFacetList::const_iterator i=assigned_facets.begin(); i!=assigned_facets.end(); ++i)
      shell.push_front(i->facet);
   return as_array(shell);
}

Function4perl(&shelling, "shelling");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
