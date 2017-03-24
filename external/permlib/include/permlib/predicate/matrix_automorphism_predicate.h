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


#ifndef MATRIXAUTOMORPHISMPREDICATE_H_
#define MATRIXAUTOMORPHISMPREDICATE_H_

#include <permlib/predicate/subgroup_predicate.h>
#include <permlib/search/partition/refinement_family.h>

#include <boost/foreach.hpp>

namespace permlib {

/// predicate for the automorphisms of a symmetric matrix
template <class PERM,class MATRIX>
class MatrixAutomorphismPredicate : public SubgroupPredicate<PERM> {
public:
	/// constructor
	MatrixAutomorphismPredicate(const MATRIX& matrix);

	virtual bool operator()(const PERM &p) const;
	virtual bool childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const;
	virtual unsigned int limit() const;
private:
	const MATRIX& m_matrix;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM,class MATRIX>
MatrixAutomorphismPredicate<PERM,MATRIX>::MatrixAutomorphismPredicate(const MATRIX& matrix) 
	: m_matrix(matrix)
{ }

template <class PERM,class MATRIX>
bool MatrixAutomorphismPredicate<PERM,MATRIX>::operator()(const PERM &p) const {
	const unsigned long n = m_matrix.dimension();
	for (unsigned long i = 0; i < n; ++i) {
		for (unsigned long j = i; j < n; ++j) {
			if (m_matrix.at(i, j) != m_matrix.at(p / i, p / j))
				return false;
		}
	}
	return true;
}

template <class PERM,class MATRIX>
bool MatrixAutomorphismPredicate<PERM,MATRIX>::childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const {
	// we can't easily restrict child nodes in general, so we don't restrict at all
	return true;
}

template <class PERM,class MATRIX>
unsigned int MatrixAutomorphismPredicate<PERM,MATRIX>::limit() const {
	// we can't easily limit the search depth, so return maximal depth
	return m_matrix.dimension();
}

}

#endif // -- MATRIXAUTOMORPHISMPREDICATE_H_
