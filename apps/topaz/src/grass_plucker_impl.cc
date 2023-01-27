/* Copyright (c) 1997-2023
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
#include "polymake/Graph.h"
#include "polymake/group/group_tools.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace topaz {

namespace gp {      

//---------------------------------------
// Stuff for reading initial data
//---------------------------------------

LabelData
make_labels(BigObject& s_in)
{
   LabelData label_data;
   if (!(s_in.lookup("VERTEX_LABELS") >> label_data.vertex_labels)) {
      const Int n = s_in.give("N_VERTICES");
      label_data.vertex_labels.resize(n);
      for (Int i=0; i<n; ++i)
         label_data.vertex_labels[i] = std::to_string(i);
   }
   label_data.max_label_width = 0;
   for (const auto& l: label_data.vertex_labels)
      assign_max(label_data.max_label_width, l.size());
   return label_data;
}
   
IndexOfFacet
make_iof(const Array<FacetAsSet>& facets)
{
   IndexOfFacet iof;
   Int next_index(0);
   Int d(0);
   
   for (const auto& F: facets) {
      if (!d)
         d = F.size();
      else if (d != F.size())
         throw std::runtime_error("make_iof: found different sizes of facets, but can only deal with simplicial complexes.");
      iof[F] = next_index++;
   }
   return iof;
}
   
SphereData
retrieve_sphere_data(BigObject s_in)
{
   SphereData sd;

   const Array<Set<Int>> in_facets = s_in.give("FACETS");

   Array<FacetAsSet> facets(in_facets.size());
   for (Int i=0; i<in_facets.size(); ++i)
      facets[i] = FacetAsSet(in_facets[i]);
   sd.facets = facets;
   if (!sd.facets.size())
      throw std::runtime_error("retrieve_sd: no facets given");

   sd.iof = make_iof(sd.facets);

   const Array<Int> orientation = s_in.give("ORIENTATION");
   sd.orientation = orientation;

   sd.label_data = make_labels(s_in);

   Array<Array<Int>> gens;
   s_in.lookup("GROUP.RAYS_ACTION.GENERATORS | GROUP.PERMUTATION_ACTION.GENERATORS") >> gens;

   if (gens.size()) {
      const group::PermlibGroup G(gens);
      sd.vertex_symmetry_group = group::all_group_elements_impl(G);
   }
   
   sd.n = 0;
   for (const auto& F: facets)
      assign_max(sd.n, F.get().back());

   ++sd.n;
   sd.d = sd.facets[0].size();
   
   return sd;
}

IntParams
retrieve_int_params(OptionSet& options,
                    const std::string& id_string)
{
   IntParams ip;
   ip.verbosity = options["verbosity"];
   ip.max_undetermined_ct = options["max_n_undetermined"];
   ip.abort_after = options["abort_after"];
   ip.cube_log_interval = options["cube_log_interval"];
   ip.tree_log_interval = options["tree_log_interval"];
   ip.debug = (ip.verbosity > 3);

   if (!ip.verbosity && ip.tree_log_interval != 10000) {
      cerr << id_string
           << ": Since tree_log_interval was changed, setting verbosity to 1"
           << endl;
      ip.verbosity = 1;
   }

   if (!ip.verbosity && ip.cube_log_interval != 100) {
      cerr << id_string
           << ": Since cube_log_interval was changed, setting verbosity to 1"
           << endl;
      ip.verbosity = 1;
   }
   return ip;
}
   
//---------------------------------------
// Stuff for initial Plucker relations   
//---------------------------------------

hash_set<Set<Int>>
make_d_plus_ones_containing_a_facet(const SphereData& sphere_data)
{
   hash_set<Set<Int>> dca;
   Set<Int> new_dp1;
   for (const auto& F: sphere_data.facets) 
      for (auto it = entire(sequence(0, sphere_data.n) - F.get()); !it.at_end(); ++it) {
         new_dp1 = F.get();
         new_dp1 += *it;
         dca += new_dp1;
      }
   return dca;
}

struct IPlusMinus {
   // the set of i such that I + i contains a facet
   Set<Int> I_plus;

   // the set of i such that I + i contains no facet
   Set<Int> I_minus;
};
   
IPlusMinus
make_I_plus_minus(const Set<Int>& I,
                  const Int n,
                  const hash_set<Set<Int>>& dp1cf)
{
   IPlusMinus ipm;
   for (Int i=0; i<n; ++i) {
      if (I.contains(i))
         continue;
      if (dp1cf.contains(Set<Int>(I + scalar2set(i))))
         ipm.I_plus += i;
      else
         ipm.I_minus += i;
   }
   return ipm;
}

Int
count_rests_containing_facet(const Set<Int>& J,
                             const Set<Int>& to_check,
                             const hash_set<Set<Int>>& dp1cf) 
{
   Int ct(0);
   Set<Int> candidate(J);
   for (auto it = entire(to_check); !it.at_end(); ++it) {
      candidate -= *it;
      if (dp1cf.contains(candidate))
         ++ct;
      candidate += *it;
   }
   return ct;
}

bool
is_plucker_rel_acceptable(const PluckerRel& pr,
                          const IntParams& ip,
                          PluckerStats& stats)
{
   if (pr.has_two_adjacent_undetermineds()) {
      ++ stats.n_two_adjacent_undetermineds;
      return false;
   }

   if ((ip.max_length_ct > 0 &&
	Int(pr.terms().size()) > ip.max_length_ct)
       ||
       (ip.max_undetermined_ct > 0 &&
	pr.n_undetermineds() > ip.max_undetermined_ct))
      return false;

   if (!pr.are_determined_signs_positive()) {
      ++ stats.n_determined_sign_negative;
      return false;
   }

   return true;
}

void
add_orbit_of_abs(const Phi phi,
                 const ExplicitGroup& G,
                 PhiOrbit& seen_phis)
{
   assert(phi.get() > 0);
   for (const Array<Int>& g: G) 
      seen_phis += image_of_abs(phi, g);
}

bool
already_in_orbit(const Set<Int>& I,
                 const Set<Int>& J,
                 const ExplicitGroup& G,
                 PhiOrbit& seen_phis)
{
   const Phi phi(PluckerHasher(I, J, SignImpl(1)).phi());
   if (seen_phis.contains(phi))
      return true;

   // else add the images to the orbit
   add_orbit_of_abs(phi, G, seen_phis);
   return false;
}

SolutionStatus
process_I_J(const Set<Int>& I,
            const Set<Int>& J,
            const Set<Int>& B,
            const hash_set<Set<Int>>& dp1cf,
            SphereData& sphere_data,
            CanonicalSolidMemoizer& csm,
            PluckerRelationMemoizer& prm,
            const IntParams& ip,
            PluckerData& pd)
{
   if ((!sphere_data.vertex_symmetry_group.size() ||
        !already_in_orbit(I, J, sphere_data.vertex_symmetry_group, sphere_data.seen_phis))
       &&
       count_rests_containing_facet(J, B, dp1cf) >=
       B.size() - ip.max_undetermined_ct + 1) {
      const SolutionStatus
         ss(process_plucker_rel(I, J, csm, prm, ip, pd,
                                [](const PluckerRel& pr) {
                                   // accept all relations here,
                                   // irrespective of sign of a certain sush
                                   return true; 
                                }));
      if (SolutionStatus::not_found != ss)
         return ss;
   }
   return SolutionStatus::not_found;
}

SolutionStatus
process_A_B(const Set<Int>& I,
            const Set<Int>& AuB,
            const Set<Int>& B,
            const IPlusMinus& ipm,
            const hash_set<Set<Int>>& dp1cf,
            SphereData& sphere_data,
            CanonicalSolidMemoizer& csm,
            PluckerRelationMemoizer& prm,
            const IntParams& ip,
            PluckerData& pd)
{
   /*
     This function uses the symmetry group stored in sphere_data
     when it calls process_I_J.

     First, make the principal undetermined term of the form [I+j0]?[A+B].
     For this, j0 in I_minus; AuB contains a facet;
     and all sets of the form AuB - j + j0 contain a facet for j in B,
     with up to ip.max_undetermined_ct - 1 exceptions

     Put the working set J here to save allocations.
   */
   Set<Int> J = AuB;
   
   if (dp1cf.contains(AuB)) {
      for (const Int j0: ipm.I_minus) {
         J += j0;   
         process_I_J(I, J, B, dp1cf, sphere_data, csm, prm, ip, pd);
         J -= j0;
      }
   } else {
      /*
        Now make the single unknown term of the form [I+j0][A+B]? .
        For this, j0 in I_plus, and AuB does not contain a facet,
        but all subsets A+B+j0 - c, for c in B, contain a facet,
        with up to ip.max_undetermined_ct - 1 exceptions
      */
      for (const Int j0: ipm.I_plus - B) {
         J += j0;
         process_I_J(I, J, B, dp1cf, sphere_data, csm, prm, ip, pd);
         J -= j0;
      }
   }
   return SolutionStatus::not_found;
}
   
