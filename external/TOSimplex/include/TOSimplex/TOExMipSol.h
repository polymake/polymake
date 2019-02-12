/* Copyright (c) 2015-2018
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

#ifndef TOEXMIPSOL_H
#define TOEXMIPSOL_H

// TODO: k-cut

#include <queue>
#include <sstream>
#include <map>

#include "TOSimplex.h"


namespace TOExMipSol {

template <class T>
struct rowElement {
	T mult;
	int index;
};

template <class T>
struct constraint {
	std::vector<rowElement<T> > constraintElements;
	int type;
	T rhs;
};

template <class T>
struct MIP {
	std::map<std::string, int> varNumbers;
	std::vector<std::string> vars;
	std::vector<T> lbounds;
	std::vector<T> ubounds;
	std::vector<bool> linf;
	std::vector<bool> uinf;
	std::vector<char> numbersystems;

	bool maximize = false;
	std::vector<constraint<T> > matrix;
	std::vector<rowElement<T> > objfunc;
	std::vector<std::string> varNames;
};


template <class T>
class BnBNode
{
	private:
		BnBNode* leftChild;
		BnBNode* rightChild;

	public:
		std::vector<unsigned int> vars;
		const unsigned int depth;
		std::vector<bool> whichBounds;
		std::vector<T> Bounds;
		const T priority;
		T bound;
		BnBNode* parent;
		bool deleted;

		BnBNode( BnBNode* parent_, const int pos, const unsigned int var, const bool whichBound, const T& Bound, const T& priority_, const T& bound_, const unsigned int depth_ ):
			leftChild( NULL ),
			rightChild( NULL ),
			vars( 1, var ),
			depth( depth_ ),
			whichBounds( 1, whichBound ),
			Bounds( 1, Bound ),
			priority( priority_ ),
			bound( bound_ ),
			parent( parent_ ),
			deleted( false )
		{
			if( pos == 1 && parent != NULL ){
				parent->leftChild = this;
			} else if( pos == 2 && parent != NULL ){
				parent->rightChild = this;
			} else if( pos == -1 && parent == NULL ){
				// ok, root node
			} else {
				throw std::runtime_error( "Wrong node position." );
			}
		}

		BnBNode( const T& bound_ ):
			leftChild( NULL ),
			rightChild( NULL ),
			vars( 0 ),
			depth( 0 ),
			whichBounds( 0 ),
			Bounds( 0 ),
			priority( 0 ),
			bound( bound_ ),
			parent( NULL ),
			deleted( false ){}

		~BnBNode() noexcept( false ){

			if( leftChild || rightChild ){

				#ifndef TO_DISABLE_OUTPUT
					if( leftChild )
						pm::cout << leftChild->deleted << std::endl;

					if( rightChild )
						pm::cout << rightChild->deleted << std::endl;

					pm::cout << this->deleted << std::endl;
				#endif

				throw std::runtime_error( "Node still has children" );
			}



			{
				if( parent ){
					if( parent->leftChild == this ){
						parent->leftChild = NULL;
					}
					if( parent->rightChild == this ){
						parent->rightChild = NULL;
					}
					if( !( parent->leftChild || parent->rightChild ) ){
						delete parent;
					}
				}

			}

		}

		void setDeleted(){

			if( this->parent && this->parent->rightChild == this ){
				this->parent->rightChild = NULL;
			} else if( this->parent && this->parent->leftChild == this ){
				this->parent->leftChild = NULL;
			}

			this->parent = NULL;



			this->deleted = true;

			if( this->leftChild ){
				this->leftChild->setDeleted();
			}
			if( this->rightChild ){
				this->rightChild->setDeleted();
			}

		}

		void setChildrenDeleted(){
			if( this->leftChild ){
				this->leftChild->setDeleted();
			}
			if( this->rightChild ){
				this->rightChild->setDeleted();
			}
		}
};


template<typename T>
class ComparePointerPriorities {
	public:
		bool operator()( T &t1, T &t2 )
		{
			return t2->priority < t1->priority;
		}
};





template <class T>
class TOMipSolver
{

	public:
		TOMipSolver();

		enum solstatus {
			UNSOLVED = -1,
			OPTIMAL = 0,
			INFEASIBLE = 1,
			UNBOUNDED = 2,
			INForUNB = 3
		};

		solstatus solve( MIP<T> mip, bool allSolutions, T& optimalValue, std::vector<T>& optimalAssignment, std::vector<std::vector<T> >* allAssignments );
		std::string solstatusToString( solstatus solStatus ){
			switch( solStatus ){
				case UNSOLVED:
					return "unsolved";
					break;
				case OPTIMAL:
					return "optimal";
					break;
				case INFEASIBLE:
					return "infeasible";
					break;
				case UNBOUNDED:
					return "unbounded";
					break;
				case INForUNB:
					return "infeasible or unbounded";
					break;
			}
			return "unknown status";
		}

	private:

		solstatus BnB( const MIP<T>& mip, TOSimplex::TOSolver<T>& plex, bool allSolutions, T& objval, std::vector<T>& optimalAssignment, std::vector<std::vector<T> >* allAssignments );


};


template <class T>
TOMipSolver<T>::TOMipSolver(){

}



template <class T>
typename TOMipSolver<T>::solstatus TOMipSolver<T>::BnB( const MIP<T>& mip, TOSimplex::TOSolver<T>& plex, bool allSolutions, T& optimalValue, std::vector<T>& optimalAssignment, std::vector<std::vector<T> >* allAssignments ){

	unsigned int numsol = 0;

	#ifndef TO_DISABLE_OUTPUT
		pm::cout << "Starte BnB" << std::endl;
	#endif
	std::priority_queue<BnBNode<T>*,std::vector<BnBNode<T>*>,ComparePointerPriorities<BnBNode<T>*>> queue;
	queue.push( new BnBNode<T>( plex.getObj() ) );

	const unsigned int n = mip.lbounds.size();
	std::vector<long long> branchPriorities( n, 0 );

	optimalAssignment.resize( 0 );
	optimalValue = T( 0 );
	if( allAssignments ){
		allAssignments->resize( 0 );
	}

	while( !queue.empty() ){

		BnBNode<T>* bs = queue.top();
		queue.pop();

		if( bs->deleted ){
			delete bs;
			continue;
		}

		if( !allSolutions && optimalAssignment.size() && bs->bound >= optimalValue ){
			#ifndef TO_DISABLE_OUTPUT
				pm::cout << "Cutoff 1: " << TOmath<T>::toShortString( bs->bound ) << " / " << TOmath<T>::toShortString( optimalValue ) << std::endl;
			#endif
			delete bs;
			continue;
		}

		std::vector<T> tmplbounds = mip.lbounds;
		std::vector<T> tmpubounds = mip.ubounds;

		{
			BnBNode<T>* tmpnode = bs;
			do{
				for( unsigned int i = 0; i < tmpnode->vars.size(); ++i ){
					if( !tmpnode->whichBounds[i] ){
						tmplbounds[ tmpnode->vars[i] ] = std::max( tmplbounds.at( tmpnode->vars[i] ), tmpnode->Bounds[i] );
					} else {
						tmpubounds[ tmpnode->vars[i] ] = std::min( tmpubounds.at( tmpnode->vars[i] ), tmpnode->Bounds[i] );
					}

					if( tmplbounds.at( tmpnode->vars[i] ) > tmpubounds.at( tmpnode->vars[i] ) ){
						bs->deleted = true;
						break;
					}
				}
				if( bs->deleted ){
					break;
				}
			} while( ( tmpnode = tmpnode->parent ) );
		}

		if( bs->deleted ){
			delete bs;
			continue;
		}

		for( unsigned int i = 0; i < n; ++i ){
			if( mip.numbersystems[i] != 'R' ){
				plex.setVarBounds( i, TOSimplex::TORationalInf<T>( tmplbounds[i] ), TOSimplex::TORationalInf<T>( tmpubounds[i] ) );
			}
		}

		if( plex.opt() ){
			#ifndef TO_DISABLE_OUTPUT
				pm::cout << "Node-LP Infeasible.";
				if( optimalAssignment.size() ){
					pm::cout << " (current best: " << optimalValue << ")";
				}
				pm::cout << std::endl;
			#endif

			if( bs->vars.size() == 1 ){
				#ifndef TO_DISABLE_OUTPUT
					pm::cout << "Backtracking" << std::endl;
				#endif

				BnBNode<T>* backTrackStart = bs->parent;
				do{
					std::vector<T> tmplbounds2 = mip.lbounds;
					std::vector<T> tmpubounds2 = mip.ubounds;

					if( backTrackStart && backTrackStart->parent ){
						if( !bs->whichBounds[0] ){
							tmplbounds2[ bs->vars[0] ] = std::max( tmplbounds2.at( bs->vars[0] ), bs->Bounds[0] );
						} else {
							tmpubounds2[ bs->vars[0] ] = std::min( tmpubounds2.at( bs->vars[0] ), bs->Bounds[0] );
						}

						BnBNode<T>* tmpnode = backTrackStart->parent;
						do{
							for( unsigned int i = 0; i < tmpnode->vars.size(); ++i ){
								if( !tmpnode->whichBounds[i] ){
									tmplbounds2[ tmpnode->vars[i] ] = std::max( tmplbounds2.at( tmpnode->vars[i] ), tmpnode->Bounds[i] );
								} else {
									tmpubounds2[ tmpnode->vars[i] ] = std::min( tmpubounds2.at( tmpnode->vars[i] ), tmpnode->Bounds[i] );
								}
							}
						} while( ( tmpnode = tmpnode->parent ) );
					} else {
						break;
					}

					for( unsigned int i = 0; i < n; ++i ){
						if( mip.numbersystems[i] != 'R' ){
							plex.setVarBounds( i, TOSimplex::TORationalInf<T>( tmplbounds2[i] ), TOSimplex::TORationalInf<T>( tmpubounds2[i] ) );
						}
					}

					if( plex.opt() || ( !allSolutions && optimalAssignment.size() && plex.getObj() > optimalValue ) ){
						#ifndef TO_DISABLE_OUTPUT
							pm::cout << "Success" << std::endl;
						#endif
						++branchPriorities[bs->vars[0]];
					} else {
						#ifndef TO_DISABLE_OUTPUT
							pm::cout << "No success" << std::endl;
						#endif
						break;
					}
				} while( ( backTrackStart = backTrackStart->parent ) );

				if( backTrackStart != bs->parent ){
//					backTrackStart->setChildrenDeleted();
					backTrackStart->vars.push_back( bs->vars[0] );
					backTrackStart->whichBounds.push_back( !bs->whichBounds[0] );
					if( !bs->whichBounds[0] ){
						// Andere Schranke!!!
						backTrackStart->Bounds.push_back( bs->Bounds[0] - T( 1 ) );
					} else {
						backTrackStart->Bounds.push_back( bs->Bounds[0] + T( 1 ) );
					}

					// TODO Objval-Schranke anpassen?
//					queue.push( backTrackStart );

				}
			} else {
				// TODO: Kommt das noch vor?
				#ifndef TO_DISABLE_OUTPUT
					pm::cout << "No backtracking." << std::endl;
				#endif
			}

			delete bs;
			continue;

		}

		T objval = plex.getObj();

		if( !allSolutions && optimalAssignment.size() && objval >= optimalValue ){
			#ifndef TO_DISABLE_OUTPUT
				pm::cout << "Cutoff 2: " << TOmath<T>::toShortString( objval ) << " / " << TOmath<T>::toShortString( optimalValue ) << std::endl;
			#endif
			delete bs;
			continue;
		}

		int branchVar = -1;
		std::vector<T> x = plex.getX();
		for( unsigned int i = 0; i < n; ++i ){
			if( mip.numbersystems[i] != 'R' && !TOmath<T>::isInt( x[i] ) ){
//				cout << "x" << i << " = " << x[i] << " fractional. Branching." << endl;
				if( branchVar == -1 || branchPriorities[i] > branchPriorities[branchVar] ){
					branchVar = i;
				}
			}
		}

		if( allSolutions && branchVar == -1 ){
			for( unsigned int i = 0; i < n; ++i ){
				if( mip.numbersystems[i] != 'R' && tmplbounds[i] != tmpubounds[i] ){
					x[i] = tmplbounds[i] + ( T(1) / T(2) );
//					cout << "x" << i << " = " << x[i] << " forced fractional. Branching." << endl;
					if( branchVar == -1 || branchPriorities[i] > branchPriorities[branchVar] ){
						branchVar = i;
					}
				}
			}
		}

		if( branchVar == -1 ){
			if( optimalAssignment.size() == 0 || optimalValue > objval ){
				optimalValue = objval;
				if( !allSolutions ){
					plex.setInfeasibilityBound( optimalValue );
				}
				optimalAssignment = plex.getX();
			}
			if( allAssignments ){
				allAssignments->push_back( plex.getX() );
			}
			#ifndef TO_DISABLE_OUTPUT
				pm::cout << "found incumbent: " << TOmath<T>::toShortString( mip.maximize ? - objval : objval ) << std::endl;
			#endif
			if( bs->parent && bs->parent->vars.size() == 1 ){
				branchPriorities[bs->vars[0]] += 10;
			}
//			{
//				std::vector<T> incumbent = plex.getX();
//				pm::cout << "x =";
//				for( unsigned int i = 0; i < incumbent.size(); ++i ){
//					pm::cout << " " << incumbent[i];
//				}
//				pm::cout << std::endl;
//			}
			++numsol;
			delete bs;
		} else {
			queue.push( new BnBNode<T>( bs, 1, branchVar, 1, TOmath<T>::floor( x[branchVar] ), objval, objval, bs->depth + 1 ) );
			queue.push( new BnBNode<T>( bs, 2, branchVar, 0, TOmath<T>::ceil( x[branchVar] ), objval, objval, bs->depth + 1 ) );
		}

	}

	if( optimalAssignment.size() ){
		if( mip.maximize ){
			optimalValue = - optimalValue;
		}
		#ifndef TO_DISABLE_OUTPUT
			pm::cout << "Solution: " << TOmath<T>::toShortString( optimalValue ) << std::endl;
			pm::cout << "Found " << numsol << " solutions" << std::endl;
		#endif
		return solstatus::OPTIMAL;
	}

	return solstatus::INFEASIBLE;
}





template <class T>
typename TOMipSolver<T>::solstatus TOMipSolver<T>::solve( MIP<T> mip, bool allSolutions, T& optimalValue, std::vector<T>& optimalAssignment, std::vector<std::vector<T> >* allAssignments ){

	#ifndef TO_DISABLE_OUTPUT
		pm::cout << "Initialisiere LP-Solver." << std::endl;
	#endif

	{
		std::vector<constraint<T> > matrix2;
		bool modified = false;
		for( unsigned int i = 0; i < mip.matrix.size(); ++i ){
			if( mip.matrix[i].constraintElements.size() == 1 && mip.matrix[i].constraintElements[0].mult != T( 0 ) ){
				modified = true;
				int var = mip.matrix[i].constraintElements[0].index;
				T rhs = mip.matrix[i].rhs / mip.matrix[i].constraintElements[0].mult;

				int type = mip.matrix[i].constraintElements[0].mult >= T( 0 ) ? mip.matrix[i].type : - mip.matrix[i].type;

				if( type < 1 ) {	// <= oder ==
					if( mip.uinf[var] || rhs < mip.ubounds[var] ){
						mip.uinf[var] = false;
						mip.ubounds[var] = rhs;
						#ifndef TO_DISABLE_OUTPUT
							pm::cout << "Info: Adjusting upper bound of variable " << mip.varNames.at( var ) << " <= " << rhs << std::endl;
						#endif
					}
				}

				if( type > -1 ) {	// >= oder ==
					if( mip.linf[var] || rhs > mip.lbounds[var] ){
						mip.linf[var] = false;
						mip.lbounds[var] = rhs;
						#ifndef TO_DISABLE_OUTPUT
							pm::cout << "Info: Adjusting lower bound of variable " << mip.varNames.at( var ) << " >= " << rhs << std::endl;
						#endif
					}
				}

			} else {
				matrix2.push_back( mip.matrix[i] );
			}
		}
		if( modified ){
			mip.matrix = matrix2;
		}
	}

	const unsigned int m = mip.matrix.size();
	const unsigned int n = mip.lbounds.size();

	unsigned int nnz = 0;
	for( unsigned int i = 0; i < m; ++i ){
		nnz += mip.matrix.at( i ).constraintElements.size();
	}

	const T hugenegint = TOmath<T>::hugenegint();
	const T hugeposint = TOmath<T>::hugeposint();

	for( unsigned int i = 0; i < n; ++i ){
		if( mip.numbersystems.at( i ) == 'G' ){
//			if( mip.linf.at( i ) || mip.lbounds.at( i ) < hugenegint ){
//				pm::cout << "Warning: unbounded / huge integer variable. Setting " << mip.varNames.at( i ) << " >= " << hugenegint << std::endl;
//				mip.linf.at( i ) = 0;
//				mip.lbounds.at( i ) = hugenegint;
//			}
//			if( mip.uinf.at( i ) || mip.ubounds.at( i ) > hugeposint ){
//				pm::cout << "Warning: unbounded / huge integer variable. Setting " << mip.varNames.at( i ) << " <= " << hugeposint << std::endl;
//				mip.uinf.at( i ) = 0;
//				mip.ubounds.at( i ) = hugeposint;
//			}
		} else if( mip.numbersystems.at( i ) == 'B' ){
			if( mip.uinf.at( i ) || mip.ubounds.at( i ) > 1 ){
				#ifndef TO_DISABLE_OUTPUT
					pm::cout << "Warning: Adjusting upper bound of binary variable " << mip.varNames.at( i ) << std::endl;
				#endif
				mip.uinf.at( i ) = false;
				mip.ubounds.at( i ) = 1;
			}
			if( mip.linf.at( i ) || mip.lbounds.at( i ) < 0 ){
				#ifndef TO_DISABLE_OUTPUT
					pm::cout << "Warning: Adjusting lower bound of binary variable " << mip.varNames.at( i ) << std::endl;
				#endif
				mip.linf.at( i ) = false;
				mip.lbounds.at( i ) = 0;
			}
		}
		if( mip.numbersystems.at( i ) != 'R' ){
			if( !TOmath<T>::isInt( mip.lbounds.at( i ) ) ){
				#ifndef TO_DISABLE_OUTPUT
					pm::cout << "Warning: Adjusting fractional bounds to integer variables " << mip.varNames.at( i ) << std::endl;
				#endif
				mip.lbounds.at( i ) = TOmath<T>::ceil( mip.lbounds.at( i ) );
			}
			if( !TOmath<T>::isInt( mip.ubounds.at( i ) ) ){
				#ifndef TO_DISABLE_OUTPUT
					pm::cout << "Warning: Adjusting fractional bounds to integer variables " << mip.varNames.at( i ) << std::endl;
				#endif
				mip.ubounds.at( i ) = TOmath<T>::floor( mip.ubounds.at( i ) );
			}
		}
	}


	std::vector<T> rows;
	rows.reserve( nnz );
	std::vector<int> colinds;
	colinds.reserve( nnz );
	std::vector<int> rowbegininds;
	rowbegininds.reserve( m + 1 );
	std::vector<T> obj( n, T( 0 ) );
	std::vector<TOSimplex::TORationalInf<T> > rowlowerbounds( m );
	std::vector<TOSimplex::TORationalInf<T> > rowupperbounds( m );
	std::vector<TOSimplex::TORationalInf<T> > varlowerbounds( n );
	std::vector<TOSimplex::TORationalInf<T> > varupperbounds( n );

	std::vector<bool> integerVariables( n, false );


	for( unsigned int i = 0; i < n; ++i ){
		if( mip.linf.at( i ) ){
			varlowerbounds.at( i ) = true;
		} else {
			varlowerbounds.at( i ) = mip.lbounds.at( i );
		}

		if( mip.uinf.at( i ) ){
			varupperbounds.at( i ) = true;
		} else {
			varupperbounds.at( i ) = mip.ubounds.at( i );
		}

		if( mip.numbersystems.at( i ) != 'R' ){
			integerVariables.at( i ) = true;
		}
	}


	for( unsigned int i = 0; i < m; ++i ){
		rowbegininds.push_back( rows.size() );
		for( unsigned int j = 0; j < mip.matrix.at( i ).constraintElements.size(); ++j ){
			rows.push_back( mip.matrix.at( i ).constraintElements.at( j ).mult );
			colinds.push_back( mip.matrix.at( i ).constraintElements.at( j ).index );
		}

		switch( mip.matrix.at( i ).type ){
			case -1:
				rowlowerbounds.at( i ) = true;
				rowupperbounds.at( i ) = mip.matrix.at( i ).rhs;
				break;
			case 1:
				rowlowerbounds.at( i ) = mip.matrix.at( i ).rhs;
				rowupperbounds.at( i ) = true;
				break;
			case 0:
				rowlowerbounds.at( i ) = mip.matrix.at( i ).rhs;
				rowupperbounds.at( i ) = mip.matrix.at( i ).rhs;
				break;
			default:
				throw std::runtime_error( "unexpected" );
				break;
		}
	}
	rowbegininds.push_back( rows.size() );

	TOSimplex::TOSolver<T> plex( rows, colinds, rowbegininds, obj, rowlowerbounds, rowupperbounds, varlowerbounds, varupperbounds );
	#ifndef TO_DISABLE_OUTPUT
		pm::cout << "LP-Solver initialisiert." << std::endl;
	#endif

	{
		int tmpstat = plex.opt();
		if( tmpstat == 1 ){
			return INFEASIBLE;
		} else if( tmpstat == 2 ){
			return INForUNB;
		}
	}

	std::vector<T> newlbounds = mip.lbounds;
	std::vector<T> newubounds = mip.ubounds;
	std::vector<bool> newlinf = mip.linf;
	std::vector<bool> newuinf = mip.uinf;

	for( unsigned int i = 0; i < n; ++i ){
		if( mip.numbersystems.at( i ) == 'G' ){
			if( mip.linf.at( i ) ){
				plex.setObj( i, T( 1 ) );
				if( !plex.opt() ){
					newlbounds.at( i ) = TOmath<T>::ceil( plex.getObj() );
					#ifndef TO_DISABLE_OUTPUT
						pm::cout << "Info: Adjusting lower bound of integer variable " << mip.varNames.at( i ) << " >= " << newlbounds.at( i ) << std::endl;
					#endif
				} else {
					pm::cerr << "Warning: unbounded / huge integer variable. Setting " << mip.varNames.at( i ) << " >= " << hugenegint << std::endl;
					newlbounds.at( i ) = hugenegint;
				}
				newlinf.at( i ) = false;
				plex.setObj( i, T( 0 ) );
			}
			if( mip.uinf.at( i ) ){
				plex.setObj( i, T( -1 ) );
				if( !plex.opt() ){
					newubounds.at( i ) = TOmath<T>::floor( - plex.getObj() );
					#ifndef TO_DISABLE_OUTPUT
						pm::cout << "Info: Adjusting upper bound of integer variable " << mip.varNames.at( i ) << " <= " << newubounds.at( i ) << std::endl;
					#endif
				} else {
					pm::cerr << "Warning: unbounded / huge integer variable. Setting " << mip.varNames.at( i ) << " <= " << hugeposint << std::endl;
					newubounds.at( i ) = hugeposint;
				}
				newuinf.at( i ) = false;
				plex.setObj( i, T( 0 ) );
			}
		}
	}

	mip.lbounds = newlbounds;
	mip.ubounds = newubounds;
	mip.linf = newlinf;
	mip.uinf = newuinf;

	for( unsigned int i = 0; i < n; ++i ){
		if( mip.linf.at( i ) ){
			plex.setVarLB( i, true );
		} else {
			plex.setVarLB( i, mip.lbounds.at( i ) );
		}

		if( mip.uinf.at( i ) ){
			plex.setVarUB( i, true );
		} else {
			plex.setVarUB( i, mip.ubounds.at( i ) );
		}
	}

	for( unsigned int i = 0; i < mip.objfunc.size(); ++i ){
		plex.setObj( mip.objfunc.at( i ).index, mip.maximize ? - mip.objfunc.at( i ).mult : mip.objfunc.at( i ).mult );
	}

	#ifndef TO_DISABLE_OUTPUT
		pm::cout << "Löse R00t-LP." << std::endl;
	#endif

	{
		std::vector<T> kCuts;
		kCuts.reserve( 16 );
		// TODO
		kCuts.push_back( T( 1 ) );
		kCuts.push_back( T( 2 ) );
		kCuts.push_back( T( 3 ) );
		kCuts.push_back( T( 4 ) );
		kCuts.push_back( T( 6 ) );
		kCuts.push_back( T( 8 ) );
		kCuts.push_back( T( 10 ) );
		unsigned int addedCuts;
		unsigned int rounds = 0;
		do{

			int rootstat = plex.opt();
			if( rootstat == 1 ){
				return INFEASIBLE;
			} else if( rootstat == 2 ){
				return INForUNB;
			}

			std::vector<T> rootSol = plex.getX();
			std::vector<std::pair<std::vector<T>, T>> cuts2add;
			unsigned int numFrac = 0;
			for( unsigned int i = 0; i < n; ++i ){
				for( unsigned int k = 0; k < kCuts.size(); ++k ){
					if( integerVariables.at( i ) && !TOmath<T>::isInt( rootSol.at( i ) ) ){
						++numFrac;	// TODO das muss eigentlich nur einmal über alle k gemacht werden??

						// Gomory-Cut
						try{
							std::pair<std::vector<T>, T> gmiCut = plex.getGMI( i, integerVariables, kCuts.at( k ) );

							unsigned int cutsize = 0;
							for( unsigned int j = 0; j < n; ++j ){
								if( gmiCut.first.at( j ) != T( 0 ) ){
									++cutsize;
								}
							}

							#ifndef TO_DISABLE_OUTPUT
								pm::cout << "Variable " << mip.varNames.at( i ) << " fractional: " << rootSol.at( i ) << " Cutsize: " << cutsize << std::endl;
							#endif

							if( cutsize <= 5 ){	// TODO

								for( unsigned int j = 0; j < n; ++j ){
									if( gmiCut.first.at( j ) == T( 0 ) ){
										continue;
									}
									#ifndef TO_DISABLE_OUTPUT
										pm::cout << " + " << gmiCut.first.at( j ) << " " << mip.varNames.at( j );
									#endif
								}
								#ifndef TO_DISABLE_OUTPUT
									pm::cout << " >= " << gmiCut.second << std::endl;
								#endif

								cuts2add.push_back( gmiCut );
							}

						} catch( std::runtime_error& ){
							continue;
						}
					}
				}
			}

			addedCuts = cuts2add.size();
			for( unsigned int i = 0; i < addedCuts; ++i ){
				plex.addConstraint( cuts2add[i].first, cuts2add[i].second, true );
			}
		} while( addedCuts != 0 && ++rounds < 3 );
	}

	return BnB( mip, plex, allSolutions, optimalValue, optimalAssignment, allAssignments );
}

}

#endif
