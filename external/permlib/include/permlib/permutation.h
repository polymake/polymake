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


#ifndef PERMUTATION_H_
#define PERMUTATION_H_

#include <permlib/common.h>

// for I/O
#include <string>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <sstream>
#include <set>
#include <list>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/foreach.hpp>
#include <boost/cstdint.hpp>

#include <boost/version.hpp>

#if BOOST_VERSION / 100 >= 1067
#include <boost/integer/common_factor_rt.hpp>
#define permlib_boost_lcm boost::integer::lcm
#else
#include <boost/math/common_factor_rt.hpp>
#define permlib_boost_lcm boost::math::lcm
#endif

namespace permlib {

namespace exports { struct BSGSSchreierExport; }

/// Permutation class storing all values explicitly
class Permutation {
public:
	/// typedef for permutation image
	typedef std::vector<dom_int> perm;
	
	/// boost shared_ptr of this class
	typedef boost::shared_ptr<Permutation> ptr;
	
	/// constructs identity permutation acting on n elements
	explicit Permutation(dom_int n);
	/// constructs permutation acting on n elements, given by string in cycle form
	Permutation(dom_int n, const std::string &cycles);
	/// constructs permutation acting on n elements, given by string in cycle form
	Permutation(dom_int n, const char* cycles);
	/// sort of copy constructor
	explicit Permutation(const perm &p);
	/// copy constructor
	Permutation(const Permutation &p) : m_perm(p.m_perm), m_isIdentity(p.m_isIdentity) {};
	/// construct from dom_int-iterator  
	template<class InputIterator>
	Permutation(InputIterator begin, InputIterator end) : m_perm(begin, end), m_isIdentity(false) {}

	/// permutation multiplication from the right
	Permutation operator*(const Permutation &p) const;
	/// permutation inplace multiplication from the right
	/**
	 * i.e. THIS := THIS * p
	 */
	Permutation& operator*=(const Permutation &p);
	/// permutation inplace multiplication from the left
	/**
	 * i.e. THIS := p * THIS
	 */
	Permutation& operator^=(const Permutation &p);
	/// permutation inversion
	Permutation operator~() const;
	/// permutation inplace inversion
	Permutation& invertInplace();
	/// equals operator
	bool operator==(const Permutation &p2) const { return m_perm == p2.m_perm; };

	/// lets permutation act on val
	inline dom_int operator/(dom_int val) const { return at(val); }
	/// lets permutation act on val
	inline dom_int at(dom_int val) const { return m_perm[val]; }

	/// lets inverse permutation act on val, i.e. compute j such that (this->at(j) == val)
	dom_int operator%(dom_int val) const;

	/// output in cycle form
	friend std::ostream &operator<< (std::ostream &out, const Permutation &p);

	/// returns true if this permutation is identity
	/**
	 * This is done by checking the image of every point.
	 */
	bool isIdentity() const;
	/// dummy stub for interface compatability with PermutationWord
	inline void flush() {};
	/// number of points this permutation acts on
	inline dom_int size() const { return m_perm.size(); }
	
	/// computes all cycles of this permutation
	/**
	 * @param includeTrivialCycles if true, the result list contains also cycles consisting of one single element
	 * @return list of pairs [minimal element of cycle, cycle length]
	 */
	std::list<std::pair<dom_int, unsigned int> > cycles(bool includeTrivialCycles = false) const;
	
	/// computes the order of this permutation
	/**
	 * @return number c such that p^c = identity
	 */
	boost::uint64_t order() const;
	
	/// restricts this permutation p to a subset S of the domain
	/**
	 * S must fullfill S^p = S
	 * @param n_proj degree of the restricted permutation (ie. number of elements in the subset S
	 * @param begin begin-iterator to subset S
	 * @param end end-iterator to subset S
	 * @return pointer to a permutation which is restricted to the given domain subset
	 */
	template<typename ForwardIterator>
	Permutation* project(unsigned int n_proj, ForwardIterator begin, ForwardIterator end) const;
	
	///updates this permutation such that pos is mapped onto val and val onto pos
	void setTransposition(dom_int pos, dom_int val);
protected:
	/// defintion of permutation behavior
	perm m_perm;

