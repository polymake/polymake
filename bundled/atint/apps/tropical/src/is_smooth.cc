/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	---
	Copyright (c) 2016-2019
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Algorithm and code by Dennis Diefenbach <diefenba@mathematik.uni-kl.de>
	(slightly modified by Simon Hampe during the rewrite for version 2).

	This file contains a function to test smoothness of tropical fans (at least for dimension 0,1,2
	and codimension 0,1).

*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/list"
#include "polymake/Vector.h"
#include "polymake/PowerSet.h"
#include "polymake/integer_linalg.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/morphism_thomog.h"
#include "polymake/tropical/lattice.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace tropical {

// DEFINITIONS OF TYPES AND FUNCTIONS ////////////////////////////////////

// This is the type returned by many functions, it is a tripel consisting of:
// -An int which is 0 if the curve is not smooth, 1 if it is smooth, 2 if it is not clear
// -if s=1 the "matroid" contains a matroid such that the corresponding matroid fan is isomrphic to the input fan
// -"map" is a Z-isomorphisem between the input fan and the matroid fan
struct result {
  int is_smooth;
  perl::Object matroid;
  Matrix<int> map;
};

// Dehomogenizes and removes vertex and leading coordinate.
Matrix<Rational> reduce_rays(const Matrix<Rational>& m)
{
  Set<int> vertex = far_and_nonfar_vertices(m).first;
  return tdehomog(m).minor(vertex, range_from(1));
}

// This functions takes the lineality space of the fan, searches a Z-isomorphisem mapping it to the last coordinates, projects the last coordinates down and returnes the projection of the fan and by reference the Z-isomorphisem
template <typename Addition>
perl::Object cut_out_lineality_space(perl::Object f, Matrix <int> &map_lineality_space);

// If the linear span U of the rays of the input fan is not the hole space, the function can find a Z-isomorphisem h that maps U to the first coordinates and gives back the image of the fan under h and by reference h
template <typename Addition>
perl::Object full_dimensional(perl::Object f, Matrix<int>& map_full_dimensional);

// Takes as an input a fan of dimension 1 and returnes 0 if they are NOT smooth and 1 if they are smooth
result smooth_dim1(perl::Object f);

// Takes as an input a fan of dimension 2 and returnes 0 if they are NOT smooth and 1 if they are smooth
result smooth_dim2(perl::Object f);

// Checks if the graph is the combinatorial graph of a matroid
result is_combinatorial_right(perl::Object f, Vector< Vector<int> > combinatorics, int N_Nodes, int N_Edges, int Fan_Ambient_Dimension);

// Checks if the Flats satisfies the axiom of a matroid
bool check_matroid(Vector<Vector<int>>& combinatorics,  Vector<Set<int>>& Flats, int Fan_Ambient_Dimension);

// Takes as an input a fan of codimension 1 and returnes 0 if they are NOT smooth and 1 if they are smooth
result smooth_codim1(perl::Object f);

