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

#include <boost/scoped_ptr.hpp>
#include <algorithm>
#include <vector>
#include <set>

#ifndef ABSTRACT_BSGS_HELPERS_H_
#define ABSTRACT_BSGS_HELPERS_H_

namespace permlib {
namespace helpers {

/// helper class to decide when an permutation action on a set is trivial or can be reduced to a subset
class SupportRestriction {
public:
	/// destructor
	virtual ~SupportRestriction() { }
	
	/**
	 * @return true if the permutation action on a set is trivial
	 */
	virtual bool canBeIgnored() const = 0;
	/**
	 * @return a possibly shrunken set on which the permutation action is non-trivial.
	 */
	virtual const std::vector<dom_int>* set() const = 0;
};

/// This class never imposes a restriction on any set.
class BaseSupportRestriction : public SupportRestriction {
public:
	/**
	 * @param support the support of the group (i.e. a set of all elements that are moved by at least one permutation)
	 * @param s the set for which we want to decide whether the permutation action is trivial
	 */
	BaseSupportRestriction(const boost::shared_ptr<std::set<dom_int> >& support, const std::vector<dom_int>& s)
		: m_support(support), m_originalSet(s) {}
	
	/**
	 * @return always false
	 */
	virtual bool canBeIgnored() const { return false; }
	/**
	 * @return always the original set
	 */
	virtual const std::vector<dom_int>* set() const { return &m_originalSet; }
protected:
	const boost::shared_ptr<std::set<dom_int> > m_support;
	const std::vector<dom_int>& m_originalSet;
};

/// This class implements canBeIgnored() but has a trivial set()
class ReducedSupportRestriction : public BaseSupportRestriction {
public:
	/**
	 * @param support the support of the group (i.e. a set of all elements that are moved by at least one permutation)
	 * @param s the set for which we want to decide whether the permutation action is trivial
	 */
	ReducedSupportRestriction(const boost::shared_ptr<std::set<dom_int> >& support, const std::vector<dom_int>& s)
		: BaseSupportRestriction(support, s) {}
	
	/**
	 * @return true iff support does not intersect s
	 */
	virtual bool canBeIgnored() const {
		BOOST_ASSERT( m_support );
		return std::find_first_of(m_support->begin(), m_support->end(), m_originalSet.begin(), m_originalSet.end()) == m_support->end();
	}
};

/// This class implements both canBeIgnored() and set()
class FullSupportRestriction : public BaseSupportRestriction {
public:
	/**
	 * @param support the support of the group (i.e. a set of all elements that are moved by at least one permutation)
	 * @param s the set for which we want to decide whether the permutation action is trivial
	 */
	FullSupportRestriction(const boost::shared_ptr<std::set<dom_int> >& support, const std::vector<dom_int>& s) 
		: BaseSupportRestriction(support, s), m_reducedSet(0) 
	{
		m_reducedSet = new std::vector<dom_int>();
		m_reducedSet->reserve(s.size());
		std::vector<dom_int> sorted(s);
		std::sort(sorted.begin(), sorted.end());
		std::set_intersection(m_support->begin(), m_support->end(), sorted.begin(), sorted.end(), std::back_inserter(*m_reducedSet));
	}
	virtual ~FullSupportRestriction() { delete m_reducedSet; }

	/**
	 * @return true iff support does not intersect s
	 */
	bool canBeIgnored() const {
		BOOST_ASSERT( m_reducedSet );
		return m_reducedSet->empty();
	}
	/**
	 * @return the intersection of support and s
	 */
	const std::vector<dom_int>* set() const {
		return m_reducedSet;
	}
private:
	std::vector<dom_int>* m_reducedSet;
};

} // end NS permlib::helpers
} // end NS permlib

#endif