	/// if set to true, permutation is identity; if set to false then it is not known whether this is identity;
	bool m_isIdentity;

	/// INTERNAL ONLY: constructs an "empty" permutation, i.e. without element mapping
	Permutation(dom_int n, bool) : m_perm(n), m_isIdentity(false) {}
	
	/// initializes permutation data from a string in cycle form
	void initFromCycleString(const std::string& cycles);
	
	friend struct permlib::exports::BSGSSchreierExport;
};


//
//     ----       IMPLEMENTATION
//

inline Permutation::Permutation(dom_int n) 
	: m_perm(n), m_isIdentity(true) 
{
	for (dom_int i=0; i<n; ++i)
		m_perm[i] = i;
}

inline void Permutation::initFromCycleString(const std::string& cycleString) {
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sepCycles(",");
	tokenizer tokens(cycleString, sepCycles);

	for (dom_int i=0; i<m_perm.size(); ++i)
		m_perm[i] = i;
	
#ifdef PERMLIB_DEBUGMODE
	boost::dynamic_bitset<> seenIndices(m_perm.size());
#endif
	
	for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
		std::stringstream ss(*tok_iter);

		unsigned int first, last, temp;
		ss >> first;
		last = first;
#ifdef PERMLIB_DEBUGMODE
		BOOST_ASSERT( !seenIndices[first-1] );
		seenIndices.set(first-1, 1);
#endif
		
		while (ss >> temp) {
#ifdef PERMLIB_DEBUGMODE
			BOOST_ASSERT( !seenIndices[temp-1] );
			seenIndices.set(temp-1, 1);
#endif
			m_perm[last-1] = temp-1;
			last = temp;
		}
		m_perm[last-1] = first-1;
	}
}


inline Permutation::Permutation(dom_int n, const std::string & cycleString) 
	: m_perm(n), m_isIdentity(false) 
{
	initFromCycleString(cycleString);
}

inline Permutation::Permutation(dom_int n, const char* cycleString) 
	: m_perm(n), m_isIdentity(false) 
{
	initFromCycleString(std::string(cycleString));
}


inline Permutation::Permutation(const perm& p) 
	: m_perm(p), m_isIdentity(false) 
{ 
#ifdef PERMLIB_DEBUGMODE
	// check that m_perm really is a permutation
	std::set<dom_int> values;
	for (dom_int i=0; i<m_perm.size(); ++i) {
		const dom_int& v = m_perm[i];
		BOOST_ASSERT( v < m_perm.size() );
		values.insert(v);
	}
	BOOST_ASSERT( values.size() == m_perm.size() );
#endif
}

inline Permutation Permutation::operator*(const Permutation &p) const {
	BOOST_ASSERT(p.m_perm.size() == m_perm.size());

	Permutation res(m_perm.size(), true);
	for (dom_int i=0; i<m_perm.size(); ++i) {
		res.m_perm[i] = p.m_perm[m_perm[i]];
	}
	return res;
}

inline Permutation& Permutation::operator*=(const Permutation &p) {
	BOOST_ASSERT(p.m_perm.size() == m_perm.size());
	m_isIdentity = false;
	perm tmp(m_perm);
	
	for (dom_int i=0; i<m_perm.size(); ++i) {
		tmp[i] = p.m_perm[m_perm[i]];
	}
	m_perm = tmp;
	
	return *this;
}

inline Permutation& Permutation::operator^=(const Permutation &p) {
	BOOST_ASSERT(p.m_perm.size() == m_perm.size());
	m_isIdentity = false;
	perm tmp(m_perm);

	for (dom_int i=0; i<m_perm.size(); ++i) {
		m_perm[i] = tmp[p.m_perm[i]];
	}
	return *this;
}

inline Permutation Permutation::operator~() const {
	Permutation res(m_perm.size(), true);
	for (dom_int i=0; i<m_perm.size(); ++i) {
		res.m_perm[m_perm[i]] = i;
	}
	return res;
}

