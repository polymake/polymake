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


#ifndef SETIMAGEPREDICATE_H_
#define SETIMAGEPREDICATE_H_

#include <permlib/predicate/subgroup_predicate.h>

#include <boost/foreach.hpp>

namespace permlib {

/// coset-type predicate for group elements that map one set of points onto another given set of points
/**
 * holds for a \f$g\f$ such that \f$\Delta^g = \Gamma\f$ for two given sets \f$\Delta,\Gamma\f$
 */
template <class PERM>
class SetImagePredicate : public SubgroupPredicate<PERM> {
public:
	/// constructor
	/**
	 * @param deltaBegin begin iterator for source list of points (unsigned long) mapped to be onto target
	 * @param deltaEnd   end   iterator for source list of points (unsigned long) mapped to be onto target
	 * @param gammaBegin begin iterator for target list of points (unsigned long)
	 * @param gammaEnd   end   iterator for target list of points (unsigned long)
	 */
	template<class InputIterator>
	SetImagePredicate(InputIterator deltaBegin, InputIterator deltaEnd, InputIterator gammaBegin, InputIterator gammaEnd);

	virtual bool operator()(const PERM &p) const;
	virtual bool childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const;
	virtual unsigned int limit() const;
private:
	//TODO: perhaps use std::set here?
	std::vector<unsigned long> m_delta;
	std::vector<unsigned long> m_gamma;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
template<class InputIterator>
SetImagePredicate<PERM>::SetImagePredicate(InputIterator deltaBegin, InputIterator deltaEnd, InputIterator gammaBegin, InputIterator gammaEnd)
	: m_delta(deltaBegin, deltaEnd), m_gamma(gammaBegin, gammaEnd)
{
	BOOST_ASSERT(m_delta.size() == m_gamma.size());
}


template <class PERM>
bool SetImagePredicate<PERM>::operator()(const PERM &p) const {
	BOOST_FOREACH(unsigned long delta_i, m_delta) {
		if (std::find(m_gamma.begin(), m_gamma.end(), p / delta_i) == m_gamma.end())
			return false;
	}
	return true;
}

template <class PERM>
bool SetImagePredicate<PERM>::childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const {
	if (std::find(m_gamma.begin(), m_gamma.end(), h / beta_i) == m_gamma.end())
		return false;
	return true;
}

template <class PERM>
unsigned int SetImagePredicate<PERM>::limit() const {
	return m_delta.size();
}

}

#endif // -- SETIMAGEPREDICATE_H_
