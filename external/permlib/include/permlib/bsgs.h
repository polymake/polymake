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


#ifndef BSGS_H_
#define BSGS_H_

#include <map>
#include <list>
#include <vector>

#include <boost/version.hpp>

#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#if BOOST_VERSION / 100 >= 1067
#include <boost/next_prior.hpp>
#else
#include <boost/utility.hpp>
#endif

#include <permlib/bsgs_core.h>

#include <permlib/transversal/orbit_set.h>
#include <permlib/transversal/transversal.h>
#include <permlib/predicate/pointwise_stabilizer_predicate.h>
#include <permlib/predicate/stabilizes_point_predicate.h>

#include <permlib/redundant_base_point_insertion_strategy.h>

namespace permlib {

template <class PERM, class TRANS>
struct BSGS;

template <class PERM, class TRANS>
std::ostream &operator<< (std::ostream &out, const BSGS<PERM,TRANS> &bsgs) {
	out << "BASE[" << bsgs.B.size() << "]" << std::endl;
	BOOST_FOREACH(unsigned long beta, bsgs.B) {
		out << static_cast<unsigned int>(beta+1) << ",";
	}
	out << std::endl;
	out << "SGS[" << bsgs.S.size() << "]" << std::endl;
	BOOST_FOREACH(const typename PERM::ptr &g, bsgs.S) {
		out << *g << ",";
	}
	out << std::endl;
	out << "U" << std::endl;
	BOOST_FOREACH(const TRANS &U, bsgs.U) {
		for (unsigned int i=0; i<bsgs.n; ++i)
			// trigger transversal depth reload
			boost::scoped_ptr<PERM> dummy(U.at(i));
		out << U.size() << "{" << U.m_statMaxDepth << "}" << ",";
	}
	out << " = " << bsgs.order() << std::endl;
	BOOST_FOREACH(const TRANS &U, bsgs.U) {
		out << U << std::endl;
	}
	return out;
}

/// Represents a base and strong generating set (BSGS)
template <class PERM, class TRANS>
struct BSGS : public BSGSCore<PERM,TRANS> {
	typedef typename BSGSCore<PERM,TRANS>::PERMlist PERMlist;
	
	/// constructs an empty group of degree n
	explicit BSGS(dom_int n);
	/// copy constructor
	/**
	 * creates a deep copy of generator list and transversals,
	 * so there is no link between the original BSGS and the copy
	 */
	BSGS(const BSGS<PERM,TRANS>& bsgs);
	
	/// assignment operator
	/**
	 * creates a deep copy of generator list and transversals,
	 * so there is no link between the original BSGS and the copy
	 */
	BSGS<PERM,TRANS>& operator=(const BSGS<PERM,TRANS>&);
    
	/// order of the group
	/**
	 * read from the transversal product
	 */
	template<typename Integer>
	Integer order() const;
	
	/// order of the group
	/**
	 * read from the transversal product
	 */
	boost::uint64_t order() const;
	
	/// sifts an element through the specified transversal range
	/**
	 * @param g permutation to sift
	 * @param siftee 
	 * @param j lowest transversal index to sift; i.e. sift through transversal U[j], U[j+1], ...
	 */
	unsigned int sift(const PERM& g, PERM& siftee, unsigned int j = 0) const;
	/// sifts an element through the specified transversal range
	/**
	 * @param g permutation to sift
	 * @param siftee 
	 * @param j lowest transversal index to sift
	 * @param k highest transversal index to sift plus one
	 */
	unsigned int sift(const PERM& g, PERM& siftee, unsigned int j, unsigned int k) const;
	/// true iff g sifts through transversal system
	bool sifts(const PERM& g) const;

