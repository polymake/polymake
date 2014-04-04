// ---------------------------------------------------------------------------
//
// This file is part of SymPol
//
// Copyright (C) 2006-2010  Thomas Rehn <thomas@carmen76.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// ---------------------------------------------------------------------------

#include <iostream>

#include "qarray.h"

namespace sympol {

QArray::QArray(ulong n, ulong rowIndex, bool homogenize) 
  : m_rowIndex(rowIndex), m_homogenize(homogenize)
{
	/*if (homogenize)
		size(n+1);
	else*/
	size(n);
}

QArray::QArray(const QArray & q) {
	m_ulN = q.m_ulN;
	m_rowIndex = q.m_rowIndex;
	m_homogenize = q.m_homogenize;

	m_aq = new mpq_t[m_ulN];
	for (ulong i=0; i<m_ulN; ++i) {
		mpq_init(m_aq[i]);
		mpq_set(m_aq[i], q.m_aq[i]);
	}
}


QArray::~QArray() {
	if (m_aq != NULL) {
		for (ulong i=0; i<m_ulN; ++i) {
			mpq_clear(m_aq[i]);
		}
		delete[] m_aq;
	}
}

void QArray::size(ulong n) {
	m_ulN = n;

	m_aq = new mpq_t[n];
	for (ulong i=0; i<n; ++i) {
		mpq_init(m_aq[i]);
	}
}

bool QArray::isRay() const {
	return mpq_sgn(m_aq[0]) == 0;
}

const mpq_t & QArray::operator[](ulong i) const {
	return m_aq[i];
}

mpq_t & QArray::operator[](ulong i) {
	return m_aq[i];
}

QArray& QArray::operator=(const QArray& a) {
	m_rowIndex = a.m_rowIndex;
	m_homogenize = a.m_homogenize;
	initFromArray(a.m_aq);
	return *this;
}

bool QArray::operator<(const QArray& a) const {
	if (m_ulN < a.m_ulN)
		return true;
	if (m_ulN > a.m_ulN)
		return false;
	
	for (unsigned int i = 0; i < m_ulN; ++i) {
		const int cmp = mpq_cmp(m_aq[i], a.m_aq[i]);
		if (cmp < 0)
			return true;
		if (cmp > 0)
			return false;
	}
	
	return false;
}

void QArray::initFromArray(ulong size, mpq_t* a) {
	BOOST_ASSERT(m_ulN >= size);
	const ulong offset = m_ulN-size;
	for (ulong i=0; i<m_ulN-offset; ++i) {
		mpq_set(m_aq[i+offset], a[i]);
	}
}

void QArray::initFromArray(ulong size, mpz_t* a) {
	BOOST_ASSERT(m_ulN >= size);
	const ulong offset = m_ulN-size;
	for (ulong i=offset; i<m_ulN; ++i) {
		mpq_set_z(m_aq[i], a[i-offset]);
	}
}

QArray& QArray::operator+=(const QArray& a) {
	for (ulong i=0; i<m_ulN; ++i) {
		mpq_add(m_aq[i], m_aq[i], a.m_aq[i]);
	}
	return *this;
}

void QArray::scalarProduct(const QArray& q, mpq_class & result, mpq_class & temp) const {
	mpq_mul(result.get_mpq_t(), q.m_aq[0], m_aq[0]);
	for (ulong j=1; j < m_ulN; ++j) {
		mpq_mul(temp.get_mpq_t(), q.m_aq[j], m_aq[j]);
		result += temp;
	}
}

void QArray::denominatorLCM(mpz_t & lcm) const {
	mpz_set(lcm, mpq_denref(m_aq[1]));

	for (ulong j=2; j < m_ulN; ++j) {
		mpz_lcm(lcm, lcm, mpq_denref(m_aq[j]));
	}

	return;
}

void QArray::normalizeArray(ulong pivotIndex) {
	if (mpq_sgn(m_aq[pivotIndex]) == 0) {
		return;
	}

	mpq_t tmp;
	mpq_init(tmp);
	mpq_abs(tmp, m_aq[pivotIndex]);
	for (ulong i = 0; i < m_ulN; ++i) {
		mpq_div(m_aq[i], m_aq[i], tmp);
	}
	mpq_clear(tmp);
}



std::istream & operator>>(std::istream & is, QArray & q) {
	mpq_class qTemp;
	ulong i = 0;
	if (q.m_homogenize) {
		++i;
	}
	for (; i < q.m_ulN; ++i) {
		is >> qTemp;
		mpq_set(q.m_aq[i], qTemp.get_mpq_t());
	}
	return is;
}

std::ostream & operator<<(std::ostream & os, const QArray & q) {
	for (ulong i = 0; i < q.m_ulN; ++i) {
		os << q.m_aq[i] << " ";
	}
	return os;
}

}
