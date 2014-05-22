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


#ifndef ORBIT_H_
#define ORBIT_H_

#include <list>

#include <boost/foreach.hpp>

namespace permlib {

/// abstract base class for orbit computation
template<class PERM,class PDOMAIN>
class Orbit {
public:
	virtual ~Orbit() {}
	
	/// true iff there exists a transversal element mapping \f$\alpha\f$ to val
	virtual bool contains(const PDOMAIN& val) const = 0;

	/// returns one element of the orbit
	virtual const PDOMAIN& element() const = 0;
	
	/// type of permutation used for this orbit
	typedef PERM PERMtype;
protected:
	/// computes orbit of beta under generators
	/**
	 * @param beta
	 * @param generators
	 * @param a ()-callable structure that defines how a PERM acts on a PDOMAIN-element
	 * @param orbitList a list of all orbit elements to be filled by the algorithm
	 */
	template<class Action>
	void orbit(const PDOMAIN& beta, const std::list<typename PERM::ptr> &generators, Action a, std::list<PDOMAIN>& orbitList);
	
	/// updates an existing orbit of beta after one element has been added
	/**
	 * if this instance of Orbit represents the orbit \f$\beta^{S_0}\f$,
	 * then after call of orbitUpdate it will represent the orbit \f$\beta^{S}\f$ where \f$S = S_0 \cup \{g\}\f$
	 * @param beta
	 * @param generators updated generators, which must include g
	 * @param g new generator which has not been there before
	 * @param a ()-callable structure that defines how a PERM acts on a PDOMAIN-element
	 * @param orbitList a list of all orbit elements to be filled by the algorithm
	 */
	template<class Action>
	void orbitUpdate(const PDOMAIN& beta, const std::list<typename PERM::ptr> &generators, const typename PERM::ptr &g, Action a, std::list<PDOMAIN>& orbitList);
	
	/// callback when the orbit algorithm constructs an element alpha_p from alpha and p
	/**
	 * @return true iff alpha_p is a new element that has not been seen before
	 */
	virtual bool foundOrbitElement(const PDOMAIN& alpha, const PDOMAIN& alpha_p, const typename PERM::ptr& p) = 0;
};

template <class PERM,class PDOMAIN>
template<class Action>
inline void Orbit<PERM,PDOMAIN>::orbit(const PDOMAIN& beta, const std::list<typename PERM::ptr> &generators, Action a, std::list<PDOMAIN>& orbitList) {
	if (orbitList.empty()) {
		orbitList.push_back(beta);
		foundOrbitElement(beta, beta, typename PERM::ptr());
	}
	BOOST_ASSERT( orbitList.size() >= 1 );
	
	PERMLIB_DEBUG(std::cout << "orbit of " << beta << std::endl;)
	typename std::list<PDOMAIN>::const_iterator it;
	for (it = orbitList.begin(); it != orbitList.end(); ++it) {
		const PDOMAIN &alpha = *it;
		BOOST_FOREACH(const typename PERM::ptr& p, generators) {
			//std::cout << "         " << orbitList.size() << std::endl;
			PDOMAIN alpha_p = a(*p, alpha);
			if (alpha_p != alpha && foundOrbitElement(alpha, alpha_p, p))
				orbitList.push_back(alpha_p);
		}
	}
	//std::cout << "orb size " << orbitList.size() << std::endl;
}

template <class PERM,class PDOMAIN>
template<class Action>
inline void Orbit<PERM,PDOMAIN>::orbitUpdate(const PDOMAIN& beta, const std::list<typename PERM::ptr> &generators, const typename PERM::ptr &g, Action a, std::list<PDOMAIN>& orbitList) {
	if (orbitList.empty()) {
		orbitList.push_back(beta);
		foundOrbitElement(beta, beta, typename PERM::ptr());
	}
	BOOST_ASSERT( orbitList.size() >= 1 );
	
	PERMLIB_DEBUG(std::cout << "orbiUpdate of " << beta << " and " << *g << std::endl;)
	const unsigned int oldSize = orbitList.size();
	// first, compute only ORBIT^g
	typename std::list<PDOMAIN>::const_iterator it;
	for (it = orbitList.begin(); it != orbitList.end(); ++it) {
		const PDOMAIN &alpha = *it;
		PDOMAIN alpha_g = a(*g, alpha);
		if (alpha_g != alpha && foundOrbitElement(alpha, alpha_g, g))
			orbitList.push_back(alpha_g);
	}
	
	if (oldSize == orbitList.size())
		return;
	
	orbit(beta, generators, a, orbitList);
}

}

#endif // -- ORBIT_H_