// This functions takes a fan and checks if it is smooth or not, i.e. it checks if there exists a matroid fan such that the corresponing matroid fan is Z-isomorphic to the given fan
template <typename Addition>
perl::ListReturn is_smooth(perl::Object f)
{
  result R;
  R.is_smooth=2;
  // Check that all tropical weights are 1
  Vector<Integer> Tropical_Weights=f.give("WEIGHTS");
  for (int i=0; i<Tropical_Weights.size(); ++i) {
    if (Tropical_Weights[i]!=1) {
      R.is_smooth=0;
    }
  }

  // Find the coarsest subdivision of the fan f
  perl::Object g;
  if (R.is_smooth!=0) {
    try {
      g=call_function("coarsen", f, 1);
    }
    catch (const std::runtime_error&) {
      // The coarsest subdivision does not exist
      R.is_smooth=0;
    }
  }

  // Check that g is a fan
  if (!g.call_method("is_fan")) {
    R.is_smooth=0;
  }

  if (R.is_smooth!=0) {
    // Restrict to the case where the fan has no lineality space
    int Lineality_Dim=g.give("LINEALITY_DIM");
    Matrix <int> map_lineality_space;
    if (Lineality_Dim>0) {
      g=cut_out_lineality_space<Addition>(g, map_lineality_space);
    }

    // Determine some elementary properties of the fan f
    int Fan_Dim= g.give("PROJECTIVE_DIM");
    int N_Rays=g.give("N_VERTICES");
    --N_Rays; // Remove origin
    Matrix<Rational> Rays=g.give("VERTICES");
    Rays = reduce_rays(Rays);
    int Linear_Span_Dim=rank(Rays);

    R.is_smooth=2; // s=0 the curve is NOT smooth, s=1 the curve is smooth, s=2 we cannot say if it is smooth or not

    // Check if the number of rays is not to big, i.e. upper bounds for the number of rays in the coarsets subdivision of the fan are known, if they are not satisfied the fan is NOT smooth
    if (Fan_Dim==1) {
      if (N_Rays>Linear_Span_Dim+1) {
        R.is_smooth=0;
      }
    } else if (Fan_Dim==2) {
      if ((Linear_Span_Dim+1)<=9) { // Upper bound for dimension 1 cells found experimentally
        int bound[7]={0,4,6,10,14,16,21};
        if (N_Rays>bound[(Linear_Span_Dim+1)-3]) {
          R.is_smooth=0;
        }
      } else if (N_Rays>((Linear_Span_Dim+1)*(Linear_Span_Dim+1))) { //Theoretical upper bound
        R.is_smooth=0;
      }
    } else if (Fan_Dim==Linear_Span_Dim-1) {
      if (N_Rays>Linear_Span_Dim+1){
        R.is_smooth=0;
      }
    }

    // If the above criterions did not suffice to determine if the curve is smooth or not the algorithem enters here
    Matrix <int> map_full_dimensional;
    if (R.is_smooth==2) {
      // Restrict to the case where the fan is full dimensional
      int Fan_Ambient_Dimension= g.give("PROJECTIVE_AMBIENT_DIM");
      int codim_linear_span=Fan_Ambient_Dimension-Linear_Span_Dim;
      if (codim_linear_span!=0) {
        g=full_dimensional<Addition>(g, map_full_dimensional);
        Fan_Ambient_Dimension= g.give("PROJECTIVE_AMBIENT_DIM");
      }
      if (Fan_Dim==0) {
        Array< Set<int> > Bases(1);
        Bases[0]=Bases[0]+0;
        perl::Object M("matroid::Matroid");
        M.take("BASES") << Bases;
        M.take("N_ELEMENTS") << 1;
        R.is_smooth=1;
        R.matroid=M;
      } else if (Fan_Dim==1) {
        R=smooth_dim1(g);
      } else if (Fan_Dim==Fan_Ambient_Dimension-1) {
        R=smooth_codim1(g);
      } else if (Fan_Dim==2) {
        R=smooth_dim2(g);
      } else if (Fan_Dim==Fan_Ambient_Dimension) {
        R.is_smooth=0;
      } else {
        R.is_smooth=2;
      }
      if (R.is_smooth==1) {
        Array< Set<int> > Bases=(R.matroid).give("BASES");
        int N_Elements=(R.matroid).give("N_ELEMENTS");
        if (codim_linear_span!=0) {
          // Compute the bases
          for (int i=0; i<codim_linear_span; ++i) {
            for (int j=0; j<Bases.size(); ++j) {
              if (Bases[j].contains(0)) {
                Set<int> NewBase=Bases[j]-0;
                NewBase=NewBase+(N_Elements);
                Bases.resize(Bases.size()+1,NewBase);
              }
            }
            N_Elements=N_Elements+1;
          }
          // Compute the map
          Matrix <int> temp(R.map.rows()+codim_linear_span, R.map.cols()+codim_linear_span);
          for (int i=0; i < R.map.rows(); ++i) {
            for (int j=0; j < R.map.rows(); ++j) {
              temp[i][j]=R.map[i][j];
            }
          }
          for (int i=0; i<codim_linear_span; ++i) {
            for (int j=0; j<codim_linear_span; ++j) {
              if (i==j) {
                temp[i + R.map.rows()][j + R.map.rows()]=1;
              } else {
                temp[i + R.map.rows()][j + R.map.rows()]=0;
              }
            }
          }
          R.map=temp*map_full_dimensional;
        }
        if (Lineality_Dim>0) {
          // Compute the matroid
          for (int j = 0; j < Bases.size(); ++j)  {
            Bases[j] += sequence(N_Elements, Lineality_Dim);
          }
          // Compute the map
          Matrix<int> temp(R.map.rows()+Lineality_Dim, R.map.cols()+Lineality_Dim);
          temp.minor( sequence(0, R.map.rows()), sequence(0, R.map.cols())) = R.map;
          // FIXME: suspicious coordinate expressions; is R.map always square?
          temp.minor( sequence(R.map.rows(), Lineality_Dim), sequence(R.map.rows(), Lineality_Dim)) = unit_matrix<int>(Lineality_Dim);
          R.map= temp*map_lineality_space;
        }

        (R.matroid).take("BASES") << Bases;
        (R.matroid).take("N_ELEMENTS") << N_Elements + Lineality_Dim;
      }
    }
  }

  perl::ListReturn S;
  S << R.is_smooth;
  if (R.is_smooth==1) {
    S << R.matroid;
    // Construct a morphism from the map
    Matrix<Rational> mapmatrix = thomog_morphism(Matrix<Rational>(R.map), zero_vector<Rational>(R.map.cols())).first;
    perl::Object mapmorphism("Morphism", mlist<Addition>());
    mapmorphism.take("MATRIX") << Addition::orientation() * mapmatrix;

    S << mapmorphism;
  }
  return S;
}

