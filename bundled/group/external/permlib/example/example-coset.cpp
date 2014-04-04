// ---------------------------------------------------------------------------
//
//  This file is part of PermLib.
//
// Copyright (c) 2009-2011 Thomas Rehn <thomas@carmen76.de>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------------------------------------------------------------------


#include <permlib/common.h>
#include <permlib/permutation.h>
#include <permlib/bsgs.h>
#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/construct/schreier_sims_construction.h>
#include <permlib/change/conjugating_base_change.h>
#include <permlib/search/classic/set_image_search.h>
#include <permlib/search/partition/set_image_search.h>

#include <iostream>

int main(int argc, char *argv[]) {
	using namespace permlib;
	
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	
	// our group will have degree 10
	const unsigned long n = 10;
	
	// group generators
	std::list<PERM::ptr> groupGenerators;
	PERM::ptr gen1(new PERM(n, std::string("1 3 5 7 9 10 2 4 6 8")));
	groupGenerators.push_back(gen1);
	PERM::ptr gen2(new PERM(n, std::string("1 5")));
	groupGenerators.push_back(gen2);
	
	// BSGS construction
	SchreierSimsConstruction<PERM, TRANSVERSAL> schreierSims(n);
	BSGS<PERM, TRANSVERSAL> bsgs = schreierSims.construct(groupGenerators.begin(),
														  groupGenerators.end());
	std::cout << "Group " << bsgs << std::endl;
	
	const unsigned long DeltaSize = 3;
	// represents the set {1,3,5}, translated by -1 as the elements of the domain are 0-based
	const unsigned long Delta[DeltaSize] = {0, 2, 4};
	const unsigned long Phi[DeltaSize]   = {1, 4, 6};

	// change the base so that is prefixed by the set
	ConjugatingBaseChange<PERM,TRANSVERSAL,
		RandomBaseTranspose<PERM,TRANSVERSAL> > baseChange(bsgs);
	baseChange.change(bsgs, Delta, Delta+DeltaSize);
	
	// prepare search without DCM pruning
	classic::SetImageSearch<BSGS<PERM,TRANSVERSAL>, TRANSVERSAL> backtrackSearch(bsgs, 0);
	backtrackSearch.construct(Delta, Delta+DeltaSize, Phi, Phi+DeltaSize);
	PERM::ptr repr = backtrackSearch.searchCosetRepresentative();
	
	if (repr)
		std::cout << "Representative " << *repr << std::endl;
	else
		std::cout << "No representative found" << std::endl;
		
	return 0;
}
