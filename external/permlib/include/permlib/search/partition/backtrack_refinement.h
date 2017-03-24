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


#ifndef BACKTRACKREFINEMENT_H_
#define BACKTRACKREFINEMENT_H_

#include <permlib/search/partition/refinement.h>

#include <list>

namespace permlib {
namespace partition {
	
/// backtrack refinement
template<class PERM>
class BacktrackRefinement : public Refinement<PERM> {
public:
	/// constructor
	explicit BacktrackRefinement(unsigned long n);
	/// constructor
	/**
	 * @param n
	 * @param alpha prefered alpha to choose for backtracking
	 */
	BacktrackRefinement(unsigned long n, unsigned long alpha);
	
	virtual unsigned int apply(Partition& pi) const;
	
	/// alpha point chosen for backtracking
	unsigned long alpha() const;
	virtual void sort(const BaseSorterByReference& sorter, const Partition* pi);
protected:
	virtual bool init(Partition& pi);
private:
	unsigned long m_alpha;
	unsigned int m_cellElementIndex;
	unsigned int m_cellIndex;
	
	typedef typename Refinement<PERM>::RefinementPtr RefinementPtr;
	
	struct RefinementSorter : public std::binary_function<RefinementPtr, RefinementPtr, bool> {
		RefinementSorter(const BaseSorterByReference& sorter, const Partition* pi) : m_sorter(sorter), m_pi(pi) {}
		
		bool operator()(RefinementPtr a, RefinementPtr b) const {
			BacktrackRefinement<PERM>* backtrackA = static_cast<BacktrackRefinement<PERM>*>(a.get());
			BacktrackRefinement<PERM>* backtrackB = static_cast<BacktrackRefinement<PERM>*>(b.get());
			if (m_pi) {
				return m_sorter(m_pi->partition[backtrackA->m_cellElementIndex], m_pi->partition[backtrackB->m_cellElementIndex]);
			}
			return m_sorter(backtrackA->m_alpha, backtrackB->m_alpha);
		}
	private:
		const BaseSorterByReference& m_sorter;
		const Partition* m_pi;
	};
	
	static const unsigned int overrideAlphaChoiceCellSizeRatio = 8;
};

template<class PERM>
BacktrackRefinement<PERM>::BacktrackRefinement(unsigned long n) 
	: Refinement<PERM>(n, Backtrack), m_alpha(-1), m_cellElementIndex(-1), m_cellIndex(-1)
{ }

template<class PERM>
BacktrackRefinement<PERM>::BacktrackRefinement(unsigned long n, unsigned long alpha_) 
	: Refinement<PERM>(n, Backtrack), m_alpha(alpha_), m_cellElementIndex(-1), m_cellIndex(-1)
{ }

template<class PERM>
unsigned int BacktrackRefinement<PERM>::apply(Partition& pi) const {
	unsigned long singleCell[1];
	singleCell[0] = pi.partition[m_cellElementIndex];  
	//singleCell[0] = t / m_alpha;
	//print_iterable(pi.partition.begin(), pi.partition.end(), 0, " partition pi");
	PERMLIB_DEBUG(std::cout << " apply bt ref   alpha =" << m_alpha << ", single cell = " << singleCell[0] << " @ " << m_cellIndex << "," << m_cellElementIndex << std::endl;)
	return pi.intersect(singleCell, singleCell+1, m_cellIndex);
}

template<class PERM>
unsigned long BacktrackRefinement<PERM>::alpha() const {
	return m_alpha;
}

template<class PERM>
void BacktrackRefinement<PERM>::sort(const BaseSorterByReference& sorter, const Partition* pi) {
	std::sort(Refinement<PERM>::m_backtrackRefinements.begin(), Refinement<PERM>::m_backtrackRefinements.end(), RefinementSorter(sorter, pi));
}

template<class PERM>
bool BacktrackRefinement<PERM>::init(Partition& pi) {
	unsigned int minCellSize = pi.partition.size();
	unsigned int minCell = 0;
	//std::cout << "m_alpha " << m_alpha << std::endl;
	
	std::vector<unsigned int>::const_iterator length = pi.partitionCellLength.begin();
	for (unsigned int j = 0; j < pi.cellCounter; ++j) {
		if (1 < *length && *length < minCellSize) {
			minCellSize = *length;
			minCell = j;
		}
		++length;
	}
	if (m_alpha == static_cast<unsigned long>(-1)) {
		this->m_cellElementIndex = pi.partitionCellBorder[minCell];
		this->m_alpha = pi.partition[pi.partitionCellBorder[minCell]];
	} else {
		const unsigned int givenMinCell = pi.partitionCellOf[m_alpha];
		const unsigned int givenMinCellSize = pi.partitionCellLength[givenMinCell];
		if (1 < givenMinCellSize && givenMinCellSize <= overrideAlphaChoiceCellSizeRatio * minCellSize) {
			minCell = givenMinCell;
			minCellSize = givenMinCellSize;
			for (unsigned int j = pi.partitionCellBorder[minCell]; j < pi.partitionCellBorder[minCell] + pi.partitionCellLength[minCell]; ++j) {
				if (pi.partition[j] == m_alpha) {
					this->m_cellElementIndex = j;
					break;
				}
			}
		} else {
			this->m_cellElementIndex = pi.partitionCellBorder[minCell];
			this->m_alpha = pi.partition[pi.partitionCellBorder[minCell]];
		}
	}
	PERMLIB_DEBUG(std::cout << "minCellSize = " << minCellSize << std::endl;)

	this->m_cellIndex = minCell;
		
	Refinement<PERM>::m_backtrackRefinements.reserve(minCellSize);
	for (unsigned int i = pi.partitionCellBorder[minCell]; i < pi.partitionCellBorder[minCell] + minCellSize; ++i) {
		BacktrackRefinement<PERM>* br = new BacktrackRefinement<PERM>(Refinement<PERM>::m_n);
		br->m_cellElementIndex = i;
		br->m_cellIndex = minCell;
		br->m_alpha = pi.partition[i];
		//print_iterable(pi.partition.begin(), pi.partition.end(), 0, " partition pi");
		PERMLIB_DEBUG(std::cout << "PREP bt alpha " << br->m_alpha << " @ " << br->m_cellIndex << " // " << br->m_cellElementIndex << std::endl;)
		typename Refinement<PERM>::RefinementPtr ref(br);
		Refinement<PERM>::m_backtrackRefinements.push_back(ref);
	}
	
	unsigned long singleCell[1];
	singleCell[0] = this->m_alpha;
	//TODO: write special case function to handle singleCell intersection
	const bool inter __attribute__((unused)) = pi.intersect(singleCell, singleCell+1, m_cellIndex);
	BOOST_ASSERT(inter);
	
	return true;
}

}
}

#endif // -- BACKTRACKREFINEMENT_H_
