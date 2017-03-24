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


#ifndef REFIMENET_FAMILY_H
#define REFIMENET_FAMILY_H

#include <permlib/search/partition/group_refinement.h>
#include <permlib/search/partition/set_stabilize_refinement.h>
#include <permlib/search/partition/set_image_refinement.h>
#include <permlib/search/partition/matrix_refinement2.h>

namespace permlib {
namespace partition {

/// represents a class of \f$\mathcal P\f$-refinements for a given problem
/**
 * In contrast to Refinement<PERM> , this refinement is not bound a certain partition.
 */
template<class PERM>
class RefinementFamily {
public:
	typedef typename Refinement<PERM>::RefinementPtr RefinementPtr;
	typedef boost::shared_ptr<Partition> PartitionPtr;

    /// virtual destructor
    virtual ~RefinementFamily() {}
	
	/// tries to initialize a suitable Refinement<PERM> for given partition
	/**
	 * @param pi partition to initialize the refinement to
	 * @return pair of the result of refinement application and refinement if a strict refinement could be found; NULL-pointers otherwise
	 */
	virtual std::pair<PartitionPtr,RefinementPtr> apply(Partition& pi) const = 0;
};

///  \f$\mathcal P\f$-refinements for group membership
template<class PERM,class TRANS>
class GroupRefinementFamily : public RefinementFamily<PERM> {
public:
	typedef typename RefinementFamily<PERM>::RefinementPtr RefinementPtr;
	typedef typename RefinementFamily<PERM>::PartitionPtr PartitionPtr;
	
	/// refinement family for group membership in given group
	explicit GroupRefinementFamily(const BSGSCore<PERM,TRANS>& bsgs) : m_bsgs(bsgs) {}
    
	virtual std::pair<PartitionPtr,RefinementPtr> apply(Partition& pi) const {
		RefinementPtr ref(new GroupRefinement<PERM,TRANS>(m_bsgs));
		GroupRefinement<PERM,TRANS> *gref = static_cast<GroupRefinement<PERM,TRANS>*>(ref.get());
		bool strictRefinement = gref->initializeAndApply(pi);
		if (strictRefinement)
			return std::make_pair(PartitionPtr(new Partition(pi)), ref);
		else
			return std::make_pair(PartitionPtr(), RefinementPtr());
	}
private:
	const BSGSCore<PERM,TRANS>& m_bsgs;
};

///  \f$\mathcal P\f$-refinements for set stabilization
template<class PERM>
class SetStabilizeRefinementFamily : public RefinementFamily<PERM> {
public:
	typedef typename RefinementFamily<PERM>::RefinementPtr RefinementPtr;
	typedef typename RefinementFamily<PERM>::PartitionPtr PartitionPtr;
	
	/// refinement family for set stabilization of given set
	/**
	 * @param n length of partitions to work with
	 * @param begin begin iterator(unsigned long) to set which is to stabilize
	 * @param end end iterator(unsigned long) to set which is to stabilize
	 */
	template<class InputIterator>
	SetStabilizeRefinementFamily(unsigned long n, InputIterator begin, InputIterator end) : m_n(n), toStab(begin, end) 
	{}

	virtual std::pair<PartitionPtr,RefinementPtr> apply(Partition& pi) const {
		RefinementPtr ref(new SetStabilizeRefinement<PERM>(m_n, toStab.begin(), toStab.end()));
		SetStabilizeRefinement<PERM> *gref = static_cast<SetStabilizeRefinement<PERM>*>(ref.get());
		bool strictRefinement = gref->initializeAndApply(pi);
		if (strictRefinement)
			return std::make_pair(PartitionPtr(new Partition(pi)), ref);
		else
			return std::make_pair(PartitionPtr(), RefinementPtr());
	}
private:
	unsigned long m_n;
	std::vector<unsigned long> toStab;
};

///  \f$\mathcal P\f$-refinements for set stabilization
template<class PERM>
class SetImageRefinementFamily : public RefinementFamily<PERM> {
public:
	typedef typename RefinementFamily<PERM>::RefinementPtr RefinementPtr;
	typedef typename RefinementFamily<PERM>::PartitionPtr PartitionPtr;
	
	/// refinement family for set stabilization of given set
	/**
	 * @param n length of partitions to work with
	 * @param begin iterator(unsigned long) begin of the set \f$\Delta\f$
	 * @param end iterator(unsigned long) end of the set \f$\Delta\f$
	 * @param beginImg iterator(unsigned long) begin of the set \f$\Gamma\f$
	 * @param endImg iterator(unsigned long) end of the set \f$\Gamma\f$
	 */
	template<class InputIterator>
	SetImageRefinementFamily(unsigned long n, InputIterator begin, InputIterator end, InputIterator beginImg, InputIterator endImg) 
		: m_n(n), delta(begin, end), phi(beginImg, endImg)
	{}
    
	virtual std::pair<PartitionPtr,RefinementPtr> apply(Partition& pi) const {
		RefinementPtr ref(new SetImageRefinement<PERM>(m_n, delta.begin(), delta.end(), phi.begin(), phi.end()));
		SetImageRefinement<PERM> *gref = static_cast<SetImageRefinement<PERM>*>(ref.get());
		bool strictRefinement = gref->initializeAndApply(pi);
		if (strictRefinement)
			return std::make_pair(PartitionPtr(new Partition(pi)), ref);
		else
			return std::make_pair(PartitionPtr(), RefinementPtr());
	}
private:
	unsigned long m_n;
	std::vector<unsigned long> delta;
	std::vector<unsigned long> phi;
};

///  \f$\mathcal P\f$-refinements for symmetric matrix automorphisms
template<class PERM,class MATRIX>
class MatrixAutomorphismRefinementFamily : public RefinementFamily<PERM> {
public:
	typedef typename RefinementFamily<PERM>::RefinementPtr RefinementPtr;
	typedef typename RefinementFamily<PERM>::PartitionPtr PartitionPtr;
	
	/// refinement family for symmetric matrix automorphisms
	/**
	 * @param n length of partitions to work with
	 * @param matrix symmetric matrix
	 */
	MatrixAutomorphismRefinementFamily(unsigned long n, const MATRIX& matrix) : m_n(n), m_matrix(matrix) 
	{}
	
	virtual std::pair<PartitionPtr,RefinementPtr> apply(Partition& pi) const {
		RefinementPtr ref(new MatrixRefinement2<PERM,MATRIX>(m_n, m_matrix));
		MatrixRefinement2<PERM,MATRIX> *gref = static_cast<MatrixRefinement2<PERM,MATRIX>*>(ref.get());
		bool strictRefinement = gref->initializeAndApply(pi);
		if (strictRefinement)
			return std::make_pair(PartitionPtr(new Partition(pi)), ref);
		else
			return std::make_pair(PartitionPtr(), RefinementPtr());
	}
private:
	unsigned long m_n;
	const MATRIX& m_matrix;
};

}
}

#endif // -- REFIMENET_FAMILY_H
