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


#ifndef BSGS_EXPORT_H_
#define BSGS_EXPORT_H_

#include <map>

#include <permlib/permutation.h>
#include <permlib/transversal/schreier_tree_transversal.h>

#include <boost/foreach.hpp>

namespace permlib { namespace exports { 

/// data structure with elementary data types to represent a BSGS based on SchreierTreeTransversal
struct BSGSSchreierData {
	/// degree of the group
	dom_int n;
	
	/// size of the base
	dom_int baseSize;
	/// base
	/**
	 * array of size baseSize
	 */
	dom_int* base;
	
	/// size of the strong generating set
	dom_int sgsSize;
	/// strong generating set
	/**
	 * two-dim array of size sgsSize * n
	 */
	dom_int** sgs;
	
	/// transversals
	/**
	 * two-dim array of size baseSize * n
	 *
	 * Each transversal is stored as a tree in an array.
	 * The elements in the array point to a number, 
	 * which is the (non-negative) index of the stored permutation 
	 * in the list variable sgs.
	 * The array entry is -2 if no element is defined or -1 if element is identity
	 *  (corresponds to base element position)
	 */
	int** transversals;
	
	~BSGSSchreierData() {
		delete[] base;
		for (unsigned int i = 0; i < sgsSize; ++i)
			delete[] sgs[i];
		delete[] sgs;
		for (unsigned int i = 0; i < baseSize; ++i)
			delete[] transversals[i];
		delete[] transversals;
	}
};


/// base class for import/export of a BSGS based on SchreierTreeTransversal
struct BSGSSchreierBase {
	/// a BSGS which uses Permutation and SchreierTreeTransversal
	typedef BSGS<Permutation, SchreierTreeTransversal<Permutation> > BSGSSchreier;
};


/// export of a BSGS based on SchreierTreeTransversal
struct BSGSSchreierExport : public BSGSSchreierBase {
	/**
	 * @param bsgs the SchreierTreeTransversal based BSGS to export
	 * @return BSGS data encapsulated in elementary data types
	 */
	BSGSSchreierData* exportData(const BSGSSchreier& bsgs) const {
		std::map<Permutation::ptr, int> generatorMap;
		BSGSSchreierData* data = new BSGSSchreierData();
		
		data->n = bsgs.n;
		data->baseSize = bsgs.B.size();
		data->base = new dom_int[data->baseSize];
		std::copy(bsgs.B.begin(), bsgs.B.end(), data->base);
		
		data->sgsSize = bsgs.S.size();
		data->sgs = new dom_int*[data->sgsSize];
		int idx = 0;
		BOOST_FOREACH(const Permutation::ptr& p, bsgs.S) {
			data->sgs[idx] = new dom_int[bsgs.n];
			std::copy(p->m_perm.begin(), p->m_perm.end(), data->sgs[idx]);
			generatorMap[p] = idx;
			++idx;
		}
		
		data->transversals = new int*[data->baseSize];
		idx = 0;
		BOOST_FOREACH(const SchreierTreeTransversal<Permutation>& trans, bsgs.U) {
			data->transversals[idx] = new int[bsgs.n];
			std::vector<int> transversalData(bsgs.n);
			for (unsigned int i = 0; i < bsgs.n; ++i) {
				if (i == bsgs.B[idx]) {
					data->transversals[idx][i] = -1;
				} else if (trans.m_transversal[i]) {
					data->transversals[idx][i] = generatorMap[trans.m_transversal[i]];
				} else {
					data->transversals[idx][i] = -2;
				}
			}
			++idx;
		}
		
		return data;
	}
};


/// import of a BSGS based on SchreierTreeTransversal
struct BSGSSchreierImport : public BSGSSchreierBase {
	/**
	 * @param data BSGS data encapsulated in elementary data types
	 * @return BSGS based on SchreierTreeTransversal which is encoded in data
	 */
	BSGSSchreier* importData(const BSGSSchreierData* data) const {
		std::map<int, Permutation::ptr> generatorMap;
		BSGSSchreier* bsgs = new BSGSSchreier(data->n);
		
		bsgs->B.resize(data->baseSize);
		std::copy(data->base, data->base + data->baseSize, bsgs->B.begin());
		
		for (unsigned int idx = 0; idx < data->sgsSize; ++idx) {
			Permutation::ptr perm(new Permutation(data->sgs[idx], data->sgs[idx] + data->n));
			bsgs->S.push_back(perm);
			generatorMap[idx] = perm;
		}
		
		Permutation::ptr identity(new Permutation(data->n));
		
		bsgs->U.resize(data->baseSize, SchreierTreeTransversal<Permutation>(data->n));
		for (unsigned int idx = 0; idx < data->baseSize; ++idx) {
			SchreierTreeTransversal<Permutation> U(data->n);
			for (unsigned int i = 0; i < data->n; ++i) {
				if (data->transversals[idx][i] >= 0) {
					U.m_transversal[i] = generatorMap[data->transversals[idx][i]];
					U.m_orbit.push_back(i);
				} else if (i == bsgs->B[idx]) {
					BOOST_ASSERT( data->transversals[idx][i] == -1 );
					U.m_transversal[i] = identity;
					U.m_orbit.push_back(i);
				}
			}
			bsgs->U[idx] = U;
		}
		
		return bsgs;
	}
};

} } // end NS

#endif // -- BSGS_EXPORT_H_
