/* Copyright (c) 1997-2016
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

#ifndef __POLYMAKE_CAYLEY_EMBEDDING_H__
#define __POLYMAKE_CAYLEY_EMBEDDING_H__

#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"
#include "polymake/vector"

namespace polymake { namespace polytope {

namespace {
   
template<typename Scalar>
Matrix<Scalar> embedding_matrix(const Matrix<Scalar>& V, int i, int m, const Scalar& t)
{
   assert(m>=1 && i>=0 && i<m);
   Matrix<Scalar> embedding_matrix(V.rows(), m);
   embedding_matrix.col(i) = t * ones_vector<Scalar>(V.rows());
   return embedding_matrix;
}

} // end anonymous namespace


template<typename Scalar>
perl::Object cayley_embedding(const Array<perl::Object>& p_array,
                              const Vector<Scalar>& t_vec,
                              perl::OptionSet options)
{
   const int m = p_array.size();

   // input sanity checks
   if (!m)
      throw std::runtime_error("cayley_embedding: empty array given.");

   bool any_pointed(false);
   for (const perl::Object& p : p_array) {
      const bool pointed = p.give("POINTED");
      if (pointed) {
         any_pointed=true;
         break;
      }
   }
   if (!any_pointed)
      throw std::runtime_error("cayley_embedding: at least one input polyhedron must be POINTED");

   Set<int> dimensions;
   std::vector<int> n_vertices(m); 

   // rays
   std::string has_VERTICES;
   bool VERTICES_out = true;
   Matrix<Scalar> V_out;

   // labels
   const bool relabel=!options["no_labels"];

   // description of resulting object
   std::ostringstream odesc;
   odesc << "Cayley embedding of ";

   for (int i=0; i<m; ++i) {
      // vertices
      const Matrix<Scalar> V = p_array[i].give_with_property_name("VERTICES | POINTS", has_VERTICES);
      n_vertices[i] = V.rows();
      dimensions += V.cols();
      if (dimensions.size() >= 2)
         throw std::runtime_error("cayley_embedding: dimension mismatch between input polytopes 0 and " + std::to_string(i));

      // rays
      VERTICES_out = VERTICES_out && has_VERTICES=="VERTICES" && far_points(V).empty();
      if (!VERTICES_out && relabel)
         throw std::runtime_error("cayley_embedding: can't produce VERTEX_LABELS since VERTICES are unknown in argument " + std::to_string(i));

      // scaling
      const Scalar t{ t_vec.empty() ? Scalar(1) : t_vec[i] };

      // output matrix
      V_out /= V | embedding_matrix(V, i, m, t);

      // name
      if (i>0) odesc << ", ";
      odesc << p_array[i].name();
   }


   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   odesc << endl;
   p_out.set_description() << odesc.str();

   p_out.take(VERTICES_out ? Str("VERTICES") : Str("POINTS")) << V_out;
   p_out.take(VERTICES_out ? Str("LINEALITY_SPACE") : Str("INPUT_LINEALITY")) << Matrix<Scalar>(0, V_out.cols());

   if (relabel) {
      std::vector<std::string> labels(accumulate(n_vertices, operations::add()));
      int v_ct(0);
      for (int i=0; i<m; ++i) {
         read_labels(p_array[i], "VERTEX_LABELS", non_const(select(labels, sequence(v_ct, n_vertices[i]))));
         for (std::vector<std::string>::iterator 
                 l     = labels.begin() + v_ct, 
                 l_end = labels.begin() + v_ct + n_vertices[i]; 
              l != l_end; ++l) {
            *l += '_';
            *l += '0' + i;
         }
         v_ct += n_vertices[i];
      }
      p_out.take("VERTEX_LABELS") << labels;
   }

   return p_out;
}

} } // end namespaces

#endif // __POLYMAKE_CAYLEY_EMBEDDING_H__


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
