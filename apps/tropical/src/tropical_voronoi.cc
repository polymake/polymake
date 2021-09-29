/* Copyright (c) 1997-2021
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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/list"
#include "polymake/ListMatrix.h"
#include <vector>
#include <list>
#include <algorithm>

namespace polymake { namespace tropical {

// Input: list of sites in the tropical projective torus. (d+1) coords
// Output: the polytropes (as shortest path matrices) and labels of the polytrope partition
void compute_polytrope_partition(BigObject tvd)
{
  const Matrix<Rational> sites = tvd.give("SITES");
  Int d = sites.cols()-1; // These are tropical projective coordinates: No 1 at the beginning
  std::list<std::pair<Matrix<Rational>, Matrix<Int>>> cells;

  // Each polytrope in the partition represented by a (d+1)x(d+1) matrix with entries a_ij where x_j-x_i <= a_ij
  // The label at (i,j) is the site with maximum s_j-s_i s.t. the cone (x_i-s_i <= x_k-s_k <= x_j-s_j \forall k) intersects (and actually, covers) this cell. (Site-centric labeling)
  // This cone, as a polytope, has matrix a_ki=s_i-s_k; a_jk= s_k-s_j (otherwise a_..=inf)

  // Insert the trivial cell with no labels
  Matrix<Rational> cell(d+1, d+1);
  Matrix<Int> label(d+1, d+1);
  for (Int i = 0; i< d+1; ++i)
    for (Int j = 0; j < d+1; ++j) {
      cell(i,j) = i == j ? 0 : Rational::infinity(1);
      label(i,j) = -1;
    }

  cells.insert(cells.end(), std::make_pair(cell,label));

  for (Int p = 0; p < sites.rows(); ++p) { // For each site
    const auto& s = sites[p];

    for (auto it = cells.begin(); it != cells.end(); ++it) { // For each cell
        cell = it->first; label = it->second;

        Rational cover_distance = Rational::infinity(1); //minimum distance c such that some cone centered at a labelled site covers this cell at distance at most c.
        for (Int i = 0; i < d+1; ++i)
          for (Int j = 0; j < d+1; ++j)
            if (i!=j && label(i, j) != -1) {
              const auto& s2 = sites[label(i, j)];
              assign_min(cover_distance, cell[i][j]-(s2[j]-s2[i]));
            }

        std::list<std::pair<Int, Int>> indices;
        for (Int i = 0; i < d+1; ++i)
          for (Int j = 0; j < d+1; ++j)
            if (i != j) {   // Find indices i,j s.t. the cone x_i-s_i <= x_k-s_k <= x_j-s_j intersects this cell

              Matrix<Rational> cell2(cell);
              for (Int k = 0; k < d+1; ++k) {
                assign_min(cell2(k, i), s[i]-s[k]);
                assign_min(cell2(j, k), s[k]-s[j]);
              }

              // Floyd-Warshall
              for (Int k2 = 0; k2 < d+1; ++k2)
                for (Int k1 = 0; k1 < d+1; ++k1)
                  for (Int k3 = 0; k3 < d+1; ++k3)
                    assign_min(cell2(k1, k3), cell2(k1, k2)+cell2(k2, k3));

              bool empty = false;

              for (Int k1 = 0; !empty && k1 < d+1; ++k1)
                for (Int k2 = k1+1; k2 < d+1; ++k2)
                  if (cell2(k1, k2)+cell2(k2, k1) <=0 ) {
                    empty = true;
                    break;
                  }

              if (!empty && -cell2(j, i)-(s[j]-s[i]) > cover_distance)
                empty = true;

              if (!empty) indices.push_back(std::make_pair(i,j));
        }

        if (indices.size() == 1) { // If there is only one, update the labels
          const Int i = indices.front().first, j = indices.front().second;
          if (it->second(i, j) == -1 || s[j]-s[i] > sites(it->second(i, j), j)-sites(it->second(i, j), i))
            it->second(i, j) = p;
        } else if (indices.size() > 1) {
          // If there is more than one, remove the cell, compute new cells
          it = cells.erase(it);

          for (auto &ij : indices) {
            const Int i = ij.first, j = ij.second;
            Matrix<Rational> cell2(cell);
            Matrix<Int> label2(label);

            for (Int k = 0; k < d+1; ++k) {
              assign_min(cell2(k, i), s[i]-s[k]);
              assign_min(cell2(j, k), s[k]-s[j]);
            }

            // Floyd-Warshall
            for (Int k2 = 0; k2 < d+1; ++k2)
              for (Int k1 = 0; k1 < d+1; ++k1)
                for (Int k3 = 0; k3 < d+1; ++k3)
                  assign_min(cell2(k1, k3), cell2(k1, k2)+cell2(k2, k3));

            // Update the label if needed
            if (label2(i, j) == -1 || s[j]-s[i]> sites(label2(i, j), j)-sites(label2(i, j), i))
              label2(i, j)= p;

            cells.insert(it, std::make_pair(cell2,label2)); // Insert the new cell
          }

          --it;
        }
    }
  }

  tvd.take("POLYTROPE_PARTITION") << cells;
}


ListReturn visualizable_cells(const Matrix<Rational>& sites, Int d, const Array<std::pair<Matrix<Rational>, Matrix<Int>>>& cells, const Rational& cutoff)
{
  ListReturn results;
  // list of ridges
  // Ax<=b -> (b|-A)
  // with d+2 coordinates

  for (auto &it: cells) { // for each cell

    // for each pair of distinct cones with label there is a polytrope\cap region:
    // for each third cone add
    //   - The corresponding facet of the polytrope
    //   - The corresponding facet of one label against all others (But not the second)
    //   - The corresponding facet of the other label against others  (not the first)
    //   - The equality corresponding to these two cones
    const Matrix<Rational>& cell = it.first;
    const Matrix<Int>& label = it.second;

    for (Int i1 = 0; i1 < d+1; ++i1)
      for (Int j1 = 0; j1 < d+1; ++j1)
        if (label(i1, j1) != -1) {
          for (Int i2 = i1; i2 < d+1; ++i2)
            for (Int j2 = 0; j2 < d+1; ++j2)
              if (label(i2, j2) != -1) {
                if (i1==j1 || i2==j2) continue;
                if (i2==i1 && j2<=j1) continue;
                const auto& s1 = sites[label(i1, j1)];
                const auto& s2 = sites[label(i2, j2)];

                ListMatrix<Vector<Rational>> ineq(0, d+2);
                for (Int i3 = 0; i3 < d+1; ++i3)
                  for (Int j3 = 0; j3 < d+1; ++j3) {
                    if (i3==j3) continue;

                    if (cell(i3, j3) != Rational::infinity(1)) { // x_j-x_i <= a_ij
                      Vector<Rational> row(d+2);
                      row[0] = cell(i3, j3);
                      row[j3+1] -= 1;
                      row[i3+1] += 1;
                      ineq /= row;
                    }

                    if (label(i3, j3)==-1) continue;
                    if ((i3==i1 && j3==j1) || (i3==i2 && j3==j2)) continue;
                    const auto& s3 = sites[label(i3, j3)];
                    {
                      // dist(x,s1)<=dist(x,s3)
                      Vector<Rational> row(d+2);  // x_j1-x_i1-s1_j1+s1_i1 <= x_j3-x_i3-s3_j3+s3_i3
                      row[0] = -s1[i1]+s1[j1]+s3[i3]-s3[j3];
                      row[i1+1] += 1;
                      row[j1+1] -= 1;
                      row[i3+1] -= 1;
                      row[j3+1] += 1;
                      ineq /= row;
                    }
                    // same for i2
                    {
                      // dist(x,s2)<=dist(x,s3)
                      Vector<Rational> row(d+2); // x_j2-x_i2-s2_j2+s2_i2 <= x_j3-x_i3-s3_j3+s3_i3
                      row[0] = -s2[i2]+s2[j2]+s3[i3]-s3[j3];
                      row[i2+1] += 1;
                      row[j2+1] -= 1;
                      row[i3+1] -= 1;
                      row[j3+1] += 1;
                      ineq /= row;
                    }
                  }

                for (Int i = 0; i < d+1; ++i)
                  for (Int j = 0; j < d+1; ++j)
                    if (i != j) { //Tropical bounding ball
                      Vector<Rational> row(d+2);
                      row[0] = cutoff;
                      row[j+1] = -1;
                      row[i+1] = 1;
                      ineq /= row;
                    }

                Matrix<Rational> eq(1, d+2); // dist(x,s1)=dist(x,s2)
                // x_j2-x_i2-s2_j2+s2_i2 = x_j1-x_i1-s1_j1+s1_i1
                eq(0, 0) = -s1[i1]+s1[j1]+s2[i2]-s2[j2];
                eq(0, i1+1) += 1;
                eq(0, j1+1) -= 1;
                eq(0, i2+1) -= 1;
                eq(0, j2+1) += 1;

                BigObject P("polytope::Polytope<Rational>",
                            "INEQUALITIES", ineq.minor(All, range(0, d)),
                            "EQUATIONS", eq.minor(All, range(0, d)));

                const bool feasible = P.give("FEASIBLE");
                if (feasible) results << P;
              }
        }
  }

  for (Int i = 0; i < sites.rows(); ++i) {
    Vector<Rational> thepoint = 1 | sites[i].slice(sequence(0,d));
    for (Int j = 0; j < d; ++j)
      thepoint[j+1] -= sites(i, d); // Projecting to the x[d]=0 hyperplane.

    results << BigObject("polytope::Polytope<Rational>", "POINTS", vector2row(thepoint));
  }

  return results;
}

Function4perl(&compute_polytrope_partition, "compute_polytrope_partition");
Function4perl(&visualizable_cells, "visualizable_cells");

} }
