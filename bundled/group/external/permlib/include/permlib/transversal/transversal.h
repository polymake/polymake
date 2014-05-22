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


#ifndef TRANSVERSAL_H_
#define TRANSVERSAL_H_

#include <permlib/sorter/base_sorter.h>
#include <permlib/transversal/orbit.h>

#include <map>
#include <list>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

namespace permlib {

template <class PERM>
class Transversal;

template <class PERM>
std::ostream &operator<< (std::ostream &out, const Transversal<PERM> &t) {
	out << "{";
	BOOST_FOREACH (boost::shared_ptr<PERM> p, t.m_transversal) {
		if (p)
			out << *p << ", ";
		else
			out << "O, ";
	}
	out << "}";
	return out;
}

/// Transversal base class corresponding to a base element \f$\alpha\f$
template <class PERM>
class Transversal : public Orbit<PERM,unsigned long> {
public:
	/// constructor
	/**
	 * @param n size of the set the group is working on
	 */
	Transversal(unsigned int n);
	/// virtual destructor
    virtual ~Transversal() {}
	
	/// returns a transversal element \f$u\f$ such that \f$\alpha^u\f$ equals val
    virtual PERM* at(unsigned long val) const = 0;
    
	/// true if Schreier generator constructed from x and the transversal element related to "to" is trivial by defintion
	virtual bool trivialByDefinition(const PERM& x, unsigned long to) const = 0;
	
	/// true iff there exists a transversal element mapping \f$\alpha\f$ to val
	virtual bool contains(const unsigned long& val) const;

	/// begin iterator of basic orbit
	std::list<unsigned long>::const_iterator begin() const { return this->m_orbit.begin(); };
	/// end iterator of basic orbit
	std::list<unsigned long>::const_iterator end() const { return this->m_orbit.end(); };
	/// pair of begin, end iterator
	std::pair<std::list<unsigned long>::const_iterator,std::list<unsigned long>::const_iterator> pairIt() const {
		return std::make_pair(begin(), end());
	}

	/// size of basic orbit / transversal
	size_t size() const { return this->m_orbit.size(); }
	
	/// size of the set the group is working on
    inline unsigned int n() const { return m_n; }

	/// sorts orbit according to order given by list of points
	/**
	 * @param Bbegin begin iterator of point list (unsigned long) inducing an order
	 * @param Bend   end   iterator of point list (unsigned long) inducing an order
	 */
    template <class InputIterator>
    void sort(InputIterator Bbegin, InputIterator Bend);
    
	/// true iff orbit is sorted
	inline bool sorted() const { return m_sorted; }
	
	/// action of a PERM on unsigned long element
	struct TrivialAction {
		/// action
		unsigned long operator()(const PERM &p, unsigned long v) const {
			return p / v;
		}
	};
	
	/// computes transversal based on orbit of \f$\alpha\f$ under generators
	/**
	 * @param alpha \f$\alpha\f$
	 * @param generators group generators for the orbit
	 */
	virtual void orbit(unsigned long alpha, const std::list<typename PERM::ptr> &generators);
	/// updates transversal based on orbit of \f$\alpha\f$ under generators where g is a new generator
	/**
	 * @param alpha \f$\alpha\f$
	 * @param generators group generators for the orbit
	 * @param g new generator that the transversal is updated for
	 */
	virtual void orbitUpdate(unsigned long alpha, const std::list<typename PERM::ptr> &generators, const typename PERM::ptr &g);
	
	/// updates transversal after group generators have been conjugated by g
	/**
	 * @param g permutation to conjugate
	 * @param gInv inverse of g for performance reasons
	 */
	virtual void permute(const PERM& g, const PERM& gInv);
	/// updates transversal after group generators have been exchanged
	/**
	 * @param generatorChange map of old generators to new generators
	 */
	virtual void updateGenerators(const std::map<PERM*,typename PERM::ptr>& generatorChange) {}
    
    virtual const unsigned long& element() const;
	
	/// to stream
    friend std::ostream &operator<< <> (std::ostream &out, const Transversal<PERM> &p);
protected:
	/// size of the set the group is working on
    unsigned int m_n;
	
	/// transversal elements
	std::vector<boost::shared_ptr<PERM> > m_transversal;
	
	/// orbit elements
    std::list<unsigned long> m_orbit;
	
	/// true if orbit is sorted (according to a previous sort(InputIterator, InputIterator) call
    bool m_sorted;
    
	/// stores that 'p' maps 'from' onto 'to'
    virtual void registerMove(unsigned long from, unsigned long to, const typename PERM::ptr &p);
	
	virtual bool foundOrbitElement(const unsigned long& alpha, const unsigned long& alpha_p, const typename PERM::ptr& p);
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
Transversal<PERM>::Transversal(unsigned int n_) 
	: m_n(n_), m_transversal(n_), m_sorted(false) 
{ }

template <class PERM>
void Transversal<PERM>::orbit(unsigned long beta, const std::list<typename PERM::ptr> &generators) {
	Orbit<PERM,unsigned long>::orbit(beta, generators, TrivialAction(), m_orbit);
}

template <class PERM>
void Transversal<PERM>::orbitUpdate(unsigned long beta, const std::list<typename PERM::ptr> &generators, const typename PERM::ptr &g) {
  Orbit<PERM,unsigned long>::orbitUpdate(beta, generators, g, TrivialAction(), m_orbit);
}

template <class PERM>
bool Transversal<PERM>::foundOrbitElement(const unsigned long& alpha, const unsigned long& alpha_p, const typename PERM::ptr& p) {
	BOOST_ASSERT( alpha_p < m_transversal.size() );
	if (!m_transversal[alpha_p]) {
		if (!p) {
			typename PERM::ptr identity(new PERM(m_n));
			registerMove(alpha, alpha_p, identity);
		} else {
			registerMove(alpha, alpha_p, p);
		}
		return true;
	}
	return false;
}

template <class PERM>
bool Transversal<PERM>::contains(const unsigned long& val) const { 
	return m_transversal[val] != 0; 
}

template <class PERM>
void Transversal<PERM>::registerMove(unsigned long from, unsigned long to, const typename PERM::ptr &p) {
	m_sorted = false; 
}


template <class PERM>
template <class InputIterator>
void Transversal<PERM>::sort(InputIterator Bbegin, InputIterator Bend) {
	this->m_orbit.sort(BaseSorter(m_n, Bbegin, Bend));
	m_sorted = true;
}

template <class PERM>
void Transversal<PERM>::permute(const PERM& g, const PERM& gInv) {
	std::vector<boost::shared_ptr<PERM> > temp(m_n);
	for (unsigned long i=0; i<m_n; ++i) {
		const unsigned long j = g / i;
		temp[j] = m_transversal[i];
	}
	std::copy(temp.begin(), temp.end(), m_transversal.begin());
	BOOST_FOREACH(unsigned long& alpha, this->m_orbit) {
		alpha = g / alpha;
	}
	m_sorted = false;
}
    
template <class PERM>
inline const unsigned long& Transversal<PERM>::element() const {
    return m_orbit.front();
}

}

#endif // -- TRANSVERSAL_H_
