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

#pragma once

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/RandomPoints.h"
#include "polymake/vector"
#include <cmath>

namespace polymake { namespace graph {

class SpringEmbedder {
protected:
   const Graph<>& G;

   bool gravity;
   double viscosity, inertion, epsilon, epsilon_2;
   double scale, eff_scale, rep, z_factor;
   double min_edge_weight, avg_edge_weight;
   Vector<double> z_ordering;
   double z_min, z_max;
   Set<Int> fixed_vertices;

   Matrix<double> V;            // current velocity
   Vector<double> barycenter;
   std::vector<double> wanted_edge_length, inv_wanted_length;

#if POLYMAKE_DEBUG
   bool debug_print;
#endif
   void init_params(const OptionSet& options);
   void calculate_forces(const Matrix<double>& X, RandomSpherePoints<double>& random_points, Matrix<double>& F);

public:
   SpringEmbedder(const Graph<>& G_arg, const OptionSet& options)
      : G(G_arg)
   {
      init_params(options);
   }

   const Graph<>& get_graph() const { return G; }

   double set_viscosity(double x) { double old=viscosity; viscosity=x; return old; }
   double set_inertion(double x) { double old=inertion; inertion=x; return old; }
   void set_eps(double eps) { epsilon=eps; epsilon_2 = eps*eps; }

   double get_viscosity() const { return viscosity; }
   double get_inertion() const { return inertion; }

   double set_repulsion(double r) { double old=rep; rep=r; return old; }
   double set_z_factor(double f) { double old=z_factor; z_factor=f; return old; }

   const Set<Int> set_fixed_vertices(const Set<Int>& v)
   {
      Set<Int> old = fixed_vertices;
      fixed_vertices = v;
      return old;
   }

   const double get_repulsion() { return rep; }
   const double get_z_factor() { return z_factor; }
   const bool has_z_ordering() { return !z_ordering.empty(); }

   template <typename Iterator>
   void start_points(Matrix<double>& X, Iterator src)
   {
      V.resize(X.rows(),X.cols());

      for (auto x=entire(rows(X));  !x.at_end();  ++x, ++src)
         *x=*src * eff_scale;
      V.fill(0.);

      if (has_z_ordering()) {
         z_min=-eff_scale;
         z_max=eff_scale;
      }
      gravity=fixed_vertices.empty();
      barycenter.resize(X.cols());
   }

   void restart(const Matrix<double>& X);

   bool calculate(Matrix<double>& X, RandomSpherePoints<double>& random_points, Int max_iterations);

#if POLYMAKE_DEBUG
   bool debug_print_enabled() const { return debug_print; }
#endif
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
