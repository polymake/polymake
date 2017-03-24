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


#ifndef ORBIT_SET_H_
#define ORBIT_SET_H_

#include <permlib/transversal/orbit.h>

namespace permlib {

/// stores an orbit in a set for fast contains() operation
template<class PERM,class PDOMAIN>
class OrbitSet : public Orbit<PERM,PDOMAIN> {
public:
	virtual bool contains(const PDOMAIN& val) const;
	
	/// true iff orbit is empty (i.e. contains no element at all)
	bool empty() const { return m_orbitSet.empty(); }
	
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
	size_t size() const {return m_orbitSet.size(); }
    
	virtual const PDOMAIN& element() const;
	
	typedef typename std::set<PDOMAIN>::const_iterator const_iterator;
	/// begin-iterator to orbit elements
	const_iterator begin() const { return m_orbitSet.begin(); }
	/// end-iterator to orbit elements
	const_iterator end() const { return m_orbitSet.end(); }
protected:
	/// orbit elements as set
	std::set<PDOMAIN> m_orbitSet;
	
	virtual bool foundOrbitElement(const PDOMAIN& alpha, const PDOMAIN& alpha_p, const typename PERM::ptr& p);
};

template <class PERM,class PDOMAIN>
inline bool OrbitSet<PERM,PDOMAIN>::foundOrbitElement(const PDOMAIN& alpha, const PDOMAIN& alpha_p, const typename PERM::ptr& p) {
	if (m_orbitSet.insert(alpha_p).second) {
		PERMLIB_DEBUG(std::cout << " o " << alpha_p <<  "  @ " << (*p) << std::endl;)
		return true;
	}
	return false;
}

template <class PERM,class PDOMAIN>
inline bool OrbitSet<PERM,PDOMAIN>::contains(const PDOMAIN& val) const {
	return m_orbitSet.find(val) != m_orbitSet.end();
}

template <class PERM,class PDOMAIN>
template<class Action>
inline void OrbitSet<PERM,PDOMAIN>::orbit(const PDOMAIN& beta, const std::list<typename PERM::ptr> &generators, Action a) {
	std::list<PDOMAIN> orbitList;
	Orbit<PERM,PDOMAIN>::orbit(beta, generators, a, orbitList);
}

template <class PERM,class PDOMAIN>
inline const PDOMAIN& OrbitSet<PERM,PDOMAIN>::element() const {
    return *(m_orbitSet.begin());
}

}

#endif // -- ORBIT_SET_H_
