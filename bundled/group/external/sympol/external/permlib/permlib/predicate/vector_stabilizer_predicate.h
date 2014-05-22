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


#ifndef VECTORSTABILIZERPREDICATE_H_
#define VECTORSTABILIZERPREDICATE_H_

#include <permlib/predicate/subgroup_predicate.h>

#include <boost/foreach.hpp>

namespace permlib {

/// predicate for the subgroup that stabilizes a given integer vector
template <class PERM>
class VectorStabilizerPredicate : public SubgroupPredicate<PERM> {
public:
	/// constructor
	/**
	 * @param begin begin iterator for an unsigned integer vector to be stabilized
	 * @param end   end   iterator for an unsigned integer vector to be stabilized
	 */
	template<class InputIterator>
	VectorStabilizerPredicate(InputIterator begin, InputIterator end);

	virtual bool operator()(const PERM &p) const;
	virtual bool childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const;
	virtual unsigned int limit() const;
private:
	std::vector<unsigned int> m_vec;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
template <class InputIterator>
VectorStabilizerPredicate<PERM>::VectorStabilizerPredicate(InputIterator begin, InputIterator end) 
	: m_vec(begin, end)
{ }

template <class PERM>
bool VectorStabilizerPredicate<PERM>::operator()(const PERM &p) const {
	for (unsigned int i = 0; i < m_vec.size(); ++i) {
		if (m_vec[p / i] != m_vec[i])
			return false;
	}
	return true;
}

template <class PERM>
bool VectorStabilizerPredicate<PERM>::childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const {
	return (m_vec[h / beta_i] == m_vec[beta_i]);
}

template <class PERM>
unsigned int VectorStabilizerPredicate<PERM>::limit() const {
	return m_vec.size();
}

}

#endif // -- VECTORSTABILIZERPREDICATE_H_