/*
  writes all plucker relations, up to symmetry,
  with at most ip.max_undetermined_ct undetermined solids   
  into pd.plucker_rel_list .
  The symmetry elimination is handled in process_A_B, which calls already_in_orbit
*/
SolutionStatus
initialize_plucker_relations(SphereData& sphere_data,
                             PluckerData& pd,
                             CanonicalSolidMemoizer& csm,
                             PluckerRelationMemoizer& prm,
                             const IntParams& ip,
                             const std::string& id_string)
{
   const hash_set<Set<Int>> dp1cf(make_d_plus_ones_containing_a_facet(sphere_data));
   const Int n(sphere_data.n), d(sphere_data.d);
   assert (dp1cf.size() &&
           dp1cf.begin()->size() == d+1);

   Int AB_ct(0);
   Set<Int> I, B, AuB;
   for (auto Iit = entire(all_subsets_of_k(sequence(0, n), d)); !Iit.at_end(); ++Iit) {
      I = *Iit;
      const IPlusMinus ipm(make_I_plus_minus(I, n, dp1cf));
      if (ipm.I_plus.size() < 2)
         continue;

      /*
         J has size d+2, and one element j0 will be responsible for causing the (principal or only) undetermined term,
         which will be either the first or the second term.
         The remaining d+1 members of J will be distributed into two sets A and B, so that
         J = | A | B | j0
         where A is the set of indices shared with I, so that |A| <= d-1,
         and B subset I+, so that |B| <= |I+|.
         Now |A| + |B| = d+1, so that 0 <= |B| = d+1-|A| <= |I+|,
         and we conclude that |A| >= d+1-|I+|, and finally d+1-|I+| <= |A| <= d-1.
      */
      for (Int cardA = std::max(Int(0), d+1-ipm.I_plus.size()); cardA <= d-1; ++cardA) {
         for (auto Ait = entire(all_subsets_of_k(I, cardA)); !Ait.at_end(); ++Ait) {
            for (auto Bit = entire(all_subsets_of_k(ipm.I_plus - *Ait, d+1-cardA)); !Bit.at_end(); ++Bit) {
               B = *Bit;
               AuB = *Ait + *Bit;
               assert (AuB.size() == d+1);
               ++AB_ct;
               const SolutionStatus ss(process_A_B(I, AuB, B, ipm, dp1cf, sphere_data, csm, prm, ip, pd));
               if (SolutionStatus::not_found != ss)
                  return ss;
            }
         }
      }
   }

   if (ip.verbosity) {
      cerr << id_string << ": Plucker relations stats:\n"
           << id_string << ":   number of ABs processed: " << AB_ct << "\n"
           << id_string << ":   number of pluckers processed: " << pd.stats.total_processed << "\n"
           << id_string << ":   n_determined_sign_negative: " << pd.stats.n_determined_sign_negative << "\n"
           << id_string << ":   plucker_rel_list size: " << pd.plucker_rel_list.size()
           << endl;      
   }
   
   return SolutionStatus::not_found;
}

