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


#ifndef SIMPLEBASECHANGE_H_
#define SIMPLEBASECHANGE_H_

#include <permlib/change/base_change.h>

namespace permlib {

/// base change by a sequence of point insertions and transpositions
template<class PERM, class TRANS, class BASETRANSPOSE>
class SimpleBaseChange : public BaseChange<PERM,TRANS> {
public:
	/// constructor
	explicit SimpleBaseChange(const BSGS<PERM,TRANS>&);
	
	/// changes base of bsgs so that it starts with the sequence given by baseBegin to baseEnd 
    template <class InputIterator>
    void change(BSGS<PERM,TRANS> &bsgs, InputIterator baseBegin, InputIterator baseEnd, bool skipRedundant = false) const;
};

//
//     ----       IMPLEMENTATION
//

template<class PERM, class TRANS, class BASETRANSPOSE>
SimpleBaseChange<PERM,TRANS,BASETRANSPOSE>::SimpleBaseChange(const BSGS<PERM,TRANS>&) 
	: BaseChange<PERM,TRANS>() 
{ }

template<class PERM, class TRANS, class BASETRANSPOSE>
template<class InputIterator>
void SimpleBaseChange<PERM,TRANS,BASETRANSPOSE>::change(BSGS<PERM,TRANS> &bsgs, InputIterator baseBegin, InputIterator baseEnd, bool skipRedundant) const {
	if (baseBegin == baseEnd)
		return;

	const unsigned long origOrder __attribute__((unused)) = bsgs.order();
	BASETRANSPOSE trans;
	
	unsigned int baseTargetPos = 0;
    while (baseBegin != baseEnd && baseTargetPos < bsgs.B.size()) {
		unsigned long alpha = *baseBegin;
		unsigned long beta = bsgs.B[baseTargetPos];
		const bool redundant = skipRedundant && this->isRedundant(bsgs, baseTargetPos, alpha);
		
		if (!redundant && beta != alpha) {
			unsigned int pos = bsgs.insertRedundantBasePoint(alpha);
			for (; pos > baseTargetPos; --pos) {
				trans.transpose(bsgs, pos-1);
				++BaseChange<PERM,TRANS>::m_statTranspositions;
			}
		}
		
		++baseBegin;
		if (!redundant)
			++baseTargetPos;
    }
	// insert remaining base points
	while (!skipRedundant && baseBegin != baseEnd) {
		const unsigned long alpha = *baseBegin;
		bsgs.insertRedundantBasePoint(alpha, baseTargetPos);
		
		++baseBegin;
		++baseTargetPos;
	}
	
	bsgs.stripRedundantBasePoints(baseTargetPos);
	BaseChange<PERM,TRANS>::m_statScheierGeneratorsConsidered += trans.m_statScheierGeneratorsConsidered;
	
	BOOST_ASSERT(origOrder == bsgs.order());
}

}

#endif // -- SIMPLEBASECHANGE_H_
