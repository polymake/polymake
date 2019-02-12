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


#ifndef PARTITION_H_
#define PARTITION_H_

#include <algorithm>
#include <list>
#include <boost/foreach.hpp>
#include <boost/dynamic_bitset.hpp>

namespace permlib {
namespace partition {
	
template<class T>
class BacktrackRefinement;

/// partition
class Partition {
public:
	/// constructs an empty partition of length n
	explicit Partition(unsigned long n);
	
	/// intersects the j-th cell of this partition with a given set
	/**
	 * @see intersects
	 * @param begin begin iterator(unsigned int) to a sorted list of elements 
	 * @param end end iterator(unsigned int) to a sorted list of elements 
	 * @param j 
	 * @return true if intersection really splits the j-th cell
	 */
	template<class ForwardIterator>
	bool intersect(ForwardIterator begin, ForwardIterator end, unsigned int j);
	/// reverts the last intersection if there is one
	bool undoIntersection();

	/// returns true iff given set actually intersects j-th cell of this partition
	/// @see intersect
	template<class ForwardIterator>
	bool intersects(ForwardIterator begin, ForwardIterator end, unsigned int j) const;
	
	typedef std::vector<unsigned int> vector_t;
	/// number of fix points in this partition
	unsigned int fixPointsSize() const;
	/// iterator to the begin of fix points
	vector_t::const_iterator fixPointsBegin() const;
	/// iterator to the end of fix points
	vector_t::const_iterator fixPointsEnd() const;
	/// number of cells in this partition
	unsigned long cells() const;
	/// size of the c-th cell
	unsigned long cellSize(unsigned int c) const;
	
	typedef vector_t::const_iterator CellIt;
	
	CellIt cellBegin(unsigned long cell) const { 
		BOOST_ASSERT(cell < cells());
		return partition.begin() + partitionCellBorder[cell];
	}
	
	CellIt cellEnd(unsigned long cell) const { 
		BOOST_ASSERT(cell < cells());
		return partition.begin() + partitionCellBorder[cell] + partitionCellLength[cell];
	}
private:
	explicit Partition(unsigned long n, bool);
	
	vector_t partition;
	vector_t partitionCellBorder;
	vector_t partitionCellLength;
	vector_t partitionCellOf;
	/// pre-allocated memory for cell intersection
	vector_t m_newCell;
	
	/// index of last cell
	unsigned int cellCounter;
	
	/// fix points; to avoid frequent allocation, space is preallocated
	/// @see fixCounter
	vector_t fix;
	/// index up to which entries in fix-array are valid
	unsigned int fixCounter;
	
	friend std::ostream& operator<<(std::ostream& out, const Partition& p);
	
