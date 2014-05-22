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


#ifndef SHALLOWSCHREIERTREETRANSVERSAL_H_
#define SHALLOWSCHREIERTREETRANSVERSAL_H_

#include <permlib/transversal/schreier_tree_transversal.h>

#include <boost/scoped_ptr.hpp>
#include <boost/dynamic_bitset.hpp>

namespace permlib {

/// Transversal class that stores elements in a shallow Schreier tree
template <class PERM>
class ShallowSchreierTreeTransversal : public SchreierTreeTransversal<PERM> {
public:
	/// constructor
    ShallowSchreierTreeTransversal(unsigned int n);

    virtual void orbit(unsigned long beta, const std::list<typename PERM::ptr> &generators);
	virtual void orbitUpdate(unsigned long alpha, const std::list<typename PERM::ptr> &generators, const typename PERM::ptr &g);
	
	virtual void updateGenerators(const std::map<PERM*,typename PERM::ptr>& generatorChange);
	/// returns a clone of this transversal
	/**
	 * the group generators that the clone may use are given by the transition map
	 * @param generatorChange transition map
	 */
	ShallowSchreierTreeTransversal<PERM> clone(const std::map<PERM*,typename PERM::ptr>& generatorChange) const;
	
	virtual void permute(const PERM& g, const PERM& gInv);
protected:
	/// ordered list of group elements that are used as cube labels
	std::list<typename PERM::ptr> m_cubeLabels;

	/// adds a new cube label where s maps beta_prime to a point that has no transversal element yet
	void addNewCubeLabel(unsigned long beta, const PERM &s, const unsigned long &beta_prime);
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
ShallowSchreierTreeTransversal<PERM>::ShallowSchreierTreeTransversal(unsigned int n_)
	: SchreierTreeTransversal<PERM>(n_)
{ }

template <class PERM>
void ShallowSchreierTreeTransversal<PERM>::orbitUpdate(unsigned long beta, const std::list<typename PERM::ptr> &generators, const typename PERM::ptr &g) {
	this->orbit(beta, generators);
}

template <class PERM>
void ShallowSchreierTreeTransversal<PERM>::orbit(unsigned long beta, const std::list<typename PERM::ptr> &generators) {
	std::vector<boost::shared_ptr<PERM> > &transversal = Transversal<PERM>::m_transversal;
    
	if (Transversal<PERM>::size() == 0) {
		Transversal<PERM>::m_orbit.push_back(beta);
		boost::shared_ptr<PERM> identity(new PERM(this->m_n));
			transversal[beta] = identity;
	}
        
	typename std::list<unsigned long>::const_iterator it;

	PERM g(this->m_n);
	typename std::list<typename PERM::ptr>::const_iterator genIt = generators.begin();
	for (it = Transversal<PERM>::m_orbit.begin(); it != Transversal<PERM>::m_orbit.end(); ++it) {
		for (genIt = generators.begin(); genIt != generators.end(); ++genIt) {
			const unsigned long &beta_prime = *it;
			if (!transversal[**genIt / beta_prime]) {
				addNewCubeLabel(beta, **genIt, beta_prime);
			}
		}
	}
}

template <class PERM>
void ShallowSchreierTreeTransversal<PERM>::addNewCubeLabel(unsigned long beta, const PERM &s, const unsigned long &beta_prime) {
	std::vector<boost::shared_ptr<PERM> > &transversal = Transversal<PERM>::m_transversal;
	boost::shared_ptr<PERM> gPath(SchreierTreeTransversal<PERM>::at(beta_prime));
	*gPath *= s;
	// will be new generator, so better flush it
	gPath->flush();
	
	// compute orbit * gPath
	//
	std::list<unsigned long> tempOrbit;
	typename std::list<unsigned long>::const_iterator orbIt = Transversal<PERM>::m_orbit.begin();
	for (; orbIt != Transversal<PERM>::m_orbit.end(); ++orbIt) {
		const unsigned long alpha = *gPath / *orbIt;
		//PERMLIB_DEBUG
		//std::cout << "g_i " << alpha << std::endl;
		if (!transversal[alpha]) {
			transversal[alpha] = gPath;
			tempOrbit.push_back(alpha);
		}
	}
	Transversal<PERM>::m_orbit.splice(Transversal<PERM>::m_orbit.end(), tempOrbit);

	m_cubeLabels.push_back(gPath);
	
	boost::shared_ptr<PERM> gPathInv(new PERM(*gPath));
	gPathInv->invertInplace();
	
	// compute inv(gPath) * ... other generators
	//

	unsigned long beta1 = *gPathInv / beta;
	if (!transversal[beta1]) {
		transversal[beta1] = gPathInv;
		Transversal<PERM>::m_orbit.push_back(beta1);
	}
	
	boost::dynamic_bitset<> omega(this->m_n);
	boost::dynamic_bitset<> todo(this->m_n);
	unsigned long i;
	omega[beta1] = 1;
	BOOST_FOREACH(const typename PERM::ptr& l, m_cubeLabels) {
		for (i = 0; i < this->m_n; ++i) {
			if (!omega[i])
				continue;
			unsigned long alpha = *l / i;
			todo[alpha] = 1;
			if (!transversal[alpha]) {
				transversal[alpha] = l;
				Transversal<PERM>::m_orbit.push_back(alpha);
			}
		}
		omega |= todo;
	}
	
	m_cubeLabels.push_front(gPathInv);
}

template <class PERM>
void ShallowSchreierTreeTransversal<PERM>::updateGenerators(const std::map<PERM*,typename PERM::ptr>& generatorChange) {
}

template <class PERM>
ShallowSchreierTreeTransversal<PERM> ShallowSchreierTreeTransversal<PERM>::clone(const std::map<PERM*,typename PERM::ptr>& generatorChange) const {
	ShallowSchreierTreeTransversal<PERM> ret(*this);
	std::map<PERM*,typename PERM::ptr> labelMap;
	BOOST_FOREACH(typename PERM::ptr& p, ret.m_cubeLabels) {
		PERM* gen = p.get();
		p = typename PERM::ptr(new PERM(*p));
		labelMap.insert(std::make_pair(gen, p));
	}
	ret.SchreierTreeTransversal<PERM>::updateGenerators(labelMap);
	return ret;
}

template <class PERM>
void ShallowSchreierTreeTransversal<PERM>::permute(const PERM& g, const PERM& gInv) {
	Transversal<PERM>::permute(g, gInv);
	BOOST_FOREACH(typename PERM::ptr& p, m_cubeLabels) {
		*p ^= gInv;
		*p *= g;
		p->flush();
	}
}

}

#endif // -- SHALLOWSCHREIERTREETRANSVERSAL_H_
