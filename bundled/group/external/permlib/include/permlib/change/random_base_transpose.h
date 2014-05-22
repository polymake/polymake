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


#ifndef RBASECHANGE_H_
#define RBASECHANGE_H_

#include <permlib/change/base_transpose.h>
#include <permlib/generator/random_schreier_generator.h>

#include <boost/scoped_ptr.hpp>

namespace permlib {

/// implementation of a randomized base transposition algorithm
/**
 * Randomly constructed Schreier generators are considered for updating the BSGS.
 * The algorithm is Las Vegas type, so the result is guaranteed to be correct,
 * the time to completion may vary.
 */
template<class PERM, class TRANS>
class RandomBaseTranspose : public BaseTranspose<PERM,TRANS> {
protected:
	typedef typename BaseTranspose<PERM,TRANS>::PERMlist PERMlist;
	
	virtual Generator<PERM>* setupGenerator(BSGS<PERM,TRANS> &bsgs, unsigned int i, const PERMlist &S_i, const TRANS &U_i) const;
};

//
//     ----       IMPLEMENTATION
//

template<class PERM, class TRANS>
Generator<PERM>* RandomBaseTranspose<PERM,TRANS>::setupGenerator(BSGS<PERM,TRANS> &bsgs, unsigned int i, const PERMlist &S_i, const TRANS &U_i) const {
    return new RandomSchreierGenerator<PERM,TRANS>(bsgs, i, U_i);
}

}

#endif // -- RBASECHANGE_H_
