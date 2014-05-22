/* Copyright (c) 2011-2013
   Thomas Opfer (Technische Universitaet Darmstadt, Germany)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
   $Id$
*/

#ifndef TOSIMPLEX_H
#define TOSIMPLEX_H

#include "TORationalInf.h"

#ifndef TO_WITHOUT_DOUBLE
#include "TOFileReaderLP.h"
#endif

#ifdef TO_WITH_CPLEX

	#include <ilcplex/cplexx.h>
	#define CPLEX_EXEC( x )									\
	{														\
		int retcode;										\
		if( ( retcode = (x) ) != 0 )						\
		{													\
			char cpxerror[CPXMESSAGEBUFSIZE];				\
			sprintf( cpxerror, "Error %d.", retcode );		\
			throw std::runtime_error( cpxerror );			\
		}													\
	}

#endif

#ifdef TO_WITH_GUROBI

	extern "C"
	{
		#include "gurobi_c.h"
	}
	#define GUROBI_EXEC( env, x )								\
	{															\
		int retcode;											\
		if( ( retcode = (x) ) != 0 )							\
		{														\
			throw std::runtime_error( GRBgeterrormsg( env ) );	\
		}														\
	}

#endif

#ifdef TO_WITH_CLP
	#include "ClpSimplex.hpp"
#endif

#ifdef TO_WITH_SOPLEX
	#include "soplex.h"
#endif

#include <iostream>
#include <algorithm>
#include <pthread.h>
#include <list>
#include <ctime>
#include <cmath>
#include <vector>
#include <stdexcept>

namespace TOSimplex {

template <class T>
class TOSolver
{

	public:
		TOSolver();
		TOSolver( const std::vector<T> &rows, const std::vector<int> &colinds, const std::vector<int> &rowbegininds, const std::vector<T> &obj, const std::vector<TORationalInf<T> > &rowlowerbounds, const std::vector<TORationalInf<T> > &rowupperbounds, const std::vector<TORationalInf<T> > &varlowerbounds, const std::vector<TORationalInf<T> > &varupperbounds );
		~TOSolver();
		void addConstraint( const std::vector<T> vec, const TORationalInf<T>& lbound, const TORationalInf<T>& ubound );
		void removeConstraint( int index );
		void setBound( int index, bool lower, TORationalInf<T> newBound );
		void setRHS( int index, TORationalInf<T> newBound );
		void setConstraintBothHandSides( int index, TORationalInf<T> newLBound, TORationalInf<T> newUBound );
		void setVarLB( int index, TORationalInf<T> newLBound );
		void setVarUB( int index, TORationalInf<T> newUBound );
		void setVarBounds( int index, TORationalInf<T> newLBound, TORationalInf<T> newUBound );
		void getBase( std::vector<int>& varStati, std::vector<int>& conStati );
		void setBase( const std::vector<int>& varStati, const std::vector<int>& conStati );
		unsigned int getNumRows();
		unsigned int getNumCols();
		void setInexactFarkasInfeasibilityGuess( std::vector<double> ray );
		std::vector<T> getFarkasInfeasibilityProof();
		int opt();
		std::vector<T> getX();
		std::vector<T> getY();
		std::vector<T> getD();
		std::pair<TORationalInf<T>,TORationalInf<T> > getConstraintBounds( const unsigned int i );
		std::pair<std::vector<T>, T> getGMI( int i, std::vector<bool> iV, unsigned int k = 1 );
		T getObj();
		void read( const char* filename );

		#ifndef TO_WITHOUT_DOUBLE
			static mpz_class mpq2mpz_floor( mpq_class a ){
				mpz_class b;
				mpz_fdiv_q( b.get_mpz_t(), a.get_num_mpz_t(), a.get_den_mpz_t() );
				return b;
			}
		#endif

	private:

		// Sortierhilfe, um Indizes eines Arrays absteigend zu Sortieren
		class ratsort {
			private:
				const std::vector<T> &Q;
			public:
				ratsort( const std::vector<T> &_Q ) : Q(_Q){}
				bool operator() ( int i, int j ) {
					return ( Q[i] > Q[j] );
				}
		};

		struct bilist {
				bilist *prev;
				bilist *next;
				int val;
				bool used;
			};

		struct transposeHelper {
			int valind;
			int ind;
		};

		struct RationalWithInd {
			T value;
			int ind;
		};

		struct DSE_reinit_helper {
			pthread_mutex_t mutex;
			int nextDSE;
			TOSolver* plex;
		};

		struct DSE_thread_helper {
			pthread_mutex_t mutex;
			pthread_cond_t mainCond;
			pthread_cond_t DSECond;
			pthread_cond_t readyCond;
			bool DSEWork;
			bool mainWork;
			bool ready;
			bool exit;
			T* tau;
			TOSolver* plex;
		};

		struct mulANT_thread_helper {
			pthread_mutex_t mutex;
			pthread_mutex_t nextColMutex;
			pthread_cond_t mainCond;
			pthread_cond_t helperCond;
			pthread_cond_t readyCond;
			T* vector;
			T* result;
			int nextCol;
			bool helperWork;
			bool mainWork;
			bool ready;
			bool exit;
			TOSolver* plex;
		};

		std::vector<T> Acolwise;
		std::vector<int> Acolwiseind;
		std::vector<int> Acolpointer;
		std::vector<T> Arowwise;
		std::vector<int> Arowwiseind;
		std::vector<int> Arowpointer;

		std::vector<T> c;
		std::vector<TORationalInf<T> > lvec;
		std::vector<TORationalInf<T> > uvec;
		TORationalInf<T> *l;
		TORationalInf<T> *u;
		std::vector<T> x;
		std::vector<T> d;
		int m, n;
		bool hasBase;
		bool hasBasisMatrix;
		int baseIter;
		std::vector<int> B;
		std::vector<int> Binv;
		std::vector<int> N;
		std::vector<int> Ninv;


		std::vector<int> Urlen;
		std::vector<int> Urbeg;
		std::vector<T> Urval;
		std::vector<int> Ucind;
		std::vector<int> Ucptr;

		int Ucfreepos;

		std::vector<int> Uclen;
		std::vector<int> Ucbeg;
		std::vector<T> Ucval;
		std::vector<int> Urind;
		std::vector<int> Urptr;

		std::vector<T> Letas;
		std::vector<int> Lind;
		std::vector<int> Llbeg;
		int Lnetaf;
		int Lneta;
		std::vector<int> Letapos;

		int halfNumUpdateLetas;

		std::vector<int> perm;
		std::vector<int> permback;

		std::vector<T> DSE;
		std::vector<T> DSEtmp;
		bool antiCycle;
		DSE_thread_helper DSEhelper;
		pthread_t DSEthread;

		pthread_t mulANTthread;
		mulANT_thread_helper mulANThelper;

		std::vector<double> rayGuess;
		std::vector<T> farkasProof;

		int lastLeavingBaseVar;

		void copyTransposeA( int orgLen, const std::vector<T>& orgVal, const std::vector<int>& orgInd, const std::vector<int>& orgPointer, int newLen, std::vector<T>& newVal, std::vector<int>& newInd, std::vector<int>& newPointer );
		static void* mulANT_threaded_helper( void* ptr );
		void mulANT_threaded( T* result, T* vector );
		void mulANTCol( int &i, T* &result, T* &vector );

		static void* DSE_threaded_helper( void* ptr );
		void FTran( T* work, T* permSpike = NULL, int* permSpikeInd = NULL, int* permSpikeLen = NULL );
		void BTran( T* work );
		void updateB( int leaving, T* permSpike, int* permSpikeInd, int* permSpikeLen );
		bool refactor();
		void recalcDSE();
		static void* recalcDSE_threaded_helper( void* ptr );
		void findPiv( const std::vector<std::vector<int> >& Urowind, const std::vector<std::vector<int> >& Ucolind, bilist* const &R, bilist* const &C, const std::vector<bilist>& Ra,  const std::vector<bilist>& Ca,  const std::vector<int>& nnzCs, const std::vector<int>& nnzRs, int &i, int &j, bool &colsingleton );
		void clearBasis();
		void removeBasisFactorization();

		void init();

		bool phase1();
		int opt( bool P1 );

