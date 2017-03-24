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


#ifndef PERMUTATIONWORD_H_
#define PERMUTATIONWORD_H_

#include <permlib/permutation.h>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

namespace permlib {

typedef boost::shared_ptr<Permutation> PermutationPtr;

/// permutation class storing permutations as words of elementary Permutation 's
/**
 * interface compatible with Permutation
 */
class PermutationWord {
public:
	/// typedef for permutation image
	typedef Permutation::perm perm;
	
	/// boost shared_ptr of this class
	typedef boost::shared_ptr<PermutationWord> ptr;
	
	/// constructs identity permutation acting on n elements
	explicit PermutationWord(unsigned int n);
	/// constructs permutation acting on n elements, given by string in cycle form
	PermutationWord(unsigned int n, const std::string &cycles);
	/// sort of copy constructor
	explicit PermutationWord(const perm &p);
	/// copy constructor
	PermutationWord(const PermutationWord &p);

	/// permutation multiplication from the right
    PermutationWord operator*(const PermutationWord &p) const;
	/// permutation inplace multiplication from the right
	/**
	 * i.e. THIS := THIS * p
	 */
    PermutationWord& operator*=(const PermutationWord &p);
    /// permutation inplace multiplication from the left
	/**
	 * i.e. THIS := p * THIS
	 */
    PermutationWord& operator^=(const PermutationWord &p);
	/// permutation inversion
    PermutationWord operator~() const;
	/// permutation inplace inversion
    PermutationWord& invertInplace();
	/// equals operator
    bool operator==(const PermutationWord &p2) const;

	/// lets permutation act on val
	inline unsigned long operator/(unsigned long val) const { return at(val); }
	/// lets permutation act on val
	unsigned long at(unsigned long val) const;
	/// lets inverse permutation act on val, i.e. compute j such that (this->at(j) == val)
	unsigned long operator%(unsigned long val) const;

	/// output
	friend std::ostream &operator<< (std::ostream &out, const PermutationWord &p);

	/// returns true if this permutation is identity
	/**
	 * This is done by checking the image of every point, so the permutation word is multiplied out.
	 * After that the completely computed permutation is added as a new element to the word generator pool
	 */
	bool isIdentity(bool flush = false) const;
    
	/// force that permutation word is multiplied out
	//TODO: it must be the other way round: isIdentity should call flush
	inline void flush() { isIdentity(true); }
	/// number of points this permutation acts on
	inline unsigned int size() const { return m_n; }
	
	/// number of generators in the word generator pool
	static unsigned long elementsNumber() { return ms_elements.size(); }
	
	/// deletes all elementary permutations from the storage. use ONLY if there is currently no PermutationWord in use anywhere
	static void clearStorage();
private:
	static std::vector<PermutationPtr> ms_elements;
	static std::vector<PermutationPtr> ms_elementsInverse;
	static void addElement(const perm &p);
	static void addElement(const perm &p, const perm &pInv);
	static void addElement(PermutationPtr p);

