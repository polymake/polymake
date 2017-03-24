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


#ifndef CLASSIC_LEXSMALLERIMAGE_SEARCH_H_
#define CLASSIC_LEXSMALLERIMAGE_SEARCH_H_

#include <permlib/search/classic/backtrack_search.h>
#include <permlib/predicate/lex_smaller_image_predicate.h>

namespace permlib {
namespace classic {

/// coset representative search for a lex-smaller set images
/**
 * tries to find a \f$g\f$ such that \f$val(\Delta^g) <_{lex} val(\Delta)\f$
 */
template<class BSGSIN,class TRANSRET>
class LexSmallerImageSearch : public BacktrackSearch<BSGSIN,TRANSRET> {
public:
	typedef typename BacktrackSearch<BSGSIN,TRANSRET>::PERM PERM;
	
	/// constructor
	/**
	 * @param bsgs BSGS of group
	 * @param pruningLevelDCM level up to which expensive double coset minimality pruning is performed; zero to disable
	 */
	LexSmallerImageSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM);
	
	/// initializes search
	/**
	 * @param begin iterator(unsigned long) begin of the set \f$\Delta\f$
	 * @param end iterator(unsigned long) end of the set \f$\Delta\f$
	 * @param beginImg iterator(unsigned long) begin of the set \f$\Gamma\f$
	 * @param endImg iterator(unsigned long) end of the set \f$\Gamma\f$
	 */
	template<class InputIteratorZ, class InputIteratorO>
	void construct(InputIteratorZ zerosBegin, InputIteratorZ zerosEnd, InputIteratorO onesBegin, InputIteratorO onesEnd);
};

template<class BSGSIN,class TRANSRET>
LexSmallerImageSearch<BSGSIN,TRANSRET>::LexSmallerImageSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM) 
	: BacktrackSearch<BSGSIN,TRANSRET>(bsgs, pruningLevelDCM, false, true)
{ }

template<class BSGSIN,class TRANSRET>
template<class InputIteratorZ, class InputIteratorO>
void LexSmallerImageSearch<BSGSIN,TRANSRET>::construct(InputIteratorZ zerosBegin, InputIteratorZ zerosEnd, InputIteratorO onesBegin, InputIteratorO onesEnd) {
	LexSmallerImagePredicate<PERM>* lexminPred = new LexSmallerImagePredicate<PERM>(this->m_bsgs.n, zerosBegin, zerosEnd, onesBegin, onesEnd);
	
	this->m_limitLevel = lexminPred->limit();
	this->m_limitBase = this->m_limitLevel;
	this->m_limitInitialized = true;
	
	BacktrackSearch<BSGSIN,TRANSRET>::construct(lexminPred, false);
}

}
}

#endif // -- CLASSIC_LEXSMALLERIMAGE_SEARCH_H_
