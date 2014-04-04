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


#ifndef ORBIT_LIST_H_
#define ORBIT_LIST_H_

#include <permlib/transversal/orbit.h>

namespace permlib {

/// stores an orbit in a sorted list
template<class PERM,class PDOMAIN>
class OrbitList : public Orbit<PERM,PDOMAIN> {
public:
	virtual bool contains(const PDOMAIN& val) const;

	/// true iff orbit is empty (i.e. contains no element at all)
	bool empty() const { return m_orbitList.empty(); }

	/// computes orbit of beta under generators
	/**
	 * @param beta
	 * @param generators
	 * @param a ()-callable structure that defines how a PERM acts on a PDOMAIN-element
	 * @see Transversal::TrivialAction
	 */
	template<class Action>
	void orbit(const PDOMAIN& beta, const std::list<typename PERM::ptr> &generators, Action a);

	/// number of orbit elements
	size_t size() const { return m_orbitList.size(); }
    
    virtual const PDOMAIN& element() const;
protected:
	/// orbit elements as set
	std::list<PDOMAIN> m_orbitList;

	virtual bool foundOrbitElement(const PDOMAIN& alpha, const PDOMAIN& alpha_p, const typename PERM::ptr& p);
};

template <class PERM,class PDOMAIN>
inline bool OrbitList<PERM,PDOMAIN>::foundOrbitElement(const PDOMAIN& alpha, const PDOMAIN& alpha_p, const typename PERM::ptr& p) {
	typename std::list<PDOMAIN>::iterator it = std::lower_bound(m_orbitList.begin(), m_orbitList.end(), alpha_p);
	if (*it != alpha_p) {
		m_orbitList.insert(it, alpha_p);
		return true;
	}
	return false;
}

template <class PERM,class PDOMAIN>
inline bool OrbitList<PERM,PDOMAIN>::contains(const PDOMAIN& val) const {
	return std::binary_search(m_orbitList.begin(), m_orbitList.end(), val);
}

template <class PERM,class PDOMAIN>
template<class Action>
inline void OrbitList<PERM,PDOMAIN>::orbit(const PDOMAIN& beta, const std::list<typename PERM::ptr> &generators, Action a) {
	// use separate list here because m_orbitList is sorted
	std::list<PDOMAIN> orbitList;
	Orbit<PERM,PDOMAIN>::orbit(beta, generators, a, orbitList);
}

template <class PERM,class PDOMAIN>
inline const PDOMAIN& OrbitList<PERM,PDOMAIN>::element() const {
    return *(m_orbitList.begin());
}

}

#endif // -- ORBIT_LIST_H_
