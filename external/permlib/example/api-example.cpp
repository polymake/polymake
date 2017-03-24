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


#include <permlib/permlib_api.h>

#include <iostream>

int main(int argc, char *argv[]) {
	using namespace permlib;

	// our group will have degree 10, i.e. act on 1..10
	const unsigned long n = 10;

	// group generators
	std::list<Permutation::ptr> groupGenerators;
	Permutation::ptr gen1(new Permutation(n, std::string("1 3 5 7 9 10 2 4 6 8")));
	groupGenerators.push_back(gen1);
	Permutation::ptr gen2(new Permutation(n, std::string("1 5")));
	groupGenerators.push_back(gen2);

	//
	// EXAMPLE 0 (BSGS): construct a base with strong generating set
	//
	boost::shared_ptr<PermutationGroup> group = construct(n, groupGenerators.begin(), groupGenerators.end());
	std::cout << "Group " << *group << std::endl;

	// size of our set(s) in this example
	const unsigned long DeltaSize = 4;
	// represents the set {1,5,8,9}, translated by -1 as the elements of the domain are 0-based
	const unsigned long Delta[DeltaSize] = {0, 4, 7, 8};

	//
	// EXAMPLE 1 (SET STAB): compute a set stabilizer
	//
	boost::shared_ptr<PermutationGroup> stabilizer = setStabilizer(*group, Delta, Delta+DeltaSize);
	std::cout << "Stabilizer of {1,5,8,9}: " << *stabilizer << std::endl;

	//
	// EXAMPLE 2 (SET IMAGE): find elements mapping one set onto another 
	//
	const unsigned long Gamma[DeltaSize] = {2, 6, 0, 9};
	Permutation::ptr repr = setImage(*group, Delta, Delta+DeltaSize, Gamma, Gamma+DeltaSize);
	if (repr)
		std::cout << "Group element mapping {1,5,8,9} to {1,3,7,10}: " << *repr << std::endl;
	else
		std::cout << "No group element found mapping {1,5,8,9} to {1,3,7,10}." << std::endl;

	const unsigned long Gamma2[DeltaSize] = {2, 6, 10, 9};
	Permutation::ptr repr2 = setImage(*group, Delta, Delta+DeltaSize, Gamma2, Gamma2+DeltaSize);
	if (repr2)
		std::cout << "Group element mapping {1,5,8,9} to {3,7,10,11}: " << *repr2 << std::endl;
	else
		std::cout << "No group element found mapping {1,5,8,9} to {3,7,10,11}." << std::endl;

	//
	// EXAMPLE 3 (ORBTIS): compute orbits of a group
	//                     in this case: the stabilizer from above
	//
	std::list<boost::shared_ptr<OrbitAsSet> > orbitList = orbits(*stabilizer);
	unsigned long orbCount = 1;
	BOOST_FOREACH(const boost::shared_ptr<OrbitAsSet>& orbit, orbitList) {
		std::cout << "Orbit #" << orbCount << " representative: " << (orbit->element()+1) << std::endl;
		++orbCount;
	}

    //
    // EXAMPLE 4 (SMALLEST SET IMAGE): compute lexicographically smallest set of an orbit of sets
    //
    // encode Gamma in a 'dset', which is a boost::dynamic_bitset
    dset dGamma(n);
    for (unsigned int i = 0; i < DeltaSize; ++i)
        dGamma.set(Gamma[i]);
    dset dGammaLeast = smallestSetImage(*group, dGamma);
    std::cout << "Lexicographically smallest set in the orbit of {1,3,7,10}:  {";
    for (unsigned int i = 0; i < n; ++i)
        if (dGammaLeast[i])
            std::cout << (i+1) << ",";
    std::cout << "}" << std::endl;

	return 0;
}
