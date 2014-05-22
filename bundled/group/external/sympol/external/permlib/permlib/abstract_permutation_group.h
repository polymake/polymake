// ---------------------------------------------------------------------------
//
//  This file is part of PermLib.
//
// Copyright (c) 2009-2012 Thomas Rehn <thomas@carmen76.de>
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

#include <vector>
#include <list>
#include <set>

#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>
#include <boost/dynamic_bitset.hpp>

#include <permlib/common.h>

#ifndef ABSTRACT_PERMUTATION_GROUP_H_
#define ABSTRACT_PERMUTATION_GROUP_H_

namespace permlib {

/// implementation type for abstract class AbstractPermutationGroup
enum AbstractGroupType {
	AGT_BSGS = 1,
	AGT_SymmetricProduct = 2
};

/// A high level interface for a permutation group
class AbstractPermutationGroup {
public:
	/// destructor
	virtual ~AbstractPermutationGroup() {}
	
	/// order of the group
	template<typename Integer>
	Integer order() const;
	
	/// order of the group
	boost::uint64_t order() const;
	
	/// computes the stabilizer of a set
	/**
	 * @param s set to be stabilized
	 * @return stabilizer; may return <i>this</i> if set is already stabilized
	 */
	virtual AbstractPermutationGroup* setStabilizer(const std::vector<dom_int>& s) const = 0;
	
	/// typedef for a list of orbits, each of which is a set
	typedef std::list<std::set<dom_int> > OrbitList;
	/// computes all orbits
	virtual OrbitList* orbits() const = 0;
	/// computes all orbits which contain a given set of elements
	/**
	 * @param s set of elements of which orbit has to be computed; vector must be sorted
	 * @return all orbits of the group which contain an elements from s
	 */
	virtual OrbitList* orbits(const std::vector<dom_int>& s) const = 0;
	
	/// checks whether a set is lexicographically minimal with respect to a given ordering of indices
	/**
	 * @param setIndices indices of the set for which minimality has to checked
	 * @param rankIndices list of indices; the order of these indices defines a partial order on {1..n}
	 * @return true iff set is minimal with respect to given ordering
	 */
	virtual bool isLexMinSet(const std::vector<dom_int>& setIndices, const std::vector<dom_int>& rankIndices) const = 0;

	/// implementation type of this abstract class
	virtual AbstractGroupType type() const = 0;
protected:
	/// fills a list with sizes of transversals along a stabilizer chain
	virtual void transversalSizes(std::vector<unsigned long>& sizes) const = 0;
};


template <typename Integer>
Integer AbstractPermutationGroup::order() const {
	Integer orderValue(1);
	std::vector<unsigned long> sizes;
	transversalSizes(sizes);
	BOOST_FOREACH(const unsigned long &s, sizes) {
		orderValue *= s;
	}
	return orderValue;
}

inline boost::uint64_t AbstractPermutationGroup::order() const {
	return order<boost::uint64_t>();
}

} // end NS

#endif