SolutionStatus
re_initialize_plucker_relations(SphereData& sphere_data,
                                PluckerData& pd,
                                SearchData& sd,
                                CanonicalSolidMemoizer& csm,
                                PluckerRelationMemoizer& prm,
                                const IntParams& ip)
{
   cerr << sd.id_string
        << ": Since no solution was found, calculate Plucker relations with up to " << ip.max_undetermined_ct << " undetermined solids" << endl;

   sphere_data.seen_phis.clear();
   pd.stats = PluckerStats();
      
   return initialize_plucker_relations(sphere_data, pd, csm, prm, ip, sd.id_string);
}

//--------------------------------------------   
// Pluckers containing a given sush
//--------------------------------------------

Array<Set<Set<Int>>>   
vertex_links(const Array<FacetAsSet>& facets,
             const Int n)
{
   Array<Set<Set<Int>>> link_of(n);

   for (Int i=0; i<n; ++i)
      for (const auto& F: facets) 
         if (F.get().contains(i))
            link_of[i] += F.get() - scalar2set(i);

   return link_of;
}

Set<Int>
removal_leaves_no_facet(const Array<FacetAsSet>& facets,
                        const Set<Int>& J)
{
   Set<Int> J0;
   
   for (auto Jm1it = entire(all_subsets_less_1(J)); !Jm1it.at_end(); ++Jm1it) {
      bool contains_no_facet(true);
      for (const auto& F: facets) {
         if (incl(F.get(), *Jm1it) <= 0) {
            contains_no_facet = false;
            break;
         }
      }
      if (contains_no_facet)
         J0 += Jm1it->skipped_element();
   }
   
   return J0;
}

Set<Int>   
vertices_whose_links_meet_I(const Set<Int>& I,
                            const Array<Set<Set<Int>>>& link_of)
{
   Set<Int> J1;
   for (Int j=0; j<link_of.size(); ++j) {
      for (const auto& G: link_of[j]) {
         if (incl(G,I) <= 0) {
            J1 += j;
            break;
         }
      }
   }
   return J1;
}

