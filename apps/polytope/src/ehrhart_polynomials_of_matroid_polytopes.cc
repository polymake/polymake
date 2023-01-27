/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

#include "polymake/client.h"
#include "polymake/polytope/matroid_polytopes.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/internal/PlainParser.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Polynomial.h"

namespace polymake { namespace polytope {

UniPolynomial<Rational, Int> polynomial_in_binomial_expression(Int a, Int b, Int k)
{
	//binom(a*x+b,k)
	auto poly = UniPolynomial<Rational, Int>(1);
	if(a<0){
		return poly;
	}
	for(Int j = 0; j<k; ++j )
		poly*= (UniPolynomial<Rational, Int>(a,1)+UniPolynomial<Rational, Int>(b-j,0))/(j+1);
	return poly;
}

UniPolynomial<Rational, Int> ehrhart_polynomial_hypersimplex(Int k, Int n)
{
	if (k==0){
		return UniPolynomial<Rational, Int>(1);
	}
	auto poly = UniPolynomial<Rational, Int>(0);
	for(Int j = 0; j<=k-1; ++j )
		poly+= pow(-1,j)*Integer::binom(n, j)*polynomial_in_binomial_expression(k-j,n-j-1,n-1);
	return poly;
}

UniPolynomial<Rational, Int> ehrhart_polynomial_minimal_matroid(Int r, Int n)
{
	//the matroid has a flat of size n-r or rank 1
	auto poly = UniPolynomial<Rational, Int>(0);
	auto factor = polynomial_in_binomial_expression(1,n-r,n-r)/Integer::binom(n-1, r-1);
	for(Int j = 0; j<=r-1; ++j )
		poly+= factor*Integer::binom(n-r-1+j, j)*polynomial_in_binomial_expression(1,j,j);
	return poly;
}

UniPolynomial<Rational, Int> ehrhart_polynomial_panhandle_matroid(Int r, Int n, Int s)
{
	//the matroid has a flat of size n-s of rank 1
	//it is minimal if s = r
	auto poly = UniPolynomial<Rational, Int>(0);
	for(Int i = 0; i<=s-r; ++i ){
		auto factor = pow(-1,i)*Integer::binom(s, i)*polynomial_in_binomial_expression(1,n-s,n-s)*(n-s);
		for(Int j = 0; j<=s-1; ++j ){
			poly+= factor/(n-s+j)*polynomial_in_binomial_expression(s-r-i,s-1-i,s-1-j)*polynomial_in_binomial_expression(1,0,j);
		}
	}
	return poly;
}

//The following is an implementation of a nesty formula with many exceptions in boundary cases
//It generalizes the above formula(s)
UniPolynomial<Rational, Int> ehrhart_polynomial_cuspidal_matroid(Int r, Int n, Int s, Int k)
{
	if( k>=n-s || s<=r-k){
		throw std::runtime_error("Illegal input parameter");
	}
	//the matroid has a flat of size n-s of rank k
	//it is a panhandle matroid if k = 1
	auto poly = UniPolynomial<Rational, Int>(0);
	for(Int a = 0; a < k; ++a){
		for(Int i = 0; i <= s; ++i){
			for(Int j = 0; j <= n-s; ++j){
				if(j>a){continue;}
				if(i>r-a){continue;}
				auto tmp = poly;
				auto factor = pow(-1,i+j)*Integer::binom(s, i)*Integer::binom(n-s, j);
				if(s<r && i==r-a-1){
					//the i = r-a-1 terms
					//b <= t+s-r
					if(a==k-1 && r+1 <s+k){break;}
					for(Int l = 0; l<n-s; ++l){
						poly+= factor*polynomial_in_binomial_expression(a-j,a-j+n-s-1,l)*polynomial_in_binomial_expression(1,s-r+1,n-l-1);
					}
					continue;
				}
				if(i==r-a){
					if(s<r){continue;}
					//the i = r-a terms
					//b <= s-r-1
					for(Int b = 0; b<s-r; ++b){
						poly+= factor*polynomial_in_binomial_expression(a-j,a-j+n-s-1+b,n-s-1)*Integer::binom(s-1-r-b,s-1);
					}
					continue;
				}

				for(Int ll = 0; ll<n-s; ++ll){
					auto part1 = polynomial_in_binomial_expression(a-j,a-j+n-s-1,ll);
					for(Int l = 0; l<s; ++l){
						if(a==k-1){
							auto part2 = polynomial_in_binomial_expression(r-a-i-1,s-i-1,l);
							poly+= factor*part1*part2*polynomial_in_binomial_expression(1,1-a,n-1-l-ll);
						}else{
							auto part2 = polynomial_in_binomial_expression(r-a-i-1,s-a-i-1,l);
							poly+= factor*part1*part2*polynomial_in_binomial_expression(1,1,n-1-l-ll);
						}
					}
				}

			}
		}
	}
	return poly;
}




UniPolynomial<Rational, Int> ehrhart_polynomial_product_simplicies(Int a, Int b)
{
	//here a is the number of verticies of the first simplex, and b those of the second
	return polynomial_in_binomial_expression(1,a-1,a-1)*polynomial_in_binomial_expression(1,b-1,b-1);
}

Function4perl(&ehrhart_polynomial_hypersimplex, "ehrhart_polynomial_hypersimplex");

Function4perl(&ehrhart_polynomial_minimal_matroid, "ehrhart_polynomial_minimal_matroid");

Function4perl(&ehrhart_polynomial_panhandle_matroid, "ehrhart_polynomial_panhandle_matroid");

Function4perl(&ehrhart_polynomial_cuspidal_matroid, "ehrhart_polynomial_cuspidal_matroid");

Function4perl(&ehrhart_polynomial_product_simplicies, "ehrhart_polynomial_product_simplicies");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
