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
#include <permlib/transversal/orbit_set.h>

#include <iostream>

// defines how a permutation acts on a unsigned long value
unsigned long action(const permlib::Permutation& p, unsigned long v) {
	return p.at(v);
}

int main(int argc, char *argv[]) {
	using namespace permlib;
	
	// we use elementary permutations
	typedef Permutation PERM;
		
	// our permutations act on 10 elements
	const unsigned long n = 10;
	
	// group generators
	std::list<PERM::ptr> groupGenerators;
	PERM::ptr gen1(new PERM(n, std::string("1 3 5 7 9 10 2 4 6 8")));
	groupGenerators.push_back(gen1);
	PERM::ptr gen2(new PERM(n, std::string("1 5")));
	groupGenerators.push_back(gen2);
		
	// our permutations simply act on integers (unsigned long)
	// we could also let the permutations act on more complex objects with another
	// action(.) function
	OrbitSet<PERM, unsigned long> orbit_5;
	orbit_5.orbit(4, groupGenerators, action);
	
	if (orbit_5.contains(3))
		std::cout << "The orbit of 5 contains 4." << std::endl;
	else
		std::cout << "The orbit of 5 doe not contain 4." << std::endl;
		
	return 0;
}
