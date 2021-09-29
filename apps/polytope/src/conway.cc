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
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/graph/conway_impl.h"

// The Conway polyhedron notation and its extension by Hart is explained here:
// https://en.wikipedia.org/wiki/Conway_polyhedron_notation

namespace polymake {
namespace polytope {

using DoublyConnectedEdgeList = graph::dcel::DoublyConnectedEdgeList;
using HalfEdge = graph::dcel::HalfEdge;
using Face = graph::dcel::Face;

BigObject dcel2polytope(const DoublyConnectedEdgeList& dcel, std::string description)
{
   const  Array<Array<Int>> VIF_CN_out=dcel.faces_as_cycles();
   BigObject p_out("Polytope");
   p_out.set_description() << description;
   IncidenceMatrix<> VIF_out(VIF_CN_out);
   p_out.take("VIF_CYCLIC_NORMAL") << VIF_CN_out;
   p_out.take("VERTICES_IN_FACETS") << VIF_out;
   p_out.take("COMBINATORIAL_DIM") << 3;
   return p_out;
}

BigObject conway_core(BigObject P, const std::string operations, const std::string out_name, const std::string error_prefix)
{
   const Int d = P.give("COMBINATORIAL_DIM");
   if (d!=3)
      throw std::runtime_error(error_prefix + ": only defined for 3-polytopes");

   const  Array<Array<Int>> VIF_CN = P.give("VIF_CYCLIC_NORMAL");
   DoublyConnectedEdgeList DCEL_in(VIF_CN);
   DoublyConnectedEdgeList DCEL_out = DCEL_in;
   for (auto it = operations.cbegin(); it != operations.cend(); ++it) {
      switch (*it) {
      case 'd': DCEL_out = DCEL_out.dual(); break;
      case 'a': DCEL_out = polymake::graph::conway_ambo_impl(DCEL_out); break;
      case 'k': DCEL_out = polymake::graph::conway_kis_impl(DCEL_out); break;
      case 's': DCEL_out = polymake::graph::conway_snub_impl(DCEL_out); break;
      case 'g': DCEL_out = polymake::graph::conway_snub_impl(DCEL_out.dual()).dual(); break;
      case 'n': DCEL_out = polymake::graph::conway_kis_impl(DCEL_out.dual()); break;
      case 'z': DCEL_out = polymake::graph::conway_kis_impl(DCEL_out).dual(); break;
      case 'j': DCEL_out = polymake::graph::conway_ambo_impl(DCEL_out).dual(); break;
      case 't': DCEL_out = polymake::graph::conway_kis_impl(DCEL_out.dual()).dual(); break;
      case 'e': DCEL_out = polymake::graph::conway_ambo_impl(polymake::graph::conway_ambo_impl(DCEL_out)); break;
      case 'o': DCEL_out = polymake::graph::conway_ambo_impl(polymake::graph::conway_ambo_impl(DCEL_out)).dual(); break;
      case 'm': DCEL_out = polymake::graph::conway_kis_impl(polymake::graph::conway_ambo_impl(DCEL_out).dual()); break;
      case 'b': DCEL_out = polymake::graph::conway_kis_impl(polymake::graph::conway_ambo_impl(DCEL_out).dual()).dual(); break;
      default: throw std::runtime_error("conway: operation undefined");
      }
   }
   return dcel2polytope(DCEL_out, out_name);
}


BigObject conway_ambo(BigObject p_in) {
   return conway_core(p_in, "a", "Ambo of " + std::string(p_in.description()), "ambo");
}


BigObject conway_dual(BigObject p_in){
   return conway_core(p_in, "d", "Dual of " + std::string(p_in.description()), "dual");
}


BigObject conway_gyro(BigObject p_in) {
   return conway_core(p_in, "g", "gyro of " + std::string(p_in.description()), "gyro");
}


BigObject conway_kis(BigObject p_in){
   return conway_core(p_in, "k", "Kis of " + std::string(p_in.description()), "kis");
}


BigObject conway_needle(BigObject p_in) {
   return conway_core(p_in, "n", "Needle of " + std::string(p_in.description()), "needle");
}


BigObject conway_propeller(BigObject p_in)
{
   const Int d = p_in.give("COMBINATORIAL_DIM");
   if (d!=3)
      throw std::runtime_error("propeller: only defined for 3-polytopes");

   const  Array<Array<Int>> VIF_CN = p_in.give("VIF_CYCLIC_NORMAL");
   DoublyConnectedEdgeList DCEL_in(VIF_CN);
   const Int n_in_facets = DCEL_in.getNumFaces();
   const Int n_in_vertices = DCEL_in.getNumVertices();
   const Int n_in_edges = DCEL_in.getNumHalfEdges()/2;
   // The faces of propellor correspond to the halfedfes and the faces of the input,
   // the vertices correspond to the vertices and haldedges.
   IncidenceMatrix<> Propeller(2*n_in_edges+n_in_facets, n_in_vertices+2*n_in_edges);
   Int new_facet_counter = 0;
   for (Int i = 0; i < DCEL_in.getNumHalfEdges(); i++) {
     // For each HalfEdge, write the new facet that contains the root node of the HalfEdge as vertex.
     const HalfEdge* current_halfedge = DCEL_in.getHalfEdge(i);
     Propeller(new_facet_counter,current_halfedge->getPrev()->getHead()->getID())=1;
     Propeller(new_facet_counter,n_in_vertices+current_halfedge->getID())=1;
     Propeller(new_facet_counter,n_in_vertices+current_halfedge->getPrev()->getID())=1;
     Propeller(new_facet_counter,n_in_vertices+current_halfedge->getPrev()->getTwin()->getID())=1;
     new_facet_counter++;
   }
   Array<Face> faces_in = DCEL_in.getFaces();
   for (Int i = 0; i < DCEL_in.getNumFaces(); i++) {
     // For each old face, write a new face using respective new vertives arising from HalfEdges
     Face face = faces_in[i];
     HalfEdge* he = face.getHalfEdge(); // This gets a random HalfEdge from the face.
     Int first_id = he->getID();
     Int next_id = he->getNext()->getID();
     while(first_id != next_id){
        Propeller(new_facet_counter,n_in_vertices+he->getID())=1;
        he = he->getNext();
        next_id = he->getID();
     }
     new_facet_counter++;
   }

   BigObject p_out("Polytope");
   p_out.set_description() << "Propeller of " << p_in.description();
   p_out.take("VERTICES_IN_FACETS") << Propeller;
   p_out.take("COMBINATORIAL_DIM") << 3;
   return p_out;
}


BigObject conway_seed()
{
   return dcel2polytope(graph::conway_seed_impl(), "Seed");
}


BigObject conway_snub(BigObject p_in) {
   return conway_core(p_in, "s", "Snub of " + std::string(p_in.description()), "snub");
}


BigObject conway(BigObject P, const std::string operations)
{
   return conway_core(P, operations, operations + " of " + std::string(P.description()), "conway");
}


UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces Ambo of a 3-polytope (Conway notation 'a')"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# @example "
                  "# >$c = cube(3);"
                  "# > $ac = conway_ambo($c);"
                  "# > print $ac->F_VECTOR;"
                  "# | 12 24 14",
                  &conway_ambo, "conway_ambo");


UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces dual of a 3-polytope (Conway notation 'd')"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# > $c = cube(3);"
                  "# > $dc = conway_dual($c);"
                  "# > print $dc->F_VECTOR;"
                  "# | 6 12 8",
                  &conway_dual, "conway_dual");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces Gyro of a 3-polytope (Conway notation 'g')"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# > $c = cube(3);"
                  "# > $gc = conway_gyro($c);"
                  "# > print $gc->F_VECTOR;"
                  "# | 38 60 24",
                  &conway_gyro, "conway_gyro");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces Kis of a 3-polytope (Conway notation 'k')"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# > $c = cube(3);"
                  "# > $kc = conway_kis($c);"
                  "# > print $kc->F_VECTOR;"
                  "# | 12 36 24",
                  &conway_kis, "conway_kis");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces Needle of a 3-polytope (Conway notation 'n')"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# > $c = cube(3);"
                  "# > $nc = conway_needle($c);"
                  "# > print $nc->F_VECTOR;"
                  "# | 14 36 24",
                  &conway_needle, "conway_needle");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces Propeller of a 3-polytope (Conway notation 'p')"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# > $c = cube(3);"
                  "# > $pc = conway_propeller($c);"
                  "# > print $pc->F_VECTOR;"
                  "# | 32 60 30",
                  &conway_propeller, "conway_propeller");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces Conway Seed (3-cube) (Conway notation 'S')"
                  "# @return Polytope"
                  "# > $c = conway_seed();"
                  "# > print $c->F_VECTOR;"
                  "# | 8 12 6",
                  &conway_seed, "conway_seed");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces Snub of a 3-polytope (Conway notation 's')"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# @example"
                  "# > $c = cube(3);"
                  "# > $sc = conway_snub($c);"
                  "# > print $sc->F_VECTOR;"
                  "# | 24 60 38",
                  &conway_snub, "conway_snub");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Applies a sequence of Conway operations to the polytope //P// (from left to right)"
                  "# @param Polytope P"
                  "# @param String operations"
                  "# 'd': conway operation dual"
                  "# 'a': conway operation ambo"
                  "# 'k': conway operation kis"
                  "# 's': conway operation snub"
                  "# 'g': conway operation gyro"
                  "# 'n': conway operation needle"
                  "# 'z': conway operation zip"
                  "# 'j': conway operation join"
                  "# 't': conway operation truncate"
                  "# 'e': conway operation expand"
                  "# 'o': conway operation ortho"
                  "# 'm': conway operation meta"
                  "# 'b': conway operation bevel"
                  "# @return Polytope"
                  "# @example"
                  "# > $s = simplex(3);"
                  "# > $as = conway($s, \"a\");"
                  "# > print isomorphic(octahedron(),$as);"
                  "# | true"
                  "# > $ss = conway($s, \"s\");"
                  "# > print isomorphic(icosahedron(),$ss);"
                  "# | true "
                  "# > $mjzkab_s = conway($s, \"mjzkab\");"
                  "# > print $mjzkab_s->F_VECTOR;"
                  "# | 5184 7776 2594",
                  &conway, "conway");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
