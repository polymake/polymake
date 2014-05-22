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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {

namespace {

template<typename Scalar>
Set<int> coordinates_to_eliminate(const Array<int>& indices, int first_coord, int last_coord, int codim, const Matrix<Scalar>& linear_span, bool revert)
{
   Set<int> coords_to_eliminate;
   if (indices.empty()) {
      bool found=false;
      for (Entire< Subsets_of_k<const sequence&> >::const_iterator i=entire(all_subsets_of_k(range(first_coord,last_coord),codim));!found&&!i.at_end(); ++i)
         if (det(linear_span.minor(All,*i))!=0) {
            coords_to_eliminate=*i;
            found=true;
         }
      if (!found) throw std::runtime_error("projection: no non-singular minor in LINEAR_SPAN!");
   } else {
      for (Entire< Array<int> >::const_iterator i=entire(indices); !i.at_end(); ++i) {
         if (*i<first_coord || *i>last_coord)
            throw std::runtime_error("projection: index out of range");
         coords_to_eliminate+=*i;
      }
      if (!revert)
         coords_to_eliminate=range(first_coord,last_coord)-coords_to_eliminate;
   }
   return coords_to_eliminate;
}

template<typename Scalar>
void process_rays(perl::Object& p_in, int first_coord, const Array<int>& indices, perl::OptionSet& options, const Matrix<Scalar>& linear_span, const Set<int>& coords_to_eliminate, perl::Object& p_out)
{
   Matrix<Scalar> Rays, lineality;
   bool points_read=false;
   std::string got_property;
  
   if (p_in.lookup_with_property_name("RAYS | INPUT_RAYS", got_property) >> Rays) {
      points_read=true;
      if ( indices.empty() )  { // if we do a full projection then vertices remain vertices, so we write back whatever we got
         p_out.take(got_property) << Rays.minor(All,~coords_to_eliminate);
         if ( p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> lineality && lineality.rows() )
            p_out.take( got_property=="RAYS" || got_property=="VERTICES" ? "LINEALITY_SPACE" : "INPUT_LINEALITY") << lineality.minor(All,~coords_to_eliminate);
         else {
            Matrix<Rational> empty;
            p_out.take( got_property=="RAYS" || got_property=="VERTICES" ? "LINEALITY_SPACE" : "INPUT_LINEALITY") << empty;
         }
      } else {
         p_out.take("INPUT_RAYS") << Rays.minor(All,~coords_to_eliminate);
         if ( p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> lineality && lineality.rows() ) 
            p_out.take("INPUT_LINEALITY") << lineality.minor(All,~coords_to_eliminate);  
         else {
            Matrix<Rational> empty;
            p_out.take("INPUT_LINEALITY") << empty;
         }
      }
   }
  
   if (!points_read && options["nofm"])
      throw std::runtime_error("projection: no rays found and Fourier-Motzkin elimination excluded");
   if ( indices.empty() && options["relabel"]) {
      // here we assume that, if VERTEX_LABELS are present in the object, then also VERTICES are known
      // otherwise this will trigger a convex hull computation
      int n_vertices = p_in.give("N_VERTICES");
      Array<std::string> labels(n_vertices);
      read_labels(p_in, "RAY_LABELS", labels);
      p_out.take("RAY_LABELS") << labels;
   }
}

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
         for (Entire< Set<int> >::const_reverse_iterator c=rentire(coords_to_eliminate); !c.at_end(); ++c) {
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
            for (Entire< Set<int> >::const_iterator i=entire(negative); !i.at_end(); ++i)
               for (Entire< Set<int> >::const_iterator k=entire(positive); !k.at_end(); ++k)
                  Combined_Ineqs /= Inequalities[*i]+Inequalities[*k];
            // delete redundant rows
            for (typename Entire< Rows< ListMatrix< Vector<Scalar> > > >::iterator i=entire(rows(Combined_Ineqs)); !i.at_end(); ) {
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
perl::Object projection_impl(perl::Object p_in, const std::string object_prefix, const std::string linear_span_name, int first_coord, const Array<int> indices, perl::OptionSet options) {
   if (!p_in.exists("RAYS | INPUT_RAYS") &&
       ! (object_prefix=="CONE" && p_in.exists("FACETS | INEQUALITIES") ) )
      throw std::runtime_error("projection is not defined for combinatorially given objects");

   const int ambient_dim = p_in.give((object_prefix + "_AMBIENT_DIM").c_str());
   const int dim = p_in.give((object_prefix + "_DIM").c_str());
   const int codim = ambient_dim-dim;
   if (indices.empty() && codim==0) return p_in; // nothing to do

   const Matrix<Scalar> linear_span = p_in.give(linear_span_name.c_str());
   if (codim!=linear_span.rows())
      throw std::runtime_error(("projection: " + linear_span_name + " has wrong number of rows").c_str());
   const int last_coord=ambient_dim-1;
   const Set<int> coords_to_eliminate = coordinates_to_eliminate(indices, first_coord, last_coord, codim, linear_span, options["revert"]);   // set of columns to project to

   perl::Object p_out(p_in.type());

   if (p_in.exists("RAYS | INPUT_RAYS"))
      process_rays(p_in, first_coord, indices, options, linear_span, coords_to_eliminate, p_out);

   if (object_prefix=="CONE" && p_in.exists("FACETS | INEQUALITIES"))
      process_facets(p_in, indices, options, linear_span, coords_to_eliminate, p_out);

   if (object_prefix=="FAN") {
      IncidenceMatrix<> MC;
      if (p_in.lookup("MAXIMAL_CONES") >> MC)
         p_out.take("MAXIMAL_CONES") << MC;
   }

   return p_out;
}

FunctionTemplate4perl("projection_impl<Scalar=Rational>($$$$$ {revert => 0, nofm => 0})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
