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


#ifndef NEWBASECHANGE_H_
#define NEWBASECHANGE_H_

#include <permlib/change/base_change.h>
#include <permlib/generator/bsgs_random_generator.h>
#include <permlib/construct/random_schreier_sims_construction.h>

namespace permlib {

/// base change by constructing a new base with random schreier sims
template<class PERM, class TRANS>
class NewBaseChange : public BaseChange<PERM,TRANS> {
public:
	/// constructor
	NewBaseChange(const BSGS<PERM,TRANS> &bsgs);
	/// destructor
	~NewBaseChange();

	/// changes base of bsgs so that it starts with the sequence given by baseBegin to baseEnd 
	template <class InputIterator>
	void change(BSGS<PERM,TRANS> &bsgs, InputIterator baseBegin, InputIterator baseEnd, bool skipRedundant = false) const;
private:
	RandomGenerator<PERM>* rng;
	RandomSchreierSimsConstruction<PERM,TRANS> rssc;
};


template<class PERM, class TRANS>
NewBaseChange<PERM,TRANS>::NewBaseChange(const BSGS<PERM,TRANS> &bsgs) 
	: rng(new BSGSRandomGenerator<PERM,TRANS>(bsgs)),
      rssc(bsgs.n, rng, bsgs.order())
{ }

template<class PERM, class TRANS>
NewBaseChange<PERM,TRANS>::~NewBaseChange() {
	delete rng; 
}

template<class PERM, class TRANS>
template <class InputIterator>
void NewBaseChange<PERM,TRANS>::change(BSGS<PERM,TRANS> &bsgs, InputIterator baseBegin, InputIterator baseEnd, bool skipRedundant) const {
	bool guarantee = false;
	bsgs = rssc.construct(bsgs.S.begin(), bsgs.S.end(), baseBegin, baseEnd, guarantee);
	BOOST_ASSERT(guarantee);
}

}

#endif // -- NEWBASECHANGE_H_
