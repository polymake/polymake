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


#ifndef GROUPINTERSECTIONPREDICATE_H_
#define GROUPINTERSECTIONPREDICATE_H_

#include <permlib/bsgs.h>
#include <permlib/predicate/subgroup_predicate.h>

#include <boost/foreach.hpp>

namespace permlib {

/// predicate for the subgroup that arises as the intersection of two given groups
template <class PERM, class TRANS>
class GroupIntersectionPredicate : public SubgroupPredicate<PERM> {
public:
	/// constructor
	/**
	 * @param g first subgroup of the intersection
	 * @param h second subgroup of the intersection
	 */
	GroupIntersectionPredicate(const BSGS<PERM, TRANS> &g, const BSGS<PERM, TRANS> &h);

	virtual bool operator()(const PERM &p) const;
	virtual bool childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const;
	virtual unsigned int limit() const;
private:
	const BSGS<PERM, TRANS> &m_G;
	const BSGS<PERM, TRANS> &m_H;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM, class TRANS>
GroupIntersectionPredicate<PERM,TRANS>::GroupIntersectionPredicate(const BSGS<PERM, TRANS> &g, const BSGS<PERM, TRANS> &h) 
	: m_G(g), m_H(h) 
{
	BOOST_ASSERT(m_G.n == m_H.n);
    BOOST_ASSERT(m_G.order() <= m_H.order());
}

template <class PERM, class TRANS>
bool GroupIntersectionPredicate<PERM,TRANS>::operator()(const PERM &p) const {
	return m_G.sifts(p) && m_H.sifts(p);
}

template <class PERM, class TRANS>
bool GroupIntersectionPredicate<PERM,TRANS>::childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const {
	//TODO: check \beta_l^{g h^{-1}} \in \beta_l^{H_{(beta_1 ... beta_{l-1})}} instead
	//      cf. Handbook of Computational Group Theory, sec 4.6.6
	PERM siftee(m_H.n);
	const unsigned int m = m_H.sift(h, siftee, 0, i+1);
	return i+1 == m;
}

template <class PERM, class TRANS>
unsigned int GroupIntersectionPredicate<PERM,TRANS>::limit() const { 
	return m_G.B.size(); 
}

}

#endif // -- GROUPINTERSECTIONPREDICATE_H_