	/// tries to find a new base element
	/**
	 * find an element which is moved by h
	 * @param h
	 * @param beta element moved by h
	 * @return true iff an element h could be found
	 */
	bool chooseBaseElement(const PERM &h, dom_int &beta) const;
	/// inserts a redundant base beta
	/**
	 * @param beta
	 * @param minPos insert point not before the minPos-th base element
	 * @return insertion position
	 */
	unsigned int insertRedundantBasePoint(unsigned int beta, unsigned int minPos = 0);
	/// strips redundant base points from the end to the minPos-th base element
	void stripRedundantBasePoints(int minPos = 0);
	
	/// removes redundant generators
	/**
	 * The remaining set S is still a strong generating set. Its size will be at most log |G|.
	 *
	 * Note that applying this method may result in a difference between transversals and 
	 * strong generating set. If you use a SchreierTree, then the transversal will no longer 
	 * automatically return elements from the strong generating set.
	 */
	void stripRedundantStrongGenerators();
	
	/// re-computes the j-th fundamental orbit with the given orbit generators
	/**
	 * @see Transversal<PERM>::orbit
	 */
	void orbit(unsigned int j, const PERMlist &generators);
	/// updates the j-th fundamental orbit with the given orbit generators and a new generator g
	/**
	 * @see Transversal<PERM>::orbitUpdate
	 */
	void orbitUpdate(unsigned int j, const PERMlist &generators, const typename PERM::ptr &g);
	
	/// adds a new group generator
	/**
	 * @param g group generator
	 * @param updateOrbit true iff transversals/orbits should be updates
	 * @return index up to which transversals/orbits need update
	 */
	int insertGenerator(const typename PERM::ptr& g, bool updateOrbit);
	/// updates transversals/orbits
	/**
	 * @param pos index up to which transversals should be updated
	 * @see insertGenerator
	 */
	void updateOrbits(int pos);
	
	/// generates a uniformly distributed random element of \f$G^{[i]}\f$
	/**
	 * @param i the stabilizer chain index to generate the random element of. If set to 0 a random element of the whole group is returned.
	 */
	PERM random(const int i = 0) const;

	/// conjugate group with a permutation
	/**
	 * If S is the generating set of this group, then after conjugation the group will be generated by c^{-1} S c.
	 * @param g permutation the group should be conjugated by
	 */
	void conjugate(const PERM& g);
	
	/// writes base, SGS and transversals
	friend std::ostream &operator<< <> (std::ostream &out, const BSGS<PERM,TRANS> &bsgs);
private:
	/// sifts an element through the specified transversal range
	template <class BaseIterator, class TransversalIterator>
	unsigned int sift(const PERM& g, PERM& siftee, BaseIterator begin, BaseIterator end, TransversalIterator beginT, TransversalIterator endT) const;

	/// id counter of all BSGS instances
	static int ms_bsgsId;
	
