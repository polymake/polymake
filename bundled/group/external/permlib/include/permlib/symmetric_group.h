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


#ifndef SYMMETRICGROUP_H_
#define SYMMETRICGROUP_H_

#include <permlib/bsgs_core.h>
#include <permlib/transversal/symmetric_group_transversal.h>

#include <boost/shared_ptr.hpp>

namespace permlib {

/// representation of a symmetric group
/**
 * Dedicated data structure for a symmetric group
 * because base and transversals are known in advance
 * and easily computed.
 * This group implementation uses a special transversal
 * that computes transversal elements on demand.
 */
template<class PERM>
struct SymmetricGroup : public BSGSCore<PERM, SymmetricGroupTransversal<PERM> > {
	/// constructs a symmetric group of degree n
	explicit SymmetricGroup(unsigned int n);
	/// copy constructor
	SymmetricGroup(const SymmetricGroup<PERM>& symGroup);
	/// assignment operator
	SymmetricGroup& operator=(const SymmetricGroup<PERM>& symGroup);
	
	/// transversal type used for the BSGS representation
	typedef SymmetricGroupTransversal<PERM> TRANS;
	
	virtual bool isSymmetricGroup() const { return true; }
private:
	void copy(const SymmetricGroup<PERM>& symGroup);
};

template<class PERM>
inline SymmetricGroup<PERM>::SymmetricGroup(unsigned int n_)
	: BSGSCore<PERM,TRANS>(-n_, n_, n_)
{
	BOOST_ASSERT(this->n > 0);
	BSGSCore<PERM,TRANS>::U.reserve(this->n);
	for (unsigned int i = 0; i < this->n; ++i) {
		BSGSCore<PERM,TRANS>::B[i] = this->n-1-i;
		BSGSCore<PERM,TRANS>::U.push_back(SymmetricGroupTransversal<PERM>(this, i));
		if (i < static_cast<unsigned int>(this->n-1)) {
			boost::shared_ptr<PERM> gen(new PERM(this->n));
			gen->setTransposition(i, i+1);
			BSGSCore<PERM,TRANS>::S.push_back(gen);
		}
	}
}

template<class PERM>
inline void SymmetricGroup<PERM>::copy(const SymmetricGroup<PERM>& symGroup) 
{
	const unsigned long& n2 = symGroup.n;
	BSGSCore<PERM,TRANS>::U.reserve(n2);
	for (unsigned int i = 0; i < n2; ++i) {
		BSGSCore<PERM,TRANS>::B[i] = symGroup.B[i];
		BSGSCore<PERM,TRANS>::U.push_back(SymmetricGroupTransversal<PERM>(this, i));
		if (i < n2-1) {
			boost::shared_ptr<PERM> gen(new PERM(n2));
			gen->setTransposition(i, i+1);
			BSGSCore<PERM,TRANS>::S.push_back(gen);
		}
	}
}

template<class PERM>
inline SymmetricGroup<PERM>::SymmetricGroup(const SymmetricGroup<PERM>& symGroup) 
	: BSGSCore<PERM,TRANS>(-symGroup.n, symGroup.n, symGroup.n)
{
	copy(symGroup);
}

template<class PERM>
inline SymmetricGroup<PERM>& SymmetricGroup<PERM>::operator=(const SymmetricGroup<PERM>& symGroup) 
{
	BOOST_ASSERT(symGroup.n == this->n);
	
	BSGSCore<PERM,TRANS>::n = symGroup.n;
	BSGSCore<PERM,TRANS>::m_id = symGroup.m_id;
	copy(symGroup);
	return *this;
}

}


#endif // SYMMETRICGROUP_H_
