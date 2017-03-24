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


#ifndef PRODUCTREPLACEMENTGENERATOR_H
#define PRODUCTREPLACEMENTGENERATOR_H

#include <permlib/generator/random_generator.h>

#include <vector>
#include <boost/iterator/indirect_iterator.hpp>

namespace permlib {

/// generates nearly-uniformly distributed random group elements using the product replacement algorithm
template <class PERM>
class ProductReplacementGenerator : public RandomGenerator<PERM> {
public:
	/// initializes class with group generators
	/**
	 * @param generatorsBegin begin iterator of PERM
	 * @param generatorsEnd   end   iterator of PERM
	 */
	template <class InputIterator>
	ProductReplacementGenerator(const unsigned int n, InputIterator generatorsBegin, InputIterator generatorsEnd);
	
	virtual PERM next();
private:
	std::vector<PERM> m_generators;
	const unsigned int m_n;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
template <class InputIterator>
ProductReplacementGenerator<PERM>::ProductReplacementGenerator(const unsigned int n, InputIterator generatorsBegin, InputIterator generatorsEnd) 
	: m_generators(boost::indirect_iterator<InputIterator, PERM>(generatorsBegin), 
				   boost::indirect_iterator<InputIterator, PERM>(generatorsEnd)),
		m_n(n)
{
	const unsigned int additionalElements = 10;
	const unsigned int oldSize = m_generators.size();
	const unsigned int replacementRounds = std::max(oldSize * 10, static_cast<unsigned int>(100));
	if (oldSize == 0)
		return;
	
	m_generators.reserve(oldSize + additionalElements + 1);
	for (unsigned int i = 0; i < additionalElements; ++i) {
		m_generators.push_back(m_generators[randomInt(oldSize)]);
	}
	m_generators.push_back(PERM(m_generators[0].size()));
	
	for (unsigned int k = 0; k < replacementRounds; ++k) {
		next();
	}
}

template <class PERM>
PERM ProductReplacementGenerator<PERM>::next() {
	if (m_generators.size() == 0) {
		return PERM(m_n);
	}
	
	unsigned int i = randomInt(m_generators.size() - 1);
	unsigned int j = randomInt(m_generators.size() - 2);
	if (j >= i) ++j;
	switch (randomInt(4)) {
	case 0:
		m_generators[i] *=  m_generators[j]; break;
	case 1:
		m_generators[i] *= ~m_generators[j]; break;
	case 2:
		m_generators[i] ^=  m_generators[j]; break;
	case 3:
		m_generators[i] ^= ~m_generators[j]; break;
	}
	m_generators[m_generators.size()-1] *= m_generators[i];
	return m_generators[m_generators.size()-1];
}

}

#endif // -- PRODUCTREPLACEMENTGENERATOR_H