	/// deep-copy transversals and group generators
	void copyTransversals(const BSGS<PERM,TRANS>& bsgs);
};

//
//     ----       IMPLEMENTATION
//

template <class PERM, class TRANS>
int BSGS<PERM,TRANS>::ms_bsgsId = 0;

template <class PERM, class TRANS>
BSGS<PERM,TRANS>::BSGS(dom_int n_) 
	: BSGSCore<PERM,TRANS>(++ms_bsgsId, n_, 0)
{}

template <class PERM, class TRANS>
BSGS<PERM,TRANS>::BSGS(const BSGS<PERM,TRANS>& bsgs)
	: BSGSCore<PERM,TRANS>(bsgs.m_id, bsgs.B, bsgs.U, bsgs.n)
{ 
	copyTransversals(bsgs);
}

template <class PERM, class TRANS>
BSGS<PERM,TRANS>& BSGS<PERM,TRANS>::operator=(const BSGS<PERM,TRANS>& bsgs) {
	if (this == &bsgs)
		return *this;
	
	this->B = bsgs.B;
	this->n = bsgs.n;
	this->m_id = bsgs.m_id;
	
	copyTransversals(bsgs);
	return *this;
}

template <class PERM, class TRANS>
template <class BaseIterator, class TransversalIterator>
unsigned int BSGS<PERM, TRANS>::sift(const PERM& g, PERM& siftee, BaseIterator begin, BaseIterator end, TransversalIterator beginT, TransversalIterator endT) const{
	unsigned int k = 0;
	siftee = g;
	BaseIterator baseIt;
	TransversalIterator transIt;
	for (baseIt = begin, transIt = beginT; baseIt != end && transIt != endT; ++baseIt, ++transIt) {
		unsigned long b = *baseIt;
		const TRANS& U_i = *transIt;
		//std::cout << " ~~~ sift " << siftee << " b" << b << std::endl;
		boost::scoped_ptr<PERM> u_b(U_i.at(siftee / b));
		if (u_b == 0)
			return k;
		u_b->invertInplace();
		siftee *= *u_b;
		++k;
	}
	return k;
}

template <class PERM, class TRANS>
unsigned int BSGS<PERM, TRANS>::sift(const PERM& g, PERM& siftee, unsigned int j) const {
	return sift(g, siftee, this->B.begin() + j, this->B.end(), this->U.begin() + j, this->U.end());
}

template <class PERM, class TRANS>
unsigned int BSGS<PERM, TRANS>::sift(const PERM& g, PERM& siftee, unsigned int j, unsigned int k) const {
	return sift(g, siftee, this->B.begin() + j, this->B.begin() + k, this->U.begin() + j, this->U.begin() + k);
}

template <class PERM, class TRANS>
bool BSGS<PERM, TRANS>::sifts(const PERM& g) const {
	PERM siftee(this->n);
	unsigned int m = sift(g, siftee);
	return this->B.size() == m && siftee.isIdentity();
}

template <class PERM, class TRANS>
bool BSGS<PERM, TRANS>::chooseBaseElement(const PERM &h, dom_int &beta) const {
	for (beta = 0; beta < this->n; ++beta) {
		if (std::find(this->B.begin(), this->B.end(), beta) != this->B.end())
			continue;
		if (h / beta != beta)
			return true;
	}
	return false;
}

template <class PERM, class TRANS>
void BSGS<PERM, TRANS>::orbit(unsigned int j, const PERMlist &generators) {
	this->U[j].orbit(this->B[j], generators);
}
	
template <class PERM, class TRANS>
void BSGS<PERM, TRANS>::orbitUpdate(unsigned int j, const PERMlist &generators, const typename PERM::ptr &g) {
	this->U[j].orbitUpdate(this->B[j], generators, g);
}

template <class PERM, class TRANS>
PERM BSGS<PERM, TRANS>::random(const int i) const {
	BOOST_ASSERT( i >= 0 );
    PERM g(this->n);
    for (int l = this->U.size()-1; l>=i ; --l) {
		//std::cout << l << " : " << U[l] << " : " << U[l].size() << std::endl;
        unsigned long beta = *(boost::next(this->U[l].begin(), randomInt(this->U[l].size())));
        boost::scoped_ptr<PERM> u_beta(this->U[l].at(beta));
        g *= *u_beta;
    }
    return g;
}

template <class PERM, class TRANS>
void BSGS<PERM, TRANS>::conjugate(const PERM& g) {
	PERM gInv(g);
	gInv.invertInplace();

	//
	// to conjugate a BSGS, all three components (B,S,U) have to be adjusted
	//

	// STEP 1: conjugate generating set S
	BOOST_FOREACH(typename PERM::ptr& p, this->S) {
		*p ^= gInv;
		*p *= g;
	}

	std::vector<dom_int> oldB(this->B);
	for (unsigned int i = 0; i < this->U.size(); ++i) {
		// STEP 2: adapt base B
		this->B[i] = g / oldB[i];
		// STEP 3: conjugate transversal U
		this->U[i].permute(g, gInv);
	}
}

template <class PERM, class TRANS>
int BSGS<PERM, TRANS>::insertGenerator(const typename PERM::ptr& g, bool updateOrbit) {
	int pos = 0;
	for (; static_cast<unsigned int>(pos) < this->B.size(); ++pos) {
		if (*g / this->B[pos] != this->B[pos])
			break;
	}
	
	if (static_cast<unsigned int>(pos) == this->B.size()) {
		dom_int beta;
		bool newBaseElement __attribute__((unused)) = chooseBaseElement(*g, beta);
		BOOST_ASSERT( newBaseElement );
		this->B.push_back(beta);
		this->U.push_back(TRANS(this->n));
	}
	
	const int insertionPosition = pos;
	this->S.push_back(g);
	
	if (updateOrbit) {
		bool groupOrderChanged = false;
		for (; pos >= 0; --pos) {
			PERMlist orbitGenerators;
			const unsigned int oldTransversalSize = this->U[pos].size();
			//std::cout << "INSERT orbit @ " << pos << " : " << g << std::endl;
			std::copy_if(this->S.begin(), this->S.end(), std::back_inserter(orbitGenerators), 
					PointwiseStabilizerPredicate<PERM>(this->B.begin(), this->B.begin() + pos));
			if (orbitGenerators.size() > 0) {
				orbitUpdate(pos, orbitGenerators, g);
				
				// group order can only increase by adding generators
				if (this->U[pos].size() > oldTransversalSize)
					groupOrderChanged = true;
			}
		}
		
		if (!groupOrderChanged) {
			this->S.pop_back();
			return -1;
		}
	}
	
	return insertionPosition;
}

template <class PERM, class TRANS>
void BSGS<PERM, TRANS>::updateOrbits(int pos) {
	if (pos < 0)
		return;
	for (; pos >= 0; --pos) {
		PERMlist orbitGenerators;
		std::copy_if(this->S.begin(), this->S.end(), std::back_inserter(orbitGenerators), 
					PointwiseStabilizerPredicate<PERM>(this->B.begin(), this->B.begin() + pos));
		if (orbitGenerators.size() > 0)
			orbit(pos, orbitGenerators);
	}
}

template <class PERM, class TRANS>
template <typename Integer>
Integer BSGS<PERM, TRANS>::order() const {
	Integer orderValue(1);
	BOOST_FOREACH(const TRANS &Ui, this->U) {
		orderValue *= Ui.size();
	}
	return orderValue;
}

template <class PERM, class TRANS>
boost::uint64_t BSGS<PERM, TRANS>::order() const {
	return order<boost::uint64_t>();
}


template <class PERM, class TRANS>
unsigned int BSGS<PERM, TRANS>::insertRedundantBasePoint(unsigned int beta, unsigned int minPos) {
	PERMlist S_i;
	TrivialRedundantBasePointInsertionStrategy<PERM,TRANS> is(*this);
	int pos = is.findInsertionPoint(beta, S_i);
	if (pos < 0)
		return -(pos+1);
    pos = std::max(static_cast<unsigned int>(pos), minPos);
	
	this->B.insert(this->B.begin() + pos, beta);
	this->U.insert(this->U.begin() + pos, TRANS(this->n));
	this->U[pos].orbit(beta, S_i);
	return pos;
}

template <class PERM, class TRANS>
void BSGS<PERM, TRANS>::stripRedundantBasePoints(int minPos) {
	for (int i = this->B.size()-1; i >= minPos; --i) {
		if (this->U[i].size() <= 1) {
			if (i == static_cast<int>(this->B.size()-1)) {
				this->B.pop_back();
				this->U.pop_back();
			} else {
				this->B.erase(this->B.begin() + i);
				this->U.erase(this->U.begin() + i);
			}
		}
	}
}


/// Class that can be used to sort a strong generating set.
/**
 * The goal is to sort a list of strong generators so that generators for the stabilizers in the
 * stabilizer chain are next to each other.
 */
template <class PERM>
class StrongGeneratingSetSorter : public std::binary_function<typename PERM::ptr, typename PERM::ptr, bool> {
public:
	/**
	 * @param baseBegin begin-iterator(dom_int) to the base relative to which the strong generating set is to be sorted
	 * @param baseEnd   end-iterator(dom_int) to the base relative to which the strong generating set is to be sorted
	 */
	template<class InputIterator>
	StrongGeneratingSetSorter(InputIterator baseBegin, InputIterator baseEnd) : m_base(baseBegin, baseEnd) { }
	