inline Permutation& Permutation::invertInplace() {
	perm tmp(m_perm);
	for (dom_int i=0; i<m_perm.size(); ++i) {
		m_perm[tmp[i]] = i;
	}
	return *this;
}

inline dom_int Permutation::operator%(dom_int val) const {
	for (dom_int i = 0; i < m_perm.size(); ++i) {
		if (m_perm[i] == val)
			return i;
	}
	// must not happen, we have a permutation!
	BOOST_ASSERT(false);
	return -1;
}

inline bool Permutation::isIdentity() const {
	if (m_isIdentity)
		return true;
	for (dom_int i=0; i<m_perm.size(); ++i)
		if (at(i) != i)
			return false;
	return true;
}

inline std::list<std::pair<dom_int, unsigned int> > Permutation::cycles(bool includeTrivialCycles) const {
	boost::dynamic_bitset<> worked(m_perm.size());
	typedef std::pair<dom_int, unsigned int> CyclePair;
	std::list<CyclePair> cycleList;
	unsigned int cycleLength = 0;
	
	for (dom_int x=0; x<m_perm.size(); ++x) {
		if (worked[x])
			continue;
		
		dom_int px, startX;
		worked.set(x, 1);
		startX = x;
		px = m_perm[x];
		if (x == px) {
			if (includeTrivialCycles)
				cycleList.push_back(CyclePair(x, 1));
			continue;
		}
		
		cycleLength = 2;
		
		while (m_perm[px] != startX) {
				worked.set(px, 1);
				px = m_perm[px];
				++cycleLength;
		}
		worked.set(px, 1);
		cycleList.push_back(CyclePair(startX, cycleLength));
	}
	
	return cycleList;
}

inline boost::uint64_t Permutation::order() const {
	typedef std::pair<dom_int, unsigned int> CyclePair;
	std::list<CyclePair> cycleList = this->cycles();
	boost::uint64_t ord = 1;
	BOOST_FOREACH(const CyclePair& cyc, cycleList) {
		ord = permlib_boost_lcm(ord, static_cast<boost::uint64_t>(cyc.second));
	}
	return ord;
}

template<typename ForwardIterator>
Permutation* Permutation::project(unsigned int n_proj, ForwardIterator begin, ForwardIterator end) const {
	std::map<dom_int, dom_int> projectionMap;
	dom_int c = 0;
	for (ForwardIterator it = begin; it != end; ++it) {
		projectionMap[*it] = c++;
	}
	
	Permutation* proj = new Permutation(n_proj);
	bool is_identity = true;
	
	while (begin != end) {
		dom_int x = *begin++;
		BOOST_ASSERT( projectionMap.find(x) != projectionMap.end() );
		BOOST_ASSERT( projectionMap.find(m_perm[x]) != projectionMap.end() );
		const dom_int proj_x = projectionMap[x];
		const dom_int proj_px = projectionMap[ m_perm[x] ];
		BOOST_ASSERT( proj_x < n_proj );
		BOOST_ASSERT( proj_px < n_proj );
		proj->m_perm[ proj_x ] = proj_px;
		
		if (proj_x != proj_px)
			is_identity = false;
	}
	
	proj->m_isIdentity = is_identity;
	
	return proj;
}

inline void Permutation::setTransposition(dom_int pos, dom_int val) {
	BOOST_ASSERT(pos < m_perm.size());
	BOOST_ASSERT(val < m_perm.size());
	
	m_perm[pos] = val;
	m_perm[val] = pos;
}

inline std::ostream& operator<<(std::ostream& out, const Permutation& p) {
	typedef std::pair<dom_int, unsigned int> CyclePair;
	bool output = false;
	
	std::list<CyclePair> cycleList = p.cycles();
	BOOST_FOREACH(const CyclePair& c, cycleList) {
		dom_int px = p / c.first;
		out << "(" << (c.first + 1) << ",";
		while (px != c.first) {
			out << (px+1);
			px = p / px;
			if (px == c.first)
				 out << ")";
			else
				out << ",";
		}
		output = true;
	}
	
	if (!output)
		out << "()";
	
	return out;
}

}

#endif // -- PERMUTATION_H_
