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


#ifndef BSGSGENERATOR_H_
#define BSGSGENERATOR_H_

#include <permlib/common.h>
#include <permlib/generator/generator.h>

#include <boost/scoped_ptr.hpp>

namespace permlib {

/// stateful generator of BSGS elements
/**
 * Constructs all group elements from a list of BSGS transversals
 */
template <class TRANS>
class BSGSGenerator : public Generator<typename TRANS::PERMtype> {
private:
	typedef typename TRANS::PERMtype PERM;
public:
	/// constructor
	/**
	 * @param U vector of BSGS transversals
	 */
	BSGSGenerator(const std::vector<TRANS>& U);
	virtual ~BSGSGenerator() {}
	
	virtual PERM next();
	virtual bool hasNext();
private:
	const std::vector<TRANS>& m_U;
	std::vector<std::list<unsigned long>::const_iterator > m_Upositions;
	bool m_hasNext;
};

//
//     ----       IMPLEMENTATION
//

template <class TRANS>
BSGSGenerator<TRANS>::BSGSGenerator(const std::vector<TRANS>& U)
		: m_U(U), m_Upositions(U.size()), m_hasNext(true)
{ 
	for (unsigned int i = 0; i < m_U.size(); ++i) {
		m_Upositions[i] = m_U[i].begin();
	}
}


template <class TRANS>
bool BSGSGenerator<TRANS>::hasNext() {
	return m_hasNext;
}


template <class TRANS>
typename BSGSGenerator<TRANS>::PERM BSGSGenerator<TRANS>::next() {
	BOOST_ASSERT( m_hasNext );
	PERM g(m_U[0].n());
	for (int i = m_Upositions.size() - 1; i >= 0; --i) {
		boost::scoped_ptr<PERM> u_beta( m_U[i].at( *m_Upositions[i] ) );
		BOOST_ASSERT( u_beta );
		g *= *u_beta;
	}
	
	// advance position
	int i;
	for (i = m_Upositions.size() - 1; i >= 0; --i) {
		m_Upositions[i]++;
		if (m_Upositions[i] == m_U[i].end())
			m_Upositions[i] = m_U[i].begin();
		else
			break;
	}
	if ( i < 0 )
		m_hasNext = false;
	
	return g;
}

}

#endif // -- BSGSGENERATOR_H_
