/* Copyright (c) 1997-2019
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
void process_facets(perl::Object& p_in, const Array<int>& indices, perl::OptionSet& options, const Matrix<Scalar>& linear_span, const Set<int>& coords_to_eliminate, perl::Object& p_out)
{
   Matrix<Scalar> Inequalities;
   bool inequalities_read = false;
  
   if (!options["nofm"]) {
      if (p_in.lookup("FACETS | INEQUALITIES") >> Inequalities) {
         inequalities_read=true;
         Inequalities /= linear_span / (-linear_span);
      }
    
      if (inequalities_read) {
         // perform Fourier-Motzkin elimination on coords
         for (auto c=entire<reversed>(coords_to_eliminate); !c.at_end(); ++c) {
            Set<int> negative, zero, positive;
            // normalize Inequalities such that there is a -1,0, or 1 in the next column to be eliminated
            for (int i=0; i<Inequalities.rows(); ++i) {
               const Scalar x=Inequalities(i,*c);
               const int s=sign(x);
               if (s) {
                  if (s<0)
                     negative += i;
                  else
                     positive += i;
                  Inequalities[i] /= abs(x);
               } else
                  zero += i;
            }
            const Set<int> remaining_coords=range(0,Inequalities.cols()-1)-(*c);
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
         p_out.take("INEQUALITIES") << Inequalities;
      }
   }
}

}  // end anonymous namespace

template <typename Scalar>
perl::Object projection_cone_impl(perl::Object p_in, const Array<int> indices, perl::OptionSet options)
{
   if ( !p_in.exists("RAYS | INPUT_RAYS") &&
        !p_in.exists("FACETS | INEQUALITIES") )
      throw std::runtime_error("projection is not defined for combinatorially given cones");
   
   int first_coord = p_in.isa("Polytope") ? 1 : 0;
   const int ambient_dim = p_in.give("CONE_AMBIENT_DIM");
   const int dim = p_in.give("CONE_DIM");
   const int codim = ambient_dim-dim;
   if (indices.empty() && codim==0) return p_in; // nothing to do

   const Matrix<Scalar> linear_span = p_in.give("LINEAR_SPAN");
   if (codim != linear_span.rows())
      throw std::runtime_error("projection: LINEAR_SPAN has wrong number of rows");
   const int last_coord=ambient_dim-1;
   const Set<int> coords_to_eliminate = coordinates_to_eliminate(indices, first_coord, last_coord, codim, linear_span, options["revert"]);   // set of columns to project to

   perl::Object p_out(p_in.type());

   if (p_in.exists("RAYS | INPUT_RAYS"))
      process_rays(p_in, first_coord, indices, options, linear_span, coords_to_eliminate, p_out);

   if (p_in.exists("FACETS | INEQUALITIES"))
      process_facets(p_in, indices, options, linear_span, coords_to_eliminate, p_out);

   return p_out;
}

template <typename Scalar>
perl::Object projection_vectorconfiguration_impl(perl::Object p_in, const Array<int> indices, perl::OptionSet options)
{
   int first_coord = p_in.isa("PointConfiguration") ? 1 : 0;
   const int ambient_dim = p_in.give("VECTOR_AMBIENT_DIM");
   const int dim = p_in.give("VECTOR_DIM");
   const int codim = ambient_dim-dim;
   if (indices.empty() && codim==0) return p_in; // nothing to do

   const Matrix<Scalar> linear_span = p_in.give("LINEAR_SPAN");
   if (codim != linear_span.rows())
      throw std::runtime_error("projection: LINEAR_SPAN has wrong number of rows");
   const int last_coord=ambient_dim-1;
   const Set<int> coords_to_eliminate = coordinates_to_eliminate(indices, first_coord, last_coord, codim, linear_span, options["revert"]);   // set of columns to project to

   perl::Object p_out(p_in.type());

   if (p_in.exists("VECTORS")){
      const Matrix<Scalar> vec = p_in.give("VECTORS") ;
      p_out.take("VECTORS") <<  vec.minor(All,~coords_to_eliminate);
   }
   return p_out;
}

template <typename Scalar>
perl::Object projection_preimage_impl(const Array<perl::Object>& pp_in)
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
   perl::Object p_out(p_in->type());
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