template <typename Addition>
perl::Object cut_out_lineality_space(perl::Object f, Matrix<int>& map_lineality_space)
{
  // A basis of the lineality space is computed
  Matrix<Rational> Lineality_Space=f.give("LINEALITY_SPACE");
  Lineality_Space = reduce_rays(Lineality_Space);
  int Fan_Ambient_Dimension=f.give("PROJECTIVE_AMBIENT_DIM");
  int Lineality_Dim=f.give("LINEALITY_DIM");
  Matrix<Rational> Matrix_Description=null_space (Lineality_Space);

  // If the lineality space is the hole space
  if (Matrix_Description.rows()==0) {
    Matrix_Description.resize(1,Fan_Ambient_Dimension);
    for (int i=0; i<Fan_Ambient_Dimension; i++) {
      Matrix_Description[0][1]=0;
    }
  }

  // Make the matrix integer without changing the kernel
  Matrix<Integer> Matrix_Description_Integer(Matrix_Description.rows(), Matrix_Description.cols());
  for (int i=0; i<Matrix_Description.rows(); ++i) {
    Integer LCM_Denominator=1;
    for (int j=0; j<Matrix_Description.cols(); ++j) {
      Integer Denominator=denominator(Matrix_Description[i][j]);
      LCM_Denominator=lcm(LCM_Denominator,Denominator);
    }
    for (int j=0; j<Matrix_Description.cols(); ++j) {
      Matrix_Description_Integer[i][j]=Matrix_Description[i][j]*LCM_Denominator;
    }
  }

  // The hermit normal form of the Matrix description is computed
  HermiteNormalForm<Integer> hnfresult = hermite_normal_form(Matrix_Description_Integer);
  Matrix<Integer> tfmatrix(hnfresult.companion);	

  // A linear map is computed that maps the lineality space with dimension k to the last k coordinates
  tfmatrix=inv(tfmatrix);
  map_lineality_space=convert_to<int>(tfmatrix);
			
  // Apply the map to the rays
  Matrix<Rational> Rays=f.give("VERTICES");
  Rays = tdehomog(Rays);
  Vector<Rational> leading_coordinate;
  if (Rays.rows() > 0) {
    leading_coordinate = Rays.col(0);
    Rays = Rays.minor(All, range_from(1));
    Rays=T(Rays);
    Rays=tfmatrix*Rays;
  }

  // Project the lineality space down and compute the remaining polyhedral fan
  Matrix <Rational> New_Rays(Rays.rows()-Lineality_Dim, Rays.cols());
  for (int i=0; i<Rays.rows()-Lineality_Dim; ++i) {
    for (int j=0; j<Rays.cols(); ++j) {
      New_Rays[i][j]=Rays[i][j];
    }
  }

  New_Rays=T(New_Rays);
  New_Rays = leading_coordinate | New_Rays;

  IncidenceMatrix<> Maximal_Cones =f.give("MAXIMAL_POLYTOPES");
  perl::Object g("Cycle", mlist<Addition>());
  g.take("VERTICES") << thomog(New_Rays);
  g.take("MAXIMAL_POLYTOPES") << Maximal_Cones;
  g.take("PROJECTIVE_AMBIENT_DIM") << (Fan_Ambient_Dimension-Lineality_Dim);
  return g;
}

