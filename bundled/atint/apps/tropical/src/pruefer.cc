/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	Implements pruefer.h
	*/

#include "polymake/tropical/pruefer.h"

namespace polymake { namespace tropical {
	using namespace atintlog::donotlog;
	//using namespace atintlog::dolog;
	//   using namespace atintlog::dotrace;

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see header
	Matrix<int> prueferSequenceFromValences(int n, Matrix<int> valences) {

		//Compute basic parameters
		int no_of_edges = valences.cols()-1;
		int seq_length = n + no_of_edges-1;


		Matrix<int> result(0,seq_length);

		//Iterate all rows of valences
		for(int v = 0; v < valences.rows(); v++) {

			//dbgtrace << "Valences: " << valences.row(v) << endl;

			//Compute for each interior vertex n+i,i=1,..k the possible distribution of
			//valence(i) - 2 entries on seq_length - (sum_j=1^(i-1) v_i) - 1 free spaces
			Vector<Array<Set<int> > > distributions;
			//For convenience, store the number of distributions per vertex in this list:
			Vector<int> entrymax;
			for(int k = 0; k < valences.cols()-1; k++) {
				//Compute number of free entries that will be available for this vertex after
				//having placed all smaller vertices (subtracting the first occurrence, of course)
				int v_sum = 0; 
				for(int i = 0; i < k; i++) { v_sum += (valences(v,i)-1);}
				distributions |= all_subsets_of_k(sequence(0, seq_length - v_sum - 1), valences(v,k)-2);	
				entrymax |= distributions[k].size();
			}

			//dbgtrace << "Entry distributions: " << entrymax << endl;


			//We find all sequences by iterating the following vector: Read as (s_1,..,s_k),
			//where s_i means we take distributions[i][s_i] to distribute vertex i on the remaining
			//free entries
			Vector<int> seq_iterator(valences.cols()-1);

			while(true) {
				//Construct sequence corresponding to the iterator
				//dbgtrace << result.rows() << endl;
				Vector<int> current_sequence(seq_length);
				Vector<int> free_entries(sequence(0, current_sequence.dim()));
				//Go through all but the last vertex
				for(int k = 0; k < valences.cols()-1; k++) {
					//Take smallest free entry for first appearance of current vector
					current_sequence[free_entries[0]] = n+k;
					free_entries = free_entries.slice(~scalar2set(0));
					//Insert remaining entries
					Set<int> entry_distro = (distributions[k])[seq_iterator[k]];
					Set<int> entries_to_set = Set<int>(free_entries.slice(entry_distro));
					current_sequence.slice( entries_to_set ) = (n+k) * ones_vector<int>(entries_to_set.size());
					free_entries = free_entries.slice(~entry_distro);
				}//END insert entries
				//Insert last vertex
				current_sequence.slice(free_entries) = (n + valences.cols()-1) * ones_vector<int>(free_entries.size());
				result /= current_sequence;

				//"Increase seq_iterator by 1"
				int current_index = seq_iterator.dim()-1;
				//Go through the iterator backwards until you find an entry that is not maximal.
				//On the way, set all entries that are maximal to 0.
				//If we have to set the first element to 0, we're done
				while(seq_iterator[current_index] == entrymax[current_index]-1 && current_index >= 0) {
					seq_iterator[current_index] = 0;
					current_index--;
				}
				if(current_index < 0) break;
				seq_iterator[current_index]++;	
			}//END iterate all Pruefer sequences      

		}//END iterate valences

		return result;

	}//END prueferSequenceFromValences

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see header
	Matrix<int> dimension_k_prueferSequence(int n, int k) {

		//First we create a polytope whose lattice points describe the possible distributions of
		//valences on the interior vertices p_1 < ... < p_(k+1)

		int vertex_count = k+1;
		int seq_length = n + k-1;

		//The sum of all valences must be (length of Pruefer sequence + #vertices)
		Matrix<Rational> eq(0,vertex_count+1);
		Vector<Rational> eqvec = ones_vector<Rational>(vertex_count); 
		eqvec = Rational(-seq_length - (k+1)) | eqvec;
		eq /= eqvec;

		//Each valence must be >= 3  
		Matrix<Rational> ineq = unit_matrix<Rational>(vertex_count);
		ineq = ( (-3) * ones_vector<Rational>(vertex_count)) | ineq;

		perl::Object p("polytope::Polytope");
		p.take("INEQUALITIES") << ineq;
		p.take("EQUATIONS") << eq;
		Matrix<int> latt = p.CallPolymakeMethod("LATTICE_POINTS");
		latt = latt.minor(All,~scalar2set(0));

		return prueferSequenceFromValences(n,latt);
	}//END dimension_k_prueferSequence

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see header
	
	// ------------------------- PERL WRAPPERS ---------------------------------------------------

	Function4perl(&prueferSequenceFromValences, "prueferSequenceFromValences($,Matrix<Int>)");

	Function4perl(&dimension_k_prueferSequence, "dimension_k_prueferSequence($,$)");

	FunctionTemplate4perl("complex_from_prueferSequences<Addition>($,Matrix<Int>)");

}}