		bool checkDualFarkas();

};


template <class T>
TOSolver<T>::TOSolver(){
	this->init();

	this->m = 0;
	this->n = 0;

	this->l = this->lvec.data();
	this->u = this->uvec.data();

	this->Arowpointer.resize( m + 1 );
}


template <class T>
TOSolver<T>::TOSolver( const std::vector<T> &rows, const std::vector<int> &colinds, const std::vector<int> &rowbegininds, const std::vector<T> &obj, const std::vector<TORationalInf<T> > &rowlowerbounds, const std::vector<TORationalInf<T> > &rowupperbounds, const std::vector<TORationalInf<T> > &varlowerbounds, const std::vector<TORationalInf<T> > &varupperbounds ){
	this->init();

	this->m = rowlowerbounds.size();
	this->n = varlowerbounds.size();

	this->Arowwise = rows;
	this->Arowwiseind = colinds;
	this->Arowpointer = rowbegininds;

	if( rows.size() != colinds.size() || rowbegininds.back() != (int) rows.size() || (int) rowupperbounds.size() != this->m || (int) obj.size() != this->n || (int) varupperbounds.size() != this->n ){
		throw std::runtime_error( "Inconsistent data." );
	}

	copyTransposeA( m, this->Arowwise, this->Arowwiseind, this->Arowpointer, n, this->Acolwise, this->Acolwiseind, this->Acolpointer );

	// Zielfunktion
	this->c = obj;

	// untere Schranken
	this->lvec.resize( n + m );
	this->l = this->lvec.data();
	for( int i = 0; i < n; ++i ){
		if( varlowerbounds[i].isInf ){
			this->l[i] = true;
		} else {
			this->l[i] = TORationalInf<T>( varlowerbounds[i].value );
		}
	}
	for( int i = 0; i < m; ++i ){
		if( rowupperbounds[i].isInf ){
			this->l[n+i] = true;
		} else {
			this->l[n+i] = TORationalInf<T>( -rowupperbounds[i].value );
		}
	}

	// obere Schranken
	this->uvec.resize( n + m );
	this->u = this->uvec.data();
	for( int i = 0; i < n; ++i ){
		if( varupperbounds[i].isInf ){
			this->u[i] = true;
		} else {
			this->u[i] = TORationalInf<T>( varupperbounds[i].value );
		}
	}
	for( int i = 0; i < m; ++i ){
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


	int maxnumetas = m+2*this->halfNumUpdateLetas;	// Mehr ETAs kann es nicht geben
	this->Llbeg.resize( maxnumetas + 1 );
	this->Llbeg[0] = 0;	// Letzten Zeiger ans "Ende" zeigen
	this->Letapos.resize( maxnumetas );
	this->Lneta = 0;
	this->Lnetaf = 0;

	this->perm.resize( m );
	this->permback.resize( m );

}


template <class T>
TOSolver<T>::~TOSolver(){
	// Andere Threads stoppen

	pthread_mutex_lock( &this->mulANThelper.mutex );
	while( !this->mulANThelper.ready ){
		pthread_cond_wait( &this->mulANThelper.readyCond, &this->mulANThelper.mutex );
	}
	this->mulANThelper.exit = true;
	this->mulANThelper.mainWork = false;
	this->mulANThelper.helperWork = true;
	pthread_cond_signal( &this->mulANThelper.helperCond );
	pthread_mutex_unlock( &this->mulANThelper.mutex );

	pthread_join( this->mulANTthread, NULL );


	if( this->DSE.size() || this->DSEhelper.ready ){
		pthread_mutex_lock( &this->DSEhelper.mutex );
		while( !this->DSEhelper.ready ){
			pthread_cond_wait( &this->DSEhelper.readyCond, &this->DSEhelper.mutex );
		}
		this->DSEhelper.exit = true;
		this->DSEhelper.mainWork = false;
		this->DSEhelper.DSEWork = true;
		pthread_cond_signal( &this->DSEhelper.DSECond );
		pthread_mutex_unlock( &this->DSEhelper.mutex );
		pthread_join( this->DSEthread, NULL );
	}

}


template <class T>
void TOSolver<T>::init(){
	#ifndef TO_DISABLE_OUTPUT
		std::cout << "Simplex initialisiert." << std::endl;
	#endif

	this->DSEhelper.ready = false;

	pthread_mutex_init( &this->mulANThelper.mutex, NULL );
	pthread_mutex_init( &this->mulANThelper.nextColMutex, NULL );
	pthread_cond_init( &this->mulANThelper.mainCond, NULL );
	pthread_cond_init( &this->mulANThelper.helperCond, NULL );
	pthread_cond_init( &this->mulANThelper.readyCond, NULL );
	this->mulANThelper.helperWork = false;
	this->mulANThelper.ready = false;
	this->mulANThelper.exit = false;
	this->mulANThelper.plex = this;
	pthread_create( &this->mulANTthread, NULL, mulANT_threaded_helper, &this->mulANThelper );

	this->halfNumUpdateLetas = 20;

	this->hasBase = false;
	this->hasBasisMatrix = false;
	this->baseIter = 0;

	this->lastLeavingBaseVar = -1;
}


template <class T>
void TOSolver<T>::addConstraint( const std::vector<T> vec, const TORationalInf<T>& lbound, const TORationalInf<T>& ubound ){

	this->farkasProof.clear();

	if( (int) vec.size() != n ){
		throw std::runtime_error( "Constraint has wrong size." );
	}

	++this->m;

	this->Arowwise.reserve( this->Arowwise.size() + n );
	this->Arowwiseind.reserve( this->Arowwiseind.size() + n );

	for( int i = 0; i < n; ++i ){
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

	// TODO Alte Basis übernehmen? Oder neue doch lieber mit CPLEX bestimmen?
	this->clearBasis();
}


template <class T>
void TOSolver<T>::removeConstraint( int index ){

	this->farkasProof.clear();

	int len = this->Arowpointer[index+1] - this->Arowpointer[index];
	int nnz = this->Arowpointer[m] - len;

	for( int i = this->Arowpointer[index]; i < nnz; ++i ){
		this->Arowwise[i] = this->Arowwise[i+len];
	}
	for( int i = this->Arowpointer[index]; i < nnz; ++i ){
		this->Arowwiseind[i] = this->Arowwiseind[i+len];
	}

	for( int i = index; i < m; ++i ){
		this->Arowpointer[i] = this->Arowpointer[i+1] - len;
	}

	this->Arowwise.resize( nnz );
	this->Arowwiseind.resize( nnz );
	this->Arowpointer.pop_back();

	--this->m;

	copyTransposeA( m, this->Arowwise, this->Arowwiseind, this->Arowpointer, n, this->Acolwise, this->Acolwiseind, this->Acolpointer );

	for( int i = n+index; i < n+m; ++i ){
		this->l[i] = this->l[i+1];
	}
	this->lvec.pop_back();
	for( int i = n+index; i < n+m; ++i ){
		this->u[i] = this->u[i+1];
	}
	this->uvec.pop_back();

	// TODO Alte Basis übernehmen? Oder neue doch lieber mit CPLEX bestimmen?
	this->clearBasis();
}


template <class T>
void TOSolver<T>::setBound( int index, bool lower, TORationalInf<T> newBound ){

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


template <class T>
void TOSolver<T>::setRHS( int index, TORationalInf<T> newBound ){

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


template <class T>
void TOSolver<T>::setConstraintBothHandSides( int index, TORationalInf<T> newLBound, TORationalInf<T> newUBound ){

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

template <class T>
void TOSolver<T>::setVarLB( int index, TORationalInf<T> newLBound ){

	this->farkasProof.clear();

    if( !newLBound.isInf ){
        this->l[index] = TORationalInf<T>( newLBound.value );
    } else {
        this->l[index] = true;
    }
}


template <class T>
void TOSolver<T>::setVarUB( int index, TORationalInf<T> newUBound ){

	this->farkasProof.clear();

    if( !newUBound.isInf ){
        this->u[index] = TORationalInf<T>( newUBound.value );
    } else {
        this->u[index] = true;
    }
}


template <class T>
void TOSolver<T>::setVarBounds( int index, TORationalInf<T> newLBound, TORationalInf<T> newUBound ){

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


#ifndef TO_WITHOUT_DOUBLE
template<>
inline void TOSolver<mpq_class>::read( const char* filename ){

	this->farkasProof.clear();

	this->Acolwise.clear();
	this->Acolwiseind.clear();
	this->Acolpointer.clear();
	this->Arowwise.clear();
	this->Arowwiseind.clear();
	this->Arowpointer.clear();

	this->c.clear();
	this->lvec.clear();
	this->uvec.clear();

	TOFileReaderLP reader;
	reader.read( filename, this->Arowwise, this->Arowwiseind, this->Arowpointer, this->c, this->lvec, this->uvec, this->m, this->n );
	this->l = this->lvec.data();
	this->u = this->uvec.data();

	// Matrix spaltenweise kopieren
	copyTransposeA( m, this->Arowwise, this->Arowwiseind, this->Arowpointer, n, this->Acolwise, this->Acolwiseind, this->Acolpointer );

	this->rayGuess.clear();

	this->clearBasis();
}
#endif


template <class T>
void TOSolver<T>::getBase( std::vector<int>& varStati, std::vector<int>& conStati ){
	varStati.resize( n );
	conStati.resize( m );
	for( int i = 0; i < m; ++i ){
		int j = B[i];
		if( j < n ){
			varStati[j] = 1;
		} else {
			conStati[j-n] = 1;
		}
	}

	for( int i = 0; i < n; ++i ){
		int j = N[i];

		int status;

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


template <class T>
void TOSolver<T>::setBase( const std::vector<int>& varStati, const std::vector<int>& conStati ){

	this->farkasProof.clear();

	int basenum = 0;
	int nbasenum = 0;

	if( (int) varStati.size() != n ){
		throw std::runtime_error( "varStati has wrong size" );
	} else if( (int) conStati.size() != m ){
		throw std::runtime_error( "conStati has wrong size" );
	}
	else {
		for( int i = 0; i < n; ++i ){
			if( varStati[i] == 1 ){
				++basenum;
			} else {
				++nbasenum;
			}
		}
		for( int i = 0; i < m; ++i ){
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

	for( int i = 0; i < n; ++i ){
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

	for( int i = n; i < n+m; ++i ){
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


template <class T>
unsigned int TOSolver<T>::getNumRows(){
	return this->m;
}


template <class T>
unsigned int TOSolver<T>::getNumCols(){
	return this->n;
}


#ifndef TO_WITHOUT_DOUBLE
template <class T>
void TOSolver<T>::setInexactFarkasInfeasibilityGuess( std::vector<double> ray ){

	this->farkasProof.clear();

	if( ray.size() != m ){
		throw std::runtime_error( "Farkas guess has wrong size." );
	}

	this->rayGuess = ray;
}
#endif


template <class T>
std::vector<T> TOSolver<T>::getFarkasInfeasibilityProof(){

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


template <class T>
void TOSolver<T>::copyTransposeA( int orgLen, const std::vector<T>& orgVal, const std::vector<int>& orgInd, const std::vector<int>& orgPointer, int newLen, std::vector<T>& newVal, std::vector<int>& newInd, std::vector<int>& newPointer ){

	newVal.clear();
	newInd.clear();
	newPointer.clear();

	newPointer.resize( newLen + 1 );
	const int& nnz = orgInd.size();

	newVal.resize( nnz );
	newInd.resize( nnz );

	newPointer[newLen] = orgPointer[orgLen];	// Das Ende Stimmt überein

	std::vector< std::list<transposeHelper> > rowstmp(newLen);

	for( int i = 0; i < orgLen; ++i ){
		const int kend = orgPointer[i+1];
		for( int k = orgPointer[i]; k < kend; ++k ){
			transposeHelper tmp;
			tmp.valind = k;
			tmp.ind = i;
			rowstmp[orgInd[k]].push_back(tmp);
		}
	}

	typename std::list<transposeHelper>::iterator it;
	int k = 0;
	for( int i = 0; i < newLen; ++i ){
		newPointer[i] = k;
		for( it = rowstmp[i].begin(); it != rowstmp[i].end(); ++it ){
			transposeHelper tmp = *it;
			newVal[k] = orgVal[tmp.valind];
			newInd[k] = tmp.ind;
			++k;
		}
	}
}


template <class T>
void* TOSolver<T>::mulANT_threaded_helper( void* ptr ){
	mulANT_thread_helper* helper = ( mulANT_thread_helper* ) ptr;

	while( true ){
		pthread_mutex_lock( &helper->mutex );
		helper->ready = true;
		pthread_cond_signal( &helper->readyCond );

		while( !helper->helperWork ){
			pthread_cond_wait( &helper->helperCond, &helper->mutex );
		}
		pthread_mutex_unlock( &helper->mutex );

		if( helper->exit ){
			pthread_exit( 0 );
		}

		const int m = helper->plex->m;
		int i;
		while( true ){
			pthread_mutex_lock( &helper->nextColMutex );
			i = helper->nextCol++;
			pthread_mutex_unlock( &helper->nextColMutex );
			if( i >= m ){
				break;
			}
			helper->plex->mulANTCol( i, helper->result, helper->vector );
		}

		pthread_mutex_lock( &helper->mutex );
		helper->helperWork = false;
		helper->mainWork = true;
		pthread_cond_signal( &helper->mainCond );
		pthread_mutex_unlock( &helper->mutex );
	}
	return ptr;
}


template <class T>
void TOSolver<T>::mulANT_threaded( T* result, T* vector ){

	std::vector<T> threadResult( n );	// Warum wird hier  benötigt??
	this->mulANThelper.result = threadResult.data();
	this->mulANThelper.vector = vector;
	this->mulANThelper.nextCol = 0;

	// Anderen Thread starten
	pthread_mutex_lock( &this->mulANThelper.mutex );
	while( !this->mulANThelper.ready ){
		pthread_cond_wait( &this->mulANThelper.readyCond, &this->mulANThelper.mutex );
	}
	this->mulANThelper.mainWork = false;
	this->mulANThelper.helperWork = true;
	pthread_cond_signal( &this->mulANThelper.helperCond );
	pthread_mutex_unlock( &this->mulANThelper.mutex );

	// Selbst auch arbeiten
	int i;
	while( true ){
		pthread_mutex_lock( &this->mulANThelper.nextColMutex );
		i = this->mulANThelper.nextCol++;
		pthread_mutex_unlock( &this->mulANThelper.nextColMutex );
		if( i >= m ){
			break;
		}
		this->mulANTCol( i, result, vector );
	}

	// Auf anderen Thread warten
	pthread_mutex_lock( &this->mulANThelper.mutex );
		while( !this->mulANThelper.mainWork ){
			pthread_cond_wait( &this->mulANThelper.mainCond, &this->mulANThelper.mutex );
		}
	pthread_mutex_unlock( &this->mulANThelper.mutex );

	// Ergebnisse des anderen Threads hinzuaddieren
	for( int i = 0; i < n; ++i ){
		result[i] += this->mulANThelper.result[i];
	}

}


template <class T>
void TOSolver<T>::mulANTCol( int &i, T* &result, T* &vector ){
	T mult = vector[i];
	const int kend = this->Arowpointer[i+1];
	for( int k = this->Arowpointer[i]; k < kend; ++k ){
		int ind = this->Ninv[this->Arowwiseind[k]];
		if( ind != -1 ){
			result[ind] += Arowwise[k] * mult;
		}
	}

	// logische Variablen
	if( Ninv[n+i] != -1 ){
		result[Ninv[n+i]] = mult;
	}
}


template <class T>
void* TOSolver<T>::DSE_threaded_helper( void* ptr ){
	DSE_thread_helper* infos = (DSE_thread_helper*) ptr;
	while( true ){
		pthread_mutex_lock( &infos->mutex );

		infos->ready = true;
		pthread_cond_signal( &infos->readyCond );

		while( !infos->DSEWork ){
			pthread_cond_wait( &infos->DSECond, &infos->mutex );
		}
		pthread_mutex_unlock( &infos->mutex );

		if( infos->exit ){
			pthread_exit( 0 );
		}

		// DSE-FTran
		infos->plex->FTran( infos->tau );

		pthread_mutex_lock( &infos->mutex );
		infos->DSEWork = false;
		infos->mainWork = true;
		pthread_cond_signal( &infos->mainCond );
		pthread_mutex_unlock( &infos->mutex );

	}

	return ptr;
}


template <class T>
void TOSolver<T>::FTran( T* work, T* permSpike, int* permSpikeInd, int* permSpikeLen ){

	// FTranL-F

	for( int j = 0; j < this->Lnetaf; ++j ){
		const int p = this->Letapos[j];
		if( work[p] != 0 ){
			T ap = work[p];
			const int kend = Llbeg[j+1];
			for( int k = Llbeg[j]; k < kend; ++k ){
				work[Lind[k]] += this->Letas[k] * ap;
			}
		}
	}


	// FTranL-U

	for( int l = this->Lnetaf; l < this->Lneta; ++l ){
		const int p = Letapos[l];
		const int kend = this->Llbeg[l+1];
		for( int k = this->Llbeg[l]; k < kend; ++k ){
			int j = Lind[k];
			if( work[j] != 0 ){
				work[p] += Letas[k] * work[j];
			}
		}
	}


	// Permutierten Spike speichern für LU-Update
	// TODO, oben irgendwie umbauen, dass die Schleife nicht bis m laufen muss
	if( permSpike ){
		int &ind = *permSpikeLen;
		ind = 0;
		for( int i = 0; i < m; ++i ){
			if( work[i] != 0 ){
				permSpike[ind] = work[i];
				permSpikeInd[ind] = i;
				++ind;
			}
		}
	}


	// FTranU

	for( int k = m-1; k >= 0; --k ){
		const int j = this->perm[k];
		if( work[j] != 0 ){
			const int ks = this->Ucbeg[j];
			const int ke = ks + this->Uclen[j];
			T aj = work[j] / this->Ucval[ks];
			work[j] = aj;
			for( int kk = ks + 1; kk < ke; ++kk  ){
				work[this->Urind[kk]] -= this->Ucval[kk] * aj;
			}
		}
	}

}


template <class T>
void TOSolver<T>::BTran( T* work ){

	// BTranU

	for( int k = 0; k < m; ++k ){
		const int j = this->perm[k];
		if( work[j] != 0 ){
			const int ks = this->Urbeg[j];
			const int ke = ks + this->Urlen[j];
			T aj = work[j] / this->Urval[ks];
			work[j] = aj;
			for( int kk = ks + 1; kk < ke; ++kk  ){
				work[this->Ucind[kk]] -= this->Urval[kk] * aj;
			}
		}
	}


	// BTranL-U

	for( int j = this->Lneta - 1; j >= this->Lnetaf; --j ){
		const int p = this->Letapos[j];
		if( work[p] != 0 ){
			T ap = work[p];
			const int kend = Llbeg[j+1];
			for( int k = Llbeg[j]; k < kend; ++k ){
				work[Lind[k]] += this->Letas[k] * ap;
			}
		}
	}


	// BTranL-F

	for( int j = this->Lnetaf - 1; j >= 0; --j ){
		const int p = this->Letapos[j];
		const int kend = Llbeg[j+1];
		for( int k = Llbeg[j]; k < kend; ++k ){
			const int i = Lind[k];
			if( work[i] != 0 ){
				work[p] += this->Letas[k] * work[i];
			}
		}
	}

}


template <class T>
void* TOSolver<T>::recalcDSE_threaded_helper( void* ptr ){
	DSE_reinit_helper* helper = (DSE_reinit_helper*) ptr;

	const int m = helper->plex->m;

	int weight;

	while( true ) {

		pthread_mutex_lock( &helper->mutex );
		weight = helper->nextDSE++;
		#ifndef TO_DISABLE_OUTPUT
			if( weight < m ){
				std::cout << "Reinitialisiere DSE-Gewicht " << weight + 1 << "/" << m << std::endl;
			}
		#endif
		pthread_mutex_unlock( &helper->mutex );

		if( weight >= m ){
			break;
		}

		std::vector<T> rhoi(m);
		rhoi[weight] = 1;
		helper->plex->BTran( rhoi.data() );
		for( int j = 0; j < m; ++j ){
			helper->plex->DSE[weight] += rhoi[j] * rhoi[j];
		}

	}

	return ptr;
}


template <class T>
void TOSolver<T>::clearBasis(){

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


template <class T>
void TOSolver<T>::removeBasisFactorization(){
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
	int maxnumetas = m+2*this->halfNumUpdateLetas;	// Mehr ETAs kann es nicht geben
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


template <class T>
void TOSolver<T>::recalcDSE(){

	this->DSE.clear();
	this->DSEtmp.clear();

	this->DSE.resize( m );
	this->DSEtmp.resize( m+n );

	const int numthreads = 4;

	DSE_reinit_helper helper;
	pthread_mutex_init( &helper.mutex, NULL );
	helper.nextDSE = 0;
	helper.plex = this;

	std::vector<pthread_t> threads( numthreads );

	for( int i = 0; i < numthreads; ++i ){
		pthread_create( &threads[i], NULL, recalcDSE_threaded_helper, &helper );
	}

	for( int i = 0; i < numthreads; ++i ){
		pthread_join( threads[i], NULL );
	}

}


template <class T>
void TOSolver<T>::findPiv( const std::vector<std::vector<int> >& Urowind, const std::vector<std::vector<int> >& Ucolind, bilist* const &R, bilist* const &C, const std::vector<bilist>& Ra,  const std::vector<bilist>& Ca,  const std::vector<int>& nnzCs, const std::vector<int>& nnzRs, int &i, int &j, bool &colsingleton ){

	const int p = 25;	// TODO adaptiv anpassen, falls es sich lohnt?

	const long long llmm = (long long) m * (long long) m;

	long long MM = llmm;
	int singletonsize = 0;
	int nn = 0;

	for( int k = 1; k <= m; ++k  ){
		{
			bilist* Cn = C;
			do {
				int jj = Cn->val;
				if( nnzCs[jj] == k ){
					// MarkCount begin

					long long M = llmm;
					for( unsigned int l = 0; l < Urowind[jj].size(); ++l ){
						int itmp = Urowind[jj][l];
						if( Ra[itmp].used ){
							long long tmp = (long long) ( nnzRs[itmp] - 1 ) * ( nnzCs[jj] - 1 );
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
						if( k > 1 && MM <= (long long) ( k - 1 ) * ( k - 1 ) ){
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
				int ii = Rn->val;
				if( nnzRs[ii] == k ){
					// MarkCount begin

					long long M = llmm;
					for( unsigned int l = 0; l < Ucolind[ii].size(); ++l ){
						int jtmp = Ucolind[ii][l];
						if( Ca[jtmp].used ){
							long long tmp = (long long) ( nnzCs[jtmp] - 1 ) * ( nnzRs[ii] - 1 );
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
						if( MM <= (long long) k * ( k - 1 ) ){
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


template <class T>
bool TOSolver<T>::refactor(){

	// std::cout << "Refaktorisiere Basismatrix (" << this->baseIter << " Iterationen vergangen)." << std::endl;

	this->baseIter = 0;

	int additionalTmpSpace = 50;	// TODO irgendwo als kontstante festlegen?

	// Temporärer Speicher für ETAs
	std::list<RationalWithInd> Llist;

	// Speicherort für temporäre Sparse-Matrix
	std::vector<std::vector<T> > Ucol(m);
	std::vector<std::vector<int> > Urowind(m);
	std::vector<std::vector<int> > Urowptr(m);
	std::vector<std::vector<T> > Urow(m);
	std::vector<std::vector<int> > Ucolind(m);
	std::vector<std::vector<int> > Ucolptr(m);

	int avg = m;
	// Basismatrix spaltenweise kopieren
	for( int i = 0; i < m; ++i ){
		const int collen = ( B[i] < n ) ? Acolpointer[B[i]+1] - Acolpointer[B[i]] : 1;
		const int clen = collen + additionalTmpSpace;
		avg += clen;
		Ucol[i].reserve( clen );
		Urowind[i].reserve( clen );
		Urowptr[i].reserve( clen );

		if( B[i] < n ){
			for( int j = this->Acolpointer[B[i]], k = 0; k < collen; ++j, ++k ){
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

	for( int i = 0; i < m; ++i ){
		Urow[i].reserve( avg );
		Ucolind[i].reserve( avg );
		Ucolptr[i].reserve( avg );
	}

	// rowwise Kopie
	for( int i = 0; i < m; ++i ){
		for( unsigned int j = 0; j < Ucol[i].size(); ++j ){
			const int row = Urowind[i][j];
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
	std::vector<int> Uiind( m );
	// zugehörige Länge
	int Uilen = 0;
	// zugehöriger erledigt-Vektor
	std::vector<bool> Uidone( m, false );

	this->Lneta = 0;
	this->Lnetaf = 0;	// TODO evtl. in this nur zurück, wenn tatsächlich faktorisiert wurde


	// verbleibende Spalten- und Zeilenindexmengen

	std::vector<bilist> PPa( m );
	bilist *PP = PPa.data();

	for( int i = 0; i < m; ++i ){
		PPa[i].val = i;
		PPa[i].used = true;
	}

	// Zeiger der inneren Elemente der Liste
	for( int i = 1; i < m - 1; ++i ){
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

	for( int i = 0; i < m; ++i ){
		QQa[i].val = i;
		QQa[i].used = true;
	}

	// Zeiger der inneren Elemente der Liste
	for( int i = 1; i < m - 1; ++i ){
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
	std::vector<int> nnzCs( m );
	std::vector<int> nnzRs( m );

	for( int i = 0; i < m; ++i ){
		nnzCs[i] = Ucol[i].size();
		nnzRs[i] = Urow[i].size();
	}


	// Basis-Permutationen...

	std::vector<int> Bneu( m );
	std::vector<int> Bneu2( m );


	for( int k = 0; k < m; ++k ){

		// Step 1: Pivotsuche

		int p = -1;
		int q = -1;
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
		for( unsigned int l = 0; l < Ucolind[p].size(); ++l ){
			--nnzCs[Ucolind[p][l]];
		}


		// Zeilen-nnz muss auch in diesem Fall aktualisiert werden.
		// TODO stimmt das denn??
		if( colsingleton ){
//			--nnzRs[p];
		}


		// Step 4

		else {
			// Neuen ETA-Vektor vorbereiten
			int Lpos = this->Llbeg[Lnetaf];
			this->Letapos[this->Lnetaf] = p;
			++this->Lnetaf;

			// Pivot-Zeile speichern
			{
				for( unsigned int l = 0; l < Urow[p].size(); ++l ){
					Up[Ucolind[p][l]] = Urow[p][l];
				}
			}

			for( unsigned int l = 0; l < Ucol[q].size(); ){
				int i = Urowind[q][l];
				if( PPa[i].used ){

					T L = - Ucol[q][l] / Up[q];

					RationalWithInd Ltmp;
					Ltmp.value = L;
					Ltmp.ind = i;
					Llist.push_back( Ltmp );

					++Lpos;


					// Zeile Ui speichern
					{
						for( unsigned int ll = 0; ll < Urow[i].size(); ++ll ){
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
							int j = QQn->val;
							if( Up[j] != 0 ){
								Ui[j] += L * Up[j];
								Uiind[Uilen++] = j;
								Uidone[j] = false;	// Dieser Wert muss ersetzt werden
							}
							QQn = QQn->next;
						} while( QQn != QQ );
					}

					// Bestehende Elemente der Zeile updaten

					for( unsigned int ll = 0; ll < Urow[i].size(); ++ll ){
						int j = Ucolind[i][ll];
						if( !Uidone[j] ){
							if( Ui[j] == 0 ){
								unsigned int cptr = Ucolptr[i][ll];

								// Letztes Element der Zeile an diese Position holen, Zeile um 1 kürzen
								unsigned int rowend = Urow[i].size() - 1;
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
								unsigned int colend = Ucol[j].size() - 1;
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


					for( int ll = 0; ll < Uilen; ++ll ){
						int j = Uiind[ll];
						if( !Uidone[j] && Ui[j] != 0 ){

							// Zeilen- und Spaltenende ermitteln, Längen anpassen
							const unsigned int rowend = Ucolind[i].size();
							const unsigned int colend = Ucol[j].size();

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
					++l;
				}
			}

			// gespeicherte Pivot-Zeile löschen
			for( unsigned int l = 0; l < Urow[p].size(); ++l ){
				Up[Ucolind[p][l]] = 0;
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
		int Lsize = Llist.size();
		this->Letas.resize( Lsize+2*this->halfNumUpdateLetas*m );
		this->Lind.resize( Lsize+2*this->halfNumUpdateLetas*m );
		typename std::list<RationalWithInd>::iterator it;
		int pos = 0;
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
		for( int i = 1; i < m; ++i ){
			Ucbeg[i] = Ucbeg[i-1] + Ucol[i-1].size();
			Urbeg[i] = Urbeg[i-1] + Urow[i-1].size() + 2 * this->halfNumUpdateLetas;
		}
		this->Ucfreepos = Ucbeg[m-1] + Ucol[m-1].size();
	} else {
		this->Ucfreepos = 0;
	}
	int arraysize = this->Ucfreepos + 2 * m * this->halfNumUpdateLetas + 1;
	this->Urval.resize( arraysize );
	this->Ucind.resize( arraysize );
	this->Ucptr.resize( arraysize );

	this->Ucval.resize( arraysize );
	this->Urind.resize( arraysize );
	this->Urptr.resize( arraysize );


	for( int i = 0; i < m; ++i ){
		// Spaltenweise
		{
			const int len = Ucol[i].size();
			this->Uclen[i] = len;
			const int cbeg = this->Ucbeg[i];
			for( int ll = 0; ll < len; ++ll ){
				int pos = cbeg + ll;
				this->Ucval[pos] = Ucol[i][ll];
				this->Urind[pos] = Urowind[i][ll];
				this->Urptr[pos] = Urowptr[i][ll] + this->Urbeg[Urowind[i][ll]];
			}

		}

		// Zeilenweise
		{
			const int len = Urow[i].size();
			this->Urlen[i] = len;
			int rbeg = this->Urbeg[i];
			for( int ll = 0; ll < len; ++ll ){
				int pos = rbeg + ll;
				this->Urval[pos] = Urow[i][ll];
				this->Ucind[pos] = Ucolind[i][ll];
				this->Ucptr[pos] = Ucolptr[i][ll] + this->Ucbeg[Ucolind[i][ll]];
			}

		}
	}


	{

		// Basis anpassen
		std::vector<int> tmpBase( m );
		std::vector<int> Ucbegold( m );
		std::vector<int> Uclenold( m );
		for( int i = 0; i < m; ++i ){
			// Bneu[Bneu2[i]]
			tmpBase[i] = this->B[Bneu[Bneu2[i]]];
			Ucbegold[i] = this->Ucbeg[i];
			Uclenold[i] = this->Uclen[i];
		}
		for( int i = 0; i < m; ++i ){
			if( this->Ucbeg[i] != Ucbegold[Bneu[Bneu2[i]]] ){
				this->Ucbeg[i] = Ucbegold[Bneu[Bneu2[i]]];
				this->Uclen[i] = Uclenold[Bneu[Bneu2[i]]];
				for( int j = 0; j < this->Uclen[i]; ++j ){
					this->Ucind[this->Urptr[this->Ucbeg[i]+j]] = i;
				}
			}

			this->B[i] = tmpBase[i];
			this->Binv[tmpBase[i]] = i;
		}

	}


	// Spalten-Diagonalelemente an erste Stellen holen
	for( int j = 0; j < m; ++j ){
		const int cbeg = this->Ucbeg[j];
		const int cend = cbeg + this->Uclen[j];

		for( int k = cbeg; k < cend; ++k ){
			if( this->Urind[k] == j ){

				{
					T tmpcval = this->Ucval[cbeg];
					this->Ucval[cbeg] = this->Ucval[k];
					this->Ucval[k] = tmpcval;
				}

				{
					int tmprind = this->Urind[cbeg];
					this->Urind[cbeg] = this->Urind[k];
					this->Urind[k] = tmprind;
				}

				{
					int tmprptr = this->Urptr[cbeg];
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
	for( int i = 0; i < m; ++i ){
		const int rbeg = this->Urbeg[i];

		const int k = this->Urptr[this->Ucbeg[i]];	// Position aus Spalten-Diagonalelement auslesen

		{
			T tmpval = this->Urval[rbeg];
			this->Urval[rbeg] = this->Urval[k];
			this->Urval[k] = tmpval;
		}

		{
			int tmpcind = this->Ucind[rbeg];
			this->Ucind[rbeg] = this->Ucind[k];
			this->Ucind[k] = tmpcind;
		}

		{
			int tmpcptr = this->Ucptr[rbeg];
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


template <class T>
void TOSolver<T>::updateB( int r, T* permSpike, int* permSpikeInd, int* permSpikeLen ){

	// zu ersetzende Spalte aus Basismatrix löschen (außer Diagonalelement, das wird sowieso ersetzt)
	// Diagonalelement vorübergehend auf 0 setzen, statt zu löschen
	this->Urval[this->Urbeg[r]] = 0;

	int colend = this->Ucbeg[r] + this->Uclen[r];
	for( int i = this->Ucbeg[r] + 1; i < colend; ++i ){
		int rind = this->Urind[i];
		int rptr = this->Urptr[i];

		// Letztes Element der Zeile an diese Position holen, Zeile um 1 kürzen
		int rowend = this->Urbeg[rind] + --this->Urlen[rind];
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
	for( int i = 0; i < *permSpikeLen; ++i ){
		int rind = permSpikeInd[i];
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
			int rowend = this->Urbeg[rind] + this->Urlen[rind]++;

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

	int t = this->permback[r];

	// Zeile Ur kopieren und dabei aus Matrix löschen (bis auf Diagonalelement)
	std::vector<T> Ur(m);	// TODO globale Arbeitsvariable machen und am Ende immer auf 0 setzen (ist automatisch 0), macht vermutlich wenig Sinn

	Ur[r] = Urval[this->Urbeg[r]];	// Diagonalelement kopieren um unten eine if-Abfrage zu vermeiden

	{
		int kend = this->Urbeg[r] + this->Urlen[r];
		for( int i = this->Urbeg[r] + 1; i < kend; ++i ){
			int cind = Ucind[i];
			Ur[cind] = Urval[i];

			int cptr = Ucptr[i];

			// Letztes Element der Spalte an diese Position holen, Zeile um 1 kürzen
			int colend = this->Ucbeg[cind] + --this->Uclen[cind];
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

	for( int k = t+1; k < m; ++k ){
		int i = this->perm[k];

		if( Ur[i] != 0 ){
			T L = - Ur[i] / this->Urval[Urbeg[i]];
			this->Letas[this->Llbeg[this->Lneta]] = L;
			this->Lind[this->Llbeg[this->Lneta]++] = i;

			Ur[i] = 0;

			const int llend = this->Urbeg[i] + this->Urlen[i];
			for( int ll = this->Urbeg[i] + 1; ll < llend; ++ll ){	// +1, weil ohne Diagonalelement
				Ur[Ucind[ll]] += L * this->Urval[ll];
			}
		}

	}

	// Bei Forrest-Tomlin ist hier nur noch das Diagonalelement != 0
	this->Urval[this->Urbeg[r]] = Ucval[this->Ucbeg[r]] = Ur[r];
	Ur[r] = 0;


	// TODO Das geht angeblich sogar in konstanter Zeit??
	int tmp = this->perm[t];
	for( int k = t; k < m - 1; ++k ){
		this->perm[k] = this->perm[k+1];
	}
	this->perm[m-1] = tmp;

	// TODO update-formel hierfür verwenden
	for( int i = 0; i < m; ++i ){
		this->permback[this->perm[i]] = i;
	}

}


template <class T>
bool TOSolver<T>::phase1(){

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

	for( int i = 0; i < n+m; ++i ){
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


	// Phase 1 - Problem lösen
	this->opt( true );

	// Zielfunktionswert bestimmen	// TODO später auslesen, falls wir den Wert zwischenspeichern/zurückgeben?
	T Z( 0 );
	for( int i = 0; i < n; ++i ){
		Z += c[i] * x[i];
	}
	bool retval = ( Z == 0 );


	// Schranken wiederherstellen
	this->u = uvec.data();
	this->l = lvec.data();

	return retval;
}


template <class T>
int TOSolver<T>::opt(){

	#ifdef TO_WITH_CPLEX

	clock_t cplextime = clock();

	if( !this->hasBase ){

		#ifndef TO_DISABLE_OUTPUT
			std::cout << "Starte CPLEX um möglicherweise Startbasis zu bestimmen..." << std::endl << std::endl;
		#endif

		int status;
		CPXENVptr env = CPXXopenCPLEX( &status ); // TODO für Nachoptimierungen auslagern und behalten
		if( status ){
			std::vector<char> error( CPXMESSAGEBUFSIZE );
			throw std::runtime_error( CPXXgeterrorstring( env, status, error.data() ) );
		}

		#ifndef TO_DISABLE_OUTPUT
			CPLEX_EXEC( CPXXsetintparam( env, CPX_PARAM_SCRIND, CPX_ON ) );
		#endif

		// CPLEX-Parameter für Optimalität und Zulässigkeit
		CPXXsetdblparam( env, CPX_PARAM_EPOPT, 1e-9 );
		CPXXsetdblparam( env, CPX_PARAM_EPRHS, 1e-9 );


		CPXLPptr lp = CPXXcreateprob( env, &status, "test" );
		if( status ){
			std::vector<char> error( CPXMESSAGEBUFSIZE );
			throw std::runtime_error( CPXXgeterrorstring( env, status, error.data() ) );
		}

		CPLEX_EXEC( CPXXchgobjsen( env, lp, CPX_MIN ) );

		try {

			// Variablen + Zielfunktion übergeben
			{
				std::vector<double> cdbl( n );
				std::vector<double> ldbl( n );
				std::vector<double> udbl( n );
				for( int i = 0; i < n; ++i ){
					cdbl[i] = this->c[i].get_d();
					if( this->l[i].isInf && this->u[i].isInf ){
						ldbl[i] = -CPX_INFBOUND;
						udbl[i] = CPX_INFBOUND;
					} else if( this->l[i].isInf && !this->u[i].isInf ){
						ldbl[i] = -CPX_INFBOUND;
						udbl[i] = this->u[i].value.get_d();
					} else if( !this->l[i].isInf && this->u[i].isInf ){
						ldbl[i] = this->l[i].value.get_d();
						udbl[i] = CPX_INFBOUND;
					} else if( !this->l[i].isInf && !this->u[i].isInf ){
						ldbl[i] = this->l[i].value.get_d();
						udbl[i] = this->u[i].value.get_d();
					}
				}
				CPLEX_EXEC( CPXXnewcols( env, lp, n, cdbl.data(), ldbl.data(), udbl.data(), NULL, NULL ) );
			}

			// Nebenbedingungen anlegen
			{
				const int numel = this->Arowpointer[m];
				std::vector<double> coeffdbl( numel );
				for( int i = 0; i < numel; ++i ){
					coeffdbl[i] = Arowwise[i].get_d();
				}
				std::vector<CPXNNZ> rowpcpx( m + 1 );
				for(int i = 0; i < m + 1; ++i ){
					rowpcpx[i] = this->Arowpointer[i];
				}
				std::vector<char> sense(m);
				std::vector<double> rhs(m);
				std::vector<double> rngval;
				rngval.reserve( m );
				std::vector<int> rngind;
				rngind.reserve( m );
				for( int i = 0; i < m; ++i ){
					const int s = n + i;
					if( !this->l[s].isInf && !this->u[s].isInf ){
						if( l[s].value.get_d() == u[s].value.get_d() ){
							sense[i] = 'E';
						} else {
							sense[i] = 'R';
							rngind.push_back( i );
							rngval.push_back( u[s].value.get_d() - l[s].value.get_d() );
						}
						rhs[i] = -u[s].value.get_d();
					} else if( this->l[s].isInf && !this->u[s].isInf ){
						sense[i] = 'G';
						rhs[i] = -u[s].value.get_d();
					} else if( !this->l[s].isInf && this->u[s].isInf ){
						sense[i] = 'L';
						rhs[i] = -l[s].value.get_d();
					}
				}
				CPLEX_EXEC( CPXXaddrows( env, lp, 0, m, numel, rhs.data(), sense.data(), rowpcpx.data(), this->Arowwiseind.data(), coeffdbl.data(), NULL, NULL ) );
				if( rngval.size() ){
					CPLEX_EXEC( CPXXchgrngval( env, lp, rngval.size(), rngind.data(), rngval.data() ) );
				}
			}


			// Lösen (Ich hatte teilweise falsche Ergebnisse wenn nur 1x)
			CPLEX_EXEC( CPXXlpopt( env, lp ) );
			CPLEX_EXEC( CPXXsetdblparam( env, CPX_PARAM_EPMRK, 0.95 ) );
			CPLEX_EXEC( CPXXlpopt( env, lp ) );

			// Lösung verarbeiten
			status = CPXXgetstat( env, lp );
			if( status == CPX_STAT_OPTIMAL ){
				#ifndef TO_DISABLE_OUTPUT
					double objval;
					CPLEX_EXEC( CPXXgetobjval( env, lp, &objval ) );
					std::cout << std::endl << "CPLEX-Optimallösung gefunden: " << objval << "." << std::endl;
				#endif
			} else {
				#ifndef TO_DISABLE_OUTPUT
				{
					char cpxstatus[CPXMESSAGEBUFSIZE];
					char* statstring = CPXXgetstatstring( env, status, cpxstatus );
					std::cout << std::endl << "Keine Optimallösung mit CPLEX gefunden." << std::endl;
					if( statstring ){
						std::cout << "CPLEX-Status = " << statstring << std::endl;
					}
					std::cout << "Deaktiviere Presolver, optimiere erneut." << std::endl << std::endl;
				}
				#endif


				// Nochmal ohne Presolver und garantiert dual starten
				CPLEX_EXEC( CPXXsetintparam( env, CPX_PARAM_PREIND, CPX_OFF ) );
				CPLEX_EXEC( CPXXsetintparam( env, CPX_PARAM_LPMETHOD, CPX_ALG_DUAL ) );


				// Lösen (Ich hatte teilweise falsche Ergebnisse wenn nur 1x)
				CPLEX_EXEC( CPXXlpopt( env, lp ) );
				CPLEX_EXEC( CPXXsetdblparam( env, CPX_PARAM_EPMRK, 0.95 ) );
				CPLEX_EXEC( CPXXlpopt( env, lp ) );

				status = CPXXgetstat( env, lp );

				#ifndef TO_DISABLE_OUTPUT
				{
					char cpxstatus[CPXMESSAGEBUFSIZE];
					char* statstring = CPXXgetstatstring( env, status, cpxstatus );
					if( statstring ){
						std::cout << std::endl << "CPLEX-Status nach weiterem Durchlauf = " << statstring << std::endl;
					}
				}
				#endif
			}

			#ifndef TO_DISABLE_OUTPUT
				std::cout << "Übertrage Basis." << std::endl;
			#endif

			// Basis übertragen
			int kB = 0;
			int kN = 0;

			{
				std::vector<int> cstat(n);
				std::vector<int> rstat(m);
				CPLEX_EXEC( CPXXgetbase( env, lp, cstat.data(), rstat.data() ) );
				for( int i = 0; i < n; ++i ){
					// 1 Basis, sonst nichtbasis
					int &s = cstat[i];
					if( s == 1 ){
						// Basisvariabls
						this->B[kB] = i;
						this->Binv[i] = kB;
						this->Ninv[i] = -1;
						++kB;
					} else {
						// Nichtbasisvariable
						if( s == 0 ){
							this->x[i] = this->l[i].value;
						} else if( s == 2 ){
							this->x[i] = this->u[i].value;
						}
						this->N[kN] = i;
						this->Ninv[i] = kN;
						this->Binv[i] = -1;
						++kN;
					}
				}

				for( int i = 0; i < m; ++i ){
					// 1 Basis, sonst nichtbasis
					int &s = rstat[i];
					const int sV = n + i;
					if( s == 1 ){
						// Basisvariabls
						this->B[kB] = sV;
						this->Binv[sV] = kB;
						this->Ninv[sV] = -1;
						++kB;
					} else {
						// Nichtbasisvariable
						if( s == 0 && !this->l[sV].isInf ){
							this->x[sV] = this->l[sV].value;
						} else {
							this->x[sV] = this->u[sV].value;
						}
						this->N[kN] = sV;
						this->Ninv[sV] = kN;
						this->Binv[sV] = -1;
						++kN;
					}
				}
			}

			if( kB != m || kN != n ){
				throw std::runtime_error( "CPLEX-Basis hat scheinbar nicht die korrekte Anzahl an Variablen/Nebenbedingungen." );
			}

			this->hasBase = true;

			if( status == CPX_STAT_INFEASIBLE ){
				#ifndef TO_DISABLE_OUTPUT
					std::cout << "CPLEX infeasible." << std::endl;
				#endif

				this->rayGuess.resize( m );
				CPLEX_EXEC( CPXXdualfarkas( env, lp,this->rayGuess.data(), NULL ) );
			}

		} catch( std::runtime_error& ex ){
			#ifndef TO_DISABLE_OUTPUT
				std::cerr << std::endl << "Unerwarteter Fehler bei Bestimmung der Basis mit CPLEX:" << std::endl << ex.what() << std::endl << std::endl;
				std::cout << "Bestimme Basis selbst." << std::endl;
			#endif
		} catch( ... ){
			#ifndef TO_DISABLE_OUTPUT
				std::cerr << std::endl << "Völlig unerwarteter Fehler bei Bestimmung der Basis mit CPLEX." << std::endl << std::endl;
				std::cout << "Bestimme Basis selbst." << std::endl;
			#endif
		}

		CPLEX_EXEC( CPXXfreeprob( env, &lp ) );
		CPLEX_EXEC( CPXXcloseCPLEX( &env ) );

	}

	cplextime = clock() - cplextime;

	#endif


	#ifdef TO_WITH_GUROBI

	clock_t gurobitime = clock();

	if( !this->hasBase ){

		#ifndef TO_DISABLE_OUTPUT
			std::cout << "Starte Gurobi um möglicherweise Startbasis zu bestimmen..." << std::endl << std::endl;
		#endif

		GRBenv *env = NULL; // TODO für Nachoptimierungen auslagern und behalten
		GRBmodel *model = NULL;

		if( GRBloadenv( &env, NULL ) || env == NULL ){
			throw std::runtime_error( GRBgeterrormsg( env ) );
		}

		#ifndef TO_DISABLE_OUTPUT
			GUROBI_EXEC( env, GRBsetintparam( env, GRB_INT_PAR_OUTPUTFLAG, 1 ) );
		#endif

		GUROBI_EXEC( env, GRBsetintparam( env, GRB_INT_PAR_INFUNBDINFO, 1 ) );
		GUROBI_EXEC( env, GRBsetdblparam( env, GRB_DBL_PAR_OPTIMALITYTOL, 1e-9 ) );
		GUROBI_EXEC( env, GRBsetdblparam( env, GRB_DBL_PAR_FEASIBILITYTOL, 1e-9 ) );

		std::vector<double> cdbl( n );
		std::vector<double> ldbl( n );
		std::vector<double> udbl( n );
		for( int i = 0; i < n; ++i ){
			cdbl[i] = this->c[i].get_d();
			if( this->l[i].isInf && this->u[i].isInf ){
				ldbl[i] = -GRB_INFINITY;
				udbl[i] = GRB_INFINITY;
			} else if( this->l[i].isInf && !this->u[i].isInf ){
				ldbl[i] = -GRB_INFINITY;
				udbl[i] = this->u[i].value.get_d();
			} else if( !this->l[i].isInf && this->u[i].isInf ){
				ldbl[i] = this->l[i].value.get_d();
				udbl[i] = GRB_INFINITY;
			} else if( !this->l[i].isInf && !this->u[i].isInf ){
				ldbl[i] = this->l[i].value.get_d();
				udbl[i] = this->u[i].value.get_d();
			}
		}


		GUROBI_EXEC( env, GRBnewmodel( env, &model, "test", n, cdbl.data(), ldbl.data(), udbl.data(), NULL, NULL ) );

		try {
			int status = 0;

			// Nebenbedingungen anlegen
			for( int i = 0; i < m; ++i ){
				const int numel = this->Arowpointer[i+1] - this->Arowpointer[i];
				std::vector<double> coeffdbl( numel );
				for( int j = 0; j < numel; ++j ){
					const int ind = this->Arowpointer[i] + j;
					coeffdbl[j] = Arowwise[ind].get_d();
				}

				const int s = n + i;

				char sense = 0;
				double rhs = 0;
				bool ranged = false;
				if( !this->l[s].isInf && !this->u[s].isInf ){
					if( l[s].value.get_d() == u[s].value.get_d() ){
						sense = GRB_EQUAL;
						rhs = -u[s].value.get_d();
					} else {
						ranged = true;
					}
				} else if( this->l[s].isInf && !this->u[s].isInf ){
					sense = GRB_GREATER_EQUAL;
					rhs = -u[s].value.get_d();
				} else if( !this->l[s].isInf && this->u[s].isInf ){
					sense = GRB_LESS_EQUAL;
					rhs = -l[s].value.get_d();
				}

				if( ranged ){
					GUROBI_EXEC( env, GRBaddrangeconstr( model, numel, &this->Arowwiseind[this->Arowpointer[i]], coeffdbl.data(), -u[s].value.get_d(), - l[s].value.get_d(), NULL ) );
				} else {
					GUROBI_EXEC( env, GRBaddconstr( model, numel, &this->Arowwiseind[this->Arowpointer[i]], coeffdbl.data(), sense, rhs, NULL ) );
				}

			}

			GUROBI_EXEC( env, GRBupdatemodel( model ) );

			GUROBI_EXEC( env, GRBoptimize( model ) );


			#ifndef TO_DISABLE_OUTPUT
				std::cout << "Übertrage Basis." << std::endl;
			#endif

			// Basis übertragen
			int kB = 0;
			int kN = 0;

			{
				std::vector<int> cstat(n);
			 	GUROBI_EXEC( env, GRBgetintattrarray( model, GRB_INT_ATTR_VBASIS, 0, n, cstat.data() ) );
				for( int i = 0; i < n; ++i ){
					// 0 Basis, sonst nichtbasis
					int &s = cstat[i];
					if( s == 0 ){
						// Basisvariabls
						this->B[kB] = i;
						this->Binv[i] = kB;
						this->Ninv[i] = -1;
						++kB;
					} else {
						// Nichtbasisvariable
						if( s == -1 ){
							this->x[i] = this->l[i].value;
						} else if( s == -2 ){
							this->x[i] = this->u[i].value;
						}
						this->N[kN] = i;
						this->Ninv[i] = kN;
						this->Binv[i] = -1;
						++kN;
					}
				}
			}

			{
				std::vector<int> rstat(m);
				GUROBI_EXEC( env, GRBgetintattrarray( model, GRB_INT_ATTR_CBASIS, 0, m, rstat.data() ) );
				for( int i = 0; i < m; ++i ){
					// 0 Basis, sonst nichtbasis
					int &s = rstat[i];
					const int sV = n + i;
					if( s == 0 ){
						// Basisvariabls
						this->B[kB] = sV;
						this->Binv[sV] = kB;
						this->Ninv[sV] = -1;
						++kB;
					} else {
						// Nichtbasisvariable
						if( s == -1 && !this->l[sV].isInf ){
							this->x[sV] = this->l[sV].value;
						} else {
							this->x[sV] = this->u[sV].value;
						}
						this->N[kN] = sV;
						this->Ninv[sV] = kN;
						this->Binv[sV] = -1;
						++kN;
					}
				}
			}

			if( kB != m || kN != n ){
				throw std::runtime_error( "Gurobi-Basis hat scheinbar nicht die korrekte Anzahl an Variablen/Nebenbedingungen." );
			}

			this->hasBase = true;

			GUROBI_EXEC( env, GRBgetintattr( model, GRB_INT_ATTR_STATUS, &status ) );

			if( status == GRB_INFEASIBLE ){
				#ifndef TO_DISABLE_OUTPUT
					std::cout << "Gurobi infeasible." << std::endl;
				#endif

				this->rayGuess.resize( m );
				GUROBI_EXEC( env, GRBgetdblattrarray( model, GRB_DBL_ATTR_FARKASDUAL, 0, m, this->rayGuess.data() ) );
				for( unsigned int i = 0; i < m; ++i ){
					this->rayGuess[i] *= -1;	// Gurobi liefert die Rays anders herum...
				}
			}

		} catch( std::runtime_error& ex ){
			#ifndef TO_DISABLE_OUTPUT
				std::cerr << std::endl << "Unerwarteter Fehler bei Bestimmung der Basis mit Gurobi:" << std::endl << ex.what() << std::endl << std::endl;
				std::cout << "Bestimme Basis selbst." << std::endl;
			#endif
		} catch( ... ){
			#ifndef TO_DISABLE_OUTPUT
				std::cerr << std::endl << "Völlig unerwarteter Fehler bei Bestimmung der Basis mit Gurobi." << std::endl << std::endl;
				std::cout << "Bestimme Basis selbst." << std::endl;
			#endif
		}

		GUROBI_EXEC( env, GRBfreemodel( model ) );
		GRBfreeenv( env );

	}

	gurobitime = clock() - gurobitime;

	#endif

	#ifdef TO_WITH_CLP
		clock_t clptime = clock();

		try
		{
			ClpSimplex model;

			model.setDblParam( ClpDualTolerance, 1e-10 );
			model.setDblParam( ClpPrimalTolerance, 1e-9 );

			std::vector<double> Actmp( this->Acolwise.size() );
			for( unsigned int i = 0; i < this->Acolwise.size(); ++i ){
				Actmp[i] = this->Acolwise[i].get_d();
			}

			std::vector<double> tmpl( n );
			std::vector<double> tmpu( n );
			std::vector<double> tmpc( n );
			for( int i = 0; i < n; ++i ){
				tmpl[i] = this->l[i].isInf ? -COIN_DBL_MAX : this->l[i].value.get_d();
				tmpu[i] = this->u[i].isInf ? COIN_DBL_MAX : this->u[i].value.get_d();
				tmpc[i] = this->c[i].get_d();
			}

			std::vector<double> tmprl( m );
			std::vector<double> tmpru( m );
			for( int i = 0; i < m; ++i ){
				const int s = n + i;
				tmprl[i] = this->u[s].isInf ? -COIN_DBL_MAX : - this->u[s].value.get_d();
				tmpru[i] = this->l[s].isInf ? COIN_DBL_MAX : - this->l[s].value.get_d();
			}

			model.loadProblem( this->n, this->m, this->Acolpointer.data(), this->Acolwiseind.data(), Actmp.data(), tmpl.data(), tmpu.data(), tmpc.data(), tmprl.data(), tmpru.data() );

			{
				ClpSolve clpoptions;
				clpoptions.setPresolveType( ClpSolve::presolveOff );
				clpoptions.setSolveType( ClpSolve::useDual );
				model.initialSolve( clpoptions );
			}
			unsigned char * clpbase = model.statusArray();
			if( !clpbase ){
				throw std::runtime_error( "No CLP basis." );
			}


			#ifndef TO_DISABLE_OUTPUT
				std::cout << "Übertrage Basis." << std::endl;
			#endif

			// Basis übertragen
			int kB = 0;
			int kN = 0;

			for( int i = 0; i < n + m; ++i ){
				char clpbasestatus = clpbase[i];
				switch( clpbasestatus ){
					case ClpSimplex::basic:
						this->B[kB] = i;
						this->Binv[i] = kB;
						this->Ninv[i] = -1;
						++kB;
						break;
					case ClpSimplex::superBasic:
					case ClpSimplex::isFree:
						this->N[kN] = i;
						this->Ninv[i] = kN;
						this->Binv[i] = -1;
						++kN;
						break;
					case ClpSimplex::isFixed:
					case ClpSimplex::atLowerBound:
						this->x[i] = this->l[i].value;
						this->N[kN] = i;
						this->Ninv[i] = kN;
						this->Binv[i] = -1;
						++kN;
						break;
					case ClpSimplex::atUpperBound:
						this->x[i] = this->u[i].value;
						this->N[kN] = i;
						this->Ninv[i] = kN;
						this->Binv[i] = -1;
						++kN;
						break;
						break;
					default:
						throw std::runtime_error( "Unexpected CLP basis status." );
						break;
				}
			}

			if( kB != m || kN != n ){
				throw std::runtime_error( "CLP-Basis hat scheinbar nicht die korrekte Anzahl an Variablen/Nebenbedingungen." );
			}

			if( model.isProvenPrimalInfeasible() && model.rayExists() ){
				double * tmpray = model.infeasibilityRay();
				if( tmpray ){
					this->rayGuess.resize( m );
					for( int i = 0; i < m; ++i ){
						this->rayGuess[i] = -tmpray[i];	// CLP liefert die Rays anders herum...
					}
					delete [] tmpray;
				}
			}

			this->hasBase = true;

		} catch( std::runtime_error& ex ){
			#ifndef TO_DISABLE_OUTPUT
				std::cerr << std::endl << "Unerwarteter Fehler bei Bestimmung der Basis mit CLP:" << std::endl << ex.what() << std::endl << std::endl;
				std::cout << "Bestimme Basis selbst." << std::endl;
			#endif
		} catch( ... ){
			#ifndef TO_DISABLE_OUTPUT
				std::cerr << std::endl << "Völlig unerwarteter Fehler bei Bestimmung der Basis mit CLP." << std::endl << std::endl;
				std::cout << "Bestimme Basis selbst." << std::endl;
			#endif
		}

		clptime = clock() - clptime;
	#endif

	#ifdef TO_WITH_SOPLEX

		clock_t soplextime = clock();

		try
		{
			soplex::SoPlex splex;

			splex.changeSense( soplex::SPxSolver::MINIMIZE );

			{
				std::vector<double> tmprl( m );
				std::vector<double> tmpru( m );
				for( int i = 0; i < m; ++i ){
					const int s = n + i;
					tmprl[i] = this->u[s].isInf ? -soplex::infinity : - this->u[s].value.get_d();
					tmpru[i] = this->l[s].isInf ? soplex::infinity : - this->l[s].value.get_d();
				}

				soplex::LPRowSet rowset( m, 0 );
				soplex::DSVector rowvec;
				rowvec.clear();

				for( unsigned int i = 0; i < m; ++i ){
					rowset.add( tmprl[i], rowvec, tmpru[i] );
				}

				splex.addRows( rowset );
			}


			{


				std::vector<double> Actmp( this->Acolwise.size() );
				for( unsigned int i = 0; i < this->Acolwise.size(); ++i ){
					Actmp[i] = this->Acolwise[i].get_d();
				}

				std::vector<double> tmpl( n );
				std::vector<double> tmpu( n );
				std::vector<double> tmpc( n );
				for( int i = 0; i < n; ++i ){
					tmpl[i] = this->l[i].isInf ? -soplex::infinity : this->l[i].value.get_d();
					tmpu[i] = this->u[i].isInf ? soplex::infinity : this->u[i].value.get_d();
					tmpc[i] = this->c[i].get_d();
				}

				soplex::LPColSet colset( n, Acolwise.size() );
				soplex::DSVector colvec;

				for( unsigned int i = 0; i < n; ++i ){
					colvec.clear();
					colvec.add( Acolpointer[i+1] - Acolpointer[i], &Acolwiseind[Acolpointer[i]], &Actmp[Acolpointer[i]] );
					colset.add( tmpc[i], tmpl[i], colvec, tmpu[i] );
				}

				splex.addCols( colset );

			}

			splex.solve();

			#ifndef TO_DISABLE_OUTPUT
				std::cout << "SoPlex:" << std::endl << splex.statistics() << std::endl;
				std::cout << "Übertrage Basis." << std::endl;
			#endif

			// Basis übertragen
			int kB = 0;
			int kN = 0;

			for( int i = 0; i < n; ++i ){
				switch( splex.getBasisColStatus( i ) ){
					case soplex::SPxSolver::BASIC:
						this->B[kB] = i;
						this->Binv[i] = kB;
						this->Ninv[i] = -1;
						++kB;
						break;
					case soplex::SPxSolver::ZERO:
						this->N[kN] = i;
						this->Ninv[i] = kN;
						this->Binv[i] = -1;
						++kN;
						break;
					case soplex::SPxSolver::FIXED:
					case soplex::SPxSolver::ON_LOWER:
						this->x[i] = this->l[i].value;
						this->N[kN] = i;
						this->Ninv[i] = kN;
						this->Binv[i] = -1;
						++kN;
						break;
					case soplex::SPxSolver::ON_UPPER:
						this->x[i] = this->u[i].value;
						this->N[kN] = i;
						this->Ninv[i] = kN;
						this->Binv[i] = -1;
						++kN;
						break;
						break;
					default:
						throw std::runtime_error( "Unexpected SoPlex basis status." );
						break;
				}
			}

			for( int i = 0; i < m; ++i ){
				const int sV = n + i;
				switch( splex.getBasisRowStatus( i ) ){
					case soplex::SPxSolver::BASIC:
						this->B[kB] = sV;
						this->Binv[sV] = kB;
						this->Ninv[sV] = -1;
						++kB;
						break;
					case soplex::SPxSolver::ZERO:
						this->N[kN] = sV;
						this->Ninv[sV] = kN;
						this->Binv[sV] = -1;
						++kN;
						break;
					case soplex::SPxSolver::FIXED:
					case soplex::SPxSolver::ON_LOWER:
						this->x[sV] = this->l[sV].value;
						this->N[kN] = sV;
						this->Ninv[sV] = kN;
						this->Binv[sV] = -1;
						++kN;
						break;
					case soplex::SPxSolver::ON_UPPER:
						this->x[sV] = this->u[sV].value;
						this->N[kN] = sV;
						this->Ninv[sV] = kN;
						this->Binv[sV] = -1;
						++kN;
						break;
						break;
					default:
						throw std::runtime_error( "Unexpected SoPlex basis status." );
						break;
				}
			}

			if( kB != m || kN != n ){
				throw std::runtime_error( "SoPlex-Basis hat scheinbar nicht die korrekte Anzahl an Variablen/Nebenbedingungen." );
			}

			if( splex.status() == soplex::SPxSolver::INFEASIBLE ){
				this->rayGuess.resize( m );
				soplex::Vector tmpray( m, this->rayGuess.data() );
				if( splex.getDualfarkas( tmpray ) < 0 ){
					this->rayGuess.clear();
					#ifndef TO_DISABLE_OUTPUT
						std::cout << "SoPlex lieferte kein Farkas-Zertifikat." << std::endl;
					#endif
				}
			}
			this->hasBase = true;
		} catch( std::runtime_error& ex ){
			#ifndef TO_DISABLE_OUTPUT
				std::cerr << std::endl << "Unerwarteter Fehler bei Bestimmung der Basis mit SoPlex:" << std::endl << ex.what() << std::endl << std::endl;
				std::cout << "Bestimme Basis selbst." << std::endl;
			#endif
		} catch( ... ){
			#ifndef TO_DISABLE_OUTPUT
				std::cerr << std::endl << "Völlig unerwarteter Fehler bei Bestimmung der Basis mit SoPlex." << std::endl << std::endl;
				std::cout << "Bestimme Basis selbst." << std::endl;
			#endif
		}

		soplextime = clock() - soplextime;

	#endif

	if( this->checkDualFarkas() ){
		#ifndef TO_DISABLE_OUTPUT
			std::cout << "DualFarkas ok! No optimization needed. Problem is infeasible." << std::endl;
		#endif

		// x und d berechnen

		{
			std::vector<T> btilde( m, T( 0 ) );

			for( int i = 0; i < n; ++i ){
				const int j = N[i];
				T xj = x[j];
				if( j < n ){
					const int cend = Acolpointer[j+1];
					for( int k = Acolpointer[j]; k < cend; ++k ){
						btilde[Acolwiseind[k]] -= Acolwise[k] * xj;
					}
				} else {
					// logische Variable
					btilde[j-n] -= xj;
				}

			}

			this->FTran( btilde.data() );
			for( int i = 0; i < m; ++i ){
				x[B[i]] = btilde[i];
			}
		}

		{

			std::vector<T> y( m, T( 0 ) );

			for( int i = 0; i < m; ++i ){
				if( B[i] < n ){
					y[i] = this->c[B[i]];
				} else {
					y[i] = T( 0 );
				}
			}
			this->BTran( y.data() );


			std::vector<T> tmp( n, T( 0 ) );

			this->mulANT_threaded( tmp.data(), y.data() );

			for( int i = 0; i < n; ++i ){
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
		for( int j = 0; j < m; ++j ){
			this->B[j] = n+j;
			this->Binv[n+j] = j;
			this->Ninv[n+j] = -1;
		}

		for( int j = 0; j < n; ++j ){
			this->N[j] = j;
			this->Binv[j] = -1;
			this->Ninv[j] = j;
		}

		this->hasBase = true;

		this->refactor();
	}

	// Perturbation (Block entfernen um Pertubation zu verhindern)
//	T cmin( 1 );
//
//	for( int i = 0; i < n; ++i ){
//		if( this->c[i] != 0 ){
//			if( c[i] < cmin && -c[i] < cmin ){
//				cmin = c[i] >= 0 ? c[i] : -c[i];
//			}
//		}
//	}
//
//	std::vector<T> cold = this->c;
//	this->c.clear();
//
//	for( int i = 0; i < n; ++i ){
//		this->c.push_back( cold[i] + cmin / T( 10000 + n + i ) );
//	}
//
//	this->opt( false );
//
//	this->c = cold;
	// Perturbation Ende

	int retval = this->opt( false );

	#ifndef TO_DISABLE_OUTPUT
		#ifdef TO_WITH_CPLEX
			std::cout << "CPLEX-Zeit: " << cplextime / (double) CLOCKS_PER_SEC << std::endl;
		#endif
		#ifdef TO_WITH_GUROBI
			std::cout << "Gurobi-Zeit: " << gurobitime / (double) CLOCKS_PER_SEC << std::endl;
		#endif
		#ifdef TO_WITH_CLP
			std::cout << "CLP-Zeit: " << clptime / (double) CLOCKS_PER_SEC << std::endl;
		#endif
		#ifdef TO_WITH_SOPLEX
			std::cout << "SoPlex-Zeit: " << soplextime / (double) CLOCKS_PER_SEC << std::endl;
		#endif

		std::cout << "Optimierungszeit: " << ( ( clock() - fulltime ) / (double) CLOCKS_PER_SEC ) << " Sekunden" << std::endl;
	#endif

	if( !retval ){
		this->rayGuess.clear();
		this->farkasProof.clear();
	}

	return retval;
}


template <class T>
int TOSolver<T>::opt( bool P1 ){

	#ifndef TO_DISABLE_OUTPUT
		std::cout << "Optimiere..." << std::endl;
		clock_t starttime = clock();
	#endif

	this->lastLeavingBaseVar = -1;


	// Step 1: Initialization
	std::vector<T> y( m );

	bool feas = true;

	do {

		// Nichtbasisvariablen in ihre Schranken weißen!
		for( int i = 0; i < n; ++i ){
			const int j = this->N[i];
			if( !l[j].isInf && !u[j].isInf && x[j] != l[j].value && x[j] != u[j].value ){
				x[j] = l[j].value;
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
				std::vector<T> btilde(m);

				for( int i = 0; i < n; ++i ){
					const int j = N[i];
					T xj = x[j];
					if( j < n ){
						const int cend = Acolpointer[j+1];
						for( int k = Acolpointer[j]; k < cend; ++k ){
							btilde[Acolwiseind[k]] -= Acolwise[k] * xj;
						}
					} else {
						// logische Variable
						btilde[j-n] -= xj;
					}

				}

				this->FTran( btilde.data() );
				for( int i = 0; i < m; ++i ){
					x[B[i]] = btilde[i];
				}
			}

			{
				for( int i = 0; i < m; ++i ){
					if( B[i] < n ){
						y[i] = this->c[B[i]];
					} else {
						y[i] = 0;
					}
				}
				this->BTran( y.data() );


				std::vector<T> tmp(n);

				this->mulANT_threaded( tmp.data(), y.data() );

				for( int i = 0; i < n; ++i ){
					if( N[i] < n ){
						d[i] =  this->c[N[i]] - tmp[i];
					} else {
						d[i] = - tmp[i];
					}
				}
			}

			dfcdone = false;
			for( int i = 0; i < n; ++i ){
				const int j = this->N[i];
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
			for( int i = 0; i < n ; ++i ){
				int j = N[i];
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
				if( !this->phase1() ){
					#ifndef TO_DISABLE_OUTPUT
						std::cout << "Phase 1: LP dual infeasible!" << std::endl << std::endl;
					#endif
					return 2;
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
	std::vector<int> Qind(n);
	std::vector<int> Qord(n);
	std::vector<T> alphartilde(n);
	T deltatilde( 0 );

	int iter = 0;
	#ifndef TO_DISABLE_OUTPUT
		clock_t itertime = clock();
	#endif

	T Zold( 0 );
	int cyclecounter = 0;
	int nocyclecounter = 0;
	this->antiCycle = false;

	clock_t time1;
	clock_t time2 = clock();

	unsigned long long time_pricing = 0;
	unsigned long long time_btran = 0;
	unsigned long long time_dse = 0;
	unsigned long long time_pivot = 0;
	unsigned long long time_ratio = 0;
	unsigned long long time_ftran = 0;
	unsigned long long time_update = 0;

	while( true ){

		if( this->DSE.size() && !this->DSEhelper.ready ){
			this->DSEhelper.plex = this;
			this->DSEhelper.DSEWork = false;
			this->DSEhelper.mainWork = false;
			this->DSEhelper.exit = false;
			pthread_mutex_init( &this->DSEhelper.mutex, NULL );
			pthread_cond_init( &this->DSEhelper.mainCond, NULL );
			pthread_cond_init( &this->DSEhelper.DSECond, NULL );
			pthread_cond_init( &this->DSEhelper.readyCond, NULL );
			pthread_create( &this->DSEthread, NULL, DSE_threaded_helper, &this->DSEhelper );
		}

		#ifndef TO_DISABLE_OUTPUT
			clock_t oldtime = itertime;
			itertime = clock();
		#endif

		// TODO weq, mit Updateformel updaten
		T Z( 0 );
		for( int i = 0; i < n; ++i ){
			Z += c[i] * x[i];
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

		Zold = Z;


		if( !( iter++ % 10 ) )
		{
			#ifndef TO_DISABLE_OUTPUT
				std::string info = P1 ? "(Duale Phase 1) Duale Unzulässigkeit: " : "(Duale Phase 2) Primaler Zielfunktionswert: ";
				std::cout << "Iteration " << iter << ".\t" << info << Z.get_d() << std::endl;
				std::cout << "Dauer: " << ( itertime - oldtime ) / (double) CLOCKS_PER_SEC << " s." << std::endl;
			#endif
		}


		// Obiges nicht mitmessen
		time1 = time2;
		time2 = clock();


		// Step 2: Pricing
//		std::cout << "Pricing" << std::endl;

		int r = 0;
		int p = m+n;
		T delta;
		bool ratioNeg;

		if( this->DSE.size() && !this->antiCycle ){
			T max( 0 );
			for( int i = 0; i < m; ++i ){
				const int b = this->B[i];
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
			for( int i = 0; i < m; ++i ){
				const int b = this->B[i];
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


		// DSE-FTran-Thread starten
		if( this->DSE.size() ){
			for( int i = 0; i < m; ++i ){
				tau[i] = rhor[i];
			}
			// Daten an Thread übergeben und ihn aufwecken
			this->DSEhelper.tau = tau.data();

			pthread_mutex_lock( &this->DSEhelper.mutex );
			while( !this->DSEhelper.ready ){
				pthread_cond_wait( &this->DSEhelper.readyCond, &this->DSEhelper.mutex );
			}
			this->DSEhelper.mainWork = false;
			this->DSEhelper.DSEWork = true;
			pthread_cond_signal( &this->DSEhelper.DSECond );
			pthread_mutex_unlock( &this->DSEhelper.mutex );
		}


		time1 = time2;
		time2 = clock();

		time_dse += time2-time1;



		// Step 4: Pivot row
//		std::cout << "Pivot row" << std::endl;

		std::vector<T> alphar(n);

		this->mulANT_threaded( alphar.data(), rhor.data() );

		time1 = time2;
		time2 = clock();

		time_pivot += time2-time1;



		// Step 5: Ratio Test
//		std::cout << "Ratio Test" << std::endl;

		if( ratioNeg ){
			for( int i = 0; i < n; ++i ){
				alphartilde[i] = - alphar[i];
				deltatilde = - delta;
			}
		} else {
			for( int i = 0; i < n; ++i ){
				alphartilde[i] = alphar[i];
				deltatilde = delta;
			}
		}


		std::vector<T> atilde(m);
		bool flip = false;
		int q = 0;
		int s = 0;
		{
			int Qlen = 0;
			for( int i = 0; i < n; ++i ){
				int j = this->N[i];
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



			// TODO nach unten oder nicht??
			if( !Qlen ){

				if( this->DSE.size() ){
					// TODO Wir warten wegen segfaults. Thread evtl. abbrechen? Break?
					pthread_mutex_lock( &this->DSEhelper.mutex );
					while( !this->DSEhelper.mainWork ){
						pthread_cond_wait( &this->DSEhelper.mainCond, &this->DSEhelper.mutex );
					}
					pthread_mutex_unlock( &this->DSEhelper.mutex );
				}

				this->lastLeavingBaseVar = p;

				#ifndef TO_DISABLE_OUTPUT
					std::cout << "dual unbounded" << std::endl << std::endl;
					std::cout << iter << " Iterationen" << std::endl;
				#endif
				return 1;
			}


			if( this->antiCycle ){	// Bland

				T min = Q[0];
				s = Qind[0];

				for( int i = 1; i < Qlen; ++i ){
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
						T tmp = deltatilde - ( this->u[q].value - this->l[q].value ) * abs( alphar[s] );
						if( tmp < 0 ){
							break;
						}

						if( x[q] == l[q].value ){
							x[q] = u[q].value;
							flip = true;


							T diff = u[q].value - l[q].value;
							if( q < n ){
								int kend = Acolpointer[q+1];
								for( int k = Acolpointer[q]; k < kend; ++k ){
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
								int kend = Acolpointer[q+1];
								for( int k = Acolpointer[q]; k < kend; ++k ){
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

		time1 = time2;
		time2 = clock();

		time_ratio += time2-time1;


		// TODO in extra Thread auslagern?
		// Step 6: FTran
//		std::cout << "FTran" << std::endl;

		std::vector<T> alphaq(m);
		if( q < n ){
			const int kend = this->Acolpointer[q+1];
			for( int k = this->Acolpointer[q]; k < kend; ++k ){
				alphaq[Acolwiseind[k]] = Acolwise[k];
			}
		} else {
			alphaq[q-n] = 1;
		}
		std::vector<T> permSpike( m );	// TODO global, damit Konstruktor nicht immer aufgerufen wird
		std::vector<int> permSpikeInd( m );
		int permSpikeLen;
		this->FTran( alphaq.data(), permSpike.data(), permSpikeInd.data(), &permSpikeLen );


		time1 = time2;
		time2 = clock();

		time_ftran += time2-time1;



		// Step 7: Basis change and update
//		std::cout << "Update" << std::endl;

		T thetaD = d[s] / alphar[s];

		for( int i = 0; i < n; ++i ){
			d[i] -= thetaD * alphar[i];
		}

		d[s] = -thetaD;

		if( flip )
		{
//			std::cout << "FLIP!!!" << std::endl;
			FTran( atilde.data() );
			for( int i = 0; i < m; ++i ){
				x[B[i]] -= atilde[i];
			}
		}


		T thetaP = delta / alphaq[r];

		for( int i = 0; i < m; ++i ){
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

			pthread_mutex_lock( &this->DSEhelper.mutex );
			while( !this->DSEhelper.mainWork ){
				pthread_cond_wait( &this->DSEhelper.mainCond, &this->DSEhelper.mutex );
			}
			pthread_mutex_unlock( &this->DSEhelper.mutex );


			time1 = time2;
			time2 = clock();
			time_dse += time2-time1;


			T mult;
			for( int i = 0; i < m; ++i ){
				if( i != r ) {
					mult = alphaq[i] / alphaq[r];
					this->DSE[i] += - 2 * mult * tau[i] + mult * mult * betar;
				}
			}
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
				for( int i = 0; i < m; ++i ){
					this->DSEtmp[B[i]] = this->DSE[i];
				}
			}

			this->refactor();

			// DSE für permutierte Basis zurückholen
			if( this->DSE.size() ){
				for( int i = 0; i < m; ++i ){
					this->DSE[i] = this->DSEtmp[B[i]];
				}
			}

			#ifndef TO_DISABLE_OUTPUT
				unsigned long long time_sum = time_pricing + time_btran + time_dse + time_pivot + time_ratio + time_ftran + time_update;

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
			this->recalcDSE();
		}


		time1 = time2;
		time2 = clock();

		time_update += time2-time1;

	}


	#ifndef TO_DISABLE_OUTPUT
		T Z;
		for( int i = 0; i < n; ++i ){
			Z += c[i] * x[i];
		}
		std::cout << "Zielfunktionswert: Double: " << Z.get_d() << std::endl;
		std::cout << "Zielfunktionswert: GMP-Float: ";
		mpf_class Zfl( 0, 256 );
		Zfl = Z;

		std::streamsize oldp = std::cout.precision(75);
		std::cout << Zfl << std::endl;
		std::cout.precision( oldp );

		std::cout << "Exakt: " << Z << std::endl;

		// simple Messung der Größe des Ergebnisses
		std::string base2res = Z.get_str(2);
//		std::cout << "Base 2: " << base2res << std::endl;
		int base2size = base2res.size() + 1;	// 1 für Vorzeichen

		// - und / nicht mitzählen
		if( base2res.find( "-" ) != std::string::npos ){
			// Ein Bit für ein Vorzeichen rechnen wir sowieso oben.
			--base2size;
		}
		if( base2res.find( "/" ) != std::string::npos ){
			// Den Bruchstrich nicht mitzählen
			--base2size;
		} else {
			// Ein Bit für den Nenner 1 dazurechnen
			++base2size;
		}
		std::cout << "Kodierungslänge: " << base2size << std::endl;

		std::cout << iter << " Iterationen" << std::endl;

		std::cout << "Zeit: " << ( ( clock() - starttime ) / (double) CLOCKS_PER_SEC ) << " Sekunden" << std::endl;
	#endif

	return 0;

}


template <class T>
std::vector<T> TOSolver<T>::getX(){
	std::vector<T> retx = this->x;
	retx.resize( n );
	return retx;
}


template <class T>
std::vector<T> TOSolver<T>::getY(){
	std::vector<T> y(m);
	for( int i = 0; i < m; ++i ){
		if( B[i] < n ){
			y[i] = this->c[B[i]];
		} else {
			y[i] = 0;
		}
	}
	this->BTran( y.data() );

	return y;
}


template <class T>
std::vector<T> TOSolver<T>::getD(){
	std::vector<T> retd( n );
	for( int i = 0; i < n; ++i ){
		if( N[i] < n ){
			retd[N[i]] = d[i];
		}
	}
	return retd;
}


template <class T>
std::pair<TORationalInf<T>,TORationalInf<T> > TOSolver<T>::getConstraintBounds( const unsigned int i ){
	std::pair<TORationalInf<T>,TORationalInf<T> > retval;
	const int s = n + i;
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


#ifndef TO_WITHOUT_DOUBLE

template<>
inline std::pair<std::vector<mpq_class>, mpq_class> TOSolver<mpq_class>::getGMI( int ind, std::vector<bool> intVars, unsigned int kcut ){

	if( ind >= n+m ){
		throw std::runtime_error( "Invalid index" );
	}

	if( !kcut ){
		throw std::runtime_error( "Invalid k." );
	}

	std::vector<mpq_class> tmp( m );
	tmp[ Binv[ ind ] ] = 1;
	BTran( tmp.data() );
	std::vector<mpq_class> gmicoeff( n );
	mulANT_threaded( gmicoeff.data(), tmp.data() );


	// Generate GMI
//	std::vector<mpq_class> s(n+m);
	mpq_class ai0;
	for( int k = 0; k < n; ++k ){
		int nind = N[k];
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
	std::vector<mpq_class> cutcoeff(n);
	mpq_class fi0 = ai0 * kcut - mpq2mpz_floor( ai0 * kcut );
	if( fi0 == 0 ){
		throw std::runtime_error( "Separation impossible." );
	}
	for( int k = 0; k < n; ++k ){
		int nind = N[k];
		mpq_class & aij = gmicoeff[k];
		if( intVars[nind] ){
			// Integer nonbasic Variables
			mpq_class fij = aij * kcut - mpq2mpz_floor( aij * kcut );
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
				cutcoeff[k] = fi0*(-aij*kcut)/(1-fi0);
			}
		}
	}

	// Cut zurückrechnen
	mpq_class beta = fi0;
	for( int k = 0; k < n; ++k ){
		int nind = N[k];
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

	std::vector<mpq_class> cut(n);
	for( int k = 0; k < n; ++k ){
		int nind = N[k];
		if( nind < n ){
			cut[nind] += cutcoeff[k];
		} else {
			if( cutcoeff[k] != 0 ){
				for( int l = Arowpointer[nind - n]; l < Arowpointer[nind - n + 1]; ++l ){
					cut[ Arowwiseind[l] ] -= cutcoeff[k] * Arowwise[l];
				}
			}
		}
	}

	return make_pair(cut, beta);
}


template <>
inline bool TOSolver<mpq_class>::checkDualFarkas(){

	if( this->rayGuess.size() && ( this->hasBasisMatrix || ( this->hasBase && this->refactor() ) ) ){

		#ifndef TO_DISABLE_OUTPUT
			std::cout << "Prüfe Farkas-Zertifikat." << std::endl;
		#endif

		unsigned int counter = 0;

		std::vector<mpq_class> ytest( m, 0 );

		{
			std::vector<mpq_class> er( m, 0 );
			std::vector<mpq_class> rayGuessEx( m );

			for( int i = 0; i < m; ++i ){
				if( this->rayGuess[i] != this->rayGuess[i] || this->rayGuess[i] > 1e30 || this->rayGuess[i] < -1e30 ){	// NaN
					return false;
				}
				rayGuessEx[i] = this->rayGuess[i];
			}

			// B^T * ray
			for( int i = 0; i < m; ++i ){
				int bi = this->B[i];
				if( bi < n ){
					int clen = Acolpointer[bi+1] - Acolpointer[bi];
					for( int j = 0, k = Acolpointer[bi]; j < clen; ++j, ++k ){
						er[i] += Acolwise[k] * rayGuessEx[Acolwiseind[k]];
					}
				} else {
					er[i] += rayGuessEx[bi-n];
				}

			}


			for( int i = 0; i < m; ++i ){

				mpq_class tmpval = er[i] + mpq_class( 1, 2 );

				mpz_class tmpval2 = mpq2mpz_floor( tmpval );
				mpq_class tmpval3 = er[i] - tmpval2;


				// 1e-3 empirisch bestimmt

				if( abs( tmpval3 ) < 1e-3 ){
					ytest[i] = tmpval2;
				} else {
					ytest[i] = er[i];
				}

				if( abs( ytest[i] ) > 1e-3 ){
					++counter;
				}

			}

		}

		if( !counter ){
			return false;
		}

		this->BTran( ytest.data() );

		std::vector<mpq_class> dtest( n, 0 );

		// d-Ray aus y-Ray berechnen
		for( int i = 0; i < m; ++i ){
			if( ytest[i] != 0 ){
				for( int j = Arowpointer[i]; j < Arowpointer[i+1]; ++j ){
					dtest[Arowwiseind[j]] -= ytest[i] * Arowwise[j];
				}
			}
		}

		bool rayok = true;
		// duale Zulässigkeit von d prüfen
		for( int i = 0; i < n; ++i ){
			if( l[i].isInf && u[i].isInf && dtest[i] != 0 ){
				rayok = false;
				break;
			} else if( !l[i].isInf && u[i].isInf && dtest[i] < 0 ){
				rayok = false;
				break;
			} else if( l[i].isInf && !u[i].isInf && dtest[i] > 0 ){
				rayok = false;
				break;
			}
		}

		if( !rayok ){
			return false;
		}

		// prüfen, ob b'y + ... > 0
		mpq_class sum = 0;
		for( int i = 0; i < m; ++i ){
			sum += ytest[i] * ( ytest[i] < 0 ? -l[n+i].value : -u[n+i].value );
		}
		for( int i = 0; i < n; ++i ){
			if( dtest[i] > 0 ){
				sum += dtest[i] * l[i].value;
			} else {
				sum += dtest[i] * u[i].value;
			}
		}

		if( sum <= 0 ){
			rayok = false;
		}

		if( rayok ){
			this->farkasProof = ytest;
		}

		return rayok;
	}

	return false;
}

#endif


template <class T>
inline bool TOSolver<T>::checkDualFarkas(){
	// Only implemented for mpq_class
	// TODO implement for other Types
	return false;
}


template <class T>
T TOSolver<T>::getObj(){
	T Z( 0 );
	for( int i = 0; i < n; ++i ){
		Z += c[i] * x[i];
	}
	return Z;
}

}

#endif
