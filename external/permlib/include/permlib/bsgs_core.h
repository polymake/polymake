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

#include <list>
#include <vector>

#ifndef BSGSCORE_H_
#define BSGSCORE_H_

namespace permlib {
	
/// core data of a base and strong generating set (BSGS)
template <class PERM, class TRANS>
struct BSGSCore {
	public:
		/// permutation type used by this BSGS
		typedef PERM PERMtype;
		/// transversal type used by this BSGS
		typedef TRANS TRANStype;
		
		typedef std::list<typename PERM::ptr> PERMlist;
		
		/// empty destructor
		virtual ~BSGSCore() {}
		
		/// base \f$B\f$
		std::vector<dom_int> B;
		/// strong generating set \f$S\f$
		PERMlist S;
		/// transversals \f$U\f$ along the stabilizer chain
		std::vector<TRANS> U;
		/// degree of group
		dom_int n;
		
		/// checks for equality by internal id only
		/**
		* internal id is preserved by copy constructor and assignment operator
		*/
		virtual bool operator==(const BSGSCore<PERM,TRANS>& bsgs) const;
		
		/// true if this structure represents a symmetric group
		virtual bool isSymmetricGroup() const { return false; }
	protected:
		/// constructs empty data structure with given group id
		explicit BSGSCore(unsigned int id) : m_id(id) {}
		/// constructs empty data structure with given group id, group degree n and base size n
		BSGSCore(unsigned int id, dom_int n_, dom_int bSize) : B(bSize), n(n_), m_id(id) {}
		/// kind of copy constructor, initializes data structure with given data
		BSGSCore(unsigned int id, const std::vector<dom_int>& B_,  const std::vector<TRANS>& U_, dom_int n_) 
			: B(B_), U(U_.size(), TRANS(n_)), n(n_), m_id(id) {}
		
		/// id of this BSGS instance
		int m_id;
	private:
		/// derived classes must implement copy constructor
		BSGSCore(const BSGSCore<PERM,TRANS>& copy) {}
		/// derived classes must implement assignment operator
		BSGSCore& operator=(const BSGSCore<PERM,TRANS>& copy) {}
};

template <class PERM, class TRANS>
bool BSGSCore<PERM, TRANS>::operator==(const BSGSCore<PERM,TRANS>& bsgs) const {
	return bsgs.m_id == this->m_id;
}

}

#endif // BSGSCORE_H_

