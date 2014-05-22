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


#ifndef SCHREIERGENERATOR_H_
#define SCHREIERGENERATOR_H_

#include <permlib/common.h>
#include <permlib/generator/generator.h>

#include <stack>
#include <boost/scoped_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace permlib {

/// stateful generator of Schreier generators
/**
 * Constructs all Schreier generators from given transversal and generators.
 *
 * The idea is something like forall(u in U) { forall(s in S) { buildSchreierGenerator(u,s); } }
 * only that U and S can be updated without iterating over the same pair (u,s) twice,
 * ensuring that every pair (u,s) is used at least once (if so many generators are requested via next() ).
 */
template <class PERM, class TRANS>
class SchreierGenerator : public Generator<PERM> {
public:
	/// const iterator to a list of PERMutations
	typedef typename std::list<typename PERM::ptr>::const_iterator PERMlistIt;
	/// const iterator to a list of points (unsigned long)
	typedef typename std::list<unsigned long>::const_iterator TRANSlistIt;
	
	/// constructor
	/**
	 * @param U transversal to build Schreier generators from
	 * @param S_begin begin iterator of group generating list to build Schreier generators from
	 * @param S_end begin iterator of group generating list to build Schreier generators from
	 */
	SchreierGenerator(const TRANS *U, PERMlistIt S_begin, PERMlistIt S_end);
	virtual ~SchreierGenerator();

	PERM next();
	bool hasNext();
	
	/// updates transversal and group generators that the Schreier generators are constructed from
	/**
	 * @param U transversal to build Schreier generators from
	 * @param S_begin begin iterator of group generating list to build Schreier generators from
	 * @param S_end begin iterator of group generating list to build Schreier generators from
	 */
	void update(TRANS *U, PERMlistIt S_begin, PERMlistIt S_end);
	
	/// updates the state of this generator
	/**
	 * Saves the current state.
	 * Before returning to the current state, all Schreier generators from S[j], S[j+1], &c. and the already
	 * visited transversals are next
	 */
	void update(unsigned int j);
private:
	PERMlistIt m_Sbegin;
	PERMlistIt m_Scurrent;
	PERMlistIt m_Send;
	const TRANS *m_U;
	TRANSlistIt m_Ubegin;
	TRANSlistIt m_Ucurrent;
	TRANSlistIt m_Uend;
	unsigned int m_posS;
	unsigned int m_posSlimit;
	unsigned int m_posU;
	unsigned int m_posUlimit;

	PERM* m_u_beta;
	unsigned long m_beta;

	std::stack<boost::tuple<unsigned int, unsigned int, unsigned int, unsigned int> > m_stackTodo;

	/// advance iterators so that they point to a new pair in the U \cross S loop
	bool advance();
	/// setup m_*beta elements
	void init();
	/// set iterators so that their position match the position indicated in m_posS and m_posU
	void reset();
};

//
//     ----       IMPLEMENTATION
//

template <class PERM, class TRANS>
SchreierGenerator<PERM, TRANS>::SchreierGenerator(const TRANS *U, PERMlistIt S_begin, PERMlistIt S_end)
		: m_Sbegin(S_begin), m_Scurrent(S_begin), m_Send(S_end),
		  m_U(U), m_Ubegin(m_U->begin()), m_Ucurrent(m_U->begin()), m_Uend(m_U->end()),
		  m_posS(0), m_posSlimit(0), m_posU(0), m_posUlimit(0), m_u_beta(0)
{
	init();
}

template <class PERM, class TRANS>
SchreierGenerator<PERM, TRANS>::~SchreierGenerator() {
	delete m_u_beta;
}

template <class PERM, class TRANS>
void SchreierGenerator<PERM, TRANS>::init() {
	m_beta = *m_Ucurrent;
	delete m_u_beta;
	m_u_beta = m_U->at(m_beta);
}


template <class PERM, class TRANS>
bool SchreierGenerator<PERM, TRANS>::hasNext() {
	if (m_Send != m_Scurrent && m_Uend != m_Ucurrent && (!m_posUlimit || m_posU < m_posUlimit)) {
		const PERM &x = **m_Scurrent;
		if (m_U->trivialByDefinition(x, x / m_beta)) {
			advance();
			return hasNext();
		}
		return true;
	}

	if (!m_stackTodo.empty()) {
		boost::tuple<unsigned int, unsigned int, unsigned int, unsigned int> lastTuple = m_stackTodo.top();
		m_stackTodo.pop();

		m_posS = boost::get<0>(lastTuple);
		m_posSlimit = boost::get<1>(lastTuple);
		m_posU = boost::get<2>(lastTuple);
		m_posUlimit = boost::get<3>(lastTuple);
		reset();

		return hasNext();
	}
	return false;
}

template <class PERM, class TRANS>
bool SchreierGenerator<PERM, TRANS>::advance() {
	++m_Scurrent;
	++m_posS;
	if (m_Scurrent == m_Send) {
		m_Scurrent = boost::next(m_Sbegin, m_posSlimit);
		m_posS = m_posSlimit;
		++m_Ucurrent;
		++m_posU;
		if (m_Ucurrent != m_Uend)
			init();
		else
			return false;
	}
	return true;
}

template <class PERM, class TRANS>
PERM SchreierGenerator<PERM, TRANS>::next() {
	BOOST_ASSERT(m_Scurrent != m_Send);
	BOOST_ASSERT(m_Ucurrent != m_Uend);

	PERMLIB_DEBUG(std::cout << "s = " << m_posS << "; u = " << m_posU << std::endl;)
	const PERM &x = **m_Scurrent;

	PERM g = *m_u_beta * x;
	boost::scoped_ptr<PERM> u_beta_ptr2(m_U->at(x / m_beta));
	u_beta_ptr2->invertInplace();
	g *= *u_beta_ptr2;

	advance();

	return g;
}

template <class PERM, class TRANS>
void SchreierGenerator<PERM, TRANS>::reset() {
	m_Scurrent = m_Sbegin;
	m_Ucurrent = m_Ubegin;
	int i = m_posS;
	while (--i>=0) ++m_Scurrent;
	i = m_posU;
	while (--i>=0) ++m_Ucurrent;

	if (m_Ucurrent != m_Uend)
		init();
}

template <class PERM, class TRANS>
void SchreierGenerator<PERM, TRANS>:: update(TRANS *U, PERMlistIt S_begin, PERMlistIt S_end) {
	if (U == m_U && S_begin == m_Sbegin && S_end == m_Send)
		return;
	m_U = U;
	m_Sbegin = S_begin;
	m_Send = S_end;
	m_Ubegin = U->begin();
	m_Uend = U->end();
	reset();
}

template <class PERM, class TRANS>
void SchreierGenerator<PERM, TRANS>:: update(unsigned int j) {
	m_stackTodo.push(boost::make_tuple(m_posS, m_posSlimit, m_posU, m_posUlimit));
	m_posUlimit = m_posU;
	m_posS = j;
	m_posSlimit = j;
	m_posU = 0;
	reset();
}

}

#endif // -- SCHREIERGENERATOR_H_
