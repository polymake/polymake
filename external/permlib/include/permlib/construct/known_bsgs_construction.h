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


#ifndef KNOWNBSGSCONSTRUCTION_H_
#define KNOWNBSGSCONSTRUCTION_H_

#include <permlib/construct/base_construction.h>
#include <permlib/bsgs.h>

namespace permlib {

/// BSGS construction from a known base and strong generating set
template <class PERM, class TRANS>
class KnownBSGSConstruction : public BaseConstruction<PERM, TRANS> {
public:
	/// constructor
	/**
	 * @param n cardinality of the set the group is acting on
	 */
	KnownBSGSConstruction(unsigned int n);

	/// sets up a BSGS data structure for a known base and strong generating set
	/**
	 * @param generatorsBegin begin iterator of strong generating set of type PERM
	 * @param generatorsEnd  end iterator of strong generating set of type PERM
	 * @param knownBaseBegin begin iterator of known base of type unsigned long
	 * @param knownBaseEnd  end iterator of known base of type unsigned long
	 */
	template <class ForwardIterator, class InputIterator>
	BSGS<PERM, TRANS> construct(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd, InputIterator knownBaseBegin, InputIterator knownBaseEnd) const;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM, class TRANS>
KnownBSGSConstruction<PERM,TRANS>::KnownBSGSConstruction(unsigned int n) 
	: BaseConstruction<PERM, TRANS>(n)
{ }


template <class PERM, class TRANS>
template <class ForwardIterator, class InputIterator>
BSGS<PERM, TRANS> KnownBSGSConstruction<PERM, TRANS>
	::construct(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd, InputIterator knownBaseBegin, InputIterator knownBaseEnd) const
{
	const unsigned int &n = this->m_n;
	BSGS<PERM, TRANS> ret(n);
	std::vector<std::list<typename PERM::ptr> > S;
	this->setup(generatorsBegin, generatorsEnd, knownBaseBegin, knownBaseEnd, ret, S);
	BOOST_ASSERT( S.size() > 0 );
	// the strong generating set of the group is the
	// stabilizer of the empty set of the given generators
	ret.S.insert(ret.S.end(), S[0].begin(), S[0].end());
	return ret;
}

}

#endif // -- KNOWNBSGSCONSTRUCTION_H_
