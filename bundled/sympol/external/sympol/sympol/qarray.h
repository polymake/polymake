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

#ifndef QARRAY_H
#define QARRAY_H

#include "common.h"
#include <gmpxx.h>

namespace sympol {

class QArray {
	public:
		QArray(){ m_aq = 0; }
		QArray(ulong n, ulong rowIndex = -1, bool homogenize = false);
		QArray(const QArray & q);
		~QArray();

		void size(ulong n);
		ulong size() const { return m_ulN; }

		const mpq_t & operator[](ulong i) const;
		mpq_t & operator[](ulong i);

		void denominatorLCM(mpz_t & lcm) const;

		bool isRay() const;

		void scalarProduct(const QArray& q, mpq_class & result, mpq_class & temp) const;

		void normalizeArray(ulong pivotIndex = 0);

		ulong index() const { return m_rowIndex; }

		template<class GMPTYPE>
		void initFromArray(GMPTYPE* a) {
			initFromArray(m_ulN, a);
		}
		void initFromArray(ulong size, mpq_t* a);
		void initFromArray(ulong size, mpz_t* a);

		QArray& operator=(const QArray& a);    
		QArray& operator+=(const QArray& a);
		
		bool operator<(const QArray& a) const;

		friend std::istream & operator>>(std::istream & is, sympol::QArray & q);
		friend std::ostream & operator<<(std::ostream & os, const sympol::QArray & q);
	protected:
		mpq_t* m_aq;
		ulong m_ulN;
		ulong m_rowIndex;
		bool m_homogenize;
};

}

#endif
