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


#ifndef REFINEMENT_H_
#define REFINEMENT_H_

#include <vector>
#include <boost/shared_ptr.hpp>

#include <permlib/search/partition/partition.h>

namespace permlib {
namespace partition {

/// type of a refinement
enum RefinementType {
	Default,
	Backtrack,
	Group
};

/// base class for a \f$\mathcal P\f$-refinement which is used in an R-base and bound to an initial partition
template<class PERM>
class Refinement {
public:
	/// constructor
	Refinement(unsigned long n, RefinementType type);
	/// destructor
	virtual ~Refinement();
	
	/// applies (left-)refinement to partition and initializes refinement for future use in R-base
	/**
	 * @return true iff this is a strict refinement
	 */
	bool initializeAndApply(Partition& pi);
	/// applies (left-)refinement to pi which is the original partition this refinement was initialized to
	/**
	 * @see undo
	 * @see apply2
	 * @return number of elementary intersections that were needed for refinement application
	 */
	virtual unsigned int apply(Partition& pi) const = 0;
	
	/// applies (right-)refinement to pi which is the image of the original partition this refinement was initialized to under t
	/**
	 * @see undo
	 * @see apply
	 * @return number of elementary intersections that were needed for refinement application
	 */
	virtual unsigned int apply2(Partition& pi, const PERM& t) const;
	
	/// reverts the last count elementary intersections of partition pi
	void undo(Partition& pi, unsigned int count) const;
	
	typedef typename boost::shared_ptr<Refinement<PERM> > RefinementPtr;
	typedef typename std::vector<RefinementPtr>::const_iterator RefinementPtrIterator;
	
	/// the type of this refinement
	RefinementType type() const;
	/// number of sibling of this refinement in the search tree
	unsigned int alternatives() const;
	/// iterator to begin of refinement siblings in the search tree
	RefinementPtrIterator backtrackBegin() const { return m_backtrackRefinements.begin(); }
	/// iterator to end of refinement siblings in the search tree
	RefinementPtrIterator backtrackEnd() const { return m_backtrackRefinements.end(); }
	
	/// initializes refinement
	virtual bool init(Partition& pi) = 0;
	
	/// sorts siblings in the search tree
	virtual void sort(const BaseSorterByReference&, const Partition*) { }
protected:
	/// length of partitions to work with
	unsigned long m_n;
	/// refinement siblings in the search tree
	std::vector<RefinementPtr> m_backtrackRefinements;
	/// indices of elementary intersections to apply during refinement application
	std::list<int> m_cellPairs;
	
	/// true iff refinement is initalized
	bool initialized() const;
private:
	bool m_initialized;
	RefinementType m_type;
};

template<class PERM>
Refinement<PERM>::Refinement(unsigned long n_, RefinementType type_) 
	: m_n(n_), m_initialized(false), m_type(type_)
{ }

template<class PERM>
Refinement<PERM>::~Refinement() 
{ }

template<class PERM>
unsigned int Refinement<PERM>::alternatives() const {
	return m_backtrackRefinements.size();
}

template<class PERM>
RefinementType Refinement<PERM>::type() const {
	return m_type;
}

template<class PERM>
bool Refinement<PERM>::initializeAndApply(Partition& pi) {
	if (!m_initialized) {
		m_initialized = true;
		return init(pi);
	}
	return false;
}

template<class PERM>
bool Refinement<PERM>::initialized() const {
	return m_initialized;
}

template<class PERM>
void Refinement<PERM>::undo(Partition& pi, unsigned int count) const {
	for (unsigned int i=0; i<count; ++i)
		pi.undoIntersection();
}

template<class PERM>
unsigned int Refinement<PERM>::apply2(Partition& pi, const PERM& t) const { 
	return this->apply(pi); 
}

}
}

#endif // -- REFINEMENT_H_
