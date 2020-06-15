/* Copyright (c) 2011-2020
   Thomas Opfer

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#ifndef TOSIMPLEX_H
#define TOSIMPLEX_H

#include "TORationalInf.h"
#include "TOmath.h"

#include <iostream>
#include <algorithm>
#include <list>
#include <ctime>
#include <cmath>
#include <vector>
#include <stdexcept>

namespace TOSimplex {

template <class T, class TInt>
class TOSolver
{

	public:
		TOSolver();
		TOSolver( const std::vector<T> &rows, const std::vector<TInt> &colinds, const std::vector<TInt> &rowbegininds, const std::vector<T> &obj, const std::vector<TORationalInf<T> > &rowlowerbounds, const std::vector<TORationalInf<T> > &rowupperbounds, const std::vector<TORationalInf<T> > &varlowerbounds, const std::vector<TORationalInf<T> > &varupperbounds );
		~TOSolver();
		void addConstraint( const std::vector<T> vec, const TORationalInf<T>& lbound, const TORationalInf<T>& ubound );
		void removeConstraint( TInt index );
		void setBound( TInt index, bool lower, TORationalInf<T> newBound );
		void setRHS( TInt index, TORationalInf<T> newBound );
		void setConstraintBothHandSides( TInt index, TORationalInf<T> newLBound, TORationalInf<T> newUBound );
		void setVarLB( TInt index, TORationalInf<T> newLBound );
		void setVarUB( TInt index, TORationalInf<T> newUBound );
		void setVarBounds( TInt index, TORationalInf<T> newLBound, TORationalInf<T> newUBound );
		void setObj( TInt index, T obj );
		void getBase( std::vector<TInt>& varStati, std::vector<TInt>& conStati );
		void setBase( const std::vector<TInt>& varStati, const std::vector<TInt>& conStati );
		TInt getNumRows();
		TInt getNumCols();
		void setInexactFarkasInfeasibilityGuess( std::vector<double> ray );
		std::vector<T> getFarkasInfeasibilityProof();
		void setInfeasibilityBound( TORationalInf<T> bound );
		TInt opt();
		std::vector<T> getX();
		std::vector<T> getY();
		std::vector<T> getD();
		std::pair<TORationalInf<T>,TORationalInf<T> > getConstraintBounds( const TInt i );
		std::pair<std::vector<T>, T> getGMI( TInt i, std::vector<bool> iV, T k = T( 1 ) );
		T getObj();
		void read( const char* filename );

	private:

		// Sortierhilfe, um Indizes eines Arrays absteigend zu Sortieren
		class ratsort {
			private:
				const std::vector<T> &Q;
			public:
				ratsort( const std::vector<T> &_Q ) : Q(_Q){}
				bool operator() ( TInt i, TInt j ) {
					return ( Q[i] > Q[j] );
				}
		};

		struct bilist {
				bilist *prev;
				bilist *next;
				TInt val;
				bool used;
			};

		struct transposeHelper {
			TInt valind;
			TInt ind;
		};

		struct RationalWithInd {
			T value;
			TInt ind;
		};

		std::vector<T> Acolwise;
		std::vector<TInt> Acolwiseind;
		std::vector<TInt> Acolpointer;
		std::vector<T> Arowwise;
		std::vector<TInt> Arowwiseind;
		std::vector<TInt> Arowpointer;

		std::vector<T> c;
		std::vector<TORationalInf<T> > lvec;
		std::vector<TORationalInf<T> > uvec;
		TORationalInf<T> *l;
		TORationalInf<T> *u;
		std::vector<T> x;
		std::vector<T> d;
		TInt m, n;
		bool hasBase;
		bool hasBasisMatrix;
		TInt baseIter;
		std::vector<TInt> B;
		std::vector<TInt> Binv;
		std::vector<TInt> N;
		std::vector<TInt> Ninv;


		std::vector<TInt> Urlen;
		std::vector<TInt> Urbeg;
		std::vector<T> Urval;
		std::vector<TInt> Ucind;
		std::vector<TInt> Ucptr;

		TInt Ucfreepos;

		std::vector<TInt> Uclen;
		std::vector<TInt> Ucbeg;
		std::vector<T> Ucval;
		std::vector<TInt> Urind;
		std::vector<TInt> Urptr;

		std::vector<T> Letas;
		std::vector<TInt> Lind;
		std::vector<TInt> Llbeg;
		TInt Lnetaf;
		TInt Lneta;
		std::vector<TInt> Letapos;

		TInt halfNumUpdateLetas;

		std::vector<TInt> perm;
		std::vector<TInt> permback;

		std::vector<T> DSE;
		std::vector<T> DSEtmp;
		bool antiCycle;

		bool hasPerturbated;

		std::vector<double> rayGuess;
		std::vector<T> farkasProof;

		TInt lastLeavingBaseVar;

		TORationalInf<T> infeasibilityBound;

		void copyTransposeA( TInt orgLen, const std::vector<T>& orgVal, const std::vector<TInt>& orgInd, const std::vector<TInt>& orgPointer, TInt newLen, std::vector<T>& newVal, std::vector<TInt>& newInd, std::vector<TInt>& newPointer );
		void mulANT( T* result, T* vector );

		void FTran( T* work, T* permSpike = nullptr, TInt* permSpikeInd = nullptr, TInt* permSpikeLen = nullptr );
		void BTran( T* work );
		void updateB( TInt leaving, T* permSpike, TInt* permSpikeInd, TInt* permSpikeLen );
		bool refactor();
		void findPiv( const std::vector<std::vector<TInt> >& Urowind, const std::vector<std::vector<TInt> >& Ucolind, bilist* const &R, bilist* const &C, const std::vector<bilist>& Ra, const std::vector<bilist>& Ca, const std::vector<TInt>& nnzCs, const std::vector<TInt>& nnzRs, TInt &i, TInt &j, bool &colsingleton );
		void clearBasis();
		void removeBasisFactorization();

		void init();

		TInt phase1();
		clock_t optExternal();
		TInt opt( bool P1 );

		bool checkDualFarkas();

		void showOptValDetails( T );

};


template <class T, class TInt>
TOSolver<T, TInt>::TOSolver(){
	this->init();

	this->m = 0;
	this->n = 0;

	this->l = this->lvec.data();
	this->u = this->uvec.data();

	this->Arowpointer.resize( m + 1 );
}


template <class T, class TInt>
TOSolver<T, TInt>::TOSolver( const std::vector<T> &rows, const std::vector<TInt> &colinds, const std::vector<TInt> &rowbegininds, const std::vector<T> &obj, const std::vector<TORationalInf<T> > &rowlowerbounds, const std::vector<TORationalInf<T> > &rowupperbounds, const std::vector<TORationalInf<T> > &varlowerbounds, const std::vector<TORationalInf<T> > &varupperbounds ){
	this->init();

	this->m = rowlowerbounds.size();
	this->n = varlowerbounds.size();

	this->Arowwise = rows;
	this->Arowwiseind = colinds;
	this->Arowpointer = rowbegininds;

	if( rows.size() != colinds.size() || rowbegininds.back() != static_cast<TInt>( rows.size() ) || static_cast<TInt>( rowupperbounds.size() ) != this->m || static_cast<TInt>( obj.size() ) != this->n || static_cast<TInt>( varupperbounds.size() ) != this->n ){
		throw std::runtime_error( "Inconsistent data." );
	}

	copyTransposeA( m, this->Arowwise, this->Arowwiseind, this->Arowpointer, n, this->Acolwise, this->Acolwiseind, this->Acolpointer );

	// Zielfunktion
	this->c = obj;

	// untere Schranken
	this->lvec.resize( n + m );
	this->l = this->lvec.data();
	for( TInt i = 0; i < n; ++i ){
		if( varlowerbounds[i].isInf ){
			this->l[i] = true;
		} else {
			this->l[i] = TORationalInf<T>( varlowerbounds[i].value );
		}
	}
	for( TInt i = 0; i < m; ++i ){
		if( rowupperbounds[i].isInf ){
			this->l[n+i] = true;
		} else {
			this->l[n+i] = TORationalInf<T>( -rowupperbounds[i].value );
		}
	}

	// obere Schranken
	this->uvec.resize( n + m );
	this->u = this->uvec.data();
	for( TInt i = 0; i < n; ++i ){
		if( varupperbounds[i].isInf ){
			this->u[i] = true;
		} else {
			this->u[i] = TORationalInf<T>( varupperbounds[i].value );
		}
	}
	for( TInt i = 0; i < m; ++i ){
		if( rowlowerbounds[i].isInf ){
			this->u[n+i] = true;
		} else {
			this->u[n+i] = TORationalInf<T>( -rowlowerbounds[i].value );
		}
	}


	this->B.resize( m );
	this->N.resize( n );
	this->Binv.resize( m + n );
	this->Ninv.resize( m + n );


	this->x.resize( n + m );
	this->d.resize( n );

	this->Urlen.resize( m );
	this->Urbeg.resize( m );

	this->Uclen.resize( m );
	this->Ucbeg.resize( m );


	TInt maxnumetas = m+2*this->halfNumUpdateLetas;	// Mehr ETAs kann es nicht geben
	this->Llbeg.resize( maxnumetas + 1 );
	this->Llbeg[0] = 0;	// Letzten Zeiger ans "Ende" zeigen
	this->Letapos.resize( maxnumetas );
	this->Lneta = 0;
	this->Lnetaf = 0;

	this->perm.resize( m );
	this->permback.resize( m );

}


template <class T, class TInt>
TOSolver<T, TInt>::~TOSolver(){

}


template <class T, class TInt>
void TOSolver<T, TInt>::init(){
	#ifndef TO_DISABLE_OUTPUT
		std::cout << "Simplex initialisiert." << std::endl;
	#endif

	this->halfNumUpdateLetas = 20;

	this->hasBase = false;
	this->hasBasisMatrix = false;
	this->baseIter = 0;

	this->lastLeavingBaseVar = -1;

	this->hasPerturbated = false;

	this->infeasibilityBound = true;
}


template <class T, class TInt>
void TOSolver<T, TInt>::addConstraint( const std::vector<T> vec, const TORationalInf<T>& lbound, const TORationalInf<T>& ubound ){

	if( static_cast<TInt>( vec.size() ) != n ){
		throw std::runtime_error( "Constraint has wrong size." );
	}

	++this->m;

	this->farkasProof.clear();

	this->Arowwise.reserve( this->Arowwise.size() + n );
	this->Arowwiseind.reserve( this->Arowwiseind.size() + n );

	for( TInt i = 0; i < n; ++i ){
		if( vec[i] != 0 ){
			this->Arowwise.push_back( vec[i] );
			this->Arowwiseind.push_back( i );
		}
	}

	this->Arowpointer.push_back( this->Arowwise.size() );

	copyTransposeA( m, this->Arowwise, this->Arowwiseind, this->Arowpointer, n, this->Acolwise, this->Acolwiseind, this->Acolpointer );

	if( ubound.isInf ){
		this->lvec.push_back( true );
	} else {
		this->lvec.push_back( TORationalInf<T>( -ubound.value  ) );
	}
	this->l = this->lvec.data();	// Pointer nach obigem push_back aktualisieren


	if( lbound.isInf ){
		this->uvec.push_back( true );
	} else {
		this->uvec.push_back( TORationalInf<T>( -lbound.value  ) );
	}
	this->u = this->uvec.data();	// Pointer nach obigem push_back aktualisieren

	// TODO Alte Basis übernehmen? Oder neue doch lieber extern neu bestimmen?
	this->clearBasis();
}


template <class T, class TInt>
void TOSolver<T, TInt>::removeConstraint( TInt index ){

	this->farkasProof.clear();

	TInt len = this->Arowpointer[index+1] - this->Arowpointer[index];
	TInt nnz = this->Arowpointer[m] - len;

	for( TInt i = this->Arowpointer[index]; i < nnz; ++i ){
		this->Arowwise[i] = this->Arowwise[i+len];
	}
	for( TInt i = this->Arowpointer[index]; i < nnz; ++i ){
		this->Arowwiseind[i] = this->Arowwiseind[i+len];
	}

	for( TInt i = index; i < m; ++i ){
		this->Arowpointer[i] = this->Arowpointer[i+1] - len;
	}

	this->Arowwise.resize( nnz );
	this->Arowwiseind.resize( nnz );
	this->Arowpointer.pop_back();

	--this->m;

	copyTransposeA( m, this->Arowwise, this->Arowwiseind, this->Arowpointer, n, this->Acolwise, this->Acolwiseind, this->Acolpointer );

	for( TInt i = n+index; i < n+m; ++i ){
		this->l[i] = this->l[i+1];
	}
	this->lvec.pop_back();
	for( TInt i = n+index; i < n+m; ++i ){
		this->u[i] = this->u[i+1];
	}
	this->uvec.pop_back();

	// TODO Alte Basis übernehmen? Oder neue doch lieber extern neu bestimmen?
	this->clearBasis();
}


template <class T, class TInt>
void TOSolver<T, TInt>::setBound( TInt index, bool lower, TORationalInf<T> newBound ){

	this->farkasProof.clear();

    if( lower ){
         if( !newBound.isInf ){
             this->u[this->n+index] = TORationalInf<T>( - newBound.value );
         } else {
             this->u[this->n+index] = true;
         }
     } else {
         if( !newBound.isInf ){
             this->l[this->n+index] = TORationalInf<T>( - newBound.value );
         } else {
             this->l[this->n+index] = true;
         }
     }
}


template <class T, class TInt>
void TOSolver<T, TInt>::setRHS( TInt index, TORationalInf<T> newBound ){

	this->farkasProof.clear();

	if( this->l[this->n+index].isInf ){
         if( !newBound.isInf ){
             this->u[this->n+index] = TORationalInf<T>( - newBound.value );
         } else {
             this->u[this->n+index] = true;
         }
     } else if( this->u[this->n+index].isInf ){
         if( !newBound.isInf ){
             this->l[this->n+index] = TORationalInf<T>( - newBound.value );
         } else {
             this->l[this->n+index] = true;
         }
     } else if( !this->l[this->n+index].isInf && !this->u[this->n+index].isInf && this->l[this->n+index].value == this->u[this->n+index].value ) {
         if( !newBound.isInf ){
             this->u[this->n+index] = TORationalInf<T>( - newBound.value );
             this->l[this->n+index] = TORationalInf<T>( - newBound.value );
         } else {
             this->u[this->n+index] = true;
             this->l[this->n+index] = true;
         }
     } else {
    	 throw std::runtime_error( "Cannot determine which bound to set." );
     }
}


template <class T, class TInt>
void TOSolver<T, TInt>::setConstraintBothHandSides( TInt index, TORationalInf<T> newLBound, TORationalInf<T> newUBound ){

	this->farkasProof.clear();

    if( !newLBound.isInf ){
        this->u[this->n+index] = TORationalInf<T>( - newLBound.value );
    } else {
        this->u[this->n+index] = true;
    }

    if( !newUBound.isInf ){
        this->l[this->n+index] = TORationalInf<T>( - newUBound.value );
    } else {
        this->l[this->n+index] = true;
    }
}

template <class T, class TInt>
void TOSolver<T, TInt>::setVarLB( TInt index, TORationalInf<T> newLBound ){

	this->farkasProof.clear();

    if( !newLBound.isInf ){
        this->l[index] = TORationalInf<T>( newLBound.value );
    } else {
        this->l[index] = true;
    }
}


template <class T, class TInt>
void TOSolver<T, TInt>::setVarUB( TInt index, TORationalInf<T> newUBound ){

	this->farkasProof.clear();

    if( !newUBound.isInf ){
        this->u[index] = TORationalInf<T>( newUBound.value );
    } else {
        this->u[index] = true;
    }
}


template <class T, class TInt>
void TOSolver<T, TInt>::setVarBounds( TInt index, TORationalInf<T> newLBound, TORationalInf<T> newUBound ){

	this->farkasProof.clear();

    if( !newLBound.isInf ){
        this->l[index] = TORationalInf<T>( newLBound.value );
    } else {
        this->l[index] = true;
    }

    if( !newUBound.isInf ){
        this->u[index] = TORationalInf<T>( newUBound.value );
    } else {
        this->u[index] = true;
    }
}


template <class T, class TInt>
void TOSolver<T, TInt>::setObj( TInt index, T obj ){
	this->c.at( index ) = obj;
}


template <class T, class TInt>
void TOSolver<T, TInt>::getBase( std::vector<TInt>& varStati, std::vector<TInt>& conStati ){
	varStati.resize( n );
	conStati.resize( m );
	for( TInt i = 0; i < m; ++i ){
		TInt j = B[i];
		if( j < n ){
			varStati[j] = 1;
		} else {
			conStati[j-n] = 1;
		}
	}

	for( TInt i = 0; i < n; ++i ){
		TInt j = N[i];

		TInt status;

		if( !l[j].isInf && x[j] == l[j].value ){
			status = 0;
		} else if( !u[j].isInf && x[j] == u[j].value ){
			status = 2;
		} else {
			status = 3;
		}

		if( j < n ){
			varStati[j] = status;
		} else {
			conStati[j-n] = status;
		}
	}
}


template <class T, class TInt>
void TOSolver<T, TInt>::setBase( const std::vector<TInt>& varStati, const std::vector<TInt>& conStati ){

	this->farkasProof.clear();

	TInt basenum = 0;
	TInt nbasenum = 0;

	if( static_cast<TInt>( varStati.size() ) != n ){
		throw std::runtime_error( "varStati has wrong size" );
	} else if( static_cast<TInt>( conStati.size() ) != m ){
		throw std::runtime_error( "conStati has wrong size" );
	}
	else {
		for( TInt i = 0; i < n; ++i ){
			if( varStati[i] == 1 ){
				++basenum;
			} else {
				++nbasenum;
			}
		}
		for( TInt i = 0; i < m; ++i ){
			if( conStati[i] == 1 ){
				++basenum;
			} else {
				++nbasenum;
			}
		}

		if( basenum != m || nbasenum != n ){
			throw std::runtime_error( "invalid basis" );
		}
	}

	basenum = 0;
	nbasenum = 0;

	for( TInt i = 0; i < n; ++i ){
		switch( varStati[i] ){
			case 1:
				this->B[basenum] = i;
				this->Binv[i] = basenum;
				this->Ninv[i] = -1;
				++basenum;
				break;
			case 0:
				// AtLower
				this->N[nbasenum] = i;
				this->Ninv[i] = nbasenum;
				this->Binv[i] = -1;
				++nbasenum;
				this->x[i] = this->l[i].value;
				break;
			case 2:
				// AtUpper
				this->N[nbasenum] = i;
				this->Ninv[i] = nbasenum;
				this->Binv[i] = -1;
				++nbasenum;
				this->x[i] = this->u[i].value;
				break;
			default:
				this->N[nbasenum] = i;
				this->Ninv[i] = nbasenum;
				this->Binv[i] = -1;
				++nbasenum;
				this->x[i] = 0;
				break;
		}
	}

	for( TInt i = n; i < n+m; ++i ){
		switch( conStati[i-n] ){
			case 1:
				this->B[basenum] = i;
				this->Binv[i] = basenum;
				this->Ninv[i] = -1;
				++basenum;
				break;
			case 0:
				// AtLower
				this->N[nbasenum] = i;
				this->Ninv[i] = nbasenum;
				this->Binv[i] = -1;
				++nbasenum;
				this->x[i] = this->l[i].value;
				break;
			case 2:
				// AtUpper
				this->N[nbasenum] = i;
				this->Ninv[i] = nbasenum;
				this->Binv[i] = -1;
				++nbasenum;
				this->x[i] = this->u[i].value;
				break;
			default:
				this->N[nbasenum] = i;
				this->Ninv[i] = nbasenum;
				this->Binv[i] = -1;
				++nbasenum;
				this->x[i] = 0;
				break;
		}
	}

	this->hasBase = true;
	this->removeBasisFactorization();

	this->d.clear();
	this->d.resize( n );

	this->DSE.clear();
	this->DSEtmp.clear();
}


template <class T, class TInt>
TInt TOSolver<T, TInt>::getNumRows(){
	return this->m;
}


template <class T, class TInt>
TInt TOSolver<T, TInt>::getNumCols(){
	return this->n;
}


template <class T, class TInt>
void TOSolver<T, TInt>::setInexactFarkasInfeasibilityGuess( std::vector<double> ray ){

	if( static_cast<TInt>( ray.size() ) != m ){
		throw std::runtime_error( "Farkas guess has wrong size." );
	}

	this->farkasProof.clear();

	this->rayGuess = ray;
}


template <class T, class TInt>
std::vector<T> TOSolver<T, TInt>::getFarkasInfeasibilityProof(){

	if( this->farkasProof.size() ){
		return this->farkasProof;
	}

	std::vector<T> ray( m );

	if( this->lastLeavingBaseVar != -1){
		if( !this->l[this->lastLeavingBaseVar].isInf && this->x[this->lastLeavingBaseVar] < this->l[this->lastLeavingBaseVar].value ){
			ray[this->Binv[this->lastLeavingBaseVar]] = -1;
		} else {
			ray[this->Binv[this->lastLeavingBaseVar]] = 1;
		}
	}

	this->BTran( ray.data() );

	this->farkasProof = ray;

	return ray;
}


template <class T, class TInt>
void TOSolver<T, TInt>::copyTransposeA( TInt orgLen, const std::vector<T>& orgVal, const std::vector<TInt>& orgInd, const std::vector<TInt>& orgPointer, TInt newLen, std::vector<T>& newVal, std::vector<TInt>& newInd, std::vector<TInt>& newPointer ){

	newVal.clear();
	newInd.clear();
	newPointer.clear();

	newPointer.resize( newLen + 1 );
	const TInt& nnz = orgInd.size();

	newVal.resize( nnz );
	newInd.resize( nnz );

	newPointer[newLen] = orgPointer[orgLen];	// Das Ende Stimmt überein

	std::vector< std::list<transposeHelper> > rowstmp(newLen);

	for( TInt i = 0; i < orgLen; ++i ){
		const TInt kend = orgPointer[i+1];
		for( TInt k = orgPointer[i]; k < kend; ++k ){
			transposeHelper tmp;
			tmp.valind = k;
			tmp.ind = i;
			rowstmp[orgInd[k]].push_back(tmp);
		}
	}

	typename std::list<transposeHelper>::iterator it;
	TInt k = 0;
	for( TInt i = 0; i < newLen; ++i ){
		newPointer[i] = k;
		for( it = rowstmp[i].begin(); it != rowstmp[i].end(); ++it ){
			transposeHelper tmp = *it;
			newVal[k] = orgVal[tmp.valind];
			newInd[k] = tmp.ind;
			++k;
		}
	}
}


template <class T, class TInt>
void TOSolver<T, TInt>::setInfeasibilityBound( TORationalInf<T> bound ){
	this->infeasibilityBound = bound;
}


template <class T, class TInt>
void TOSolver<T, TInt>::mulANT( T* result, T* vector ){
	for( TInt i = 0; i < m; ++i ){
		if( vector[i] == 0 ){
			continue;
		}
		const TInt kend = this->Arowpointer[i+1];
		for( TInt k = this->Arowpointer[i]; k < kend; ++k ){
			TInt ind = this->Ninv[this->Arowwiseind[k]];
			if( ind != -1 ){
				result[ind] += Arowwise[k] * vector[i];
			}
		}

		// logische Variablen
		if( Ninv[n+i] != -1 ){
			result[Ninv[n+i]] = vector[i];
		}
	}
}


template <class T, class TInt>
void TOSolver<T, TInt>::FTran( T* work, T* permSpike, TInt* permSpikeInd, TInt* permSpikeLen ){

	// FTranL-F

	for( TInt j = 0; j < this->Lnetaf; ++j ){
		const TInt p = this->Letapos[j];
		if( work[p] != 0 ){
			T ap = work[p];
			const TInt kend = Llbeg[j+1];
			for( TInt k = Llbeg[j]; k < kend; ++k ){
				work[Lind[k]] += this->Letas[k] * ap;
			}
		}
	}


	// FTranL-U

	for( TInt li = this->Lnetaf; li < this->Lneta; ++li ){
		const TInt p = Letapos[li];
		const TInt kend = this->Llbeg[li+1];
		for( TInt k = this->Llbeg[li]; k < kend; ++k ){
			TInt j = Lind[k];
			if( work[j] != 0 ){
				work[p] += Letas[k] * work[j];
			}
		}
	}


	// Permutierten Spike speichern für LU-Update
	// TODO, oben irgendwie umbauen, dass die Schleife nicht bis m laufen muss
	if( permSpike ){
		TInt &ind = *permSpikeLen;
		ind = 0;
		for( TInt i = 0; i < m; ++i ){
			if( work[i] != 0 ){
				permSpike[ind] = work[i];
				permSpikeInd[ind] = i;
				++ind;
			}
		}
	}


	// FTranU

	for( TInt k = m-1; k >= 0; --k ){
		const TInt j = this->perm[k];
		if( work[j] != 0 ){
			const TInt ks = this->Ucbeg[j];
			const TInt ke = ks + this->Uclen[j];
			T aj = work[j] / this->Ucval[ks];
			work[j] = aj;
			for( TInt kk = ks + 1; kk < ke; ++kk  ){
				work[this->Urind[kk]] -= this->Ucval[kk] * aj;
			}
		}
	}

}


template <class T, class TInt>
void TOSolver<T, TInt>::BTran( T* work ){

	// BTranU

	for( TInt k = 0; k < m; ++k ){
		const TInt j = this->perm[k];
		if( work[j] != 0 ){
			const TInt ks = this->Urbeg[j];
			const TInt ke = ks + this->Urlen[j];
			T aj = work[j] / this->Urval[ks];
			work[j] = aj;
			for( TInt kk = ks + 1; kk < ke; ++kk  ){
				work[this->Ucind[kk]] -= this->Urval[kk] * aj;
			}
		}
	}


	// BTranL-U

	for( TInt j = this->Lneta - 1; j >= this->Lnetaf; --j ){
		const TInt p = this->Letapos[j];
		if( work[p] != 0 ){
			T ap = work[p];
			const TInt kend = Llbeg[j+1];
			for( TInt k = Llbeg[j]; k < kend; ++k ){
				work[Lind[k]] += this->Letas[k] * ap;
			}
		}
	}


	// BTranL-F

	for( TInt j = this->Lnetaf - 1; j >= 0; --j ){
		const TInt p = this->Letapos[j];
		const TInt kend = Llbeg[j+1];
		for( TInt k = Llbeg[j]; k < kend; ++k ){
			const TInt i = Lind[k];
			if( work[i] != 0 ){
				work[p] += this->Letas[k] * work[i];
			}
		}
	}

}


template <class T, class TInt>
void TOSolver<T, TInt>::clearBasis(){

	this->farkasProof.clear();

	this->hasBase = false;

	this->removeBasisFactorization();

	this->B.clear();
	this->B.resize( m );
	this->Binv.clear();
	this->Binv.resize( m + n );
	this->N.clear();
	this->N.resize( n );
	this->Ninv.clear();
	this->Ninv.resize( m + n );

	this->x.clear();
	this->x.resize( n + m );
	this->d.clear();
	this->d.resize( n );

	this->DSE.clear();
	this->DSEtmp.clear();
}


template <class T, class TInt>
void TOSolver<T, TInt>::removeBasisFactorization(){
	this->hasBasisMatrix = false;

	this->Urlen.clear();
	this->Urbeg.clear();
	this->Urval.clear();
	this->Ucind.clear();
	this->Ucptr.clear();

	this->Uclen.clear();
	this->Ucbeg.clear();
	this->Ucval.clear();
	this->Urind.clear();
	this->Urptr.clear();

	this->Urlen.resize( m );
	this->Urbeg.resize( m );
	this->Uclen.resize( m );
	this->Ucbeg.resize( m );

	this->halfNumUpdateLetas = 20;
	TInt maxnumetas = m+2*this->halfNumUpdateLetas;	// Mehr ETAs kann es nicht geben
	this->Letas.clear();
	this->Lind.clear();
	this->Llbeg.clear();
	this->Llbeg.resize( maxnumetas + 1 );
	this->Llbeg[0] = 0;	// Letzten Zeiger ans "Ende" zeigen
	this->Letapos.clear();
	this->Letapos.resize( maxnumetas );
	this->Lneta = 0;
	this->Lnetaf = 0;

	this->perm.clear();
	this->perm.resize( m );
	this->permback.clear();
	this->permback.resize( m );
}


template <class T, class TInt>
void TOSolver<T, TInt>::findPiv( const std::vector<std::vector<TInt> >& Urowind, const std::vector<std::vector<TInt> >& Ucolind, bilist* const &R, bilist* const &C, const std::vector<bilist>& Ra, const std::vector<bilist>& Ca, const std::vector<TInt>& nnzCs, const std::vector<TInt>& nnzRs, TInt &i, TInt &j, bool &colsingleton ){

	const TInt p = 25;	// TODO adaptiv anpassen, falls es sich lohnt?

	const T llmm = T( m ) * T( m );

	T MM = llmm;
	TInt singletonsize = 0;
	TInt nn = 0;

	for( TInt k = 1; k <= m; ++k  ){
		{
			bilist* Cn = C;
			do {
				TInt jj = Cn->val;
				if( nnzCs[jj] == k ){
					// MarkCount begin

					T M = llmm;
					for( TInt li = 0; li < static_cast<TInt>( Urowind[jj].size() ); ++li ){
						TInt itmp = Urowind[jj][li];
						if( Ra[itmp].used ){
							T tmp = T( nnzRs[itmp] - 1 ) * T( nnzCs[jj] - 1 );
							if( k == 1 ){
								if( nnzRs[itmp] > singletonsize ){
									singletonsize = nnzRs[itmp];
									M = tmp;	// 0
									i = itmp;
									j = jj;
								}
							} else if( tmp < M ){
								M = tmp;
								i = itmp;
								j = jj;
								if( M == 0 ){
									break;
								}
							}
						}
					}

					// MarkCount end

					if( M < MM ){
						MM = M;
						if( k > 1 && MM <= T( k - 1 ) * T( k - 1 ) ){
							return;
						}
					}

					++nn;

					if( k > 1 && nn >= p && MM < llmm ){
						return;
					}
				}
				Cn = Cn->next;
			} while( Cn != C );
			if( k == 1 && MM < llmm ){
				colsingleton = true;
				return;
			}
		}

		{
			bilist* Rn = R;
			do {
				TInt ii = Rn->val;
				if( nnzRs[ii] == k ){
					// MarkCount begin

					T M = llmm;
					for( TInt li = 0; li < static_cast<TInt>( Ucolind[ii].size() ); ++li ){
						TInt jtmp = Ucolind[ii][li];
						if( Ca[jtmp].used ){
							T tmp = T( nnzCs[jtmp] - 1 ) * T( nnzRs[ii] - 1 );
							if( tmp < M ){
								M = tmp;
								j = jtmp;
								i = ii;
								if( M == 0 ){
									break;
								}
							}
						}
					}

					// MarkCount end

					if( M < MM ){
						MM = M;
						if( MM <= T( k ) * T( k - 1 ) ){
							return;
						}
					}

					++nn;

					if( nn >= p && MM < llmm ){
						return;
					}
				}
				Rn = Rn->next;
			} while( Rn != R );
		}
	}
}


template <class T, class TInt>
bool TOSolver<T, TInt>::refactor(){

	// std::cout << "Refaktorisiere Basismatrix (" << this->baseIter << " Iterationen vergangen)." << std::endl;

	this->baseIter = 0;

	TInt additionalTmpSpace = 50;	// TODO irgendwo als kontstante festlegen?

	// Temporärer Speicher für ETAs
	std::list<RationalWithInd> Llist;

	// Speicherort für temporäre Sparse-Matrix
	std::vector<std::vector<T> > Ucol(m);
	std::vector<std::vector<TInt> > Urowind(m);
	std::vector<std::vector<TInt> > Urowptr(m);
	std::vector<std::vector<T> > Urow(m);
	std::vector<std::vector<TInt> > Ucolind(m);
	std::vector<std::vector<TInt> > Ucolptr(m);

	TInt avg = m;
	// Basismatrix spaltenweise kopieren
	for( TInt i = 0; i < m; ++i ){
		const TInt collen = ( B[i] < n ) ? Acolpointer[B[i]+1] - Acolpointer[B[i]] : 1;
		const TInt clen = collen + additionalTmpSpace;
		avg += clen;
		Ucol[i].reserve( clen );
		Urowind[i].reserve( clen );
		Urowptr[i].reserve( clen );

		if( B[i] < n ){
			for( TInt j = this->Acolpointer[B[i]], k = 0; k < collen; ++j, ++k ){
				Ucol[i].push_back( this->Acolwise[j] );
				Urowind[i].push_back( this->Acolwiseind[j] );
			}
		} else {
			// Logische Variablen sind nicht explizit gespeichert
			Ucol[i].push_back( T( 1 ) );
			Urowind[i].push_back( B[i] - n );
		}
		Urowptr[i].resize( Urowind[i].size() );
	}
	avg /= ( m + 1 );	// Durchschnittliche maximale Spaltenlänge als temporäre Zeilenlänge nehmen

	for( TInt i = 0; i < m; ++i ){
		Urow[i].reserve( avg );
		Ucolind[i].reserve( avg );
		Ucolptr[i].reserve( avg );
	}

	// rowwise Kopie
	for( TInt i = 0; i < m; ++i ){
		for( TInt j = 0; j < static_cast<TInt>( Ucol[i].size() ); ++j ){
			const TInt row = Urowind[i][j];
			Urow[row].push_back( Ucol[i][j] );
			Urowptr[i][j] = Ucolind[row].size();
			Ucolind[row].push_back( i );
			Ucolptr[row].push_back( j );
		}
	}

	// temporärer Speicher, um die Pivot-Zeile speichern
	std::vector<T> Up( m );

	// temporärer Speicher für die geupdatete Zeile
	std::vector<T> Ui( m );
	// benutzte Indizes
	std::vector<TInt> Uiind( m );
	// zugehörige Länge
	TInt Uilen = 0;
	// zugehöriger erledigt-Vektor
	std::vector<bool> Uidone( m, false );

	this->Lneta = 0;
	this->Lnetaf = 0;	// TODO evtl. in this nur zurück, wenn tatsächlich faktorisiert wurde


	// verbleibende Spalten- und Zeilenindexmengen

	std::vector<bilist> PPa( m );
	bilist *PP = PPa.data();

	for( TInt i = 0; i < m; ++i ){
		PPa[i].val = i;
		PPa[i].used = true;
	}

	// Zeiger der inneren Elemente der Liste
	for( TInt i = 1; i < m - 1; ++i ){
		PPa[i].prev = &(PPa[i-1]);
		PPa[i].next = &(PPa[i+1]);
	}

	// Zeiger der Randelemente
	if( m >= 2 ){
		PPa[0].next = &(PPa[1]);
		PPa[0].prev = &(PPa[m-1]);

		PPa[m-1].next = &(PPa[0]);
		PPa[m-1].prev = &(PPa[m-2]);
	} else if( m == 1 ){
		PPa[0].next = &(PPa[0]);
		PPa[0].prev = &(PPa[0]);
	}


	std::vector<bilist> QQa( m );
	bilist* QQ = QQa.data();

	for( TInt i = 0; i < m; ++i ){
		QQa[i].val = i;
		QQa[i].used = true;
	}

	// Zeiger der inneren Elemente der Liste
	for( TInt i = 1; i < m - 1; ++i ){
		QQa[i].prev = &(QQa[i-1]);
		QQa[i].next = &(QQa[i+1]);
	}

	// Zeiger der Randelemente
	if( m >= 2 ){
		QQa[0].next = &(QQa[1]);
		QQa[0].prev = &(QQa[m-1]);

		QQa[m-1].next = &(QQa[0]);
		QQa[m-1].prev = &(QQa[m-2]);
	} else if( m == 1 ){
		QQa[0].next = &(QQa[0]);
		QQa[0].prev = &(QQa[0]);
	}


	// Number of nonzeros in Pivot-Submatrix
	std::vector<TInt> nnzCs( m );
	std::vector<TInt> nnzRs( m );

	for( TInt i = 0; i < m; ++i ){
		nnzCs[i] = Ucol[i].size();
		nnzRs[i] = Urow[i].size();
	}


	// Basis-Permutationen...

	std::vector<TInt> Bneu( m );
	std::vector<TInt> Bneu2( m );


	for( TInt k = 0; k < m; ++k ){

		// Step 1: Pivotsuche

		TInt p = -1;
		TInt q = -1;
		bool colsingleton = false;

		this->findPiv( Urowind, Ucolind, PP, QQ, PPa, QQa, nnzCs, nnzRs, p, q, colsingleton );

		if( p == -1 || q == -1 ){
			throw std::runtime_error( "This should not happen. Basis matrix singular! Contact author." );
		}


		// Step 2

		Bneu[k] = q;
		Bneu2[p] = k;

		// P-Matrix: P(k,p) = 1;
		this->perm[k] = p;
		this->permback[p] = k;


		// Step 3

		PPa[p].used = false;
		PPa[p].prev->next = PPa[p].next;
		PPa[p].next->prev = PPa[p].prev;

		if( PP == &PPa[p] ){
			PP = PP->next;
		}

		QQa[q].used = false;
		QQa[q].prev->next = QQa[q].next;
		QQa[q].next->prev = QQa[q].prev;

		if( QQ == &QQa[q] ){
			QQ = QQ->next;
		}

		// Spalten-nnz aktualisieren, hier finden einige sinnlose Anpassungen statt, diese sparen aber die Überprüfung, ob nötig
		for( TInt li = 0; li < static_cast<TInt>( Ucolind[p].size() ); ++li ){
			--nnzCs[Ucolind[p][li]];
		}


		// Zeilen-nnz muss auch in diesem Fall aktualisiert werden.
		// TODO stimmt das denn??
		if( colsingleton ){
//			--nnzRs[p];
		}


		// Step 4

		else {
			// Neuen ETA-Vektor vorbereiten
			TInt Lpos = this->Llbeg[Lnetaf];
			this->Letapos[this->Lnetaf] = p;
			++this->Lnetaf;

			// Pivot-Zeile speichern
			{
				for( TInt li = 0; li < static_cast<TInt>( Urow[p].size() ); ++li ){
					Up[Ucolind[p][li]] = Urow[p][li];
				}
			}

			for( TInt li = 0; li < static_cast<TInt>( Ucol[q].size() ); ){
				TInt i = Urowind[q][li];
				if( PPa[i].used ){

					T L = - Ucol[q][li] / Up[q];

					RationalWithInd Ltmp;
					Ltmp.value = L;
					Ltmp.ind = i;
					Llist.push_back( Ltmp );

					++Lpos;


					// Zeile Ui speichern
					{
						for( TInt ll = 0; ll < static_cast<TInt>( Urow[i].size() ); ++ll ){
							Ui[Ucolind[i][ll]] = Urow[i][ll];
							Uidone[Ucolind[i][ll]] = true;	// Vorerst ist nichts zu tun
						}
					}

					Uilen = 0;

					Ui[q] = 0;
					Uiind[Uilen++] = q;
					Uidone[q] = false;	// Dieser Wert muss ersetzt werden

					{
						bilist* QQn = QQ;
						do {
							TInt j = QQn->val;
							if( Up[j] != 0 ){
								Ui[j] += L * Up[j];
								Uiind[Uilen++] = j;
								Uidone[j] = false;	// Dieser Wert muss ersetzt werden
							}
							QQn = QQn->next;
						} while( QQn != QQ );
					}

					// Bestehende Elemente der Zeile updaten

					for( TInt ll = 0; ll < static_cast<TInt>( Urow[i].size() ); ++ll ){
						TInt j = Ucolind[i][ll];
						if( !Uidone[j] ){
							if( Ui[j] == 0 ){
								TInt cptr = Ucolptr[i][ll];

								// Letztes Element der Zeile an diese Position holen, Zeile um 1 kürzen
								TInt rowend = Urow[i].size() - 1;
								if( ll < rowend ){
									Urow[i][ll] = Urow[i][rowend];
									Ucolind[i][ll] = Ucolind[i][rowend];
									Ucolptr[i][ll] = Ucolptr[i][rowend];
									Urowptr[Ucolind[i][ll]][Ucolptr[i][ll]] = ll;
								}
								Urow[i].pop_back();
								Ucolind[i].pop_back();
								Ucolptr[i].pop_back();

								// Letztes Element der Spalte an diese Position holen, Spalte um 1 kürzen
								TInt colend = Ucol[j].size() - 1;
								if( cptr < colend ){
									Ucol[j][cptr] = Ucol[j][colend];
									Urowind[j][cptr] = Urowind[j][colend];
									Urowptr[j][cptr] = Urowptr[j][colend];
									Ucolptr[Urowind[j][cptr]][Urowptr[j][cptr]] = cptr;
								}
								Ucol[j].pop_back();
								Urowind[j].pop_back();
								Urowptr[j].pop_back();

								// nnz anpassen
								--nnzRs[i];
								--nnzCs[j];

								// Das verschobene Element muss ggf. noch geprüft werden
								--ll;
							} else {
								Urow[i][ll] = Ui[j];
								Ucol[Ucolind[i][ll]][Ucolptr[i][ll]] = Ui[j];
							}
							Uidone[j] = true;
						}
						Ui[j] = 0;
					}


					for( TInt ll = 0; ll < Uilen; ++ll ){
						TInt j = Uiind[ll];
						if( !Uidone[j] && Ui[j] != 0 ){

							// Zeilen- und Spaltenende ermitteln, Längen anpassen
							const TInt rowend = Ucolind[i].size();
							const TInt colend = Ucol[j].size();

							// An letzte Stelle der Zeile schreiben
							Urow[i].push_back( Ui[j] );
							Ucolind[i].push_back( j );
							Ucolptr[i].push_back( colend );

							// Letztes Element der Spalte an diese Position holen
							Ucol[j].push_back( Ui[j] );
							Urowind[j].push_back( i );
							Urowptr[j].push_back( rowend );

							// nnz anpassen
							if( j != q ){
								++nnzRs[i];
								++nnzCs[j];
							}
						}
						Uidone[j] = true;
						Ui[j] = 0;
					}

				} else {
					// Im obigen Fall muss das verschobene Element auch geprüft werden.
					++li;
				}
			}

			// gespeicherte Pivot-Zeile löschen
			for( TInt li = 0; li < static_cast<TInt>( Urow[p].size() ); ++li ){
				Up[Ucolind[p][li]] = 0;
			}

			Llbeg[Lnetaf] = Lpos;	// Startposition für nächsten ETA-Vektor
		}

	}

	// Alte Zerlegung löschen

	this->Urval.clear();
	this->Ucind.clear();
	this->Ucptr.clear();

	this->Ucval.clear();
	this->Urind.clear();
	this->Urptr.clear();

	this->Letas.clear();
	this->Lind.clear();

	// ETAs speichern
	{
		TInt Lsize = Llist.size();
		this->Letas.resize( Lsize+2*this->halfNumUpdateLetas*m );
		this->Lind.resize( Lsize+2*this->halfNumUpdateLetas*m );
		typename std::list<RationalWithInd>::iterator it;
		TInt pos = 0;
		for( it = Llist.begin(); it != Llist.end(); ++it ){
			RationalWithInd L = *it;
			this->Letas[pos] = L.value;
			this->Lind[pos] = L.ind;
			++pos;
		}
	}

	// Temporäre Matrix in dauerhafte kopieren

	if( m ){
		Ucbeg[0] = 0;
		Urbeg[0] = 0;
		for( TInt i = 1; i < m; ++i ){
			Ucbeg[i] = Ucbeg[i-1] + Ucol[i-1].size();
			Urbeg[i] = Urbeg[i-1] + Urow[i-1].size() + 2 * this->halfNumUpdateLetas;
		}
		this->Ucfreepos = Ucbeg[m-1] + Ucol[m-1].size();
	} else {
		this->Ucfreepos = 0;
	}
	TInt arraysize = this->Ucfreepos + 2 * m * this->halfNumUpdateLetas + 1;
	this->Urval.resize( arraysize );
	this->Ucind.resize( arraysize );
	this->Ucptr.resize( arraysize );

	this->Ucval.resize( arraysize );
	this->Urind.resize( arraysize );
	this->Urptr.resize( arraysize );


	for( TInt i = 0; i < m; ++i ){
		// Spaltenweise
		{
			const TInt len = Ucol[i].size();
			this->Uclen[i] = len;
			const TInt cbeg = this->Ucbeg[i];
			for( TInt ll = 0; ll < len; ++ll ){
				TInt pos = cbeg + ll;
				this->Ucval[pos] = Ucol[i][ll];
				this->Urind[pos] = Urowind[i][ll];
				this->Urptr[pos] = Urowptr[i][ll] + this->Urbeg[Urowind[i][ll]];
			}

		}

		// Zeilenweise
		{
			const TInt len = Urow[i].size();
			this->Urlen[i] = len;
			TInt rbeg = this->Urbeg[i];
			for( TInt ll = 0; ll < len; ++ll ){
				TInt pos = rbeg + ll;
				this->Urval[pos] = Urow[i][ll];
				this->Ucind[pos] = Ucolind[i][ll];
				this->Ucptr[pos] = Ucolptr[i][ll] + this->Ucbeg[Ucolind[i][ll]];
			}

		}
	}


	{

		// Basis anpassen
		std::vector<TInt> tmpBase( m );
		std::vector<TInt> Ucbegold( m );
		std::vector<TInt> Uclenold( m );
		for( TInt i = 0; i < m; ++i ){
			// Bneu[Bneu2[i]]
			tmpBase[i] = this->B[Bneu[Bneu2[i]]];
			Ucbegold[i] = this->Ucbeg[i];
			Uclenold[i] = this->Uclen[i];
		}
		for( TInt i = 0; i < m; ++i ){
			if( this->Ucbeg[i] != Ucbegold[Bneu[Bneu2[i]]] ){
				this->Ucbeg[i] = Ucbegold[Bneu[Bneu2[i]]];
				this->Uclen[i] = Uclenold[Bneu[Bneu2[i]]];
				for( TInt j = 0; j < this->Uclen[i]; ++j ){
					this->Ucind[this->Urptr[this->Ucbeg[i]+j]] = i;
				}
			}

			this->B[i] = tmpBase[i];
			this->Binv[tmpBase[i]] = i;
		}

	}


	// Spalten-Diagonalelemente an erste Stellen holen
	for( TInt j = 0; j < m; ++j ){
		const TInt cbeg = this->Ucbeg[j];
		const TInt cend = cbeg + this->Uclen[j];

		for( TInt k = cbeg; k < cend; ++k ){
			if( this->Urind[k] == j ){

				{
					T tmpcval = this->Ucval[cbeg];
					this->Ucval[cbeg] = this->Ucval[k];
					this->Ucval[k] = tmpcval;
				}

				{
					TInt tmprind = this->Urind[cbeg];
					this->Urind[cbeg] = this->Urind[k];
					this->Urind[k] = tmprind;
				}

				{
					TInt tmprptr = this->Urptr[cbeg];
					this->Urptr[cbeg] = this->Urptr[k];
					this->Urptr[k] = tmprptr;
				}

				this->Ucptr[this->Urptr[cbeg]] = cbeg;
				this->Ucptr[this->Urptr[k]] = k;

				break;
			}
		}
	}

	// Zeilen-Diagonalelemente an erste Stellen holen
	for( TInt i = 0; i < m; ++i ){
		const TInt rbeg = this->Urbeg[i];

		const TInt k = this->Urptr[this->Ucbeg[i]];	// Position aus Spalten-Diagonalelement auslesen

		{
			T tmpval = this->Urval[rbeg];
			this->Urval[rbeg] = this->Urval[k];
			this->Urval[k] = tmpval;
		}

		{
			TInt tmpcind = this->Ucind[rbeg];
			this->Ucind[rbeg] = this->Ucind[k];
			this->Ucind[k] = tmpcind;
		}

		{
			TInt tmpcptr = this->Ucptr[rbeg];
			this->Ucptr[rbeg] = this->Ucptr[k];
			this->Ucptr[k] = tmpcptr;
		}

		this->Urptr[this->Ucptr[rbeg]] = rbeg;
		this->Urptr[this->Ucptr[k]] = k;

	}


	this->Lneta = this->Lnetaf;

	this->hasBasisMatrix = true;

	return true;

}


template <class T, class TInt>
void TOSolver<T, TInt>::updateB( TInt r, T* permSpike, TInt* permSpikeInd, TInt* permSpikeLen ){

	// zu ersetzende Spalte aus Basismatrix löschen (außer Diagonalelement, das wird sowieso ersetzt)
	// Diagonalelement vorübergehend auf 0 setzen, statt zu löschen
	this->Urval[this->Urbeg[r]] = 0;

	TInt colend = this->Ucbeg[r] + this->Uclen[r];
	for( TInt i = this->Ucbeg[r] + 1; i < colend; ++i ){
		TInt rind = this->Urind[i];
		TInt rptr = this->Urptr[i];

		// Letztes Element der Zeile an diese Position holen, Zeile um 1 kürzen
		TInt rowend = this->Urbeg[rind] + --this->Urlen[rind];
		if( rptr < rowend ){
			this->Urval[rptr] = this->Urval[rowend];
			this->Ucind[rptr] = this->Ucind[rowend];
			this->Ucptr[rptr] = this->Ucptr[rowend];
			this->Urptr[this->Ucptr[rptr]] = rptr;
		}
	}

	// permuted Spike hinzufügen
	colend = this->Ucfreepos;
	this->Ucbeg[r] = colend;
	for( TInt i = 0; i < *permSpikeLen; ++i ){
		TInt rind = permSpikeInd[i];
		if( rind == r ){
			// Diagonalelement
			this->Ucval[this->Ucbeg[r]] = permSpike[i];
			this->Urind[this->Ucbeg[r]] = rind;
			this->Urptr[this->Ucbeg[r]] = this->Urbeg[r];
			this->Urval[this->Urbeg[r]] = permSpike[i];
			this->Ucptr[this->Urbeg[r]] = this->Ucbeg[r];
		} else {
			// Zeilen- und Spaltenende ermitteln, Längen anpassen
			++colend;
			TInt rowend = this->Urbeg[rind] + this->Urlen[rind]++;

			// An letzte Stelle der Zeile schreiben
			this->Urval[rowend] = permSpike[i];
			this->Ucind[rowend] = r;
			this->Ucptr[rowend] = colend;

			// An letzte Stelle der Spalte schreiben
			this->Ucval[colend] = permSpike[i];
			this->Urind[colend] = rind;
			this->Urptr[colend] = rowend;
		}
	}
	this->Uclen[r] = colend - this->Ucbeg[r] + 1;
	this->Ucfreepos += this->Uclen[r];

	TInt t = this->permback[r];

	// Zeile Ur kopieren und dabei aus Matrix löschen (bis auf Diagonalelement)
	std::vector<T> Ur(m);	// TODO globale Arbeitsvariable machen und am Ende immer auf 0 setzen (ist automatisch 0), macht vermutlich wenig Sinn

	Ur[r] = Urval[this->Urbeg[r]];	// Diagonalelement kopieren um unten eine if-Abfrage zu vermeiden

	{
		TInt kend = this->Urbeg[r] + this->Urlen[r];
		for( TInt i = this->Urbeg[r] + 1; i < kend; ++i ){
			TInt cind = Ucind[i];
			Ur[cind] = Urval[i];

			TInt cptr = Ucptr[i];

			// Letztes Element der Spalte an diese Position holen, Zeile um 1 kürzen
			colend = this->Ucbeg[cind] + --this->Uclen[cind];
			if( cptr < colend ){
				this->Ucval[cptr] = this->Ucval[colend];
				this->Urind[cptr] = this->Urind[colend];
				this->Urptr[cptr] = this->Urptr[colend];
				this->Ucptr[this->Urptr[cptr]] = cptr;
			}
		}
	}
	this->Urlen[r] = 1;


	this->Llbeg[this->Lneta+1] = this->Llbeg[this->Lneta];	// Neuen ETA-Vektor hinzufügen
	this->Letapos[this->Lneta++] = r;

	for( TInt k = t+1; k < m; ++k ){
		TInt i = this->perm[k];

		if( Ur[i] != 0 ){
			T L = - Ur[i] / this->Urval[Urbeg[i]];
			this->Letas[this->Llbeg[this->Lneta]] = L;
			this->Lind[this->Llbeg[this->Lneta]++] = i;

			Ur[i] = 0;

			const TInt llend = this->Urbeg[i] + this->Urlen[i];
			for( TInt ll = this->Urbeg[i] + 1; ll < llend; ++ll ){	// +1, weil ohne Diagonalelement
				Ur[Ucind[ll]] += L * this->Urval[ll];
			}
		}

	}

	// Bei Forrest-Tomlin ist hier nur noch das Diagonalelement != 0
	this->Urval[this->Urbeg[r]] = Ucval[this->Ucbeg[r]] = Ur[r];
	Ur[r] = 0;


	// TODO Das geht angeblich sogar in konstanter Zeit??
	TInt tmp = this->perm[t];
	for( TInt k = t; k < m - 1; ++k ){
		this->perm[k] = this->perm[k+1];
	}
	this->perm[m-1] = tmp;

	// TODO update-formel hierfür verwenden
	for( TInt i = 0; i < m; ++i ){
		this->permback[this->perm[i]] = i;
	}

}


template <class T, class TInt>
TInt TOSolver<T, TInt>::phase1(){

	#ifndef TO_DISABLE_OUTPUT
		std::cout << "Duale Phase 1 gestartet." << std::endl;
	#endif

	std::vector<TORationalInf<T> > ltmp( n + m );
	std::vector<TORationalInf<T> > utmp( n + m );

	this->l = ltmp.data();
	this->u = utmp.data();

	TORationalInf<T> zero;
	TORationalInf<T> m1;
	m1.value = -1;
	TORationalInf<T> one;
	one.value = 1;


	// Schranken entsprechend Phase 1 anpassen

	for( TInt i = 0; i < n+m; ++i ){
		if( lvec[i].isInf && uvec[i].isInf ){
			this->l[i] = m1;
			this->u[i] = one;
		} else if( lvec[i].isInf && !uvec[i].isInf ){
			this->l[i] = m1;
			this->u[i] = zero;
		} else if( !lvec[i].isInf && uvec[i].isInf ) {
			this->l[i] = zero;
			this->u[i] = one;
		} else {
			this->l[i] = zero;
			this->u[i] = zero;
		}
	}


	TInt retval = -1;

	// Phase 1 - Problem lösen
	if( this->opt( true ) >= 0 ){
		// Zielfunktionswert bestimmen	// TODO später auslesen, falls wir den Wert zwischenspeichern/zurückgeben?
		T Z( 0 );
		for( TInt i = 0; i < n; ++i ){
			Z += c[i] * x[i];
		}
		retval = Z == 0 ? 0 : 1;
	}

	// Schranken wiederherstellen
	this->u = uvec.data();
	this->l = lvec.data();

	return retval;
}


template <class T, class TInt>
inline clock_t TOSolver<T, TInt>::optExternal(){
	// has to be implemented separately for specific classes
	return 0;
}


template <class T, class TInt>
TInt TOSolver<T, TInt>::opt(){

	#ifndef TO_DISABLE_OUTPUT
		clock_t externalTime = optExternal();
	#else
		optExternal();
	#endif

	if( this->checkDualFarkas() ){
		#ifndef TO_DISABLE_OUTPUT
			std::cout << "DualFarkas ok! No optimization needed. Problem is infeasible." << std::endl;
		#endif

		// x und d berechnen

		{
			std::vector<T> btilde( m, T( 0 ) );

			for( TInt i = 0; i < n; ++i ){
				const TInt j = N[i];
				T xj = x[j];
				if( j < n ){
					const TInt cend = Acolpointer[j+1];
					for( TInt k = Acolpointer[j]; k < cend; ++k ){
						btilde[Acolwiseind[k]] -= Acolwise[k] * xj;
					}
				} else {
					// logische Variable
					btilde[j-n] -= xj;
				}

			}

			this->FTran( btilde.data() );
			for( TInt i = 0; i < m; ++i ){
				x[B[i]] = btilde[i];
			}
		}

		{

			std::vector<T> y( m, T( 0 ) );

			for( TInt i = 0; i < m; ++i ){
				if( B[i] < n ){
					y[i] = this->c[B[i]];
				} else {
					y[i] = T( 0 );
				}
			}
			this->BTran( y.data() );


			std::vector<T> tmp( n, T( 0 ) );

			this->mulANT( tmp.data(), y.data() );

			for( TInt i = 0; i < n; ++i ){
				if( N[i] < n ){
					d[i] =  this->c[N[i]] - tmp[i];
				} else {
					d[i] = - tmp[i];
				}
			}
		}

		return 1;
	}

	#ifndef TO_DISABLE_OUTPUT
		clock_t fulltime = clock();
	#endif

	if( !this->hasBase || ( !this->hasBasisMatrix && !this->refactor() ) ){

		// DSE-Gewichte für logische Basis initialisieren.
		this->DSE.clear();
		this->DSEtmp.clear();
		this->DSE.resize( m, T( 1 ) );
		this->DSEtmp.resize( m + n );

		// logische Basis setzen
		for( TInt j = 0; j < m; ++j ){
			this->B[j] = n+j;
			this->Binv[n+j] = j;
			this->Ninv[n+j] = -1;
		}

		for( TInt j = 0; j < n; ++j ){
			this->N[j] = j;
			this->Binv[j] = -1;
			this->Ninv[j] = j;
		}

		this->hasBase = true;

		this->refactor();
	}

	TInt retval = -1;

	do{
		retval = this->opt( false );

		// Perturbation
		if( retval == -1 ){

			#ifndef TO_DISABLE_OUTPUT
				std::cout << "Perturbiere." << std::endl;
			#endif

			T cmin( 1 );

			for( TInt i = 0; i < n; ++i ){
				if( this->c[i] != 0 ){
					if( c[i] < cmin && -c[i] < cmin ){
						cmin = c[i] >= 0 ? c[i] : -c[i];
					}
				}
			}

			std::vector<T> cold = this->c;
			this->c.clear();
			this->c.reserve( n );

			for( TInt i = 0; i < n; ++i ){
				this->c.push_back( cold[i] + cmin / ( T( 10000 ) + T( n ) + T( i ) ) );
			}

			this->hasPerturbated = true;

			TORationalInf<T> infeasibilityBoundOld = this->infeasibilityBound;
			this->infeasibilityBound = true;

			this->opt( false );

			#ifndef TO_DISABLE_OUTPUT
				std::cout << "Ende Perturbierung." << std::endl;
			#endif

			this->infeasibilityBound = infeasibilityBoundOld;

			this->c = cold;
		}
	} while( retval == -1 );


	#ifndef TO_DISABLE_OUTPUT
		std::cout << "Zeit externer Löser (sofern eingebunden): " << externalTime / (double) CLOCKS_PER_SEC << std::endl;
		std::cout << "Optimierungszeit: " << ( ( clock() - fulltime ) / (double) CLOCKS_PER_SEC ) << " Sekunden" << std::endl;
	#endif

	if( !retval ){
		this->rayGuess.clear();
		this->farkasProof.clear();
	}

	return retval;
}


template <class T, class TInt>
TInt TOSolver<T, TInt>::opt( bool P1 ){

	#ifndef TO_DISABLE_OUTPUT
		std::cout << "Optimiere..." << std::endl;
		clock_t starttime = clock();
	#endif

	// Widersprüchliche Schranken rausfiltern
	for( TInt i = 0; i < n + m; ++i ){
		if( !l[i].isInf && !u[i].isInf && l[i].value > u[i].value ){
			#ifndef TO_DISABLE_OUTPUT
				std::cout << "Infeasible (widersprüchliche Schranken)." << std::endl;
			#endif
			return 1;
		}
	}

	this->lastLeavingBaseVar = -1;


	// Step 1: Initialization
	std::vector<T> y( m );

	bool feas = true;

	do {

		// Nichtbasisvariablen in ihre Schranken weißen!
		for( TInt i = 0; i < n; ++i ){
			const TInt j = this->N[i];
			if( !l[j].isInf && !u[j].isInf ){
				if( x[j] != l[j].value && x[j] != u[j].value ){
					x[j] = l[j].value;
				}
			} else if( !l[j].isInf && u[j].isInf && x[j] != l[j].value ){
				x[j] = l[j].value;
			} else if( l[j].isInf && !u[j].isInf && x[j] != u[j].value ){
				x[j] = u[j].value;
			} else if( l[j].isInf && u[j].isInf && x[j] != 0 ){
				x[j] = 0;
			}
		}

		bool dfcdone = false;
		do {
			{
				std::vector<T> btilde( m );

				for( TInt i = 0; i < n; ++i ){
					const TInt j = N[i];
					T xj = x[j];
					if( j < n ){
						const TInt cend = Acolpointer[j+1];
						for( TInt k = Acolpointer[j]; k < cend; ++k ){
							btilde[Acolwiseind[k]] -= Acolwise[k] * xj;
						}
					} else {
						// logische Variable
						btilde[j-n] -= xj;
					}

				}

				this->FTran( btilde.data() );
				for( TInt i = 0; i < m; ++i ){
					x[B[i]] = btilde[i];
				}
			}

			{
				for( TInt i = 0; i < m; ++i ){
					if( B[i] < n ){
						y[i] = this->c[B[i]];
					} else {
						y[i] = 0;
					}
				}
				this->BTran( y.data() );


				std::vector<T> tmp( n, T( 0 ) );

				this->mulANT( tmp.data(), y.data() );

				for( TInt i = 0; i < n; ++i ){
					if( N[i] < n ){
						d[i] =  this->c[N[i]] - tmp[i];
					} else {
						d[i] = - tmp[i];
					}
				}
			}

			dfcdone = false;
			for( TInt i = 0; i < n; ++i ){
				const TInt j = this->N[i];
				if( !l[j].isInf && !u[j].isInf && l[j].value != u[j].value ){
					if( x[j] == l[j].value && d[i] < 0 ){
						x[j] = u[j].value;
						dfcdone = true;
					} else if( x[j] == u[j].value && d[i] > 0 ){
						x[j] = l[j].value;
						dfcdone = true;
					}
				}
			}

			#ifndef TO_DISABLE_OUTPUT
				if( dfcdone ){
					std::cout << "DFC done." << std::endl;
				}
			#endif
		} while( dfcdone );

		{
			feas = true;
			for( TInt i = 0; i < n ; ++i ){
				TInt j = N[i];
				if( !l[j].isInf && !u[j].isInf && l[j].value == u[j].value ){
					continue;
				}
				if( !l[j].isInf && l[j].value == x[j] && d[i] < 0 ){
					feas = false;
					break;
				}
				if( !u[j].isInf && u[j].value == x[j] && d[i] > 0 ){
					feas = false;
					break;
				}
				if( l[j].isInf && u[j].isInf && x[j] == 0 && d[i] != 0 ){
					feas = false;
					break;
				}
			}
			if( !feas ){
				#ifndef TO_DISABLE_OUTPUT
					std::cout << "Basis dual infeasible. Starte Phase 1." << std::endl << std::endl;
				#endif
				if( P1 ){
					// Should not happen
					throw std::runtime_error( "Phase 1 rekursiv aufgerufen!" );
				}
				TInt p1retval = this->phase1();
				if( p1retval > 0 ){
					#ifndef TO_DISABLE_OUTPUT
						std::cout << "Phase 1: LP dual infeasible!" << std::endl << std::endl;
					#endif
					return 2;
				}
				if( p1retval == -1 ){
					return -1;
				}

				#ifndef TO_DISABLE_OUTPUT
					std::cout << "Phase 1 fertig." << std::endl << std::endl;
				#endif
			}
		}
	} while( !feas );



	std::vector<T> tau(m);

	// Arbeitsvariablen für Ratio-Test
	std::vector<T> Q(n);
	std::vector<TInt> Qind(n);
	std::vector<TInt> Qord(n);
	std::vector<T> alphartilde(n);
	T deltatilde( 0 );

	TInt iter = 0;

	#ifndef TO_DISABLE_OUTPUT
		clock_t itertime = clock();
	#endif

	T Zold( 0 );
	TInt cyclecounter = 0;
	TInt nocyclecounter = 0;
	this->antiCycle = false;

	bool dualUnbounded = false;
	bool infeasibleDueToBound = false;

	clock_t time1;
	clock_t time2 = clock();

	clock_t time_pricing = 0;
	clock_t time_btran = 0;
	clock_t time_dse = 0;
	clock_t time_pivot = 0;
	clock_t time_ratio = 0;
	clock_t time_ftran = 0;
	clock_t time_update = 0;

	T Z( 0 );

	while( true ){

		#ifndef TO_DISABLE_OUTPUT
			clock_t oldtime = itertime;
			itertime = clock();
		#endif

		// TODO weq, mit Updateformel updaten
		Z = 0;
		for( TInt i = 0; i < n; ++i ){
			Z += c[i] * x[i];
		}

		if( !this->infeasibilityBound.isInf && Z >= this->infeasibilityBound.value ){
			infeasibleDueToBound = true;
			break;
		}

		// Kreiselvermeidung
		if( Z == Zold && ++cyclecounter > 5 && !this->antiCycle ){
			#ifndef TO_DISABLE_OUTPUT
				std::cout << "Starte Routine zur Vermeidung von Kreiseln." << std::endl;
			#endif
			this->antiCycle = true;
		} else if( Z != Zold ){
			cyclecounter = 0;
			if( this->antiCycle ){
				++nocyclecounter;
				if( nocyclecounter > 5 ){
					#ifndef TO_DISABLE_OUTPUT
						std::cout << "Beende Routine zur Vermeidung von Kreiseln." << std::endl;
					#endif
					this->antiCycle = false;
					nocyclecounter = 0;
				}
			}
		}

		// Perturbieren
		if( !this->hasPerturbated && cyclecounter > 25 ){
			return -1;
		}

		Zold = Z;


		if( !( iter++ % 10 ) )
		{
			#ifndef TO_DISABLE_OUTPUT
				std::string info = P1 ? "(Duale Phase 1) Duale Unzulässigkeit: " : "(Duale Phase 2) Primaler Zielfunktionswert: ";
				std::cout << "Iteration " << iter << ".\t" << info << TOmath<T>::toShortString( Z ) << std::endl;
				std::cout << "Dauer: " << ( itertime - oldtime ) / (double) CLOCKS_PER_SEC << " s." << std::endl;
			#endif
		}


		// Obiges nicht mitmessen
		time1 = time2;
		time2 = clock();


		// Step 2: Pricing
//		std::cout << "Pricing" << std::endl;

		TInt r = 0;
		TInt p = m+n;
		T delta;
		bool ratioNeg;

		if( this->DSE.size() && !this->antiCycle ){
			T max( 0 );
			for( TInt i = 0; i < m; ++i ){
				const TInt b = this->B[i];
				T deltai( 0 );
				if( !this->l[b].isInf && this->x[b] < this->l[b].value ){
					deltai =  this->l[b].value - this->x[b];
				} else if( !this->u[b].isInf && this->x[b] > this->u[b].value ){
					deltai = this->u[b].value - this->x[b];
				}
				if( deltai != 0 ){
					T tmp = deltai * deltai / this->DSE[i];
					if( tmp > max ){
						max = tmp;
						r = i;
						p = b;
					}
				}
			}
		} else {
			// Bland
			for( TInt i = 0; i < m; ++i ){
				const TInt b = this->B[i];
				if( b < p ){
					if( !this->l[b].isInf && this->x[b] < this->l[b].value ){
						r = i;
						p = b;
					} else if( !this->u[b].isInf && this->x[b] > this->u[b].value ){
						r = i;
						p = b;
					}
				}
			}
		}

		if( p == m+n ){
			#ifndef TO_DISABLE_OUTPUT
				std::cout << "OPTIMAL!!!" << std::endl;
			#endif
			break;
		} else {

			delta = x[p];
			if( !this->l[p].isInf && this->x[p] < this->l[p].value ){
				delta -= this->l[p].value;
				ratioNeg = true;
			} else {
				delta -= this->u[p].value;
				ratioNeg = false;
			}

//			std::cout << "(Noch) nicht optimal." << std::endl;
		}


		time1 = time2;
		time2 = clock();

		time_pricing += time2-time1;

		// Step 3: BTran
//		std::cout << "BTran" << std::endl;

		std::vector<T> rhor(m);
		rhor[r] = 1;

		this->BTran( rhor.data() );

		time1 = time2;
		time2 = clock();

		time_btran += time2-time1;

		TInt q = 0;
		TInt s = 0;
		std::vector<T> permSpike( m );	// TODO global, damit Konstruktor nicht immer aufgerufen wird
		std::vector<TInt> permSpikeInd( m );
		TInt permSpikeLen = 0;

		#pragma omp parallel
		{
			#pragma omp master
			{

				// DSE-FTran-Thread starten
				if( this->DSE.size() ){
					for( TInt i = 0; i < m; ++i ){
						tau[i] = rhor[i];
					}

					#pragma omp task
					FTran( tau.data() );	// DSE-FTran
				}


				time1 = time2;
				time2 = clock();

				time_dse += time2-time1;



				// Step 4: Pivot row
		//		std::cout << "Pivot row" << std::endl;

				std::vector<T> alphar( n, T( 0 ) );

				this->mulANT( alphar.data(), rhor.data() );

				time1 = time2;
				time2 = clock();

				time_pivot += time2-time1;



				// Step 5: Ratio Test
		//		std::cout << "Ratio Test" << std::endl;

				if( ratioNeg ){
					for( TInt i = 0; i < n; ++i ){
						alphartilde[i] = - alphar[i];
						deltatilde = - delta;
					}
				} else {
					for( TInt i = 0; i < n; ++i ){
						alphartilde[i] = alphar[i];
						deltatilde = delta;
					}
				}


				std::vector<T> atilde(m);
				bool flip = false;
				{
					TInt Qlen = 0;
					for( TInt i = 0; i < n; ++i ){
						TInt j = this->N[i];
						if(
								( this->l[j].isInf && this->u[j].isInf && alphartilde[i] != 0 )
								|| ( ( this->u[j].isInf || this->u[j].value != this->x[j] ) && !this->l[j].isInf && this->x[j] == this->l[j].value && alphartilde[i] > 0 )
								|| ( ( this->l[j].isInf || this->l[j].value != this->x[j] ) && !this->u[j].isInf && this->x[j] == this->u[j].value && alphartilde[i] < 0 )
						){

							Qind[Qlen] = i;
							Qord[Qlen] = Qlen;
							Q[Qlen] = d[i] / alphartilde[i];
							++Qlen;

						}
					}


					if( !Qlen ){
						// Aufhören, weil dual unbeschränkt
						this->lastLeavingBaseVar = p;
						dualUnbounded = true;
					} else {
						if( this->antiCycle ){	// Bland

							T min = Q[0];
							s = Qind[0];

							for( TInt i = 1; i < Qlen; ++i ){
								if( Q[i] < min ){
									min = Q[i];
									s = Qind[i];
								} else if( Q[i] == min && N[Qind[i]] < N[s] ){
									s = Qind[i];
								}
							}

							q = N[s];

						} else {

							// Q absteigend sortieren
							{
								ratsort sorter( Q );
								std::sort( Qord.data(), Qord.data() + Qlen, sorter );
							}


							// BFRT, inkl. Bound-Flips
							while( Qlen && deltatilde >= 0 ){

								s = Qind[Qord[--Qlen]];
								q = N[s];
								if( this->l[q].isInf || this->u[q].isInf ){
									break;
								} else {
									T tmp = deltatilde - ( this->u[q].value - this->l[q].value ) * ( alphar[s] >= 0 ? alphar[s] : - alphar[s] );
									if( tmp < 0 ){
										break;
									}

									if( x[q] == l[q].value ){
										x[q] = u[q].value;
										flip = true;


										T diff = u[q].value - l[q].value;
										if( q < n ){
											TInt kend = Acolpointer[q+1];
											for( TInt k = Acolpointer[q]; k < kend; ++k ){
												atilde[Acolwiseind[k]] += diff * Acolwise[k];
											}
										} else {
											atilde[q-n] += diff;
										}


									} else {
										x[q] = l[q].value;
										flip = true;

										T diff = l[q].value - u[q].value;
										if( q < n ){
											TInt kend = Acolpointer[q+1];
											for( TInt k = Acolpointer[q]; k < kend; ++k ){
												atilde[Acolwiseind[k]] += diff * Acolwise[k];
											}
										} else {
											atilde[q-n] += diff;
										}

									}

									deltatilde = tmp;
								}
							}


							if( ratioNeg ){
								delta = -deltatilde;
							} else {
								delta = deltatilde;
							}


						}
					}
				}

				if( !dualUnbounded ){

					time1 = time2;
					time2 = clock();

					time_ratio += time2-time1;


					// TODO in extra Thread auslagern?
					// Step 6: FTran
			//		std::cout << "FTran" << std::endl;

					std::vector<T> alphaq(m);
					if( q < n ){
						const TInt kend = this->Acolpointer[q+1];
						for( TInt k = this->Acolpointer[q]; k < kend; ++k ){
							alphaq[Acolwiseind[k]] = Acolwise[k];
						}
					} else {
						alphaq[q-n] = 1;
					}

					this->FTran( alphaq.data(), permSpike.data(), permSpikeInd.data(), &permSpikeLen );


					time1 = time2;
					time2 = clock();

					time_ftran += time2-time1;



					// Step 7: Basis change and update
			//		std::cout << "Update" << std::endl;

					T thetaD = d[s] / alphar[s];

					for( TInt i = 0; i < n; ++i ){
						d[i] -= thetaD * alphar[i];
					}

					d[s] = -thetaD;

					if( flip )
					{
			//			std::cout << "FLIP!!!" << std::endl;
						FTran( atilde.data() );
						for( TInt i = 0; i < m; ++i ){
							x[B[i]] -= atilde[i];
						}
					}


					T thetaP = delta / alphaq[r];

					for( TInt i = 0; i < m; ++i ){
						x[this->B[i]] -= thetaP * alphaq[i];
					}


					x[q] += thetaP;

					// Update DSE
					if( this->DSE.size() ){
						T betar = this->DSE[r];
						this->DSE[r] = betar / ( alphaq[r] * alphaq[r]);

						time1 = time2;
						time2 = clock();
						time_update += time2-time1;

						// Auf DSE-FTran warten
						#pragma omp taskwait

						time1 = time2;
						time2 = clock();
						time_dse += time2-time1;


						T mult;
						for( TInt i = 0; i < m; ++i ){
							if( i != r ) {
								mult = alphaq[i] / alphaq[r];
								this->DSE[i] += - 2 * mult * tau[i] + mult * mult * betar;
							}
						}
					}
				}
			}
		}

		if( dualUnbounded ){
			break;
		}

		this->B[r] = q;
		this->Binv[q] = r;
		this->Binv[p] = -1;

		this->N[s] = p;
		this->Ninv[p] = s;
		this->Ninv[q] = -1;


		if( ++this->baseIter % this->halfNumUpdateLetas ){
			this->updateB( r, permSpike.data(), permSpikeInd.data(), &permSpikeLen );
		} else {

			// DSE zwischenspeichern
			if( this->DSE.size() ){
				for( TInt i = 0; i < m; ++i ){
					this->DSEtmp[B[i]] = this->DSE[i];
				}
			}

			this->refactor();

			// DSE für permutierte Basis zurückholen
			if( this->DSE.size() ){
				for( TInt i = 0; i < m; ++i ){
					this->DSE[i] = this->DSEtmp[B[i]];
				}
			}

			#ifndef TO_DISABLE_OUTPUT
				clock_t time_sum = time_pricing + time_btran + time_dse + time_pivot + time_ratio + time_ftran + time_update;

				if( time_sum > 0) {
					std::cout << "Pricing: " << 100 * time_pricing / time_sum << std::endl;
					std::cout << "BTran: " << 100 * time_btran / time_sum << std::endl;
					std::cout << "DSE-FTran: " << 100 * time_dse / time_sum << std::endl;
					std::cout << "Pivot: " << 100 * time_pivot / time_sum << std::endl;
					std::cout << "Ratio Test: " << 100 * time_ratio / time_sum << std::endl;
					std::cout << "Ftran: " << 100 * time_ftran / time_sum << std::endl;
					std::cout << "Update: " << 100 * time_update / time_sum << std::endl;
				}
			#endif

		}


		if( !this->DSE.size() && 4 * iter > m ){
			// DSE-Gewichte bestimmen
			this->DSE.clear();
			this->DSEtmp.clear();

			this->DSE.resize( m );
			this->DSEtmp.resize( m+n );

			#pragma omp parallel for
			for( TInt weight = 0; weight < m; ++weight )
			{
					std::vector<T> rhoi(m);
					rhoi[weight] = 1;
					BTran( rhoi.data() );
					for( TInt j = 0; j < m; ++j ){
						DSE[weight] += rhoi[j] * rhoi[j];
					}
			}
		}


		time1 = time2;
		time2 = clock();

		time_update += time2-time1;
		
	}
	

	#ifndef TO_DISABLE_OUTPUT
		if( dualUnbounded ){
			std::cout << "dual unbounded" << std::endl;
		} else if( infeasibleDueToBound ){
			std::cout << "infeasibility bound reached" << std::endl;
			std::cout << "Cutoff: " << TOmath<T>::toShortString( Z ) << " >= " << TOmath<T>::toShortString( this->infeasibilityBound.value ) << std::endl;
		} else {
			Z = 0;
			for( TInt i = 0; i < n; ++i ){
				Z += c[i] * x[i];
			}
			std::cout << "Zielfunktionswert: Exakt: " << Z << std::endl;
			std::cout << "Zielfunktionswert: gekuerzt: " << TOmath<T>::toShortString( Z ) << std::endl;
			this->showOptValDetails( Z );
		}

		std::cout << iter << " Iterationen" << std::endl;

		std::cout << "Zeit: " << ( ( clock() - starttime ) / (double) CLOCKS_PER_SEC ) << " Sekunden" << std::endl;
	#endif

	if( infeasibleDueToBound ){
		return 3;
	}

	if( dualUnbounded ){
		return 1;
	}

	return 0;

}


template <class T, class TInt>
std::vector<T> TOSolver<T, TInt>::getX(){
	std::vector<T> retx = this->x;
	retx.resize( n );
	return retx;
}


template <class T, class TInt>
std::vector<T> TOSolver<T, TInt>::getY(){
	std::vector<T> y(m);
	for( TInt i = 0; i < m; ++i ){
		if( B[i] < n ){
			y[i] = this->c[B[i]];
		} else {
			y[i] = 0;
		}
	}
	this->BTran( y.data() );

	return y;
}


template <class T, class TInt>
std::vector<T> TOSolver<T, TInt>::getD(){
	std::vector<T> retd( n );
	for( TInt i = 0; i < n; ++i ){
		if( N[i] < n ){
			retd[N[i]] = d[i];
		}
	}
	return retd;
}


template <class T, class TInt>
std::pair<TORationalInf<T>,TORationalInf<T> > TOSolver<T, TInt>::getConstraintBounds( const TInt i ){
	std::pair<TORationalInf<T>,TORationalInf<T> > retval;
	const TInt s = n + i;
	if( this->u[s].isInf ){
		retval.first = true;
	} else {
		retval.first = TORationalInf<T>( -this->u[s].value );
	}
	if( this->l[s].isInf ){
		retval.second = true;
	} else {
		retval.second = TORationalInf<T>( -this->l[s].value );
	}
	return retval;
}


template <class T, class TInt>
inline std::pair<std::vector<T>, T> TOSolver<T, TInt>::getGMI( TInt ind, std::vector<bool> intVars, T kcut ){

	if( ind >= n+m ){
		throw std::runtime_error( "Invalid index" );
	}

	if( kcut <= T( 0 ) || !TOmath<T>::isInt( kcut ) ){
		throw std::runtime_error( "Invalid k." );
	}

	std::vector<T> tmp( m, T( 0 ) );
	tmp[ Binv[ ind ] ] = 1;
	BTran( tmp.data() );
	std::vector<T> gmicoeff( n, T( 0 ) );
	mulANT( gmicoeff.data(), tmp.data() );


	// Generate GMI
	T ai0;
	for( TInt k = 0; k < n; ++k ){
		TInt nind = N[k];
		if( !l[nind].isInf && x[nind] == l[nind].value ){
			// Lower Bound
			ai0 -= gmicoeff[k] * l[nind].value;
//			s[nind] = x[nind] - l[nind].value;
		} else if( !u[nind].isInf && x[nind] == u[nind].value ) {
			// Upper Bound
			ai0 -= gmicoeff[k] * u[nind].value;
			gmicoeff[k] *= -1;
//			s[nind] = u[nind].value - x[nind];
		} else {
//			s[nind] = x[nind];
		}
	}


	// Cut from row
	std::vector<T> cutcoeff(n);
	T fi0 = ai0 * kcut - TOmath<T>::floor( ai0 * kcut );
	if( fi0 == 0 ){
		throw std::runtime_error( "Separation impossible." );
	}
	for( TInt k = 0; k < n; ++k ){
		TInt nind = N[k];
		T & aij = gmicoeff[k];
		if( nind < static_cast<TInt>( intVars.size() ) && intVars[nind] ){
			// Integer nonbasic Variables
			T fij = aij * kcut - TOmath<T>::floor( aij * kcut );
			if( fij <= fi0 ){
				cutcoeff[k] = fij;
			} else {
				cutcoeff[k] = fi0*(1-fij)/(1-fi0);
			}
		} else {
			// Real nonbasic variables
			if( aij >= 0 ){
				cutcoeff[k] = aij * kcut;
			} else {
				cutcoeff[k] = fi0 * ( -aij * kcut ) / ( 1 - fi0 );
			}
		}
	}

	// Cut zurückrechnen
	T beta = fi0;
	for( TInt k = 0; k < n; ++k ){
		TInt nind = N[k];
		if( !l[nind].isInf && x[nind] == l[nind].value ){
			// Lower Bound
			beta += cutcoeff[k] * l[nind].value;
		} else if( !u[nind].isInf && x[nind] == u[nind].value ) {
			// Upper Bound
			beta -= cutcoeff[k] * u[nind].value;
			cutcoeff[k] *= -1;
		} else {
		}
	}

	std::vector<T> cut(n);
	for( TInt k = 0; k < n; ++k ){
		TInt nind = N[k];
		if( nind < n ){
			cut[nind] += cutcoeff[k];
		} else {
			if( cutcoeff[k] != 0 ){
				for( TInt li = Arowpointer[nind - n]; li < Arowpointer[nind - n + 1]; ++li ){
					cut[ Arowwiseind[li] ] -= cutcoeff[k] * Arowwise[li];
				}
			}
		}
	}

	return make_pair(cut, beta);
}


template <class T, class TInt>
inline bool TOSolver<T, TInt>::checkDualFarkas(){
	// has to be implemented separately for specific classes
	return false;
}


template <class T, class TInt>
T TOSolver<T, TInt>::getObj(){
	T Z( 0 );
	for( TInt i = 0; i < n; ++i ){
		Z += c[i] * x[i];
	}
	return Z;
}


template <class T, class TInt>
inline void TOSolver<T, TInt>::showOptValDetails( T optval ){
	// not requiered, has to be implemented separately for specific classes
}


}

#endif