template <typename Addition>
perl::Object full_dimensional(perl::Object f, Matrix<int>& map_full_dimensional)
{
  // A basis of the space generated by the rays of f is computed
  Matrix<Rational> Rays=f.give("VERTICES");
  Vector<Rational> leading_coordinate = Rays.col(0);
  Rays = tdehomog(Rays).minor(All, range_from(1));
  Set<int> Basis = basis_rows(Rays);

  // Special case: If the space is only a point, we need a separate computation
  if (Basis.size() == 0) {
    int proj_amb = f.give("PROJECTIVE_AMBIENT_DIM");
    map_full_dimensional = unit_matrix<int>(proj_amb);
    perl::Object point_result("Cycle", mlist<Addition>());
    point_result.take("VERTICES") << Matrix<Rational>(0, proj_amb+2);
    point_result.take("MAXIMAL_POLYTOPES") << f.give("MAXIMAL_POLYTOPES");
    point_result.take("PROJECTIVE_AMBIENT_DIM") << proj_amb;
    return point_result;
  }

  // The basis is written to the matrix Basis_Subspace
  Matrix<Rational> Basis_Subspace = Rays.minor(Basis,All);
  int k = Basis.size();

  // A description of the subspace as the kernel of a map is searched
  Matrix<Rational> Matrix_Description=null_space (Basis_Subspace);

  // Make the entries of Matrix_Description integers without changing the kernel
  Matrix<Integer> Matrix_Description_Integer = polymake::common::eliminate_denominators_in_rows(Matrix_Description);

  // The hermit normal form of the transposed is computed
  HermiteNormalForm<Integer> hnfresult = hermite_normal_form(Matrix_Description_Integer);
  Matrix<Integer> tfmatrix(hnfresult.companion);

  // NOTE: the above matrix maps the vectors of the standard bases to the basis of the subspace extended to a bases of the hole space, the matrix is a Z-isomorphisem

  // The order of the columns is permuted such that the vecotrs e_{1}, ..., e_{k} are mapped to a basis of the subspace by tfmatrix
  Matrix<Integer> tfmatrix_temp(tfmatrix.rows(), tfmatrix.cols());
  for (int i=0; i<tfmatrix_temp.rows(); ++i) {
    for (int j=0; j<k; ++j) {
      tfmatrix_temp[i][j]=tfmatrix[i][tfmatrix.cols()-k+j];
    }
  }
  for (int i=0; i<tfmatrix_temp.rows(); ++i) {
    for (int j=k; j<tfmatrix.cols(); ++j) {
      tfmatrix_temp[i][j]=tfmatrix[i][j-k];
    }
  }

  // The matrix represents a Z-isomorphisem mapping the subspace of dimension k to the vectors e_{1},...,e_{k}
  if (Rays.cols()==0) {
    int Fan_Ambient_Dimension=f.give("PROJECTIVE_AMBIENT_DIM");
    map_full_dimensional = unit_matrix<int>(Fan_Ambient_Dimension);
  } else {
    map_full_dimensional = convert_to<int>(tfmatrix_temp);
  }
  tfmatrix=inv(tfmatrix_temp);
  map_full_dimensional = convert_to<int>(inv(map_full_dimensional));

  // Computes the immage of the rays under the Z-isomorphism constructed above
  Rays=T(Rays);
  Rays=tfmatrix*Rays;

  // Ignore the last coordinates and compute the associated fan
  Matrix <Rational> New_Rays(k, Rays.cols());
  for (int i=0; i<k; ++i) {
    for (int j=0; j<Rays.cols(); ++j) {
      New_Rays[i][j]=Rays[i][j];
    }
  }
  New_Rays=T(New_Rays);
  New_Rays = leading_coordinate | New_Rays;
  IncidenceMatrix<> Maximal_Cones =f.give("MAXIMAL_POLYTOPES");
  perl::Object g("Cycle", mlist<Addition>());
  g.take("VERTICES") << thomog(New_Rays);
  g.take("MAXIMAL_POLYTOPES") << Maximal_Cones;
  return g;
}

result smooth_dim1(perl::Object f)
{
  result R;
  Matrix<Rational> Rays_Rational=f.give("VERTICES");
  Rays_Rational = reduce_rays(Rays_Rational);
  // Search the correpsonding integer primitive vectors
  Matrix<Integer> Rays(Rays_Rational.rows(), Rays_Rational.cols());
  for (int i=0; i<Rays.rows(); ++i) {
    Integer LCM_Denominator=1;
    for (int j=0; j<Rays.cols(); ++j) {
      Integer Denominator=denominator(Rays_Rational[i][j]);
      LCM_Denominator=lcm(LCM_Denominator,Denominator);
    }
    for (int j=0; j<Rays.cols(); ++j) {
      Rays_Rational[i][j]=Rays_Rational[i][j]*LCM_Denominator;
    }
    Integer GCD_Numerator=1;
    for (int j=0; j<Rays.cols(); ++j) {
      GCD_Numerator=gcd(numerator(Rays_Rational[i][j]),GCD_Numerator);
    }
    for (int j=0; j<Rays.cols(); ++j){
      Rays[i][j]=Rays_Rational[i][j]/GCD_Numerator;
    }
  }
  Rays=T(Rays);

  Rays.resize(Rays.rows(), Rays.cols()-1);

  // Compute the Hermit normal form of the Rays
  HermiteNormalForm<Integer> hnfresult = hermite_normal_form(Rays);
  Matrix<Integer> tfmatrix(hnfresult.companion);

  // Compute the determinant of HNF
  Integer determinant=det(hnfresult.hnf);
  if (determinant==1 or determinant==-1) {
    Array<Set<int>> Bases((Rays.cols()*(Rays.cols()+1))/2);
    int k=0;
    for (int i=0; i<Rays.cols()+1; ++i) {
      for (int j=i+1; j<Rays.cols()+1; ++j) {
        Bases[k]=Bases[k]+i+j;
        ++k;
      }
    }

    perl::Object M("matroid::Matroid");
    M.take("BASES") << Bases;
    M.take("N_ELEMENTS") << Rays.cols()+1;

    // Prepare the output
    R.is_smooth=1;
    R.matroid=M;
    R.map=convert_to<int>(tfmatrix);
  } else {
    R.is_smooth=0;
  }

  return R;
}

