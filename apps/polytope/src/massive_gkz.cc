#ifndef POLYMAKE_SKELETON_LATTICE_H
#define POLYMAKE_SKELETON_LATTICE_H

#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/integer_linalg.h"
#include "polymake/Array.h"

#include "polymake/Set.h"
#include "polymake/list"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/lattice_builder.h"
#include "polymake/graph/LatticePermutation.h"
#include "polymake/topaz/hasse_diagram.h"
#include "polymake/IncidenceMatrix.h"



namespace polymake { 
namespace polytope { 
namespace lattice {


   using graph::Lattice;
   using graph::lattice::BasicDecoration;

// this class is for constructing a certain sublattice of the full lattice of the simplicial cmplex
class SkeletonCut {
   const IncidenceMatrix<>& maximal_faces;
   Lattice<BasicDecoration> boundary_lattice;

public:
   SkeletonCut(const IncidenceMatrix<>& mfaces) : maximal_faces(mfaces) {
      const int total = maximal_faces.cols();
      graph::lattice::BasicClosureOperator<> cop(total, maximal_faces);
      graph::lattice::TrivialCut<BasicDecoration> cut;
      graph::lattice::BasicDecorator<> dec(0, Set<int>());

      Lattice<BasicDecoration> init_lattice;
      boundary_lattice = Lattice<BasicDecoration>(graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
         cop, cut, dec, 0, graph::lattice_builder::Primal(), init_lattice));
   }

   inline bool operator()(const graph::lattice::BasicDecoration& data) const {
      for (auto f : boundary_lattice.nodes_of_rank(data.rank))
         if (incl(data.face,boundary_lattice.face(f)) <= 0)
            return true;
      return false;
   }
};


// INPUT: facets are maximal simplices of the triangulation, boundary_faces are the points in the boundary, sorted by facets
// OUTPUT: the lattice of massive faces (a k-face of the triangulation is "massive" if it is contained in a k-face of the convex hull of the points of the triangulation)
perl::Object skeleton_lattice(const IncidenceMatrix<>& facets, const IncidenceMatrix<>& boundary_faces)
{
   const int total = facets.rows();
   if (total == 0)
      throw std::runtime_error("skeleton_lattice: empty facets matrix not supported");
   const Set<int> artificial_set = scalar2set(-1);
   SkeletonCut cut(boundary_faces);
   topaz::SimplicialClosure<BasicDecoration> closure(facets);
   topaz::SimplicialDecorator decorator(facets[0].size()+1, artificial_set);

   Lattice<BasicDecoration> result(graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
            closure, cut, decorator, 0, graph::lattice_builder::Dual()));
   return static_cast<perl::Object>(result);
}



// the actual calculation of the massive GKZ vector
Vector<Integer> massive_gkz_vector(const Matrix<Integer>& points, const IncidenceMatrix<>& pts_in_facets, const IncidenceMatrix<>& max_cells, const int dim)
{
   const int n_pts = points.rows();
   Lattice<BasicDecoration> massive_lattice(skeleton_lattice(max_cells,pts_in_facets));
   
   Vector<Integer> gkz(n_pts);
   Array<Array<Set<int>>> massive_faces(dim+1);
   for(int i = 0; i < dim+1; ++i)
      {
         // we go through the lattice of massive faces and write those of dim = i in massive_faces[i]
         massive_faces[i].append(massive_lattice.nodes_of_rank(i+1).size(), entire(attach_member_accessor(select(massive_lattice.decoration(), massive_lattice.nodes_of_rank(i+1)), ptr2type<graph::lattice::BasicDecoration, Set<int>, &graph::lattice::BasicDecoration::face>())));
         
         // we calculate the signed lattice volume of the i-dim. massive faces and add it to the appropriate entries of gkz
         
         for(int j = 0; j < massive_faces[i].size(); ++j)
         {
            Matrix<Integer> face_vertices(points.minor(massive_faces[i][j],All));
            auto hnf = (hermite_normal_form(face_vertices)).hnf;
            Integer vol = det(hnf.minor(All,sequence(0,hnf.rows())));
            for(auto elem: massive_faces[i][j])
               {
                  gkz[elem] += pow((-1),dim-i)*vol;
               }
         }
      }
   return gkz;
}


} // namespace lattice


// INPUT: a PointConfiguration and a  TRIANGULATION of it as a subobject   
// OUTPUT: the massive GKZ vector as defined in GKZ book Chapter 11 

Vector<Integer> massive_gkz_vector(perl::Object point_config, perl::Object gsc, int dim)
{
   const Matrix<Integer> points = point_config.give("POINTS");
   const IncidenceMatrix<> pts_in_facets = point_config.give("CONVEX_HULL.POINTS_IN_FACETS");
   const IncidenceMatrix<> max_cells = gsc.give("FACETS");
   return lattice::massive_gkz_vector(points, pts_in_facets, max_cells,dim);
}

Function4perl(&massive_gkz_vector,"massive_gkz_vector(PointConfiguration,topaz::SimplicialComplex,Int)");


} // namespace polytope

} // namespace polymake

#endif
