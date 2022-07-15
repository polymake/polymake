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
//#include "polymake/topaz/grass_plucker_cube.h"
#include <list>

namespace polymake { namespace topaz {

namespace gp {    

#if POLYMAKE_DEBUG
   CanonicalSolidMemoizer* global_csm;   // for debug printing
#endif

class ComputationTooLong {};
   
void
complete_tree_with_leaves(SearchData& sd,
                          GP_Tree& t,
                          const IntParams& ip)
{
   // make a copy of the current_sushes because t will be modified.
   // In fact, if t is a cube tree, it may lose any number
   // of sushes while being completed, so we need to be
   // very ocd about this
   SushVector current_sushes_to_complete(t.sush_vector());
   while (current_sushes_to_complete.size()) {
      const Sush
         sush(current_sushes_to_complete.back()),
         neg_sush(-sush);
      current_sushes_to_complete.pop_back();
      if (sd.leaf_of_sush.exists(neg_sush))
         t.add_tree(sd.tree_list[sd.leaf_of_sush[neg_sush]], sush, sd, ip);
   }
}                          

bool
trees_intersect(const GP_Tree& small,
                const GP_Tree& large)
{
   for (const PhiOrCubeIndex phi: small.node_support())
      if (large.node_support().contains(phi) ||
          large.node_support().contains(PhiOrCubeIndex(-phi)))
         return true;
   return false;
}

bool
more_than_one_sush_in_common(const GP_Tree& small,
                             const GP_Tree& large)
{
   Int n_common_sushes(0);
   for (const Sush sush: small.sush_vector())
      if (large.sushes_set().contains(sush) ||
          large.sushes_set().contains(Sush(-sush))) {
         ++n_common_sushes;
         if (2 <= n_common_sushes)
            return true;
      }
   return false;
}
   
bool
compatible_along_sush(const GP_Tree& t1,
                      const GP_Tree& t2,
                      const Sush sush)
{
   if ( t1.phi_containing_hungry_sush(sush) ==
       -t2.phi_containing_hungry_sush(Sush(-sush)))
      return false;
   
   if (t1.size() <= t2.size()
       ? trees_intersect(t1, t2)
       : trees_intersect(t2, t1))
      return false;

   return (t1.sush_vector().size() <=
           t2.sush_vector().size())
      ? !more_than_one_sush_in_common(t1, t2)
      : !more_than_one_sush_in_common(t2, t1);
           
}
   
/*
 * @throws ComputationTooLong
 */
SolutionStatus
add_tree_to_existing_trees(SearchData& sd,
                           const IntParams& ip,
                           const GP_Tree& t)
{
   for (const auto& sush: t.sush_vector()) {
      if (!sd.nonleaf_trees_of_sush.exists(Sush(-sush)))
         continue;
      for (const auto& ti: sd.nonleaf_trees_of_sush[Sush(-sush)]) {
         if (!compatible_along_sush(t, sd.tree_list[ti], sush))
            continue;
         GP_Tree combined_tree(t);
         combined_tree.add_tree(sd.tree_list[ti], sush, sd, ip);
         if (0 == combined_tree.sush_vector().size()) {
            const SolutionStatus ss = process_tree(sd, ip, combined_tree,
                                                   TreeAddingAction::dont_add_to_existing,
                                                   TreeCompletingAction::complete);
            assert(SolutionStatus::found_tree == ss);
            return ss;
         }
         if (sd.seen_tree_sushes.find(combined_tree.sush_vector()) !=
             sd.seen_tree_sushes.end())
            continue;
         assert(combined_tree.sush_vector().size() == t.sush_vector().size() + sd.tree_list[ti].sush_vector().size() - 2);
         if (SolutionStatus::found_tree == process_tree(sd, ip, combined_tree,
                                                        TreeAddingAction::dont_add_to_existing,
                                                        TreeCompletingAction::complete))
            return SolutionStatus::found_tree;
      }
   }
   return SolutionStatus::not_found;
}                           

Map<Int,Int>
tree_stats(const SearchData& sd)
{
   Map<Int,Int> of_size;
   for (const auto& t: sd.tree_list)
      of_size[t.size()]++;
   return of_size;
}

/*
 * @throws ComputationTooLong
 */
SolutionStatus
process_tree(SearchData& sd,
             const IntParams& ip,
             GP_Tree& t,
             const TreeAddingAction taa,
             const TreeCompletingAction tca)
{
   if (TreeCompletingAction::complete == tca)
      complete_tree_with_leaves(sd, t, ip);

   if (0 == t.sush_vector().size()) {
      t.set_index(sd.next_tree_index);
      sd.tree_list.push_back(t);
      ++sd.next_tree_index;
      return SolutionStatus::found_tree;
   }

   if (Int(sd.tree_list.size()) >= ip.abort_after)
      throw ComputationTooLong();
   
   if (sd.seen_tree_sushes.find(t.sush_vector()) !=
       sd.seen_tree_sushes.end())
      return SolutionStatus::not_found;

   t.set_index(sd.next_tree_index);
   sd.tree_list.push_back(t);
   ++sd.next_tree_index;
   sd.seen_tree_sushes.insert(t.sush_vector());

   if (ip.verbosity &&
       0 == sd.tree_list.size() % ip.tree_log_interval)
      cerr << sd.id_string
           << ": tree sizes(" << sd.tree_list.size() << "): " << tree_stats(sd) << endl;
   
   if (1 == t.sush_vector().size()) {
      if (!sd.leaf_of_sush.exists(t.sush_vector().front()))
         sd.leaf_of_sush[t.sush_vector().front()] = t.index();
   } else {
      for (const auto& sush: t.sush_vector())
         sd.nonleaf_trees_of_sush[sush].push_back(t.index());
   }

   if (TreeAddingAction::add_to_existing == taa &&
       SolutionStatus::found_tree == add_tree_to_existing_trees(sd, ip, t))
      return SolutionStatus::found_tree;

   return SolutionStatus::not_found;
}


std::list<Sush>
sush_queue_from_pr_list(const PluckerData& pd,
                        hash_set<Sush>& sushes_not_to_add)
{
   std::list<Sush> sush_queue;
   for (const auto& pr: pd.plucker_rel_list)
      for (const Sush sush: pr.sush_vector()) {
         if (sushes_not_to_add.contains(sush))
            continue;
         sush_queue.push_back(sush);
         sushes_not_to_add += sush;
      }
   return sush_queue;
}

/*
 * @throws ComputationTooLong
 */
SolutionStatus
process_queue(std::list<Sush>& sush_queue,
              hash_set<Sush>& sushes_not_to_add,
              SearchData& sd,
              CanonicalSolidMemoizer& csm,
              PluckersContainingSushMemoizer& pcsm,
              const IntParams& ip)
{
   while (sush_queue.size()) {
      if (ip.verbosity >= 2) {
         cerr << "sush_queue size: " << sush_queue.size()
              << ": ";
         auto it = sush_queue.begin();
         for (Int i=0; i<10 && it != sush_queue.end(); ++i, ++it)
            cerr << *it << " ";
         cerr << endl;
      }
      
      const Sush current_sush = sush_queue.front(); sush_queue.pop_front();
      // the following call to pcsm[] remembers all symmetric images
      // in both pcsm and prm to avoid constructing the images of the PluckerRel 
      for (const auto& phi: pcsm[Sush(-current_sush)]) {
         GP_Tree t(sd.next_tree_index, Phi(phi.get()), csm);
         if (SolutionStatus::found_tree == process_tree(sd, ip, t,
                                                        TreeAddingAction::add_to_existing,
                                                        TreeCompletingAction::complete))
            return SolutionStatus::found_tree;

         for (const auto& sush: t.sush_vector()) {
            const Sush to_process(abs(sush.get()));
            if (!sushes_not_to_add.contains(to_process)) {
               sush_queue.push_back(to_process);
               sushes_not_to_add += to_process;
            }
         }
      }
   }
   return SolutionStatus::not_found;
}

/*
 * @throws ComputationTooLong
 */
SolutionStatus
find_trees(SphereData& sphere_data,
           IntParams& ip,
           SearchData& sd,
           PluckerData& pd,
           CanonicalSolidMemoizer& csm,
           PluckerRelationMemoizer& prm,
           PluckersContainingSushMemoizer& pcsm)
{
   /*
     Since initialize_plucker_relations only recorded the representatives up to
     symmetry of the Plucker relations, we first put the
     sushes of these representatives into a queue.
     
     The queue will contain one instance of each sush that needs to be checked
     for the existence of a tree with -sush. 
   */
   hash_set<Sush> sushes_not_to_add;
   std::list<Sush> sush_queue = sush_queue_from_pr_list(pd, sushes_not_to_add);

   /*
     Now expand the representatives into their orbits 
     so that the trees have all possible nodes to latch on to.
   */
   if (SolutionStatus::found_tree == initialize_tree_list(sd, sphere_data, ip, pd, csm))
      return SolutionStatus::found_tree;

   /*
     Now the work happens: 
     we go through all sushes and see if we can join trees along them
    */
   return process_queue(sush_queue, sushes_not_to_add, sd, csm, pcsm, ip);
}

BigObject
grass_plucker(BigObject s_in,
              OptionSet options)
{
   SearchData sd;
   sd.id_string = std::string(s_in.name() + "(" + s_in.description());
   if ('\n' == sd.id_string[sd.id_string.size()-1])
      sd.id_string[sd.id_string.size()-1] = ')';
   else
      sd.id_string += ")";

   SphereData sphere_data = retrieve_sphere_data(s_in);
   IntParams ip = retrieve_int_params(options, sd.id_string);

   if (ip.verbosity) 
      cerr << sd.id_string
           << ": d=" << sphere_data.d - 1
           << ", f_0=" << sphere_data.n
           << ", f_" << sphere_data.d-1 << "=" << sphere_data.facets.size()
           << ", |Aut|=" <<
         (sphere_data.vertex_symmetry_group.size() ?
          sphere_data.vertex_symmetry_group.size()
          : 1)
           << endl
           << sd.id_string << ": enumerating GP polynomials with 1 undetermined solid"
           << endl;
      
   PermutationSignMemoizer psm;
   CanonicalSolidMemoizer csm(sphere_data, psm);
   SelfTamingMemoizer stm(sphere_data);

#if POLYMAKE_DEBUG
   global_csm = &csm; // for debug printing
#endif
   
   // first only enumerate Pluckers with at most 1 undetermined solid
   const Int original_max_undetermined_ct = ip.max_undetermined_ct;
   ip.max_undetermined_ct = 1;

   PluckerRelationMemoizer prm(csm);
   PluckersContainingSushMemoizer pcsm(sphere_data, csm, prm, stm, ip);
   PluckerData pd;

   /*
     This function writes one representative, up to symmetry, 
     of phis with <= initial_undetermined_ct sushes
     into pd.plucker_rel_list
    */
   if (initialize_plucker_relations(sphere_data, pd, csm, prm, ip, sd.id_string) == SolutionStatus::found_single_positive_pr) {
      sd.tree_list.emplace_back(TreeIndex(0),
                                PhiOrCubeIndex(pd.plucker_rel_list.back().phi().get()),
                                pd.plucker_rel_list.back().sush_vector());
      return make_solution(sd, csm);
   }

   if (ip.verbosity)
      cerr << sd.id_string << ": found " << pd.plucker_rel_list.size()
           << " initial Plucker relations with up to 1 undetermined solid"
           << endl;

   // restore the maximal number of undetermined solids
   ip.max_undetermined_ct = original_max_undetermined_ct;

   try {
      if (find_trees(sphere_data, ip, sd, pd, csm, prm, pcsm) != SolutionStatus::not_found)
         return make_solution(sd, csm);
   } catch(const ComputationTooLong&) {
      cerr << sd.id_string
           << ": Computation aborted after " << sd.tree_list.size() << " trees. This value can changed with the option abort_after." << endl;
      return BigObject("GrassPluckerCertificate");
   }
      
   
   // // no trees found. try with cubes
   // if (find_cubes(sphere_data, ip, sd, pd, csm, prm, stm, pcsm, sd.id_string) != SolutionStatus::not_found)
   //    return make_solution(sd, csm);

   
   return BigObject("GrassPluckerCertificate");
}

} // end namespace gp

    
UserFunction4perl("# @category Other"
                  "# Combinatorial search for Grassmann-Plucker tree certificates"
                  "# as described in J. Pfeifle, Positive Plücker tree certificates for non-realizability, Experimental Math. 2022 https://doi.org/10.1080/10586458.2021.1994487"
                  "# and J. Pfeifle, A polymake implementation of Positive Plücker tree certificates for non-realizability, MEGA 2022"
                  "# @param SimplicialComplex S"
                  "# @option Int verbosity default 1, max 2"
                  "# @option Int max_n_undetermined the maximal allowed number of undetermined solids in a GP relation. Default 3"
                  "# @option Int abort_after stop the computation after how many trees. Default 10000000"
                  "# @option Int tree_log_interval after how many trees a log message occurs. Default 10000"
                  "# @option Int cube_log_interval after how many cubes a log message occurs. Default 100"
                  "# @return GrassPluckerCertificate",
                  &gp::grass_plucker, "grass_plucker(SimplicialComplex, { verbosity => 0, max_n_undetermined => 3, abort_after => 10000000, cube_log_interval => 100, tree_log_interval => 10000 })");


} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