result smooth_dim2(perl::Object f)
{
  result R;
  R.is_smooth=0;

  // Some elementary properties are extracted from f
  Set<int> far_face = f.give("FAR_VERTICES");
  IncidenceMatrix<> MaximalCones = f.give("MAXIMAL_POLYTOPES");
  MaximalCones = MaximalCones.minor(All,far_face);
  int Fan_Ambient_Dimension= f.give("PROJECTIVE_AMBIENT_DIM");

  // The combinatorial graph associated to the fan is computed and some elementary properties are extracted
  perl::Object graph_from_edges = call_function("graph::graph_from_edges", MaximalCones);
  int N_Nodes=graph_from_edges.give("N_NODES");
  int N_Edges=graph_from_edges.give("N_EDGES");

  // The combinatroics matrix is initialized, it has the following structure:
  // One row for each node of the combinatorial graph and one row for each edge of the combinatorial graph.
  // If the row corresponds to a node, in COLUMN 1 is written which one (by enumerating it from 0 to N_Node-1), and COLUMN 2 is zero. COLUMN 3 contains 1 if the node corresponds to a flat of rank 1 and 2 if it corresponds to a flat of rank 2.
  // If the row corresponds to an edge, COLUMN1 and COLUMN2 indicates which nodes they are connecting. COLUMN 3 is 0 if the edge is not split, 1 if it is split and the splitting point corresponds to a flat of rank 1 and 2 if it is split and the splitting point corrsponds to a flat of rank 2.

  Vector <Vector <int> > combinatorics((N_Nodes+N_Edges), Vector<int>(3,0));
  for (int i=0; i<N_Nodes; ++i) {
    combinatorics[i][0]=i;
    combinatorics[i][1]=0;
    combinatorics[i][2]=0;
  }
  Array<Set<int>> Edges=graph_from_edges.call_method("EDGES");
  for (int i=0; i<N_Edges; ++i) {
    int j=0;
    for (auto it=entire(Edges[i]); !it.at_end(); ++it) {
      combinatorics[(N_Nodes+i)][j]=*it;
      ++j;
    }
    combinatorics[(N_Nodes+i)][2]=0;
  }

  // Tries to distribute the flats in the combinatorial graph such that they fullfill the flat axioms and the corrisponding matroid has a matroid fan Z-isomorphic to the input fan
  bool stop=false; // stop=true when a matroid was found such that the corresponding matroid fan is Z-isomorphic to the input fan
  int k=0; // k indicates the row of the matrix combinatorics that is analised
  while (!stop) {
    if (k==-1) { //the algorithm has checked all possibilities
      stop=true;
    } else if (k==N_Nodes+N_Edges) { //the algorithem has found a possible distribution of flats
      // Check if it has the right number of rank 1 flats, since the fan is full dimensional there must be Fan_Ambient_Dimension+1 rank 1 flats
      int n=0;
      for (int i=0; i<N_Nodes+N_Edges; ++i) {
        if (combinatorics[i][2]==1){
          ++n;
        }
      }
      if (n==(Fan_Ambient_Dimension+1)) {
        // The functions checks if the combinatorics found corresponds to the flat combinatorics of a matroid. If it corresponds to a matroid and there is a Z-isomorphisem that maps the bergman fan of the matroid to the given one then the functions returns R.is_smooth=1, R.matroid the matroid, R.map the Z-isomorphisem. Otherways R.is_smooth=0 is returned
        R=is_combinatorial_right(f, combinatorics, N_Nodes, N_Edges, Fan_Ambient_Dimension);
        if (R.is_smooth==1) {
          return R;
        }
      }
      --k;
    } else {
      if (k<N_Nodes) { // A node can only be a rank 1 or rank 2 flat
        if (combinatorics[k][2]<=1) {
          combinatorics[k][2]=combinatorics[k][2]+1;
          ++k;
        } else {
          combinatorics[k][2]=0;
          --k;
        }
      } else {
        // An edge can be split or not, if it is split the splitting point can be a flat of rank 1 or 2
        if (combinatorics[k][2]<=2) {
          // Let node 1 and node 2 be the nodes connected by the edge. If the edge is not split one node must be correspond to a rank 1 flat, the other to a rank 2 flat, i.e rank1 --- rank2 or rank 2 ---- rank 1. If the edge is split only the possibilities rank 1 --- rank 2 ----rank1 and rank 2 ---- rank 1 ---- rank 2 are possible
          if ((combinatorics[k][2]+1==1 && combinatorics[combinatorics[k][0]][2]==2 && combinatorics[combinatorics[k][1]][2]==2)
              || (combinatorics[k][2]+1==2 && combinatorics[combinatorics[k][0]][2]==1 && combinatorics[combinatorics[k][1]][2]==1
                  || (combinatorics[k][2]+1==3 && ((combinatorics[combinatorics[k][0]][2]==1 && combinatorics[combinatorics[k][1]][2]==2) || (combinatorics[combinatorics[k][0]][2]==2 && combinatorics[combinatorics[k][1]][2]==1))))) {
            combinatorics[k][2]=combinatorics[k][2]+1;
            ++k;
          } else {
            combinatorics[k][2]=combinatorics[k][2]+1;
          }
        } else {
          combinatorics[k][2]=0;
          --k;
        }
      }
    }
  }
  return R;
}

