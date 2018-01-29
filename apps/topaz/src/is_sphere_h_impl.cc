/* Copyright (c) 1997-2018
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

#include "polymake/topaz/is_sphere_h.h"
#include "polymake/topaz/random_discrete_morse.h"

namespace polymake { namespace topaz {

typedef Integer coefficient_type;
typedef SimplicialComplex_as_FaceMap<int> base_complex_type;

// checks whether the homology is the same as for a sphere of the same dimension
// HD: Hasse diagram of pure simplicial complex
bool is_homology_sphere(const Lattice<BasicDecoration>& HD)
{
   const int dim = HD.rank()-2;

   const SimplicialComplex_as_FaceMap<int> SC(
         attach_member_accessor( select(HD.decoration(), HD.nodes_of_rank(HD.rank()-1)),
            ptr2type< BasicDecoration, Set<int>, &BasicDecoration::face>()
            ));
   Complex_iterator<coefficient_type,SparseMatrix<coefficient_type>,base_complex_type,false,false> h_it(SC,dim,0);
   if (h_it->betti_number!=1 || h_it->torsion.size()!=0)
      return false; // top rank homology free of rank 1
   for (++h_it; !h_it.at_end(); ++h_it) {
      if (h_it->betti_number !=0 || h_it->torsion.size() !=0)
         return false; // low rank (reduced) homology trivial
   }

   return true; // passed all tests
}

// implementation of sphere recognition heuristics algorithm described in Sphere Recognition: Heuristics and Examples by Joswig, Lutz, Tsuruga; arxiv 1405.3848
// HD: Hasse diagram of pure simplicial complex
// return values: 1=true, 0=false, -1=undef
int is_sphere_h(const Lattice<BasicDecoration>& HD, const pm::SharedRandomState& random_source, int strategy, const int n_stable_rounds)
{
   const int dim = HD.rank()-2;

   Array<int> sph(dim);
   sph[0]=sph[dim-1]=1;

   Map<Array<int>, int> M = random_discrete_morse(HD, UniformlyRandom<long>(random_source), strategy, 0, n_stable_rounds, sph, Array<int>(), "");
   if(M[sph]) return 1; // found spherical acyclic matching

   if (!is_homology_sphere(HD)) return 0; // check first if homology fits

   // strategic options
   int max_relax=0, heating=0, preheat=0;

   if (strategy == 0) {
      max_relax= 70;
      heating= 30;
      preheat= 30;
   }

   if (strategy == 1) {
      max_relax= 120;
      heating= 50;
      preheat= 70;
   }

   BistellarComplex BC(HD,random_source,false,true);

   int stable_rounds = 0;
   int relax = 0;
   int neg_relax = 0;
   int full_dim_move = preheat;
   int chilly_moves = 0;
   int up_moves = 0;
   int warm_moves = preheat;
   int min_facets = BC.n_facets();

   for ( ; stable_rounds<n_stable_rounds; ++stable_rounds) {
      //    cerr << "stable_rounds: " << stable_rounds << "     min_facets: " << min_facets << endl;

      const int facets = BC.n_facets();
      if (facets < min_facets) {
         min_facets = facets;
         stable_rounds = 0;
      }

      if (facets == dim+2)  // is sphere
         return 1;

      if (relax < neg_relax)
         relax = 0;

      if (relax > max_relax) {  // heating up
         relax = 0;
         up_moves = heating;
         warm_moves = heating;
         if (strategy == 1)
            chilly_moves = heating;
      }

      if (full_dim_move > 0) {
         --full_dim_move;
         BC.zero_move();
         continue;
      }

      if (chilly_moves > 0) {
         --chilly_moves;
         BC.min_rev_move(dim-1);  // very hot move indeed
         continue;
      }

      if (up_moves > 0) {
         --up_moves;
         BC.min_rev_move(dim/2 + 1);  // up move
         continue;
      }

      if (warm_moves > 0) {
         --warm_moves;
         BC.min_rev_move(dim/2);  // up or eaven move
         continue;
      }

      // make smallest reversed move
      const int move = BC.min_rev_move();

      if ( move >= (dim+1)/2 && move != 0 ) {  // up or eaven move
         neg_relax = 0;
         ++relax;
      }

      else
         ++neg_relax;
   }

   return -1;
}

// return values: 1=true, 0=false, -1=undef
int is_ball_or_sphere_h(const Lattice<BasicDecoration>& HD, const pm::SharedRandomState& random_source, const int strategy, const int n_stable_rounds)
{
  const Boundary_of_PseudoManifold B=boundary_of_pseudo_manifold(HD);
  if (B.empty())
    return is_sphere_h(HD, random_source, strategy, n_stable_rounds);

  // compute C + cone(bound(C))
  std::list< Set<int> > S;
  int v=0;

  for (auto f=entire(HD.nodes_of_rank(HD.rank()-2)); !f.at_end(); ++f) {
    S.push_back(HD.face(*f));
    const int w=HD.face(*f).back();
    if (w>=v)
      v=w+1;
  }

  for (Entire<Boundary_of_PseudoManifold>::const_iterator b=entire(B); !b.at_end(); ++b)
    S.push_back(b->face+v);

  return is_sphere_h(S, random_source, strategy, n_stable_rounds);
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