	template<class T>
	friend class BacktrackRefinement;
};

inline std::ostream& operator<<(std::ostream& out, const Partition& p) {
	out << "[";
	Partition::vector_t::const_iterator border = p.partitionCellBorder.begin();
	Partition::vector_t::const_iterator length = p.partitionCellLength.begin();
	for (unsigned int j = 0; j < p.cellCounter; ++j) {
		for (unsigned int i = *border; i < *border + *length; ++i) {
			out << (p.partition[i] + 1) << " ";
		}
		out << "| ";
		++border;
		++length;
	}	
	out << "]|(";
	int countFix = p.fixCounter;
	BOOST_FOREACH(unsigned long alpha, p.fix) {
		if (--countFix < 0)
			break;
		out << (alpha+1) << ",";
	}
	out << ")";
	return out;
}

inline Partition::Partition(unsigned long n) 
	: partition(n), partitionCellBorder(n), partitionCellLength(n), partitionCellOf(n), m_newCell(n), cellCounter(1), fix(n), fixCounter(0)
{
	for (unsigned int i=0; i<n; ++i) {
		partition[i] = i;
		// partitionCellOf is already zero
	}
	partitionCellBorder[0] = 0;
	partitionCellLength[0] = n;
}

inline Partition::Partition(unsigned long n, bool) 
	: partition(n), partitionCellBorder(n), partitionCellLength(n), partitionCellOf(n), m_newCell(n), cellCounter(0), fix(n), fixCounter(0)
{ }

inline unsigned long Partition::cells() const {
	return cellCounter;
}

inline unsigned int Partition::fixPointsSize() const {
	return fixCounter;
}
inline Partition::vector_t::const_iterator Partition::fixPointsBegin() const {
	return fix.begin();
}
inline Partition::vector_t::const_iterator Partition::fixPointsEnd() const {
	return fix.begin() + fixCounter;
}
inline unsigned long Partition::cellSize(unsigned int c) const {
	return partitionCellLength[c]; 
}

template<class ForwardIterator>
inline bool Partition::intersects(ForwardIterator begin, ForwardIterator end, unsigned int j) const {
	while (begin != end) {
		//std::cout << " B " << *begin << " < " << partitionCellOf[*begin] << " < " << j << std::endl;
		if (partitionCellOf[*begin++] == j)
			return true;
	}
	return false;
}

/// ASSUME INPUT IS SORTED!!!
template<class ForwardIterator>
inline bool Partition::intersect(ForwardIterator otherCellBegin, ForwardIterator otherCellEnd, unsigned int j) {
	if (!intersects(otherCellBegin, otherCellEnd, j))
		return false;
	
	vector_t& newCell = m_newCell;
	
	ForwardIterator otherCellIt = otherCellBegin;
	vector_t::iterator cellIt;
	vector_t::reverse_iterator newCellIt;
	vector_t::reverse_iterator newCellBeginIt;
	vector_t::iterator newCell2It;
	bool createdNewCell = false;
	const unsigned int partitionCellSize = partitionCellLength[j];
	if (j >= cellCounter)
		return false;
	if (partitionCellSize <= 1)
		return false;
	vector_t::iterator cellBeginIt = partition.begin() + partitionCellBorder[j];
	vector_t::iterator cellEndIt   = partition.begin() + (partitionCellBorder[j] + partitionCellLength[j]);
	//print_iterable(cellBeginIt, cellEndIt, 1, " ^ cell");
	newCellBeginIt  = newCell.rbegin() + (partition.size() - partitionCellSize);
	newCellIt       = newCellBeginIt;
	newCell2It      = newCell.begin();
	unsigned int newCellCounter = 0;
	
	for (cellIt = cellBeginIt; cellIt != cellEndIt; ++cellIt) {
		while (otherCellIt != otherCellEnd && *otherCellIt < *cellIt) {
			++otherCellIt;
		}
		if (otherCellIt != otherCellEnd && *cellIt == *otherCellIt) {
			*newCell2It = *cellIt;
			++newCell2It;
			if (newCellCounter == 0) {
				/*std::cout << "copy into new cell ";
				print_iterable(partition.begin() + borderLo, cellIt, 1);
				std::cout << std::endl;*/
				newCellIt = std::copy(cellBeginIt, cellIt, newCellIt);
			}
			++newCellCounter;
		} else if (newCellCounter) {
			*newCellIt = *cellIt;
			++newCellIt;
		}
	}
	
	if (newCellCounter > 0 && newCellCounter < partitionCellSize) {
		std::reverse(newCellBeginIt, newCellIt);
		std::copy(newCell.begin(), newCell.begin() + partitionCellSize, cellBeginIt);
		/*std::cout << "new cell[" << partitionCellSize << "] = ";
		print_iterable(newCell.begin(), newCell.begin() + partitionCellSize, 1);
		std::cout << std::endl;*/
		vector_t::iterator fixIt = fix.begin() + fixCounter;
		
		if (newCellCounter == 1) {
			*fixIt = newCell[0];
			++fixIt;
			++fixCounter;
		}
		if (newCellCounter == partitionCellSize - 1) {
			*fixIt = newCell[partitionCellSize - 1];
			++fixIt;
			++fixCounter;
		}
		
		/*
		for (unsigned int i = partitionCellBorder[j]; i < partitionCellBorder[j] + partitionCellLength[j]; ++i) {
			std::cout << partition[i]+1 << " ";
		}
		std::cout << std::endl;
		std::cout << "new cell counter " << newCellCounter << std::endl;
		*/
		
		partitionCellLength[j] = newCellCounter;
		
		//std::cout << "cellCounter " << cellCounter << std::endl;
		partitionCellBorder[cellCounter] = partitionCellBorder[j] + newCellCounter;
		partitionCellLength[cellCounter] = partitionCellSize - newCellCounter;
		for (unsigned int i = partitionCellBorder[cellCounter]; i < partitionCellBorder[j] + partitionCellSize; ++i) {
			BOOST_ASSERT( i < partition.size() );
			BOOST_ASSERT( partition[i] < partitionCellOf.size() );
			partitionCellOf[partition[i]] = cellCounter;
		}
		++cellCounter;
		
		createdNewCell = true;
	}
		
	return createdNewCell;
}

inline bool Partition::undoIntersection() {
	if (partitionCellBorder[cellCounter-1] < 1)
		return false;
	--cellCounter;
	unsigned int splitFromCellNumber = partitionCellOf[ partition[partitionCellBorder[cellCounter] - 1] ];
	
	BOOST_ASSERT(partitionCellBorder[splitFromCellNumber] < partitionCellBorder[cellCounter]);
	BOOST_ASSERT(partitionCellLength[cellCounter] > 0 );
	//std::cout << "split from " << splitFromCellNumber << std::endl;
	//std::cout << "merge " << partitionCellBorder[splitFromCellNumber] << " " << partitionCellBorder[cellCounter] << " " << (partitionCellBorder[cellCounter] + partitionCellLength[cellCounter]) << std::endl;
	
	for (unsigned int i=partitionCellBorder[cellCounter]; i<partitionCellBorder[cellCounter] + partitionCellLength[cellCounter]; ++i) {
		partitionCellOf[partition[i]] = splitFromCellNumber;
	}
	std::inplace_merge(partition.begin() + partitionCellBorder[splitFromCellNumber],
					   partition.begin() + partitionCellBorder[cellCounter],
					   partition.begin() + (partitionCellBorder[cellCounter] + partitionCellLength[cellCounter]));
	
	
	if (partitionCellLength[cellCounter] == 1) {
		--fixCounter;
		fix[fixCounter] = 0;
	}
	if (partitionCellLength[splitFromCellNumber] == 1) {
		--fixCounter;
		fix[fixCounter] = 0;
	}
	
	partitionCellLength[splitFromCellNumber] += partitionCellLength[cellCounter];
	
	partitionCellLength[cellCounter] = 0;
	partitionCellBorder[cellCounter] = 0;
	
	return true;
}

}
}

#endif // -- PARTITION_H_