result is_combinatorial_right(perl::Object f, Vector<Vector<int>> combinatorics, int N_Nodes, int N_Edges, int Fan_Ambient_Dimension)
{
  result R;
  R.is_smooth=0;

  // Construct the flats using th distribution of the flats in the combinatorial graph
  Set<int> empty;
  Vector<Set<int>> Flats(N_Nodes+N_Edges, empty);

  for (int i = 0, n = 0;  i < N_Nodes + N_Edges; ++i) {
    if (combinatorics[i][2]==1) {
      // Insert succsessivly the numbers 1,...,Fan_Ambient_Dimension+1 into the rank 1 flats
      Flats[i].insert(n);
      // Reconstruct the rank 2 flats using the rank 1 flats
      if (i<N_Nodes) {
        for (int j=N_Nodes; j<N_Nodes+N_Edges; ++j) {
          if (combinatorics[j][2]==2 && (combinatorics[j][0]==combinatorics[i][0] || combinatorics[j][1]==combinatorics[i][0])) {
            Flats[j].insert(n);
          }
          if (combinatorics[j][2]==3 && combinatorics[j][0]==combinatorics[i][0]) {
            Flats[combinatorics[j][1]].insert(n);
          }
          if (combinatorics[j][2]==3 && combinatorics[j][1]==combinatorics[i][0]) {
            Flats[combinatorics[j][0]].insert(n);
          }
        }
      } else {
        for (int j=0; j<N_Nodes; ++j) {
          if (combinatorics[j][2]==2 && (combinatorics[j][0]==combinatorics[i][0] || combinatorics[j][0]==combinatorics[i][1])) {
            Flats[j].insert(n);
          }
        }
      }
      ++n;
    }
  }

  // Check that all rank 2 flats are different
  bool rank2_different=true;
  for (int i=0; i<N_Nodes+N_Edges; ++i) {
    if (combinatorics[i][2]==2) {
      for (int j=0; j<N_Nodes+N_Edges; ++j) {
        if (i!=j && combinatorics[j][2]==2 && Flats[i]==Flats[j]) {
          rank2_different=false;
        }
      }
    }
  }

  if (rank2_different==true) {
    // Check if the flats fullfill the flat axioms
    // The functions returns true if the flats fullfill the flat axioms, false otherwise
    if (check_matroid(combinatorics, Flats, Fan_Ambient_Dimension)) {
      // Check if the fan corresponding to the matroid above is z-isomorphic to the given one
      // Construct the Rays corresponding to the Flats
      Matrix<Integer> Rays_Matroid_Fan(Fan_Ambient_Dimension,N_Nodes);
      for (int j=0; j<N_Nodes; ++j) {
        for (int i=0; i<Fan_Ambient_Dimension; ++i) {
          if (Flats[j].contains(i)) {
            Rays_Matroid_Fan[i][j]=1;
          }
        }
        if (Flats[j].contains(Fan_Ambient_Dimension)) {
          for (int k=0; k<Fan_Ambient_Dimension; ++k) {
            Rays_Matroid_Fan[k][j]=Rays_Matroid_Fan[k][j]-1;
          }
        }
      }

      // Construct a map that maps the rays of the matroid fan to the rays of the input fan
      // Extract some of the vectors of the matroid fan such that they are a basis of the hole space and write them into the matrix U
      Set<int> Basis = basis_cols(Rays_Matroid_Fan);

      auto it=entire(Basis);
      Matrix<Integer> U(Basis.size(), Basis.size());
      for (int j=0; j<Fan_Ambient_Dimension; ++j, ++it) {
        for (int i=0; i<Fan_Ambient_Dimension; ++i) {
          U[i][j]=Rays_Matroid_Fan[i][*it];
        }
      }

      U=inv(U);

      // Take the rays of the input fan and compute the corresponding primitive vactors
      Matrix<Rational> Rays_Fan=f.give("VERTICES");
      Rays_Fan = reduce_rays(Rays_Fan);
      Rays_Fan=T(Rays_Fan);

      for (int j=0; j<Rays_Fan.cols(); ++j) {
        Integer LCM_Denominator=1;
        for (int i=0; i<Rays_Fan.rows(); ++i) {
          Integer Denominator=denominator(Rays_Fan[i][j]);
          LCM_Denominator=lcm(LCM_Denominator,Denominator);
        }
        for (int i=0; i<Rays_Fan.rows(); ++i) {
          Rays_Fan[i][j]=Rays_Fan[i][j]*LCM_Denominator;
        }
        Integer GCD_Numerator=1;
        for (int i=0; i<Rays_Fan.rows(); ++i) {
          GCD_Numerator=gcd(numerator(Rays_Fan[i][j]),GCD_Numerator);
        }
        for (int i=0; i<Rays_Fan.rows(); ++i) {
          Rays_Fan[i][j]=Rays_Fan[i][j]/GCD_Numerator;
        }
      }

      // Take the rays of the input fan that correspond to the rays of the matroid fan extracted above and write them into the matrix V
      it=entire(Basis);
      Matrix<Integer> V(Basis.size(), Basis.size());
      for (int j=0; j<Fan_Ambient_Dimension; ++j, ++it) {
        for (int i=0; i<Fan_Ambient_Dimension; ++i) {
          V[i][j]=Rays_Fan[i][*it];
        }
      }

      Matrix<Integer> A=V*U; // searched map

      // Check if the map is a Z-isomorphisem and maps all rays of the matroid fan to the input fan
      if ((det(A)==1 || det(A)==-1) && Rays_Fan==A*Rays_Matroid_Fan) {

        // Compute the Basis of the Matroid with the Flats found above
        Vector<Set<int>> Bases;
        int n=0;

        for (int i=0; i<Fan_Ambient_Dimension+1; ++i) {
          for (int j=i+1; j<Fan_Ambient_Dimension+1; ++j) {
            for (int k=j+1; k<Fan_Ambient_Dimension+1; ++k) {
              bool possible=true;
              Set<int> Possible_Base{i, j, k};
              for (int l=0; l<N_Nodes+N_Edges; ++l) {
                if (combinatorics[l][2]==2 && (Flats[l]*Possible_Base).size()==Possible_Base.size()) {
                  possible=false;
                }
              }
              if (possible) {
                Bases.resize(n+1);
                Bases[n]=Possible_Base;
                ++n;
              }
            }
          }
        }

        perl::Object M("matroid::Matroid");
        M.take("BASES") << Bases;
        M.take("N_ELEMENTS") << Fan_Ambient_Dimension+1;

        R.is_smooth=1;
        R.matroid=M;
        R.map=convert_to<int>(inv(A));
      }
    }
  }
  return R;
}

