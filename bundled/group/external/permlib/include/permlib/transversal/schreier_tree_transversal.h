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


#ifndef SCHREIERTRANSVERSAL_H_
#define SCHREIERTRANSVERSAL_H_

#include <permlib/transversal/transversal.h>

namespace permlib {

namespace exports { struct BSGSSchreierExport; struct BSGSSchreierImport; }

/// Transversal class that stores transversal elements in a Schreier tree
template <class PERM>
class SchreierTreeTransversal : public Transversal<PERM> {
public:
	/// constructor
	SchreierTreeTransversal(unsigned int n);

    virtual bool trivialByDefinition(const PERM& x, unsigned long to) const;
    virtual PERM* at(unsigned long val) const;
	
	virtual void updateGenerators(const std::map<PERM*,typename PERM::ptr>& generatorChange);
	
	/// returns a clone of this transversal
	/**
	 * the group generators that the clone may use are given by the transition map
	 * @param generatorChange transition map
	 */
	SchreierTreeTransversal<PERM> clone(const std::map<PERM*,typename PERM::ptr>& generatorChange) const;
	
	/// maximal depth of tree structure representing the transversal
    mutable unsigned int m_statMaxDepth;
protected:
    virtual void registerMove(unsigned long from, unsigned long to, const typename PERM::ptr &p);
	
	friend struct permlib::exports::BSGSSchreierExport;
	friend struct permlib::exports::BSGSSchreierImport;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
SchreierTreeTransversal<PERM>::SchreierTreeTransversal(unsigned int n_) 
	: Transversal<PERM>(n_), m_statMaxDepth(0) 
{ }

template <class PERM>
bool SchreierTreeTransversal<PERM>::trivialByDefinition(const PERM& x, unsigned long to) const {
	return *Transversal<PERM>::m_transversal[to] == x; 
}

template <class PERM>
void SchreierTreeTransversal<PERM>::registerMove(unsigned long from, unsigned long to, const typename PERM::ptr &p) {
	Transversal<PERM>::registerMove(from, to, p);
	Transversal<PERM>::m_transversal[to] = p;
}


template <class PERM>
PERM* SchreierTreeTransversal<PERM>::at(unsigned long val) const {
	const std::vector<boost::shared_ptr<PERM> > &transversal = Transversal<PERM>::m_transversal;

	if (transversal[val] == 0) {
		return 0;
	}

	unsigned int depth = 1;
	PERM *res = new PERM(*transversal[val]);
	const PERM* inv = 0;
	//std::cout << "Schreier " << *res << std::endl;
	unsigned long pred = *res % val;
	//TODO: reserve space for PermutationWord-res beforehand (we know how long the m_word vector will be)
	while (pred != val) {
		inv = transversal[pred].get();
		BOOST_ASSERT(inv);
		PERMLIB_DEBUG(std::cout << "Schreier2 " << inv << " / " << val << " , " << pred << std::endl;)
		*res ^= *inv;
		val = pred;
		pred = *inv % pred;
		++depth;
	}
	m_statMaxDepth = std::max(m_statMaxDepth, depth);
	//std::cout << "Schreier3 " << *res << std::endl;
	return res;
}

template <class PERM>
void SchreierTreeTransversal<PERM>::updateGenerators(const std::map<PERM*,typename PERM::ptr>& generatorChange) {
	unsigned int missedCount = 0;
	BOOST_FOREACH(typename PERM::ptr& p, this->m_transversal) {
		if (!p)
			continue;
		//std::cout << "require " << p.get() << std::endl;
		typename std::map<PERM*,typename PERM::ptr>::const_iterator pIt = generatorChange.find(p.get());
		//BOOST_ASSERT( pIt != generatorChange.end() );
		if (pIt != generatorChange.end()) {
			p = (*pIt).second;
		} else {
			++missedCount;
			//std::cout << "missed " << p.get() << " = " << *p << std::endl;
		}
	}
	// we always miss the identity -- and not anything else
	BOOST_ASSERT( missedCount == 1 );
}

template <class PERM>
SchreierTreeTransversal<PERM> SchreierTreeTransversal<PERM>::clone(const std::map<PERM*,typename PERM::ptr>& generatorChange) const {
	SchreierTreeTransversal<PERM> ret(*this);
	ret.updateGenerators(generatorChange);
	return ret;
}

}

#endif // -- SCHREIERTRANSVERSAL_H_
