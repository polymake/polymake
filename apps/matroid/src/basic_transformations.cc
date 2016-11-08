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
#include "polymake/Array.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace matroid {

	typedef Array<Set <int> > SArray;

	SArray bases_to_circuits(const SArray& bases,const int n)
	{
		if(bases.empty()) {
			return SArray(n, entire(all_subsets_of_k(sequence(0,n),1)));
		}
		const int  r=bases[0].size();
		if (r==0) {
			Array<Set<int> > c(n);
			for (int i=0; i<n;++i)
				c[i]=scalar2set(i);
			return c;
		}
		if (r==n) return Array<Set<int> >(0);
		int n_circuits=0;
		std::vector<Set<int> > circuits;
		for (int k=1; k<=r; ++k)
			for (Entire< Subsets_of_k<const sequence&> >::const_iterator j=entire(all_subsets_of_k(sequence(0,n),k));!j.at_end();++j) {
				bool is_circuit=true;
				for (Entire<std::vector<Set<int> > >::const_iterator i=entire(circuits); is_circuit&&!i.at_end();++i)
					if (incl(*i,*j)<=0) is_circuit=false;
				for (Entire<SArray>::const_iterator i = entire(bases); is_circuit&&!i.at_end(); ++i){
					const int l=incl(*i,*j);
					if (l==0||l==1) is_circuit=false;
				}
				if (is_circuit) {
					circuits.push_back(Set<int>(*j));
					++n_circuits;
				}
			}
		for (Entire< Subsets_of_k<const sequence&> >::const_iterator j=entire(all_subsets_of_k(sequence(0,n),r+1));!j.at_end();++j) {
			bool is_circuit=true;
			for (Entire<std::vector<Set<int> > >::const_iterator i=entire(circuits); is_circuit&&!i.at_end();++i)
				if (incl(*i,*j)<=0) is_circuit=false;
			if (is_circuit) {
				circuits.push_back(Set<int>(*j));
				++n_circuits;
			}
		}
		return SArray(n_circuits,entire(circuits));
	}








	SArray circuits_to_bases(const SArray& circuits,const int n)
	{
		if (circuits.empty()) return Array<Set<int> >(1,Set<int>(sequence(0,n)));
		int n_bases=0;
		std::vector<Set<int> > bases;
		int r=1;
		for (int k=n; k>=r; --k)
			for (Entire< Subsets_of_k<const sequence&> >::const_iterator j=entire(all_subsets_of_k(sequence(0,n),k));!j.at_end();++j) {
				bool is_basis=true;
				for (Entire<SArray>::const_iterator i = entire(circuits); is_basis&&!i.at_end(); ++i){
					if (incl(*i,*j)<=0) is_basis=false;
				}
				if (is_basis) {
					bases.push_back(Set<int>(*j));
					++n_bases;
					r=k;
				}
			}
		//If we found no basis, the empty set is the basis 
		if(bases.size() == 0) return SArray(1);
		return SArray(n_bases,entire(bases));
	}


	SArray circuits_to_bases_rank(const SArray& circuits,const int n,const int rank)
	{
		if (circuits.empty()) return Array<Set<int> >(1,Set<int>(sequence(0,n)));
		int n_bases=0;
		std::vector<Set<int> > bases;
		for (Entire< Subsets_of_k<const sequence&> >::const_iterator j=entire(all_subsets_of_k(sequence(0,n),rank));!j.at_end();++j) {
			bool is_basis=true;
			for (Entire<SArray>::const_iterator i = entire(circuits); is_basis&&!i.at_end(); ++i){
				if (incl(*i,*j)<=0) is_basis=false;
			}
			if (is_basis) {
				bases.push_back(Set<int>(*j));
				++n_bases;
			}
		}
		return SArray(n_bases,entire(bases));
	}

	SArray circuits_to_hyperplanes(const SArray& circuits,const int n,const int rank)
	{
		if(rank==0){
			return SArray(0);
		}
		Set<Set<int> > hyp;
		for(Entire< Subsets_of_k<const sequence&> >::const_iterator j=entire(all_subsets_of_k(sequence(0,n),rank-1));!j.at_end();++j) {
			bool is_ind=true;
			Set<int> h=*j;
			for (Entire<SArray>::const_iterator c = entire(circuits); is_ind&&!c.at_end(); ++c){
				if (incl(*c,*j)<=0) is_ind=false;
				if((*c-*j).size()==1){
					h+=*c;
				}
			}
			if(is_ind){
				hyp+=h;
			}
		}
		return SArray(hyp);
	}

	Function4perl(&bases_to_circuits, "bases_to_circuits");
	Function4perl(&circuits_to_bases, "circuits_to_bases");
	Function4perl(&circuits_to_bases_rank, "circuits_to_bases_rank");
	Function4perl(&circuits_to_hyperplanes, "circuits_to_hyperplanes");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