bool check_matroid(Vector<Vector<int>>& combinatorics,  Vector<Set<int>>& Flats, int Fan_Ambient_Dimension)
{
  // The second property of flats is checked
  for (int i=0; i<combinatorics.size(); ++i) {
    if (combinatorics[i][2]==2) {
      for (int j=0; j<combinatorics.size(); ++j) {
        if (i!=j and combinatorics[j][2]==2) {
          if ((Flats[i]*Flats[j]).size()>1) {
            return false;
          }
        }
      }
    }
  }

  // The third property of flats is checked
  for (int n=0; n<=Fan_Ambient_Dimension; ++n) {
    // Search flats containing n
    Vector<Set<int>> Flats_containing_n; // and in reality setminus n
    for (int i=0; i<combinatorics.size(); ++i) {
      if (combinatorics[i][2]==2 && Flats[i].contains(n)) {
        Flats_containing_n |= Flats[i]-n;
      }
    }

    // Check that they are disjoint
    for (int i=0; i<Flats_containing_n.size(); ++i) {
      for (int j=i+1; j<Flats_containing_n.size(); ++j) {
        if ((Flats_containing_n[i]*Flats_containing_n[j]).size()!=0) {
          return false;
        }
      }
    }

    // Check that the union is the complement of {n}
    Set<int> Union;
    for (int i=0; i<Flats_containing_n.size(); ++i) {
      Union += Flats_containing_n[i];
    }
    if (Union.size()!=Fan_Ambient_Dimension) {
      return false;
    }
  }

  return true;
}

