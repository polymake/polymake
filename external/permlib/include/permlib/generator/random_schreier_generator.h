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


#ifndef RANDOMSCHREIERGENERATOR_H_
#define RANDOMSCHREIERGENERATOR_H_

#include <permlib/generator/random_generator.h>

namespace permlib {

/// generates a uniformly distributed random element of \f$G^{[i]}_\alpha\f$
/**
 * The class has nothing to do with Schreier generators, it just creates elements of the stabilizer of a subgroup.
 * As it can be used as a replacement for Schreier generators, the name is not completely wrong.
 */
template <class PERM,class TRANS>
class RandomSchreierGenerator : public RandomGenerator<PERM> {
public:
	/// constructor
	/**
	 * @param bsgs group
	 * @param i stabilizer chain index for the supergroup
	 * @param U a transversal for \f$G^{[i]}\f$ modulo \f$G^{[i]}_\alpha\f$
	 */
	RandomSchreierGenerator(const BSGS<PERM,TRANS> &bsgs, unsigned int i, const TRANS &U);
	virtual PERM next();
private:
	const BSGS<PERM,TRANS> &m_bsgs;
	unsigned int m_i;
	const TRANS &m_U;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM,class TRANS>
RandomSchreierGenerator<PERM,TRANS>::RandomSchreierGenerator(const BSGS<PERM,TRANS> &bsgs, unsigned int i, const TRANS &U) 
	: m_bsgs(bsgs), m_i(i), m_U(U) 
{ }

template <class PERM,class TRANS>
PERM RandomSchreierGenerator<PERM,TRANS>::next() { 
	PERM g = m_bsgs.random(m_i);
	boost::scoped_ptr<PERM> u_g(m_U.at(g / m_bsgs.B[m_i]));
	u_g->invertInplace();
	g *= *u_g;
	return g;
}

}

#endif // -- RANDOMSCHREIERGENERATOR_H_
