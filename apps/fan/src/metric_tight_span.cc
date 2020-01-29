/* Copyright (c) 1997-2020
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
#include "polymake/list"
#include "polymake/Rational.h"
#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/polytope/hypersimplex.h"

namespace polymake { namespace fan {

BigObject metric_tight_span(const Matrix<Rational> dist, OptionSet options)
{
   const bool extend = options["extended"];
   const Int d = dist.rows();
   //opt["no_facets"] = true;
   BigObject hy = polytope::hypersimplex(2, d, OptionSet());
   BigObject sd("PointConfiguration");
   Matrix<Rational> points = hy.give("VERTICES");
   if (extend) points/=(ones_vector<Rational>(d)|2*unit_matrix<Rational>(d));
   sd.take("POINTS") << points;
   
   Vector<Rational> w( d*(d-1)/2 );
   if (extend) w = Vector<Rational>( d+d*(d-1)/2 );
   Int k = 0;
   for (Int i = 0; i < d; ++i)
      for (Int j = i+1; j < d; ++j) {
      w[k]=-dist(i,j);
      ++k;
    }
   
   BigObject sp("SubdivisionOfPoints");
   sp.take("WEIGHTS") << w;
   sp.attach("METRIC") << dist;
   sd.take("POLYTOPAL_SUBDIVISION") << sp;
   return sd;
}

BigObject metric_extended_tight_span(const Matrix<Rational> dist)
{
   OptionSet opts("extended",true);
   BigObject sd = metric_tight_span(dist,opts);
   BigObject ts("PolyhedralComplex");
   Matrix<Rational> vert = sd.give("POLYTOPAL_SUBDIVISION.TIGHT_SPAN.VERTICES");
   ts.take("VERTICES") << vert;
   Array<std::string> label(vert.rows() );
   Int k = 0;
   for (auto row = entire(rows(vert)); !row.at_end(); ++row, ++k){
         std::string vlabel("");
         for (Int j = 0; j < vert.cols(); ++j) 
            if ((*row)[j]==0) vlabel += std::to_string(j) ;
          label[k] = vlabel;
   }

   ts.take("VERTEX_LABELS") << label;
   ts.take("GRAPH.NODE_LABELS") << label;
   ts.take("MAXIMAL_POLYTOPES") << sd.give("POLYTOPAL_SUBDIVISION.TIGHT_SPAN.MAXIMAL_POLYTOPES");

   return ts;
}

Matrix<Rational> thrackle_metric(const Int n)
{
   Matrix<Rational> d(n,n);

   if (n<2) throw std::runtime_error("n >= 2 required");
  
   for (Int i = 1; i <= n; ++i)
      for (Int j = i+1; j <= n; ++j)
         d(i-1,j-1) = d(j-1,i-1) = (j-i)*(n-(j-i));

   return d;
}

BigObject ts_thrackle_metric(const Int n)
{
   return metric_tight_span( thrackle_metric(n), OptionSet() );
}

Matrix<Rational> max_metric(const Int n)
{
   if (n<2)
      throw std::runtime_error("max_metric: n >= 2 required");

   Matrix<Rational> d(n,n);

   for (Int i = 1; i <= n; ++i)
      for (Int j = i+1; j <= n; ++j)
         d(i-1,j-1) = d(j-1,i-1) = 1 + Rational(1,n*n+i*n+j);

  return d;
}

BigObject ts_max_metric(const Int n)
{
   return metric_tight_span( max_metric(n), OptionSet());
}

Matrix<Rational> min_metric(const Int n)
{
   if (n<2)
      throw std::runtime_error("min_metric: n >= 2 required");

   Matrix<Rational> d(n,n);

   for (Int i = 1; i <= n; ++i)
      for (Int j = i+1; j <= n; ++j)
         switch (n%3) {
         case 0:
         case 1:
            d(i-1,j-1) = d(j-1,i-1) = (i-1)/3==(j-1)/3 ? Rational(2) : 1 + Rational(1,n*n+i*n+j);
            break;
         case 2:
            d(i-1,j-1) = d(j-1,i-1) = ((i-1)/3==(j-1)/3) && (i<n) && (j<n) ? Rational(2) : 1 + Rational(1,n*n+i*n+j);
         }

   return d;
}

BigObject ts_min_metric(const Int n)
{
   return metric_tight_span( min_metric(n), OptionSet() );
}

UserFunction4perl("# @category Finite metric spaces"
                  "# Compute a metric such that the f-vector of its tight span is minimal among all metrics with //n// points."
                  "#\t See Herrmann and Joswig: Bounds on the f-vectors of tight spans, Contrib. Discrete Math., Vol.2, (2007)"
                  "# @param Int n the number of points"
                  "# @return Matrix"
                  "# @example To compute the min-metric of five points and display the f-vector of its tight span, do this:"
                  "# > $M = min_metric(5);"
                  "# > $PC = metric_tight_span($M,extended=>1);"
                  "# > print $PC->POLYTOPAL_SUBDIVISION->TIGHT_SPAN->F_VECTOR;"
                  "# | 16 20 5",
                  &min_metric, "min_metric");

UserFunction4perl("# @category Finite metric spaces"
                  "# Compute a metric such that the f-vector of its tight span is maximal among all metrics with //n// points."
                  "#\t See Herrmann and Joswig: Bounds on the f-vectors of tight spans, Contrib. Discrete Math., Vol.2, (2007)"
                  "# @param Int n the number of points"
                  "# @return Matrix"
                  "# @example To compute the max-metric of five points and display the f-vector of its tight span, do this:"
                  "# > $M = max_metric(5);"
                  "# > $PC = metric_tight_span($M,extended=>1);"
                  "# > print $PC->POLYTOPAL_SUBDIVISION->TIGHT_SPAN->F_VECTOR;"
                  "# | 16 20 5",
                  &max_metric, "max_metric");

UserFunction4perl("# @category Finite metric spaces"
                  "# Compute a thrackle  metric on //n// points."
                  "# This metric can be interpreted as a lifting function for the thrackle triangulation."
                  "#\t See De Loera, Sturmfels and Thomas: Gröbner bases and triangulations of the second hypersimplex, Combinatorica 15 (1995)"
                  "# @param Int n the number of points"
                  "# @return Matrix"
                  "# @example To compute the thrackle-metric of five points and display the f-vector of its tight span, do this:"
                  "# > $M = thrackle_metric(5);"
                  "# > $PC = metric_extended_tight_span($M);"
                  "# > print $PC->F_VECTOR;"
                  "# | 16 20 5",
                  &thrackle_metric,"thrackle_metric");

UserFunction4perl("# @category Finite metric spaces"
                  "# Compute [[SubdivisionOfPoints]] with a tight span of th thrackle  metric on //n// points."
                  "# This metric can be interpreted as a lifting function which induces the thrackle triangulation of the second hypersimplex."
                  "#\t See De Loera, Sturmfels and Thomas: Gröbner bases and triangulations of the second hypersimplex, Combinatorica 15 (1995)"
                  "# @param Int n the number of points"
                  "# @return SubdivisionOfPoints"
                  "# @example To compute the $f$-vector, do this:"
                  "# > print tight_span_min_metric(5)->POLYTOPAL_SUBDIVISION->TIGHT_SPAN->F_VECTOR;"
                  "# | 11 15 5",
                  &ts_thrackle_metric,"tight_span_thrackle_metric");

UserFunction4perl("# @category Finite metric spaces"
                  "# Compute a [[SubdivisionOfPoints]] with a tight span of a metric such that the f-vector is maximal among all metrics with //n// points."
                  "#\t See Herrmann and Joswig: Bounds on the f-vectors of tight spans, Contrib. Discrete Math., Vol.2, (2007)"
                  "# @param Int n the number of points"
                  "# @return SubdivisionOfPoints"
                  "# @example To compute the f-vector of the tight span with maximal f-vector, do this:"
                  "# > print tight_span_max_metric(5)->POLYTOPAL_SUBDIVISION->TIGHT_SPAN->F_VECTOR;"
                  "# | 11 15 5", 
                  &ts_max_metric, "tight_span_max_metric");

UserFunction4perl("# @category Finite metric spaces"
                  "# Compute a [[SubdivisionOfPoints]] with a tight span of a metric such that the f-vector is minimal among all metrics with //n// points."
                  "#\t See Herrmann and Joswig: Bounds on the f-vectors of tight spans, Contrib. Discrete Math., Vol.2, (2007)"
                  "# @param Int n the number of points"
                  "# @return SubdivisionOfPoints"
                  "# @example To compute the f-vector of the tight span with minimal f-vector, do this:"
                  "# > print tight_span_min_metric(5)->POLYTOPAL_SUBDIVISION->TIGHT_SPAN->F_VECTOR;"
                  "# | 11 15 5", 
                  &ts_min_metric, "tight_span_min_metric");

UserFunction4perl("# @category Finite metric spaces"
                  "# Computes a [[SubdivisionOfPoints]] with a weight function which is induced from a mertic."
                  "# @param Matrix<Rational> M a metric"
                  "# @option Bool extended If true, the extended tight span is computed."
                  "# @return SubdivisionOfPoints"
                  "# @example To compute the thrackle-metric of five points and display the f-vector of its tight span, do this:"
                  "# > $M = thrackle_metric(5);"
                  "# > $PC = metric_tight_span($M,extended=>1);"
                  "# > print $PC->POLYTOPAL_SUBDIVISION->TIGHT_SPAN->F_VECTOR;"
                  "# | 16 20 5",
                  &metric_tight_span, "metric_tight_span($;{extended=>0})");

UserFunction4perl("# @category Finite metric spaces"
                  "# Computes a extended tight span which is a [[PolyhedralComplex]] with induced from a mertic."
                  "# @param Matrix<Rational> M a metric"
                  "# @return PolyhedralComplex"
                  "# @example To compute the thrackle-metric of five points and display the f-vector of its tight span, do this:"
                  "# > $M = thrackle_metric(5);"
                  "# > $PC = metric_extended_tight_span($M);"
                  "# > print $PC->F_VECTOR;"
                  "# | 16 20 5",
                  &metric_extended_tight_span, "metric_extended_tight_span");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
