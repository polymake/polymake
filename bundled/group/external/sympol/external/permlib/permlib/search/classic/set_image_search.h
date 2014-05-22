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


#ifndef CLASSIC_SET_IMAGE_SEARCH_H_
#define CLASSIC_SET_IMAGE_SEARCH_H_

#include <permlib/search/classic/backtrack_search.h>
#include <permlib/predicate/set_image_predicate.h>

namespace permlib {
namespace classic {

/// coset representative search for a set image based on classical backtracking
/**
 * tries to find a \f$g\f$ such that \f$\Delta^g = \Gamma\f$ for two given sets \f$\Delta,\Gamma\f$
 */
template<class BSGSIN,class TRANSRET>
class SetImageSearch : public BacktrackSearch<BSGSIN,TRANSRET> {
public:
	typedef typename BacktrackSearch<BSGSIN,TRANSRET>::PERM PERM;
	
	/// constructor
	/**
	 * @param bsgs BSGS of group
	 * @param pruningLevelDCM level up to which expensive double coset minimality pruning is performed; zero to disable
	 */
	SetImageSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM);
	
	/// initializes search
	/**
	 * @param begin iterator(unsigned long) begin of the set \f$\Delta\f$
	 * @param end iterator(unsigned long) end of the set \f$\Delta\f$
	 * @param beginImg iterator(unsigned long) begin of the set \f$\Gamma\f$
	 * @param endImg iterator(unsigned long) end of the set \f$\Gamma\f$
	 */
	template<class InputIterator>
	void construct(InputIterator begin, InputIterator end, InputIterator beginImg, InputIterator endImg);
};

template<class BSGSIN,class TRANSRET>
SetImageSearch<BSGSIN,TRANSRET>::SetImageSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM) 
	: BacktrackSearch<BSGSIN,TRANSRET>(bsgs, pruningLevelDCM, false, true)
{ }

template<class BSGSIN,class TRANSRET>
template<class InputIterator>
void SetImageSearch<BSGSIN,TRANSRET>::construct(InputIterator begin, InputIterator end, InputIterator beginImg, InputIterator endImg) {
	SetImagePredicate<PERM>* stabPred = new SetImagePredicate<PERM>(begin, end, beginImg, endImg);
	
	this->m_limitLevel = stabPred->limit();
	this->m_limitBase = this->m_limitLevel;
	this->m_limitInitialized = true;
	
	BacktrackSearch<BSGSIN,TRANSRET>::construct(stabPred, false);
}

}
}

#endif // -- CLASSIC_SET_IMAGE_SEARCH_H_