template<typename SushSignAcceptanceCriterion>   
void
cancel_d_subsets_of_J0_by_I(const Set<Int>& J0,
                            const Set<Int>& J,
                            const Set<Int>& dont_cancel_these_j,
                            SphereData& sphere_data,
                            CanonicalSolidMemoizer& csm,
                            PluckerRelationMemoizer& prm,
                            PluckerData& local_pd,
                            const IntParams& ip,
                            hash_set<Set<Int>>& seen_candidate_Is,
                            SushSignAcceptanceCriterion ssac)
{
   // do the case where all of J0 is cancelled by I
   if (J0.size() >= sphere_data.d) {
      for (auto I_it = entire(all_subsets_of_k(J0, sphere_data.d)); !I_it.at_end(); ++I_it) {
         const Set<Int> I(*I_it);
         if (seen_candidate_Is.contains(I) ||
             (J-I).size() < 3)
            continue;
         seen_candidate_Is += I;
         process_plucker_rel(I, J, csm, prm, ip, local_pd, ssac);
      }
   } else { // J0 needs to be supplemented, and is maybe empty
      for (auto restI_it = entire(all_subsets_of_k(sequence(0,sphere_data.n) - dont_cancel_these_j - J0,
                                                   sphere_data.d-J0.size())); !restI_it.at_end(); ++restI_it) {
         const Set<Int> I(J0 + *restI_it);
         if (seen_candidate_Is.contains(I) ||
             (J-I).size() < 3)
            continue;
         seen_candidate_Is += I;
         process_plucker_rel(I, J, csm, prm, ip, local_pd, ssac);
      }
   }
}   
   
template<typename SetType, typename SushSignAcceptanceCriterion>
void
tame_rest_of_J0(const GenericSet<SetType>& Jc,
                const Set<Int>& Jt,
                const Set<Int>& J,
                SphereData& sphere_data,
                CanonicalSolidMemoizer& csm,
                PluckerRelationMemoizer& prm,
                PluckerData& local_pd,
                const Array<Set<Set<Int>>>& link_of,
                const IntParams& ip,
                hash_set<Set<Int>>& seen_candidate_Is,
                SushSignAcceptanceCriterion ssac)
{
   assert (Jt.size());
   for (const auto& G1: link_of[Jt.front()]) {
      if (incl(Jc, G1) == 1)
         continue;

      for (Int i=0; i<sphere_data.n; ++i) {
         if (G1.contains(i))
            continue;
         const Set<Int> candidate_I(G1 + scalar2set(i));
         if (seen_candidate_Is.contains(candidate_I) ||
             (J-candidate_I).size() < 3) 
            continue;

         auto Jt_it = entire(Jt); ++Jt_it;
         bool accept_I(true);
         while (!Jt_it.at_end()) {
            const auto& link(link_of[*Jt_it]);
            if (std::find_if(link.begin(), link.end(), [&candidate_I](const Set<Int>& G) {
                                                          return incl(G, candidate_I) <= 0;
                                                       }) == link.end()) {
               accept_I = false;
               break;
            }
            ++Jt_it;
         }
         if (!accept_I) { // there is some link such that no ridge is contained in candidate_I
            continue;
         }
         
         seen_candidate_Is += candidate_I;
         process_plucker_rel(candidate_I, J, csm, prm, ip, local_pd, ssac);
      }
   }
}

template<typename SushSignAcceptanceCriterion>   
void
pluckers_with_given_J(const Set<Int>& J,
                      const Set<Int>& dont_cancel_these_j,
                      SphereData& sphere_data,
                      CanonicalSolidMemoizer& csm,
                      PluckerRelationMemoizer& prm,
                      PluckerData& local_pd,
                      const Array<Set<Set<Int>>>& link_of,
                      IntParams& ip,
                      SushSignAcceptanceCriterion ssac)
{
   // first process all I = F
   for (const auto& F: sphere_data.facets)
      if (!(dont_cancel_these_j * F.get()).size())
         process_plucker_rel(F.get(), J, csm, prm, ip, local_pd, ssac);

   /*
     now adjust I to the structure of J.
     J0 is the set of all j in J such that J-j contains no facet
   */
   const Set<Int> J0 = removal_leaves_no_facet(sphere_data.facets, J);

   /*
     - Split J0 into two parts, J0 = Jc + Jt
     - Jc will be canceled by incorporating it into I
     - Jt will be tamed by enumerating all Is such that
          I cup jt contains a facet, for all jt in Jt
   */
   hash_set<Set<Int>> seen_candidate_Is;
   const Set<Int> to_cancel(J0 - dont_cancel_these_j);
   for (Int n_cancel = 0; n_cancel <= std::min(sphere_data.d, to_cancel.size()); ++n_cancel) {
      for (auto Jc_it = entire(all_subsets_of_k(to_cancel, n_cancel)); !Jc_it.at_end(); ++Jc_it) {
         const Set<Int> Jt(J0 - *Jc_it);
         if (Jt.size())
            tame_rest_of_J0(*Jc_it, Jt, J, sphere_data, csm, prm, local_pd, link_of, ip, seen_candidate_Is, ssac);
         else
            cancel_d_subsets_of_J0_by_I(to_cancel, J, dont_cancel_these_j, sphere_data, csm, prm, local_pd, ip, seen_candidate_Is, ssac);
      }
   }
}