	mutable std::vector<int> m_word;
	unsigned int m_n;
};

std::vector<PermutationPtr> PermutationWord::ms_elements(1);
std::vector<PermutationPtr> PermutationWord::ms_elementsInverse(1);

inline PermutationWord::PermutationWord(unsigned int n) 
	: m_n(n) 
{ }

inline PermutationWord::PermutationWord(unsigned int n, const std::string &cycles) 
	: m_n(n) 
{
	Permutation *pp = new Permutation(n, cycles);
	ms_elements.push_back(PermutationPtr(pp));
	ms_elementsInverse.push_back(PermutationPtr(new Permutation(~*pp)));
	m_word.reserve(2);
	m_word.push_back(ms_elements.size()-1);
}

inline PermutationWord::PermutationWord(const PermutationWord &p) 
	: m_word(p.m_word), m_n(p.m_n) 
{ }

inline PermutationWord::PermutationWord(const perm &p) 
	: m_n(p.size()) 
{
	addElement(p);
	m_word.reserve(2);
	m_word.push_back(ms_elements.size()-1);
}

inline void PermutationWord::addElement(const perm &p) {
	PermutationPtr gen(new Permutation(p));
	ms_elements.push_back(gen);
	PermutationPtr genInv(new Permutation(~*gen));
	ms_elementsInverse.push_back(genInv);
}
inline void PermutationWord::addElement(PermutationPtr p) {
	ms_elements.push_back(p);
	PermutationPtr genInv(new Permutation(~*p));
	ms_elementsInverse.push_back(genInv);
}

inline void PermutationWord::addElement(const perm &p, const perm &pInv) {
	PermutationPtr gen(new Permutation(p));
	ms_elements.push_back(gen);
	PermutationPtr genInv(new Permutation(pInv));
	ms_elementsInverse.push_back(genInv);
}


inline PermutationWord PermutationWord::operator*(const PermutationWord &p) const {
	PermutationWord res(*this);
	res *= p;
	return res;
}

inline PermutationWord& PermutationWord::operator*=(const PermutationWord &p) {
	if (m_word.empty()) {
		m_word.insert(m_word.end(), p.m_word.begin(), p.m_word.end());
		return *this;
	} else if (p.m_word.empty())
		return *this;

	m_word.insert(m_word.end(), p.m_word.begin(), p.m_word.end());
	return *this;
}

inline PermutationWord& PermutationWord::operator^=(const PermutationWord &p) {
	if (m_word.empty()) {
		m_word.insert(m_word.end(), p.m_word.begin(), p.m_word.end());
		return *this;
	} else if (p.m_word.empty())
		return *this;
	
	m_word.insert(m_word.begin(), p.m_word.begin(), p.m_word.end());
	return *this;
}


inline PermutationWord& PermutationWord::invertInplace() {
	std::vector<int> oldWord(m_word);
	for (unsigned int i=0; i<oldWord.size(); ++i) {
		m_word[i] = -oldWord[oldWord.size() - 1 - i];
	}
	return *this;
}

inline PermutationWord PermutationWord::operator~() const {
	PermutationWord inv(*this);
	for (unsigned int i=0; i<m_word.size(); ++i) {
		inv.m_word[i] = -m_word[m_word.size() - 1 - i];
	}
	return inv;
}

inline unsigned long PermutationWord::at(unsigned long val) const {
	unsigned long ret = val;
	BOOST_FOREACH(int e, m_word) {
		if (e > 0)
			ret = *(ms_elements[e]) / ret;
		else
			ret = *(ms_elementsInverse[-e]) / ret;
	}
	return ret;
}

inline unsigned long PermutationWord::operator%(unsigned long val) const {
	unsigned long ret = val;
	for (std::vector<int>::reverse_iterator lit = m_word.rbegin(); lit != m_word.rend(); ++lit) {
		int e = *lit;
		if (e < 0)
			ret = *(ms_elements[-e]) / ret;
		else
			ret = *(ms_elementsInverse[e]) / ret;
	}
	return ret;
}

inline bool PermutationWord::isIdentity(bool flush_) const {
	if (m_word.empty()) {
		return true;
	}
	if (m_word.size() == 1) {
		if (flush_)
			return true;
		int e = m_word.front();
		if (e>0)
			return ms_elements[e]->isIdentity();
		else
			return ms_elementsInverse[-e]->isIdentity();
	}

	perm mult(m_n);
	perm multInv(m_n);

	PermutationPtr resMult(new Permutation(m_n));
	BOOST_FOREACH(int e, m_word) {
		*resMult *= (e > 0)
				? *ms_elements[e].get()
				: *ms_elementsInverse[-e].get();
	}

	const bool permIsIdentity = resMult->isIdentity();

	if (!permIsIdentity) {
		addElement(resMult);
		m_word.clear();
		m_word.reserve(2);
		m_word.push_back(ms_elements.size()-1);
	}


	return permIsIdentity;
}

inline std::ostream &operator<< (std::ostream &out, const PermutationWord &p) {
    out << "W[";
    BOOST_FOREACH(int g, p.m_word) {
        out << g << ",";
    }
    out << "]";
    return out;
}

inline bool PermutationWord::operator==(const PermutationWord &p2) const {
	if (m_n != p2.m_n)
		return false;

	//TODO: is this correct or do we need the old code (below) or keep references to all perm instances?
    //note: this is not correct: compare result of \inv{g} * g and g * \inv{g}, but this can be fixed by   .... -x x ...... stripping when multiplying
    // in other cases it might have a good chance to work as intended
	//
	// NOTE: old code deleted from below during clean-up
	return m_word == p2.m_word;
}

inline void PermutationWord::clearStorage() {
	ms_elements.clear();
	ms_elements.resize(1);
	ms_elementsInverse.clear();
	ms_elementsInverse.resize(1);
}

}

#endif // -- PERMUTATIONWORD_H_
