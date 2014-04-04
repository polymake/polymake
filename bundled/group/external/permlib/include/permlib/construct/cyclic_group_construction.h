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


#ifndef CYCLIC_GROUP_CONSTRUCTION_H_
#define CYCLIC_GROUP_CONSTRUCTION_H_

#include <permlib/construct/known_bsgs_construction.h>

namespace permlib {

/// BSGS construction for a cyclic group of given order
template <class TRANS>
class CyclicGroupConstruction : public KnownBSGSConstruction<typename TRANS::PERMtype, TRANS> {
public:
	/// constructor
	/**
	 * @param n order of the cyclic group
	 */
	CyclicGroupConstruction(unsigned int n);

	/// sets up a BSGS data structure for a cyclic group
	BSGS<typename TRANS::PERMtype, TRANS> construct() const;
};

//
//     ----       IMPLEMENTATION
//

template <class TRANS>
CyclicGroupConstruction<TRANS>::CyclicGroupConstruction(unsigned int n) 
	: KnownBSGSConstruction<typename TRANS::PERMtype, TRANS>(n)
{ }


template <class TRANS>
BSGS<typename TRANS::PERMtype, TRANS> CyclicGroupConstruction<TRANS>
	::construct() const
{
	typedef typename TRANS::PERMtype PERM;
	const unsigned int &n = this->m_n;
	typename PERM::perm genPerm(n);
	for (unsigned int i = 0; i < n; ++i)
		genPerm[i] = (i + 1) % n;
	std::list<typename PERM::ptr> cyclicGenerators;
	cyclicGenerators.push_back(typename PERM::ptr(new PERM(genPerm)));
	const dom_int knownBase[1] = { 0 };
	
	return KnownBSGSConstruction<PERM, TRANS>::construct(cyclicGenerators.begin(), cyclicGenerators.end(), knownBase, knownBase + 1);
}

}

#endif // -- KNOWNBSGSCONSTRUCTION_H_