template<typename SushSignAcceptanceCriterion>   
void
pluckers_with_given_I(const Set<Int>& I,
                      SphereData& sphere_data,
                      CanonicalSolidMemoizer& csm,
                      PluckerRelationMemoizer& prm,
                      PluckerData& local_pd,
                      const Array<Set<Set<Int>>>& link_of,
                      IntParams& ip,
                      SelfTamingMemoizer& self_taming_memoizer,
                      SushSignAcceptanceCriterion ssac)
{
   // if I is a facet, take all Js
   if (std::find(sphere_data.facets.begin(), sphere_data.facets.end(), FacetAsSet(I)) != sphere_data.facets.end()) {
      for (auto J_it = entire(all_subsets_of_k(sequence(0, sphere_data.n) - I, sphere_data.d+2)); !J_it.at_end(); ++J_it)
         process_plucker_rel(I, Set<Int>(*J_it), csm, prm, ip, local_pd, ssac);
      return;
   }

   /*
     If I is not a facet, maybe some I + j contains a facet F,
     which has the form F = I - i + j. Therefore, I - i in link j.
     So J_tame_by_I = { j: exists G in link j s.t. G subset I }, 0 <= |J1| <= d
     now take a subset J_tame_by_I_sel (possibly empty) of J_tame_by_I_sel
     and extend it to a complete J = J_tame_by_I_sel cup J2
   */
   for (auto J_tame_by_I_sel_it = entire(all_subsets(vertices_whose_links_meet_I(I, link_of))); !J_tame_by_I_sel_it.at_end(); ++J_tame_by_I_sel_it) {

      /*
         for all j in J2,
         either j in I (j gets canceled),
         or J - j must contain a facet (j is tamed by J)
      */
      const Set<Int> J_tame_by_I_sel(*J_tame_by_I_sel_it);

      // J_cancel will be a subset of I that is incorporated into J to cancel that part of I
      for (Int J_cancel_card = 0; J_cancel_card <= std::min(I.size(), sphere_data.d + 2 - J_tame_by_I_sel.size()); ++J_cancel_card) {
         for (auto J_cancel_it = entire(all_subsets_of_k(I, J_cancel_card)); !J_cancel_it.at_end(); ++J_cancel_it) {
            const Set<Int> Jpart(J_tame_by_I_sel + *J_cancel_it);
            /*
              so far, Jpart consists of vertices J_tame_by_I_sel whose link meets I, so that I union that vertex contains a facet,
              and vertices J_cancel that duplicate and thus cancel part of the members in I.
              Now supplement Jpart by j's in [n] - Jpart such that J - j contains a facet (a self-tamed J), for all such j's
              Each such facet will contain Jpart, so we only need to look at facets containing Jpart
            */
            for (const Set<Int>& self_tamed_J: self_taming_memoizer.possible_self_tamed_Js(Jpart)) {
               const Set<Int> J(Jpart + self_tamed_J);
               if ((J-I).size() >= 3) 
                  process_plucker_rel(I, J, csm, prm, ip, local_pd, ssac);
            }
         }
      }
   }
}

const std::vector<PluckerRel>
pluckers_containing_sush(SphereData& sphere_data,
                         CanonicalSolidMemoizer& csm,
                         PluckerRelationMemoizer& prm,
                         const Sush& sush,
                         SelfTamingMemoizer& stm,
                         IntParams& ip)
{
   PluckerData local_pd;

   const auto link_of = vertex_links(sphere_data.facets, sphere_data.n);
   Set<Int> solid(UndeterminedSolidHasher(sush).solid().get());
   
   for (Int j=0; j<sphere_data.n; ++j) {
      if (solid.contains(j)) {
         solid -= j;
         pluckers_with_given_I(solid, sphere_data, csm, prm, local_pd, link_of, ip, stm,
                               [sush](const PluckerRel& pr) {
                                  return pr.has_sush(sush);
                               });
         solid += j;
      } else {
         solid += j;
         pluckers_with_given_J(solid, scalar2set(j), sphere_data, csm, prm, local_pd, link_of, ip,
                               [sush](const PluckerRel& pr) {
                                  return pr.has_sush(sush);
                               });
         solid -= j;
      }
   }

   if (ip.verbosity >= 4) cerr << "Self-taming memoizer size " << stm.size()
                                       << ", duplicates: " << stm.n_duplicates() << " / " << stm.n_accessed() << endl;

   return local_pd.plucker_rel_list;
}

