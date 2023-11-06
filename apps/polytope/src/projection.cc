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

#include "polymake/polytope/projection.h"

namespace polymake { namespace polytope {

namespace {


template<typename Scalar>
void process_facets(BigObject& p_in, const Array<Int>& indices, const Set<Int>& coords_to_eliminate, BigObject& p_out)
{
   Matrix<Scalar> Inequalities, equations;
   bool inequalities_read = false, equations_read = false;
   inequalities_read = p_in.lookup("FACETS | INEQUALITIES") >> Inequalities;
   equations_read = p_in.lookup("LINEAR_SPAN | EQUATIONS") >> equations;
  
   if(equations_read){

      // We want to use equations to eliminate as many coordinates of
      // coords_to_eliminate as possible. This is done in the following way:
      // 1. Select a basis of rows such that the rank of the minor with
      // coords_to_eliminate becomes maximal.
      // 2. Select columns such that we have a basis in the corresponding minor
      // 3. Transform such that the minor becomes the identity matrix.
      // 4. Use this minor to eliminate as many columns as possible.
      // 5. The unused equations give rise to equations of the projection,
      // extract the corresponding minor in equations_result. There is a small
      // argument here to make that equations_result cannot have non-zero
      // entries in the columns of coords_to_eliminate. The key is that
      // otherwise step 1 cannot have been a basis.
      Set<Int> br(basis_rows(equations.minor(All, coords_to_eliminate)));
      Set<Int> bc_tmp(basis_cols(equations.minor(br, coords_to_eliminate)));
      Set<Int> bc(select(coords_to_eliminate, bc_tmp));
      Matrix<Scalar> transform(inv(equations.minor(br, bc)));
      Matrix<Scalar> U(unit_matrix<Scalar>(equations.rows()));
      U.minor(br, br) = transform;
      equations = U*equations;

      // Now zip br+bc together and use these to eliminate cols.
      auto diags = attach_operation(br, bc, operations::pair_maker());
      Matrix<Scalar> equations_result(equations.minor(~br, All));

      for(const auto p: diags){
         if(inequalities_read){
            for(Int jj=0; jj<Inequalities.rows(); jj++){
               Scalar pivot(Inequalities(jj, p.second));
               Inequalities.row(jj) -= pivot * equations.row(p.first);
            }
         }
         for(Int jj=0; jj<equations_result.rows(); jj++){
            Scalar pivot(equations_result(jj, p.second));
            equations_result.row(jj) -= pivot * equations.row(p.first);
         }
      }
      p_out.take("EQUATIONS") << remove_zero_rows(equations_result.minor(All, ~coords_to_eliminate));
   }

 
   if (inequalities_read) {
      // perform Fourier-Motzkin elimination on coords
      for (auto c=entire<reversed>(coords_to_eliminate); !c.at_end(); ++c) {
         Set<Int> negative, zero, positive;
         // normalize Inequalities such that there is a -1,0, or 1 in the next column to be eliminated
         for (Int i = 0; i < Inequalities.rows(); ++i) {
            const Scalar x = Inequalities(i, *c);
            const Int s = sign(x);
            if (s == 0) {
               zero += i;
            } else {
               if (s < 0)
                  negative += i;
               else
                  positive += i;
               Inequalities[i] /= abs(x);
            }
         }
         const Set<Int> remaining_coords = range(0, Inequalities.cols()-1)-(*c);
         Inequalities = Inequalities.minor(All,remaining_coords);
         ListMatrix< Vector<Scalar> > Combined_Ineqs=Inequalities.minor(zero,All);
         for (auto i=entire(negative); !i.at_end(); ++i)
            for (auto k=entire(positive); !k.at_end(); ++k)
               Combined_Ineqs /= Inequalities[*i]+Inequalities[*k];
         // delete redundant rows
         for (auto i=entire(rows(Combined_Ineqs)); !i.at_end(); ) {
            const typename Rows< ListMatrix< Vector<Scalar> > >::iterator j(i++);
            if (is_zero(*j)) Combined_Ineqs.delete_row(j);
         }
         Inequalities = Combined_Ineqs;
      } 
      p_out.take("INEQUALITIES") << remove_zero_rows(Inequalities);
   }
}

}  // end anonymous namespace

template <typename Scalar>
BigObject projection_cone_impl(BigObject p_in, const Array<Int> indices, OptionSet options)
{
   if ( !p_in.exists("RAYS | INPUT_RAYS") &&
        !p_in.exists("FACETS | INEQUALITIES") )
      throw std::runtime_error("projection is not defined for combinatorially given cones");
   if ( !p_in.exists("RAYS | INPUT_RAYS") && options["nofm"] )
      throw std::runtime_error("projection: no rays found and Fourier-Motzkin elimination excluded");
   
   const Int ambient_dim = p_in.give("CONE_AMBIENT_DIM");
   const Int dim = p_in.give("CONE_DIM");
   const Int codim = ambient_dim-dim;
   if (indices.empty() && codim==0) return p_in; // nothing to do

   const Set<Int> coords_to_eliminate = coordinates_to_eliminate<Scalar>(indices, ambient_dim, codim, p_in, options["revert"]);   // set of columns to project to

   BigObject p_out(p_in.type());

   if (p_in.exists("RAYS | INPUT_RAYS"))
      process_rays<Scalar>(p_in, indices, options, coords_to_eliminate, p_out);

   if (p_in.exists("FACETS | INEQUALITIES") && !options["nofm"])
      process_facets<Scalar>(p_in, indices, coords_to_eliminate, p_out);

   return p_out;
}

template <typename Scalar>
BigObject projection_vectorconfiguration_impl(BigObject p_in, const Array<Int>& indices, OptionSet options)
{
   const Int ambient_dim = p_in.give("VECTOR_AMBIENT_DIM");
   const Int dim = p_in.give("VECTOR_DIM");
   const Int codim = ambient_dim-dim;
   if (indices.empty() && codim==0) return p_in; // nothing to do

   const Matrix<Scalar> linear_span = p_in.give("LINEAR_SPAN");
   if (codim != linear_span.rows())
      throw std::runtime_error("projection: LINEAR_SPAN has wrong number of rows");
   const Set<Int> coords_to_eliminate = coordinates_to_eliminate<Scalar>(indices, ambient_dim, codim, p_in, options["revert"]);   // set of columns to project to

   BigObject p_out(p_in.type());

   if (p_in.exists("VECTORS")){
      const Matrix<Scalar> vec = p_in.give("VECTORS") ;
      p_out.take("VECTORS") <<  vec.minor(All,~coords_to_eliminate);
   }
   return p_out;
}

template <typename Scalar>
BigObject projection_preimage_impl(const Array<BigObject>& pp_in)
{
   auto p_in = entire(pp_in);

   const bool 
      is_poc = p_in->isa("Polytope") || p_in->isa("PointConfiguration"),
      is_cone = p_in->isa("Cone");

   const std::string 
      rays_section = p_in->isa("Polytope")
      ? "VERTICES | POINTS"
      : p_in->isa("PointConfiguration")
      ? "POINTS"
      : p_in->isa("Cone")
      ? "RAYS | INPUT_RAYS"
      : "VECTORS",

      lin_space_section = is_poc
      ? "AFFINE_HULL"
      : "LINEAR_SPAN",
      
      output_rays = is_cone
      ? "INPUT_RAYS"
      : "VECTORS";

   Matrix<Scalar> Rays = p_in->give(rays_section);
   Matrix<Scalar> LinSpace = p_in->give(lin_space_section);

   const auto type_name = p_in->type().name();
   BigObject p_out(p_in->type());
   std::string descr_names = p_in->name();

   while (! (++p_in).at_end()) {
      if (p_in->type().name() != type_name)
         throw std::runtime_error("projection_preimage: cannot mix objects of different type");

      const Matrix<Scalar> V = p_in->give(rays_section);
      const Matrix<Scalar> L = p_in->give(lin_space_section);
      if (V.rows() != Rays.rows())
         throw std::runtime_error("projection_preimage: mismatch in the number of rays or points");
      if (is_poc)
         Rays |= V.minor(All, range_from(1));
      else
         Rays |= V;

      if (is_cone) {
         LinSpace |= zero_matrix<Scalar>(LinSpace.rows(), L.cols()-1);
         LinSpace /= L.col(0) | zero_matrix<Scalar>(L.rows(), LinSpace.cols()-1) | L.minor(All, range_from(1));
      }      
      descr_names += ", " + p_in->name();
   }

   p_out.set_description() << "preimage under projection of " << descr_names << endl;
   if (is_cone)
      p_out.take("INPUT_LINEALITY") << LinSpace;
   p_out.take(output_rays) << Rays;
   return p_out;
}

FunctionTemplate4perl("projection_cone_impl<Scalar=Rational>(Cone $ {revert => 0, nofm => 0})");

FunctionTemplate4perl("projection_vectorconfiguration_impl<Scalar=Rational>(VectorConfiguration $ {revert => 0, nofm => 0})");

FunctionTemplate4perl("projection_preimage_impl<Scalar=Rational>($)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
