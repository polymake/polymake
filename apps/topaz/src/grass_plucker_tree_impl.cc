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

#include "polymake/topaz/grass_plucker.h"
#include <list>

namespace polymake { namespace topaz {

namespace gp {      

   
void
GP_Tree::incorporate_nodes(const GP_Tree& other,
                           const Sush common_sush,
                           const PhiOrCubeIndex this_phi,
                           const PhiOrCubeIndex other_phi) {
   nodes_.reserve(nodes_.size() + other.nodes_.size());
   for (const auto& n: other.nodes_) {
      nodes_.push_back(n);
      node_support_ += n.self;
      if (n.self == other_phi)
         nodes_.back().upstream.push_back(std::make_pair(this_phi, common_sush));
   }
}

   
void
GP_Tree::remove_one_sush(const PhiOrCubeIndex phi,
                         const Sush sush) {
   auto& the_sushes(hungry_sushes_at_[phi]);
   
   // these vectors are small, so little time will be spent searching and erasing      
   auto it(std::find(the_sushes.begin(), the_sushes.end(), sush));
   assert(it != the_sushes.end());

   the_sushes.erase(it);
}

void   
clean_hungry_sushes_at(HungrySushesAt& hungry_sushes_at)
{
   std::vector<PhiOrCubeIndex> to_erase;
   for (const auto& phi_sushes: hungry_sushes_at)
      if (!phi_sushes.second.size())
         to_erase.push_back(phi_sushes.first);

   for (const PhiOrCubeIndex phi: to_erase) {
      const auto it = hungry_sushes_at.find(phi);
      assert (it != hungry_sushes_at.end());
      hungry_sushes_at.erase(it);
   }
}
   
void
GP_Tree::remove_sush(const Sush sush)
{
   for (const auto& phi_sushes: hungry_sushes_at_) 
      if (std::find(phi_sushes.second.begin(), phi_sushes.second.end(), sush)
          != phi_sushes.second.end()) 
         remove_one_sush(phi_sushes.first, sush);

   clean_hungry_sushes_at(hungry_sushes_at_);
         
   auto it = std::find(sush_vector_.begin(), sush_vector_.end(), sush);
   if (it != sush_vector_.end())
      sush_vector_.erase(it);

   sushes_set_ -= sush;
}
   
void
GP_Tree::complete_coupling(const GP_Tree& other,
                           const Sush common_sush,
                           const PhiOrCubeIndex this_phi,
                           const PhiOrCubeIndex other_phi) {
   // the common_sush is on this tree, and -common_sush on the other

   incorporate_nodes(other, common_sush, this_phi, other_phi);

   for (const auto& other_phi_sushes: other.hungry_sushes_at()) 
      if (other_phi_sushes.second.size())
         hungry_sushes_at_[other_phi_sushes.first].insert(hungry_sushes_at_[other_phi_sushes.first].end(),
                                                          other_phi_sushes.second.begin(),
                                                          other_phi_sushes.second.end());

   if (-1 != common_sush) 
      remove_sush_from_hungry_sushes_at(this_phi, other_phi, common_sush);
      
   SushVector combined_sush_vector;
   combined_sush_vector.reserve(sush_vector_.size() + other.sush_vector().size() - 2);
   hash_set<Sush> combined_sushes_set;

   for (const Sush sush: sush_vector_)
      if (common_sush != sush) {
         combined_sush_vector.push_back(sush);
         combined_sushes_set += sush;
      }

   for (const Sush sush: other.sush_vector())
      if (-common_sush != sush) {
         combined_sush_vector.push_back(sush);
         combined_sushes_set += sush;
      }
      
   sush_vector_ = combined_sush_vector;
   sushes_set_ = combined_sushes_set;
}

// fake tree node inside a cube
GP_Tree::GP_Tree(const VertexId vertex_id,
                 const CubeIndex cube_id)
   : index_(TreeIndex(vertex_id.get()))
{
   nodes_.emplace_back(PhiOrCubeIndex(vertex_id.get()));
   node_support_ += PhiOrCubeIndex(vertex_id.get());
}
   
void
GP_Tree::flat_insert_from(const GP_Tree& other,
                          const SushVector& signing) {
   incorporate_nodes(other, Sush(0), nodes_.front().self, other.nodes_.front().self);
   for (const auto& phi_sushes: other.hungry_sushes_at()) 
      hungry_sushes_at_.insert(phi_sushes);


   for (const Sush sush: other.sushes_set())
      sushes_set_ += sush;

   sush_vector_ = SushVector(sushes_set_.begin(), sushes_set_.end());
   std::sort(sush_vector_.begin(), sush_vector_.end());
   
   for (const Sush sush: signing) 
      remove_sush(sush);
}

bool is_cube_vertex(const GP_Tree_Node& node)
{
   return abs(node.self.get()) < (Int(1) << max_n_vertices);
}

const VertexId
GP_Tree::cube_vertex_upstream_of(const PhiOrCubeIndex phi) const {
   std::list<PhiOrCubeIndex> queue;
   queue.push_back(phi);
   while (queue.size()) {
      const PhiOrCubeIndex current_phi = queue.front(); queue.pop_front();
      auto it = std::find_if(nodes_.begin(), nodes_.end(),
                          [current_phi](const GP_Tree_Node& n) {
                             return n.self == current_phi;
                          });
      assert(it != nodes_.end());
      if (is_cube_vertex(it->self))
         return VertexId(it->self.get() % MAGIC_VERTEX_MULTIPLE);
      
      for (const TreeConnector& tc: it->upstream)
         queue.push_back(tc.first);
   }
   return VertexId(-1);
}

CubeIndex
cube_id_of_vertex_id(const VertexId vid,
                     const NodeContainer& nodes)
{
   auto it = std::find_if(nodes.begin(), nodes.end(),
                           [vid](const GP_Tree_Node&n) {
                              return n.self == vid;
                           });
   assert(it != nodes.end());
   assert(1 == it->upstream.size());
   return CubeIndex(it->upstream.front().first.get());
}

void
GP_Tree::add_tree(const GP_Tree& other,
                  const Sush common_sush,
                  SearchData& sd,
                  const IntParams& ip) {
   assert(nodes_.size() >= 1);

   PhiOrCubeIndex this_phi, other_phi;
   std::tie(this_phi, other_phi) = common_phis(other, common_sush);
   assert (-1 != this_phi);

   /*
   const VertexId vid(this->cube_vertex_upstream_of(this_phi));
   if (VertexId(-1) != vid) {
      const CubeIndex cid(cube_id_of_vertex_id(vid, this->nodes_));
      this->prune_from(cid, vid, this_phi, sd, ip);
   }
   */
   
   complete_coupling(other, common_sush, this_phi, other_phi);
}

   

} // end namespace gp
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