Int
image_of(const Int bitset,
         const Array<Int>& g,
         const Int offset)
{
   assert(bitset > 0);
   Int image(0);
   for (Int i=0; i < std::min(Int(max_n_vertices - 1), g.size()); ++i)
      if (bitset & (Int(1) << (i + offset)))
         image |= (Int(1) << (g[i] + offset));
   return image;
}

Phi
image_of_abs(const Phi phi,
             const Array<Int>& g)
{
   assert(phi.get() > 0);
   return Phi(image_of(phi.get() &  low_bitmask, g, 0) |
              image_of(phi.get() & high_bitmask, g, max_n_vertices));
 
}


//--------------------------------------------   
// Initial trees
//--------------------------------------------

PhiOrbit
orbit_of_abs(const Phi phi,
             const Array<Array<Int>>& G)
{
   assert(phi.get() > 0);
   PhiOrbit phi_orbit;
   add_orbit_of_abs(phi, G, phi_orbit);

   return phi_orbit;
}

SolutionStatus
initialize_tree_list(SearchData& sd,
                     const SphereData& sphere_data,
                     IntParams& ip,
                     const PluckerData& pd,
                     CanonicalSolidMemoizer& csm)
{
   // for the space of this function, reduce output
   const Int original_tpi = ip.tree_log_interval;
   ip.tree_log_interval = 1000000000;

   PhiOrbit initial_phi_orbit;
   if (ip.verbosity)
      cerr << sd.id_string
           << ": initializing tree list from " << pd.plucker_rel_list.size() << " Plucker relation reps" << endl;
   for (const auto& pr0: pd.plucker_rel_list) {
      assert(1 == pr0.sush_vector().size());

      const Phi abs_phi(abs(pr0.phi()));
      if (initial_phi_orbit.contains(abs_phi))
         throw std::runtime_error("initialize_tree_list: intersecting orbits");
      
      if (sphere_data.vertex_symmetry_group.size()) {
         for (const Phi phi: orbit_of_abs(abs_phi, sphere_data.vertex_symmetry_group)) {
            initial_phi_orbit += phi;
            
            PluckerRel pr(phi, csm);
            if (pr.are_determined_signs_positive()) {
               GP_Tree t(sd.next_tree_index, PhiOrCubeIndex(pr.phi().get()), pr.sush_vector());
               if (SolutionStatus::found_tree ==
                   process_tree(sd, ip, t,
                                TreeAddingAction::add_to_existing,
                                TreeCompletingAction::complete))
                  return SolutionStatus::found_tree;
            }
            
            pr.invert_sign();
            if (pr.are_determined_signs_positive()) {
               GP_Tree t(sd.next_tree_index, PhiOrCubeIndex(pr.phi().get()), pr.sush_vector());
               if (SolutionStatus::found_tree ==
                   process_tree(sd, ip, t,
                                TreeAddingAction::add_to_existing,
                                TreeCompletingAction::complete))
                  return SolutionStatus::found_tree;
            }
         }
      } else {
         GP_Tree t(sd.next_tree_index, PhiOrCubeIndex(pr0.phi().get()), pr0.sush_vector());
         if (SolutionStatus::found_tree ==
             process_tree(sd, ip, t,
                          TreeAddingAction::add_to_existing,
                          TreeCompletingAction::complete))
            return SolutionStatus::found_tree;
      }
   }

   if (ip.verbosity)
      cerr << sd.id_string
           << ": Made " <<
         (sphere_data.vertex_symmetry_group.size()
          ? initial_phi_orbit.size()
          : pd.plucker_rel_list.size())
           << " images of initial trees in total" << endl;

   // restore the output
   ip.tree_log_interval = original_tpi;
   
   return SolutionStatus::not_found;
}

std::vector<Set<Int>>
facets_containing_H_rests(const Set<Int>& H,
                          const Array<FacetAsSet>& facets)
{
   std::vector<Set<Int>> fcH;
   for (const auto& F: facets)
      if (incl(H, F.get()) <= 0)
         fcH.push_back(F.get() - H);
   return fcH;
}