result smooth_codim1(perl::Object f)
{
  result R;
  Matrix<Rational> Rays_Rational=f.give("VERTICES");
  Rays_Rational = reduce_rays(Rays_Rational);
  // FIXME: reuse lattice_tools
  // Make the Rays integers
  Matrix<Integer> Rays(Rays_Rational.rows(), Rays_Rational.cols());
  for (int i=0; i<Rays.rows(); ++i) {
    Integer LCM_Denominator=1;
    for (int j=0; j<Rays.cols(); ++j) {
      Integer Denominator=denominator(Rays_Rational[i][j]);
      LCM_Denominator=lcm(LCM_Denominator,Denominator);
    }
    for (int j=0; j<Rays.cols(); ++j) {
      Rays_Rational[i][j] *= LCM_Denominator;
    }
    Integer GCD_Numerator=1;
    for (int j=0; j<Rays.cols(); ++j) {
      GCD_Numerator=gcd(numerator(Rays_Rational[i][j]),GCD_Numerator);
    }
    for (int j=0; j<Rays.cols(); ++j) {
      Rays[i][j]=Rays_Rational[i][j]/GCD_Numerator;
    }
  }

  Rays=T(Rays);
  Rays.resize(Rays.rows(), Rays.cols()-1);

  // Compute the Hermit normal form
  HermiteNormalForm<Integer> hnfresult = hermite_normal_form(Rays);
  Matrix<Integer> tfmatrix(hnfresult.companion);

  // Compute the determinant of HNF
  Integer determinant=det(hnfresult.hnf);

  if (determinant==1 || determinant==-1) {
    Set<int> far_face = f.give("FAR_VERTICES");
    IncidenceMatrix<> MaximalCones = f.give("MAXIMAL_POLYTOPES");
    MaximalCones = MaximalCones.minor(All,far_face);
    int N_Maximal_Cones_Real=MaximalCones.rows();
    int Fan_Ambient_Dimension=f.give("PROJECTIVE_AMBIENT_DIM");
    int N_Maximal_Cones_Expected=(Fan_Ambient_Dimension*(Fan_Ambient_Dimension+1))/2;
    if (N_Maximal_Cones_Real==N_Maximal_Cones_Expected) {
      Array<Set<int>> Bases(Rays.cols()+1);
      const auto E = range(0, Rays.cols());
      for (int i=0; i<=Rays.cols(); ++i) {
        Bases[i]=E-i;
      }
      perl::Object M("matroid::Matroid");
      M.take("BASES") << Bases;
      M.take("N_ELEMENTS") << Rays.cols()+1;

      R.is_smooth=1;
      R.matroid=M;
      R.map=convert_to<int>(tfmatrix);
    } else {
      R.is_smooth=0;
    }
  } else {
    R.is_smooth=0;
  }
  return R;
}

UserFunctionTemplate4perl("# @category Matroids"
                          "#Takes a weighted fan and returns if it is smooth "
                          "# (i.e. isomorphic to a Bergman fan B(M)/L for some matroid M) or not. "
                          "# The algorithm works for fans of dimension 0,1,2 and "
                          "# codimension 0,1! For other dimensions the algorithm "
                          "# could give an answer but it is not guaranteed. "
                          "# @param Cycle<Addition> a tropical fan F"
                          "# @return List( Int s, Matroid M, Morphism<Addition> A ). If s=1 then F is smooth, the "
                          "# corresponding matroid fan is Z-isomorphic to the matroid fan "
                          "# associated to M. The Z-isomorphism is given by A, i.e."
                          "# B(M)/L = affine_transform(F,A)"
                          "# If s=0, F is not smooth. If s=2 the algorithm is not able to determine "
                          "# if F is smooth or not. ",
                          "is_smooth<Addition>(Cycle<Addition>)");
} }
