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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/list"
#include "polymake/SparseMatrix.h"
#include <vector>
#include <list>
#include <algorithm>

namespace polymake { namespace tropical {

  // a= min(a,b)
  void inline relax(Rational &a, const Rational &b) {if (b<a) a=b;}

  // Input: list of sites in the tropical projective torus. (d+1) coords
  // Output: the polytropes (as shortest path matrices) and labels of the polytrope partition
  Array<std::pair<Matrix<Rational>, Matrix<int>>> compute_polytrope_partition(const Matrix<Rational> & sites) {
    int d= sites.cols()-1; // These are tropical projective coordinates: No 1 at the  beginning
    std::list<std::pair<Matrix<Rational>, Matrix<int>>> cells;

    // Each polytrope in the partition represented by a (d+1)x(d+1) matrix with entries a_ij where x_j-x_i <= a_ij
    // The label at (i,j) is the site with maximum s_j-s_i s.t. the cone (x_i-s_i <= x_k-s_k <= x_j-s_j \forall k) intersects (and actually, covers) this cell. (Site-centric labeling)
    // This cone, as a polytope, has matrix a_ki=s_i-s_k; a_jk= s_k-s_j (otherwise a_..=inf)

    // Insert the trivial cell with no labels
    Matrix <Rational> cell(d+1, d+1); Matrix <int> label(d+1,d+1);
    for (int i=0;i<d+1;++i) for (int j=0; j<d+1; ++j) {cell(i,j)= (i==j)?0:Rational::infinity(1); label(i,j)= -1;}
    cells.insert(cells.end(), std::make_pair(cell,label));

    for (int p=0; p<sites.rows(); ++p) { // For each site
      Vector<Rational> s= sites[p];

      for (auto it= cells.begin(); it!= cells.end(); ++it) { // For each cell
        cell= it->first; label= it->second;

        Rational cover_distance= Rational::infinity(1); //minimum distance c such that some cone centered at a labelled site covers this cell at distance at most c.
        for (int i=0; i<d+1; ++i) for(int j=0; j<d+1; ++j) if(i!=j) if(label[i][j]!=-1){
          Vector<Rational> s2= sites[label[i][j]];
          relax(cover_distance, cell[i][j]-(s2[j]-s2[i]));
        }

        std::list<std::pair<int,int>> indices; //
        for (int i=0; i<d+1; ++i) for(int j=0; j<d+1; ++j) if(i!=j){// Find indices i,j s.t. the cone x_i-s_i <= x_k-s_k <= x_j-s_j intersects this cell

          Matrix<Rational> cell2(cell);
          for(int k=0; k<d+1; ++k) relax(cell2[k][i], s[i]-s[k]), relax(cell2[j][k], s[k]-s[j]);

          // Floyd-Warshall
          //for(int k1=0; k1<d+1; ++k1) for(int k2=0; k2<d+1; ++k2) relax(cell2[k1][k2], cell2[k1][i]+cell2[i][k2]);
          //for(int k1=0; k1<d+1; ++k1) for(int k2=0; k2<d+1; ++k2) relax(cell2[k1][k2], cell2[k1][j]+cell2[j][k2]);
          for(int k2=0; k2<d+1; ++k2) for(int k1=0; k1<d+1; ++k1) for(int k3=0; k3<d+1; ++k3)
            relax(cell2[k1][k3], cell2[k1][k2]+cell2[k2][k3]);

          bool empty=false;

          //for (int k=0; k<d+1; ++k)
          //  if(cell2[k][k]<0) empty=true;
          for(int k1=0; k1<d+1; ++k1) for(int k2=k1+1; k2<d+1; ++k2)
            if(cell2[k1][k2]+cell2[k2][k1]<=0) empty=true;

          if (-cell2[j][i]-(s[j]-s[i])>cover_distance) empty=true;

          if(!empty) indices.push_back(std::make_pair(i,j));
        }

        if (indices.size()==1) { // If there is only one, update the labels
          int i= indices.front().first, j=indices.front().second;
          if (it->second[i][j]==-1 || s[j]-s[i]> sites[it->second[i][j]][j]-sites[it->second[i][j]][i])
            it->second(i,j)= p;
        }
        else if (indices.size()>1) { // If there is more than one, remove the cell, compute new cells
          it=cells.erase(it);

          for(auto &ij : indices) {
            int i=ij.first, j=ij.second; Matrix<Rational> cell2(cell); Matrix<int> label2(label);

            for(int k=0; k<d+1; ++k) relax(cell2[k][i], s[i]-s[k]), relax(cell2[j][k], s[k]-s[j]);

            // Floyd-Warshall
            //for(int k1=0; k1<d+1; ++k1) for(int k2=0; k2<d+1; ++k2) relax(cell2[k1][k2], cell2[k1][i]+cell2[i][k2]);
            //for(int k1=0; k1<d+1; ++k1) for(int k2=0; k2<d+1; ++k2) relax(cell2[k1][k2], cell2[k1][j]+cell2[j][k2]);
            for(int k2=0; k2<d+1; ++k2) for(int k1=0; k1<d+1; ++k1) for(int k3=0; k3<d+1; ++k3)
              relax(cell2[k1][k3], cell2[k1][k2]+cell2[k2][k3]);

            // Update the label if needed
            if (label2[i][j]==-1 || s[j]-s[i]> sites[label2[i][j]][j] - sites[label2[i][j]][i]) label2[i][j]= p;

            cells.insert(it, std::make_pair(cell2,label2)); // Insert the new cell
          }

          it--;
    } } }

    return Array<std::pair<Matrix<Rational>,Matrix<int>>>(cells.size(), cells.begin());
  }