	/// true iff p1 stabilizes more base points (in increasing order) than p2
	bool operator()(const typename PERM::ptr& p1, const typename PERM::ptr& p2) const {
		BOOST_FOREACH(const dom_int b, m_base) {
			if ( p1->at(b) == b  &&  p2->at(b) != b )
				return true;
			if ( p1->at(b) != b )
				return false;
		}
		return false;
	}
private:
	std::vector<dom_int> m_base;
};

template <class PERM, class TRANS>
void BSGS<PERM, TRANS>::stripRedundantStrongGenerators() {
	PERMlist sortedSGS(this->S);
	sortedSGS.sort(StrongGeneratingSetSorter<PERM>(this->B.begin(), this->B.end()));
	
	PERMlist filteredSGS;
	OrbitSet<PERM, dom_int>* oldOrbit = new OrbitSet<PERM, dom_int>();
	dom_int oldBaseElement = static_cast<dom_int>(-1);
	BOOST_FOREACH(const typename PERM::ptr& gen, sortedSGS) {
		if (gen->isIdentity())
			continue;
		filteredSGS.push_back(gen);
	
		// Compute to which base element this strong generator belongs.
		// That is, gen stabilizes all base points up to baseElement.
		// No generator can stabilize all base elements because we excluded
		//  identity permutations before.
		dom_int baseElement = this->B.front();
		BOOST_FOREACH(const dom_int b, this->B) {
			baseElement = b;
			if (*gen / b != b)
				break;
		}
		PERMLIB_DEBUG(std::cout << "gen " << *gen << "   @ " << baseElement << std::endl;)

		OrbitSet<PERM, dom_int>* newOrbit = new OrbitSet<PERM, dom_int>();
		newOrbit->orbit(baseElement, filteredSGS, typename Transversal<PERM>::TrivialAction());
		if (oldBaseElement == baseElement && newOrbit->size() == oldOrbit->size()) {
			delete newOrbit;
			PERMLIB_DEBUG(std::cout << "  removed\n";)
			filteredSGS.pop_back();
		} else {
			delete oldOrbit;
			oldOrbit = newOrbit;
		}
		oldBaseElement = baseElement;
	}
	delete oldOrbit;
	
	this->S = filteredSGS;
}

template <class PERM, class TRANS>
void BSGS<PERM,TRANS>::copyTransversals(const BSGS<PERM,TRANS>& bsgs) {
	std::map<PERM*,typename PERM::ptr> genMap;
	BOOST_FOREACH(const typename PERM::ptr& p, bsgs.S) {
		typename PERM::ptr deepcopy(new PERM(*p));
		//std::cout << "found " << p.get() << " = " << *p << std::endl;
		genMap.insert(std::make_pair(p.get(), deepcopy));
		this->S.push_back(deepcopy);
	}
	
	BOOST_ASSERT(this->B.size() == bsgs.B.size());
	BOOST_ASSERT(bsgs.B.size() == bsgs.U.size());
	this->U.clear();
	this->U.resize(bsgs.U.size(), TRANS(bsgs.n));
	BOOST_ASSERT(this->U.size() == bsgs.U.size());
	
	for (unsigned int i=0; i<this->U.size(); ++i) {
		this->U[i] = bsgs.U[i].clone(genMap);
	}
}

}

#endif // -- BSGS_H_