SignImpl
sgn(const Int j,
    const Set<Int>& I,
    const Set<Int>& J)
{
   Int ct(0);

   auto Jit = entire<reversed>(J);
   while (!Jit.at_end() && j < *Jit) 
      ++ct, ++Jit;

   auto Iit = entire<reversed>(I);
   while (!Iit.at_end() && j < *Iit) 
      ++ct, ++Iit;
      
   return (ct % 2)
      ? SignImpl(1)
      : SignImpl(-1);
}

MaybeUndeterminedSign operator*(const MaybeUndeterminedSign& s, const MaybeUndeterminedSign& t)
{
   return { SignImpl(s.sign_or_signature() * t.sign_or_signature()),
            (s.determined() && t.determined()
             ? SignDeterminedStatus::determined
             : SignDeterminedStatus::undetermined)
   };
}

MaybeUndeterminedSign operator*(const MaybeUndeterminedSign& s, const SignImpl& t)
{
   return { SignImpl(s.sign_or_signature() * t.get()), s.determined_status() };
}
   
MaybeUndeterminedSign operator-(const MaybeUndeterminedSign& s)
{
   return { SignImpl(-s.sign_or_signature()), s.determined_status() };
}

bool operator==(const MaybeUndeterminedSign& s, const MaybeUndeterminedSign& t)
{
   return
      s.sign_or_signature() == t.sign_or_signature() &&
      s.determined_status() == t.determined_status();
}

bool operator!=(const MaybeUndeterminedSign& s, const MaybeUndeterminedSign& t)
{
   return !(s==t);
}

void
write_solid_rep(const Array<Int>& solid,
                const bool determined,
                Map<Array<Int>, Int>& index_of_solid,
                std::ostringstream& os)
{
   if (!index_of_solid.exists(solid)) {
      const Int next_index_of_solid = index_of_solid.size();
      index_of_solid[solid] = next_index_of_solid;
   }
   os << "[" << index_of_solid[solid];
   if (!determined)
      os << "?";
   os << "]";
}

std::string
string_rep(const PluckerRel& pr,
           Map<Array<Int>, Int>& index_of_solid,
           std::ostringstream& os)
{
   os.str("");
   for (const auto& term: pr.terms()) {
      os << ( term.sign().sign_or_signature() == 1
              ? "+"
              : "-" );
      write_solid_rep(term.r1().representative(), term.r1().sign().determined(), index_of_solid, os);
      write_solid_rep(term.r2().representative(), term.r2().sign().determined(), index_of_solid, os);
   }
   return os.str();
}

SushVector
pretty_order(const SushVector& sushes)
{
   Map<Int, Sush> sush_with_abs_value;
   for (const auto& sush: sushes)
      sush_with_abs_value[abs(sush.get())] = sush;
   SushVector ordered_sushes;
   for (const auto& abs_sush: sush_with_abs_value)
      ordered_sushes.push_back(abs_sush.second);
   return ordered_sushes;
}

void
fill_prs_and_hungry_sushes(const SearchData& sd,
                           const Graph<Undirected>& G,
                           const std::vector<Int>& hashes_in_order,
                           const Map<Int, Int>& index_of_phi,
                           Map<Array<Int>, Int>& index_of_solid,
                           NodeMap<Undirected, Array<Set<Int>>>& plucker_relations,
                           Array<std::string>& node_labels,
                           Array<Array<Int>>& hungry_sushes_at,
                           CanonicalSolidMemoizer& csm)
{
   std::ostringstream os;
   for (const auto& node_index: index_of_phi) {
      const Int n(node_index.first);
      const Int i(node_index.second);
      
      if (n < first_cube_index) {
         // it's a tree index
         if (abs(n) < (Int(1) << max_n_vertices)) {
            // it's a fake tree node
            /*
            assert (n == hashes_in_order[i]);
            hungry_sushes_at[i] = sd.cube_list[neighboring_cube_index(G, hashes_in_order, i)].sush_selection_at_vertex()[vertex_index_from_fake_tree_index(n)];
            os.str("");
            os << "case";
            for (const SignedUndeterminedSolidHash sush: pretty_order(hungry_sushes_at[i])) {
               os << " "
                  << (sush < 0 ? "-" : "+");
               write_solid_rep(Array<Int>(UndeterminedSolidHasher(sush).solid()), false, index_of_solid, os);
            }
            node_labels[i] = os.str();
            */
         } else {
            // it's a Plucker node
            const PluckerHasher ph{ Phi(n) };
            const PluckerRel pr(Phi(n), csm);
            Array<Set<Int>> pr_data(3);
            pr_data[0] = scalar2set(sign(n));
            pr_data[1] = ph.I();
            pr_data[2] = ph.J();
            plucker_relations[i] = pr_data;
            node_labels[i] = string_rep(pr, index_of_solid, os);
            hungry_sushes_at[i] = Array<Int>(pr.sush_vector().size());
            for (std::size_t j = 0; j < pr.sush_vector().size(); ++j)
               hungry_sushes_at[i][j] = pr.sush_vector()[j];
         }
      } else {
         // it's a cube node
         /*
         os.str("");
         os << "decide";
         for (const SignedUndeterminedSolidHash sush: pretty_order(sd.cube_list[n - first_cube_index].sush_selection_explored())) {
            os << " ";
            write_solid_rep(Array<Int>(UndeterminedSolidHasher(sush).solid()), false, index_of_solid, os);
          }
         */
         node_labels[i] = os.str();
      }
   }   
}

