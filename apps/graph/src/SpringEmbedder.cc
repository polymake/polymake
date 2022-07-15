/* Copyright (c) 1997-2022
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

#include "polymake/graph/SpringEmbedder.h"
#include <cmath>

namespace polymake { namespace graph {

void SpringEmbedder::init_params(const OptionSet& options)
{
   if (!(options["eps"] >> epsilon)) epsilon=1e-4;
   epsilon_2=epsilon*epsilon;

   if (!(options["viscosity"] >> viscosity)) viscosity=1;
   if (!(options["inertion"] >> inertion)) inertion=1;
   if (!(options["scale"] >> scale)) scale=1;
   epsilon_2*=scale*scale;

   if (!(options["balance"] >> rep)) rep=1;
   if (!(options["z-factor"] >> z_factor)) z_factor=1;

   if (options["z-ordering"] >> z_ordering) {
      auto obj=entire(z_ordering);
      double z_order_min=*obj, z_order_max=z_order_min;
      while (!(++obj).at_end())
         pm::assign_min_max(z_order_min, z_order_max, *obj);

      const double z_mid=(z_order_max+z_order_min)/2;
      z_order_max-=z_order_min;
      if (z_order_max > 1e-3*scale) {
         for (obj=entire(z_ordering); !obj.at_end(); ++obj)
            *obj=(*obj-z_mid)/z_order_max;
      } else {
         z_ordering.clear();
      }
   } else {
      z_ordering.clear();
   }

   G.init_edge_map(wanted_edge_length);
   G.init_edge_map(inv_wanted_length);

   if (options["edge-weights"] >> wanted_edge_length) {
      min_edge_weight = std::numeric_limits<double>::infinity();
      avg_edge_weight = 0;

      for (auto e = entire(wanted_edge_length); !e.at_end(); ++e) {
         if (*e <= 0)
            throw std::runtime_error("non-positive edge length encountered");
         pm::assign_min(min_edge_weight, *e);
         avg_edge_weight += *e;
      }
      avg_edge_weight /= min_edge_weight * double(G.edges());
   } else {
      min_edge_weight = 1/scale;
      avg_edge_weight = scale;
      fill_range(entire(wanted_edge_length), avg_edge_weight);
   }

   for (auto e = entire(wanted_edge_length), ie = entire(inv_wanted_length);
        !e.at_end(); ++e, ++ie)
      *ie = min_edge_weight/(*e);

   gravity = true;

   const double n_nodes = double(G.nodes());
   eff_scale = avg_edge_weight/4*std::sqrt(n_nodes);
   const double avg_deg = 2*double(G.edges())/n_nodes;
   if (avg_deg >= 3) {
      const double x = 2*M_PI/avg_deg;
      eff_scale *= std::sqrt(std::sin(x)/x);
   }
#if POLYMAKE_DEBUG
   debug_print = get_debug_level() > 1;
   if (debug_print) {
      cout << "initial adjustment:"
              "\n  min_edge_weight=" << min_edge_weight
           << "\n  avg_edge_weight=" << avg_edge_weight
           << "\n  avg_degree=" << avg_deg
           << "\n  n_nodes=" << n_nodes
           << "\n  eff_scale=" << eff_scale
           << endl;
   }
#endif
}

void SpringEmbedder::restart(const Matrix<double>& X)
{
   V.fill(0.);
   gravity=fixed_vertices.empty();
   if (has_z_ordering()) {
      z_min=accumulate(cols(X).back(), operations::min());
      z_max=accumulate(cols(X).back(), operations::max());
   }
}

void SpringEmbedder::calculate_forces(const Matrix<double>& X, RandomSpherePoints<double>& random_points, Matrix<double>& F)
{
   auto f = rows(F).begin();
   auto z = entire(z_ordering);
   double new_z_min = X[0].back(), new_z_max=new_z_min;
   z_max -= z_min;
   if (gravity) barycenter.fill(0.);

   for (auto this_node = entire(nodes(G)); !this_node.at_end(); ++this_node, ++f) {
      f->fill(0.);
      if (gravity) barycenter += X[*this_node];

      if (!z.at_end()) {
         const double this_z=X[*this_node].back();
         f->back() = ((*z)*z_max - this_z) * z_factor;
         pm::assign_min_max(new_z_min, new_z_max, this_z);
         ++z;
      }

      auto edge = this_node.out_edges().begin();

      for (auto n2 = entire(nodes(G)); n2 != this_node; ++n2) {
         const Vector<double> delta = X[*n2] - X[*this_node];
         const double delta_sqr=sqr(delta);
         if (delta_sqr>epsilon_2) {
            const double delta_abs=std::sqrt(delta_sqr);

            if (!edge.at_end() && edge.to_node()==*n2) {
               // the nodes are neighbors
               const Vector<double> attraction= (inv_wanted_length[*edge] - 1/delta_abs) * delta;
               *f += attraction;
               F[*n2] -= attraction;
               ++edge;

            } else {
               // the nodes are not neighbors
               const Vector<double> repulsion= (rep/delta_sqr/delta_abs) * delta;
               *f -= repulsion;
               F[*n2] += repulsion;
            }
         } else {
            // the nodes have been glued together: apply a moderate repulsion force in a random direction
            const Vector<double> repulsion=random_points.get();
            *f -= repulsion;
            F[*n2] += repulsion;
            if (!edge.at_end() && edge.index()==*n2) ++edge;
         }
      }
   }

   z_min = new_z_min;
   z_max = new_z_max;
   if (gravity) {
      barycenter /= double(X.rows());
      if (!z_ordering.empty()) barycenter.back() = 0;
   }

   for (auto fi = entire(fixed_vertices); !fi.at_end(); ++fi)
      F[*fi].fill(0);

   // cap the force magnitude to prevent explosions
   constexpr double max_f_proj = 1e6;
   constexpr double max_f_sqr = 3 * max_f_proj * max_f_proj;

   for (auto ff = entire(rows(F)); !ff.at_end(); ++ff) {
      const double f_sqr = sqr(*ff);
      if (f_sqr > max_f_sqr) *ff *= max_f_sqr / f_sqr;
   }
}

namespace {

void calc_internal_constants(double& a, double& b, double& c, double& d, double viscosity, double inertion)
{
   a=std::exp(-viscosity/inertion);
   b=(1-a)/viscosity;
   c=inertion*b;
   d=(1-c)/viscosity;
}

}

bool SpringEmbedder::calculate(Matrix<double>& X, RandomSpherePoints<double>& random_points, Int max_iterations)
{
   const Int n_nodes = G.nodes();
   Matrix<double> F(n_nodes,X.cols());  // forces

   const double viscosity_increasing_factor=1.2;

   Int incr_viscosity = 0, decr_viscosity = 0;
   double a, b, c, d;
   calc_internal_constants(a, b, c, d, viscosity, inertion);

   for (Int iter = 0; iter < max_iterations; ++iter) {
      calculate_forces(X, random_points, F);
#if POLYMAKE_DEBUG
      if (debug_print) cout << "iteration " << iter << endl;
#endif

      Int oscillated = 0, moved = 0;
      auto x = rows(X).begin(), f = rows(F).begin(), v = rows(V).begin();
      for (auto this_node = entire(nodes(G)); !this_node.at_end();  ++this_node, ++x, ++f, ++v) {
#if POLYMAKE_DEBUG
         if (debug_print) cout << '[' << *this_node << "]: x=(" << *x << "); f=(" << *f << "); v=(" << *v << ")\n";
#endif
         Vector<double> v1 = a*(*v) + b*(*f),
                        dx = c*(*v) + d*(*f);
         if (sqr(dx) >= epsilon_2) {
            if ((*v) * v1 < 0)
               ++oscillated;
            else
               ++moved;
         }
         *v=v1;
         *x+=dx;
         if (gravity) *x-=barycenter;
      }

      if (oscillated*2 >= n_nodes) {
         if (++incr_viscosity>=2) {
            viscosity *= viscosity_increasing_factor;
            calc_internal_constants(a,b,c,d,viscosity,inertion);
            incr_viscosity=0;
#if POLYMAKE_DEBUG
            if (debug_print) cout << "++ viscosity ++\n";
#endif
         }
         decr_viscosity=0;
      } else {
         if (!moved) return true;
         if (!oscillated) {
            if (++decr_viscosity>=10 && viscosity>1 ||
                decr_viscosity>=200 && viscosity>0.75) {
               viscosity /= viscosity_increasing_factor;
               calc_internal_constants(a,b,c,d,viscosity,inertion);
               decr_viscosity=0;
#if POLYMAKE_DEBUG
               if (debug_print) cout << "-- viscosity --\n";
#endif
            }
         } else {
            decr_viscosity=0;
         }
         incr_viscosity=0;
      }
#if POLYMAKE_DEBUG
      if (debug_print)
         cout << "-------------------------------------------\n"
                 "moved=" << moved << " oscillated=" << oscillated << " viscosity=" << viscosity << '\n';
#endif
   }
   return false;
}

} }

// Local Variables:
// c-basic-offset:3
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
