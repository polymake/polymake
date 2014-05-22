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


#ifndef EXPLICITTRANSVERSAL_H_
#define EXPLICITTRANSVERSAL_H_

#include <permlib/transversal/transversal.h>

namespace permlib {

/// Transversal class that stores all transversal elements explicitly
template <class PERM>
class ExplicitTransversal : public Transversal<PERM> {
public:
	/// constructor
    ExplicitTransversal(unsigned int n);

    virtual PERM* at(unsigned long val) const;
    virtual bool trivialByDefinition(const PERM& x, unsigned long to) const;
	
	virtual void permute(const PERM& g, const PERM& gInv);
	
	/// returns a clone of this transversal
	/**
	 * @param generatorChange not used by this implementation; just for API consistence
	 */
	ExplicitTransversal<PERM> clone(const std::map<PERM*,typename PERM::ptr>& generatorChange) const;

	/// maximal depth of "tree" structure representing the transversal; identical to 1 for explicit transversal
    static const unsigned int m_statMaxDepth;
protected:
    virtual void registerMove(unsigned long from, unsigned long to, const typename PERM::ptr &p);
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
const unsigned int ExplicitTransversal<PERM>::m_statMaxDepth = 1;

template <class PERM>
ExplicitTransversal<PERM>::ExplicitTransversal(unsigned int n_) 
	: Transversal<PERM>(n_) 
{ }

template <class PERM>
bool ExplicitTransversal<PERM>::trivialByDefinition(const PERM& x, unsigned long to) const {
	// we cannot infer this information from the transversal
	return false;
}

template <class PERM>
PERM* ExplicitTransversal<PERM>::at(unsigned long val) const {
	if (!Transversal<PERM>::m_transversal[val])
		return 0;
	return new PERM(*(Transversal<PERM>::m_transversal[val]));
}

template <class PERM>
void ExplicitTransversal<PERM>::registerMove(unsigned long from, unsigned long to, const typename PERM::ptr &p) {
	Transversal<PERM>::registerMove(from, to, p);

	std::vector<boost::shared_ptr<PERM> > &transversal = Transversal<PERM>::m_transversal;

    if (!transversal[from])
        transversal[to] = boost::shared_ptr<PERM>(new PERM(*p));
    else {
        transversal[to] = boost::shared_ptr<PERM>(new PERM(*transversal[from]));
        (*transversal[to]) *= *p;
    }
}

template <class PERM>
void ExplicitTransversal<PERM>::permute(const PERM& g, const PERM& gInv) {
	Transversal<PERM>::permute(g, gInv);
	BOOST_FOREACH(typename PERM::ptr& p, Transversal<PERM>::m_transversal) {
		if (p) {
			*p ^= gInv;
			*p *= g;
		}
	}
}

template <class PERM>
ExplicitTransversal<PERM> ExplicitTransversal<PERM>::clone(const std::map<PERM*,typename PERM::ptr>& generatorChange) const {
	ExplicitTransversal<PERM> ret(*this);
	BOOST_FOREACH(typename PERM::ptr& p, ret.m_transversal) {
		if (!p)
			continue;
		p = boost::shared_ptr<PERM>(new PERM(*p));
	}
	return ret;
}

}

#endif // -- EXPLICITTRANSVERSAL_H_
