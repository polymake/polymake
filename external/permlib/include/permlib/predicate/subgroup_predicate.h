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


#ifndef SUBGROUPPREDICATE_H_
#define SUBGROUPPREDICATE_H_

#include <functional>

namespace permlib {

template <class PERM>
class RefinementFamily;

/// abstract base class for subgroup (and coset) predicates
template <class PERM>
class SubgroupPredicate : public std::unary_function<PERM, bool> {
public:
	/// virtual destructor
	virtual ~SubgroupPredicate() {}
	
	/// true iff group element fulfills predicate
	virtual bool operator()(const PERM &) const = 0;
	
	/// checks if a given group element should not be followed in backtrack search
	/**
	 * If returns false then element h that arises at backtrack level i with corresponding
	 * base element beta_i will not extend to a "complete" group element that fulfills the predicate
	 *
	 * @param h (partial) group element, arising in the backtrack search
	 * @param i backtrack recursion level
	 * @param beta_i base element corresponding to current backtrack recursion level
	 */
	virtual bool childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const = 0;
	
	/// limit of recursion depth in backtrack search
	/**
	 * only images of the #{limit()} first base points have to be considered
	 */
	virtual unsigned int limit() const = 0;
};

}

#endif // -- SUBGROUPPREDICATE_H_
