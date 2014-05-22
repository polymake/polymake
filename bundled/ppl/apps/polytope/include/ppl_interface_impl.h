/* Copyright (c) 1997-2014
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

#ifndef POLYMAKE_POLYTOPE_PPL_INTERFACE_IMPL_H
#define POLYMAKE_POLYTOPE_PPL_INTERFACE_IMPL_H

#include <cstddef> // needed for gcc 4.9, see http://gcc.gnu.org/gcc-4.9/porting_to.html
#include <gmpxx.h> //for mpz/mpq-handling
#include "polymake/polytope/ppl_interface.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/linalg.h"
// the following requires an entry in Makefile.inc
#include <ppl.hh>

#include <fenv.h>

namespace PPL = Parma_Polyhedra_Library;



namespace polymake { namespace polytope { namespace ppl_interface {

//! PPL tends to modify the floating point operation mode, in particular, the rounding direction.
//! This seems to happen during initial load of the ppl library,
//! therefore the mode preferred by PPL is captured in a singleton living in this shared object (polytope).
//! Since this shared object depends on libppl, its global constructors will always be executed after libppl is loaded.

class fp_mode_setter
{
public:
   fp_mode_setter()
   {
      fesetround(captured.mode);
   }

   ~fp_mode_setter()
   {
      fesetround(FE_TONEAREST);
   }

private:
   class init
   {
   public:
      init()
      {
         // ensure libppl is really loaded
         version=PPL::version_major();
         mode=fegetround();
         fesetround(FE_TONEAREST);
      }

      int mode;
      int version;
   };

   static init captured;
};

namespace {

     // Constructs an (integral) mpz-vector by multiplying with lcm of denominators.
     // Note: this is different from 'primitive', e.g. (2/3,4/3)->(2,4), not (1,2)
     template <typename Coord>
     std::vector<mpz_class> convert_to_mpz(const Vector<Coord>& v, const Integer& denom) {

        Vector<Integer> v_multi(denom*v); //This cast works since denom*v is integral by construction!! 

        std::vector<mpz_class> mpz_vec(v.dim());
        for ( int i = 0; i < v.dim(); ++i ) {
           mpz_vec[i] = mpz_class(v_multi[i].get_rep());
        }
        return mpz_vec;
     }
        
     // Translates a ppl generator into a (homogenized) polymake vector,
     // depending on its type (point, ray, line).
     template <typename Coord>
     Vector<Coord> ppl_gen_to_vec(const PPL::Generator& gen, const bool isCone) {

        unsigned int n = gen.space_dimension();
        int dim = n+1;
        Vector<Coord> vec(dim);
         
        for ( int i = 0; i < dim-1; ++i ) {
           Integer num( mpz_class( gen.coefficient(PPL::Variable(i)) ).get_mpz_t() );
           Coord entry(num, 1); // FIXME: only works for Rational!
           vec[i+1] = entry;
        }

        if ( gen.is_point() ) { // in case of isCone, generators are rays or lines except for (0,...,0)

           // gen.divisor only works for PPL-(closure-)points
           Integer denom(mpz_class(gen.divisor()).get_mpz_t());  
           Coord factor(1,denom);
           vec = factor * vec;
           vec[0] = 1;

        } else {
           assert( gen.is_ray() || gen.is_line() );
        } 

        return vec;
     }

     // Translates a ppl constraint (n entries, inhomogeneous term as an attribute)
     // into a polymake vector.    
     template <typename Coord>
     Vector<Coord> ppl_constraint_to_vec(const PPL::Constraint& cons, const bool isCone) {

        unsigned int n = cons.space_dimension();
        Vector<Coord> vec(n + 1);
        vec[0] = cons.inhomogeneous_term(); // will be overwritten if isCone = true
        for ( unsigned int i = 0; i < n; ++i ) {
           Integer coeff( mpz_class( cons.coefficient(PPL::Variable(i)) ).get_mpz_t() ); 
           vec[i+1] = coeff;
        }
        return vec;
     }


     template <typename Coord>     
     PPL::C_Polyhedron construct_ppl_polyhedron_H(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations, const bool isCone) {
        
        /* Linear expressions in H representations store the inhomogeneous term at index 0,
         * and the variables' coefficients at indices 1, 2, ..., space_dim. 
         */
        
        PPL::Constraint_System cs;

        int num_columns = std::max(Inequalities.cols(),Equations.cols());

        // insert inequalities
        for ( typename Entire< Rows < Matrix<Coord> > >::const_iterator row_it = entire(rows(Inequalities)); !row_it.at_end(); ++row_it ) {
              Integer lcm_of_row_denom(lcm(denominators(*row_it))); 
              std::vector<mpz_class> coefficients = convert_to_mpz<Coord>(*row_it, lcm_of_row_denom); 
              // PPL variables have indices 0, 1, ..., space_dim-1.
              PPL::Linear_Expression e;
              for (int j = num_columns-1; j >= 1; j-- ) {
                 e += coefficients[j] * PPL::Variable(j-1);
              }
              e += coefficients[0];
              
              cs.insert(e >= 0);
           } 

        // insert equations
        for ( typename Entire< Rows < Matrix<Coord> > >::const_iterator row_it = entire(rows(Equations)); !row_it.at_end(); ++row_it ) {
              Integer lcm_of_row_denom(lcm(denominators(*row_it))); 
              std::vector<mpz_class> coefficients = convert_to_mpz<Coord>(*row_it, lcm_of_row_denom); 
              // PPL variables have indices 0, 1, ..., space_dim-1.
              PPL::Linear_Expression e;
              for (int j = num_columns-1; j >= 1; j-- ) {
                 e += coefficients[j] * PPL::Variable(j-1);
              }
              e += coefficients[0];
              
              cs.insert(e == 0);
           } 
              
           PPL::C_Polyhedron ppl_poly(cs);
           return ppl_poly;
            
         }


     template <typename Coord>     
     PPL::C_Polyhedron construct_ppl_polyhedron_V(const Matrix<Coord>& Points, const Matrix<Coord>& Lineality, const bool isCone) {
        
        // The V representation 
        PPL::Generator_System gs;
        int num_columns = std::max(Points.cols(), Lineality.cols()); // necessary if only one matrix has entries

        /* Cones need an additional point (0,...,0) in ppl.
         * Furthermore, the cone is translated into a polytope.
         */
        if (isCone) {
           PPL::Generator v = PPL::point(0*PPL::Variable(num_columns-2)); //origin
           gs.insert(v);
        }

        // insert points/rays
        for ( typename Entire< Rows < Matrix<Coord> > >::const_iterator row_it = entire(rows(Points)); !row_it.at_end(); ++row_it ) {
              Integer lcm_of_row_denom(lcm(denominators(*row_it))); 
              std::vector<mpz_class> coefficients = convert_to_mpz<Coord>(*row_it, lcm_of_row_denom); 
              // PPL variables have indices 0, 1, ..., space_dim-1.
              PPL::Linear_Expression e;
              for (unsigned j = num_columns - 1; j >= 1; j-- ) {
                 e += coefficients[j] * PPL::Variable(j-1);
              }

              PPL::Generator v;
              if (coefficients[0] != 0) {
                 v = PPL::point(e, lcm_of_row_denom.gmp() ); // v is a point
                 gs.insert(v);
              } else {
                 v = PPL::ray(e); // v is a ray
                 gs.insert(v);
              }
           } 


        // insert linealities
        for ( typename Entire< Rows < Matrix<Coord> > >::const_iterator row_it = entire(rows(Lineality)); !row_it.at_end(); ++row_it ) {
              Integer lcm_of_row_denom(lcm(denominators(*row_it))); 
              std::vector<mpz_class> coefficients = convert_to_mpz<Coord>(*row_it, lcm_of_row_denom); 
              // PPL variables have indices 0, 1, ..., space_dim-1.
              PPL::Linear_Expression e;
              for (unsigned j = num_columns - 1; j >= 1; j-- ) {
                 e += coefficients[j] * PPL::Variable(j-1);
              }
              PPL::Generator l = line(e);
              gs.insert(l);
           } 
              
           PPL::C_Polyhedron ppl_poly(gs);
           return ppl_poly;
            
         }


     } // End of namespace




  /* FIXME (1): Don't know how to handle Float and Rational
     simultaneously w.r.t. defining the optimal value */
  template <typename Coord>
  solver<Coord>::solver() {}


  template <typename Coord>
  typename solver<Coord>::matrix_pair
  solver<Coord>::enumerate_facets(const Matrix<Coord>& Points, const Matrix<Coord>& Lineality, const bool isCone, const bool primal)
  {
     PPL::C_Polyhedron polyhedron = construct_ppl_polyhedron_V(Points, Lineality, isCone);
     PPL::Constraint_System cs = polyhedron.minimized_constraints();
     ListMatrix< Vector<Coord> > facet_list;
     ListMatrix< Vector<Coord> > affine_hull_list;

     int num_columns = std::max(Points.cols(),Lineality.cols());
     Vector<Coord> triv_ineq(1|zero_vector<Coord>(num_columns - 1));

     for (PPL::Constraint_System::const_iterator csi = cs.begin(); csi != cs.end(); ++csi) {
        const PPL::Constraint& c = *csi;
        Vector<Coord> row = ppl_constraint_to_vec<Coord>(c, isCone);
        if ( !( isCone && row == triv_ineq ) ) { 
           if (c.is_inequality()) {
              facet_list /= row;
           } else {
              assert(c.is_equality());
              affine_hull_list  /= row;
           }
        }
     }

     Matrix<Coord> facets(facet_list);
     Matrix<Coord> affine_hull(affine_hull_list);
     return typename solver<Coord>::matrix_pair(facets, affine_hull);
  }


  template <typename Coord>
  typename solver<Coord>::matrix_pair
  solver<Coord>::enumerate_vertices(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations, const bool isCone, const bool primal )
  {
     PPL::C_Polyhedron polyhedron = construct_ppl_polyhedron_H(Inequalities, Equations, isCone);
     PPL::Generator_System gs = polyhedron.minimized_generators();
     ListMatrix< Vector<Coord> > vertex_list;
     ListMatrix< Vector<Coord> > lin_space_list;

     int num_columns = std::max(Inequalities.cols(),Equations.cols());
     Vector<Coord> cone_origin(1|zero_vector<Coord>( num_columns - 1 ));

     for (PPL::Generator_System::const_iterator gsi = gs.begin(); gsi != gs.end(); ++gsi) {
        const PPL::Generator& g = *gsi;
        Vector<Coord> row = ppl_gen_to_vec<Coord>(g, isCone);
        if ( !(isCone && row == cone_origin) ) {
           if (g.is_point() || g.is_ray()) {
              vertex_list /= row;
           } else {
              assert(g.is_line());
              lin_space_list  /= row;
           }
        }
     }

     Matrix<Coord> vertices(vertex_list);
     Matrix<Coord> lin_space(lin_space_list);
     return typename solver<Coord>::matrix_pair(vertices, lin_space);
  }


  template <typename Coord>
  typename solver<Coord>::lp_solution
  solver<Coord>::solve_lp(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations,
  const Vector<Coord>& Objective, bool maximize) 
  {
     // establish the PPL rounding mode temporarily
     fp_mode_setter fp_mode;

     int num_columns = std::max(Inequalities.cols(),Equations.cols())-1;
     if ( num_columns == -1 ) {
        throw infeasible();
     }
     PPL::C_Polyhedron polyhedron = construct_ppl_polyhedron_H(Inequalities, Equations, 0); // isCone = 0
 
     // Linear program
     Integer lcm_of_obj_denom(lcm(denominators(Objective))); 
     std::vector<mpz_class> objective = convert_to_mpz<Coord>(Objective, lcm_of_obj_denom);
 
     PPL::Linear_Expression e;
     for (unsigned j = num_columns; j >= 1; j--) {
        e += objective[j] * PPL::Variable(j-1);
     }
     e += objective[0];
     PPL::Coefficient bound_n, bound_d;   // same as mpz_class
     bool is_opt;
     PPL::Generator g_opt = PPL::point();
     bool solvable;
     if ( maximize ) {
        solvable = polyhedron.maximize(e, bound_n, bound_d, is_opt, g_opt);
     } else {
        solvable = polyhedron.minimize(e, bound_n, bound_d, is_opt, g_opt);
     }
     if ( !solvable ) { // ppl returns false if input is infeasible OR unbounded!
        if ( polyhedron.is_empty() ) {
           throw infeasible();
        } else {          
           throw unbounded();
        }
     }

     Vector<Coord> opt_sol( ppl_gen_to_vec<Coord>(g_opt, 0) );
     Integer bound_n_Int(mpz_class(bound_n).get_mpz_t());
     Integer bound_d_Int(mpz_class(bound_d).get_mpz_t());

     // see FIXME (1)
     //Coord opt_val(bound_n_Int / bound_d_Int); //dividing an Integer by an Integer returns an Integer, not a Rational


     //opt_val needs to be divided additionally by the lcm of the denominators
     //of the objective vector since the constructed Linear_Expression e
     //has been multiplied by this factor in 'convert_to_mpz'.
     //Note that ppl seems to work only with integral objective functions. 
     Rational opt_val(bound_n_Int, (bound_d_Int*lcm_of_obj_denom) );

     return lp_solution(opt_val, opt_sol);

  }




} } }

#endif // POLYMAKE_POLYTOPE_PPL_INTERFACE_IMPL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