  perl::ListReturn visualizable_cells(Matrix<Rational> sites, int d, Array<std::pair<Matrix<Rational>, Matrix<int>>> cells) {

    perl::ListReturn results;
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
      Matrix<Rational> cell= it.first; Matrix<int> label= it.second;

      for (int i1=0; i1<d+1; ++i1) for(int j1=0; j1<d+1; ++j1) if (label[i1][j1]!=-1)
        for (int i2=i1; i2<d+1; ++i2) for(int j2=0; j2<d+1; ++j2) if (label[i2][j2]!=-1) {
          if(i1==j1 || i2==j2) continue;
          if(i2==i1 && j2<=j1) continue;
          Vector<Rational> s1= sites[label[i1][j1]], s2= sites[label[i2][j2]];

          Matrix<Rational> ineq(0, d+2);
          for (int i3=0; i3<d+1; ++i3) for(int j3=0; j3<d+1; ++j3) {
            if(i3==j3) continue;

            if(cell[i3][j3]!=Rational::infinity(1)) { // x_j-x_i <= a_ij
              Vector<Rational> row(d+2);
              row[0]=cell[i3][j3]; row[j3+1]-=1; row[i3+1]+=1;
              ineq/=row;
            }

            if(label[i3][j3]==-1) continue;
            if((i3==i1 && j3==j1) || (i3==i2 && j3==j2)) continue;
            Vector<Rational> s3= sites[label[i3][j3]];

            //dist(x,s1)<=dist(x,s3)
            Vector<Rational> row(d+2); // x_j1-x_i1-s1_j1+s1_i1 <= x_j3-x_i3-s3_j3+s3_i3
            row[0]= -s1[i1]+s1[j1]+s3[i3]-s3[j3]; row[i1+1]+=1; row[j1+1]-=1; row[i3+1]-=1; row[j3+1]+=1;
            ineq/=row;

            //same for i2
            //dist(x,s2)<=dist(x,s3)
            row= Vector<Rational>(d+2); // x_j2-x_i2-s2_j2+s2_i2 <= x_j3-x_i3-s3_j3+s3_i3
            row[0]= -s2[i2]+s2[j2]+s3[i3]-s3[j3]; row[i2+1]+=1; row[j2+1]-=1; row[i3+1]-=1; row[j3+1]+=1;
            ineq/=row;
          }

          for (int i=0; i<d+1; ++i) for(int j=0; j<d+1; ++j) if (i!=j) { //Tropical bounding ball
            Vector<Rational> row(d+2);
            row[0]=20; row[j+1]=-1; row[i+1]=1;
            ineq/=row;
          }

          ineq= ineq.minor(All, sequence(0,d+1));
          perl::Object P("polytope::Polytope<Rational>");
          P.take("INEQUALITIES")<<ineq;

          Matrix<Rational> eq(1,d+2); //dist(x,s1)=dist(x,s2)
          // x_j2-x_i2-s2_j2+s2_i2 = x_j1-x_i1-s1_j1+s1_i1
          eq[0][0]= -s1[i1]+s1[j1]+s2[i2]-s2[j2];
          eq[0][i1+1]+=1; eq[0][j1+1]-=1; eq[0][i2+1]-=1; eq[0][j2+1]+=1;
          eq= eq.minor(All, sequence(0,d+1));
          P.take("EQUATIONS")<<eq;

          bool feasible;
          P.give("FEASIBLE")>>feasible;
          if(feasible) results<<P;
        }
    }

    for(int i=0; i<sites.rows(); ++i) {
      Matrix<Rational> thepoint= Matrix<Rational>(1,1) | sites.minor(sequence(i,1), sequence(0,d));
      for(int j=0; j<d; ++j) thepoint[0][j+1]-= sites[i][d]; // Projecting to the x[d]=0 hyperplane.

      thepoint[0][0]=1;
      perl::Object P("polytope::Polytope<Rational>");
      P.take("POINTS")<<thepoint.minor(All, sequence(0,d+1));
      results<<P;
    }

    return results;
  }

  Function4perl(&compute_polytrope_partition, "compute_polytrope_partition");
  Function4perl(&visualizable_cells, "visualizable_cells");
} }