void
build_edge_map(const Graph<Undirected>& G,
               const std::vector<Int>& hashes_in_order,
               Map<Array<Int>, Int>& index_of_solid,
               Array<Array<Int>>& hungry_sushes_at,
               EdgeMap<Undirected, Array<Array<Int>>>& undetermined_solids,
               EdgeMap<Undirected, std::string>& edge_labels)
{
   std::ostringstream os;
   for (auto e = entire(edges(G)); !e.at_end(); ++e) {
      const Set<Int>
         from_sushes(hungry_sushes_at[e.from_node()]),
         to_sushes  (hungry_sushes_at[e.to_node()]);
      
      // first add the sushes corresponding to an edge from cube node to cube vertex node
      Set<Int> edge_sushes(from_sushes * to_sushes);
      
      // then add the sushes corresponding to a regular edge
      Set<Int> neg_to_sushes;
      for (const auto sush: to_sushes)
         neg_to_sushes += -sush;

      for (const auto sush: from_sushes * neg_to_sushes)
         edge_sushes += sush;

      Array<Array<Int>> undetermined_solids_at_e(edge_sushes.size());
      auto ait = entire(undetermined_solids_at_e);
      
      bool first_sush(true);
      os.str("");
      for (const auto sush: edge_sushes) {
         const Array<Int> solid(UndeterminedSolidHasher(Sush(sush)).solid().get());
         Array<Int> sush_array;
         if (sush < 0) {
            sush_array.resize(1);
            sush_array[0] = -1;
         }
         sush_array.append(solid);
         *ait = solid;
         ++ait;
         
         if (first_sush) {
            first_sush = false;
         } else {
            os << " ";
         }
         write_solid_rep(solid, false, index_of_solid, os);
      }
      undetermined_solids[*e] = undetermined_solids_at_e;
      edge_labels[*e] = edge_sushes.size()
         ? os.str()
         : "decide";
   }
}

BigObject
make_solution(const SearchData& sd,
              CanonicalSolidMemoizer& csm)
{
   const GP_Tree& solution_tree(sd.tree_list.back());
   Int next_index(0);
   Map<Int, Int> index_of_phi;
   std::vector<Int> hashes_in_order;
   for (const auto& n: solution_tree.nodes()) {
      hashes_in_order.push_back(n.self.get());
      index_of_phi[n.self.get()] = next_index++;
   }
      
   Graph<Undirected> G(solution_tree.size());
   for (const auto& from: solution_tree.nodes()) 
      for (const auto& to: from.upstream) 
         G.edge(index_of_phi[from.self.get()], index_of_phi[to.first.get()]);
   
   NodeMap<Undirected, Array<Set<Int>>> plucker_relations(G);
   Array<std::string> node_labels(G.nodes());
   Array<Array<Int>> hungry_sushes_at(G.nodes());
   Map<Array<Int>, Int> index_of_solid;
   fill_prs_and_hungry_sushes(sd, G, hashes_in_order, index_of_phi, index_of_solid, plucker_relations, node_labels, hungry_sushes_at, csm);

   EdgeMap<Undirected, Array<Array<Int>>> undetermined_solids(G);
   EdgeMap<Undirected, std::string> edge_labels(G);
   build_edge_map(G, hashes_in_order, index_of_solid, hungry_sushes_at, undetermined_solids, edge_labels);

   Array<Array<Int>> solids_in_order(index_of_solid.size());
   for (const auto& solid_index: index_of_solid)
      solids_in_order[solid_index.second] = solid_index.first;
   
   BigObject Cert("GrassPluckerCertificate");
   Cert.take("ADJACENCY") << G;
   Cert.take("PLUCKER_RELATIONS") << plucker_relations;
   Cert.take("NODE_LABELS") << node_labels;
   Cert.take("EDGE_LABELS") << edge_labels;
   Cert.take("UNDETERMINED_SOLIDS") << undetermined_solids;
   Cert.take("SOLIDS") << solids_in_order;
   return Cert;
}
      
} // end namespace gp
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
