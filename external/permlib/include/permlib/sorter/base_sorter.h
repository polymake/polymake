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


#ifndef BASESORTER_H
#define BASESORTER_H

#include <boost/utility.hpp>

namespace permlib {

/// A sorter that sorts a sequence with respect to a given input ordering
template<class ORDER>
class OrderedSorter : public std::binary_function<unsigned long, unsigned long, bool> {
public:
	/// true iff a preceeds b in given sequence
	bool operator() (unsigned long a, unsigned long b) const {
		BOOST_ASSERT(a < m_size && b < m_size);
		return m_order[a] < m_order[b];
	}
protected:
	/// constructor for direct vector usage
	/**
	 * @param size size of domain which the order applies to
	 */
	explicit OrderedSorter(unsigned int size) 
		: m_size(size),
		// initialize m_size elements with value m_size
		m_order(m_size, m_size)
	{}
	
	/// constructor for reference use
	explicit OrderedSorter(ORDER order) 
		: m_size(order.size()), m_order(order)
	{}
	
	/// size of domain which the order applies to
	unsigned int m_size;
	/// array which defines the order of points
	ORDER m_order;
};

/// A sorter that sorts a sequence (e.g. \f$\Omega\f$) with respect to a given input ordering (e.g. a base)
/**
 * note that copying (as it is implicitly done e.g. when used with std::sort) is expensive
 * in this cases try BaseSorterByReference instead
 */
class BaseSorter : public OrderedSorter<std::vector<unsigned long> > {
public:
	/// constructor
	/**
	 * @param size size of sequence to be sorted
	 * @param begin begin iterator for partial sequence that induces the ordering
	 * @param end   end   iterator for partial sequence that induces the ordering
	 */
	template <class InputIterator>
	BaseSorter(unsigned int size, InputIterator begin, InputIterator end) 
		: OrderedSorter<std::vector<unsigned long> >(size)
	{
		fillOrder(begin, end, m_order);
	}
	
	/// constructs an ordering array
	/**
	 * @param begin begin iterator for partial sequence that induces the ordering
	 * @param end   end   iterator for partial sequence that induces the ordering
	 * @param order vector to store the ordering array
	 */
	template <class InputIterator>
	static void fillOrder(InputIterator begin, InputIterator end, std::vector<unsigned long>& order) {
		InputIterator it;
		unsigned int i = 0;
		// base elements first
		for (it = begin; it != end; ++it) {
			order[*it] = ++i;
		}
	}
};


/// A sorter that sorts a sequence (e.g. \f$\Omega\f$) with respect to a given input ordering (e.g. a base)
/**
 * This class uses a reference to a given ordering array to determine the order of elements
 */
class BaseSorterByReference : public OrderedSorter<const std::vector<unsigned long>&> {
public:
	/// constructor
	explicit BaseSorterByReference(const std::vector<unsigned long>& order) : OrderedSorter<const std::vector<unsigned long>& >(order)
	{ }
	
	/// constructs an ordering array with the same parameters as BaseSorter for use with BaseSorterByReference
	template <class InputIterator>
	static std::vector<unsigned long> createOrder(unsigned int size, InputIterator begin, InputIterator end) {
		std::vector<unsigned long> order(size,size);
		BaseSorter::fillOrder(begin, end, order);
		return order;
	}
};

}

#endif // -- BASESORTER_H
