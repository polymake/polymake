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


#ifndef GROUPSORTER_H
#define GROUPSORTER_H

#include <boost/foreach.hpp>

namespace permlib {

/// A sorter that sorts a sequence of permutations with respect to a ordering induced by a base
template <class PERM>
class GroupSorter : public std::binary_function<PERM, PERM, bool>{
public:
	/// constructor
	/**
	 * @param size size of sequence to be sorted
	 * @param begin begin iterator for partial sequence that induces the ordering
	 * @param end   end   iterator for partial sequence that induces the ordering
	 */
	template <class InputIterator>
	GroupSorter(unsigned int size, InputIterator begin, InputIterator end) :
		// initialize m_size elements with value m_size
		m_order(size, size),
		m_base(begin, end)
	{
		InputIterator it;
		unsigned int i = 0;
		// base elements first
		for (it = begin; it != end; ++it) {
			m_order[*it] = ++i;
		}
	}

	/// true iff a < b
	bool operator() (PERM a, PERM b) const {
		BOOST_FOREACH(unsigned long beta, m_base) {
			if (m_order[a / beta] < m_order[b / beta])
				return true;
		}
		return false;
	}
private:
	std::vector<unsigned long> m_order;
	std::vector<unsigned long> m_base;
};

}

#endif // -- GROUPSORTER_H
