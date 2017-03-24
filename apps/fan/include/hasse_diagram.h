/* Copyright (c) 1997-2015
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

#ifndef POLYMAKE_FAN_HASSE_DIAGRAM_H
#define POLYMAKE_FAN_HASSE_DIAGRAM_H

#include "polymake/PowerSet.h"
#include "polymake/graph/Closure.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/BasicLatticeTypes.h"
#include "polymake/graph/lattice_builder.h"

namespace polymake { namespace fan { 
   
   namespace lattice {
  

      using namespace graph::lattice;
      /*
       * Given a closure operator, this can iterate over all closed sets lying above a given set.
       */
      template <typename ClosureOperator>
         class complex_closures_above_iterator {

            public:
               typedef typename ClosureOperator::ClosureData ClosureData;
               typedef std::forward_iterator_tag iterator_category;
               typedef ClosureData value_type;
               typedef const value_type& reference;
               typedef const value_type* pointer;
               typedef ptrdiff_t difference_type;

               complex_closures_above_iterator() {}

               template <typename Iterator>
                  complex_closures_above_iterator(const ClosureOperator& cop, const ClosureData& H, 
                        Iterator inter_it) : CO(&cop) {
                     bool empty_set_occured = false;
                     const Set<int>& dual_face = H.get_dual_face();
                     const int df_size = dual_face.size();
                     if(df_size) {
                        FacetList flist(CO->total_set_size());
                        for(; !inter_it.at_end(); ++inter_it) {
                           const Set<int> hc = dual_face * (*inter_it);
                           const int hc_size = hc.size();
                           if(hc_size == 0)
                              empty_set_occured = true;
                           else if(hc_size != df_size)
                              flist.replaceMax(hc);
                        }
                        for(auto mf = entire(flist); !mf.at_end(); ++mf) {
                           data.push_back(ClosureData(*CO, *mf));
                        }
                        if(flist.size() == 0 && empty_set_occured)
                           data.push_back(ClosureData(*CO,Set<int>()));
                     }
                     it = entire(data);
                  }

               // The following are only for dual mode:

               // Iterator for 1- and 2-dimensional cones: We don't need to intersect
               complex_closures_above_iterator(const ClosureOperator& cop, const Set<int>& dual_face) : CO(&cop) {
                  for(auto subface = entire(Subsets_less_1< Set<int> >(dual_face)); !subface.at_end();
                        ++subface) {
                     data.push_back(ClosureData(*CO, *subface));
                  }
                  it = entire(data);
               }

               // Iterator for maximal cones: Just list them
               complex_closures_above_iterator(const ClosureOperator& cop) : CO(&cop) {
                  auto cone_it = ensure(rows(CO->get_maximal_cones()),
                        (pm::cons<pm::indexed,pm::end_sensitive>*)0).begin();
                  for(;!cone_it.at_end(); ++cone_it)
                     data.push_back(ClosureData(*cone_it, cone_it.index()));
                  it = entire(data);
               }

               //Iterator for facets of a maximal cone
               complex_closures_above_iterator(const ClosureOperator& cop, const IncidenceMatrix<>& facets) : CO(&cop) {
                  for(auto fc = entire(rows(facets)); !fc.at_end(); ++fc) 
                     data.push_back(ClosureData(*CO, *fc));
                  it = entire(data);
               }


               reference operator* () const { return it.operator*(); }
               pointer operator->() const { return it.operator->(); }
               complex_closures_above_iterator& operator++ () { ++it; return *this; }
               const complex_closures_above_iterator operator++ (int) { complex_closures_above_iterator copy = *this; operator++(); return copy; }
               bool at_end() const { return it.at_end(); }

            protected:
               const ClosureOperator* CO;
               std::list<ClosureData> data;
               typename Entire<std::list<ClosureData> >::const_iterator it;
         };

      //To preserve the artificial node in dual mode, the closure of the empty set needs to be redefined.
      //In primal mode, the intersection of all cones might be a vertex, but the closure of the
      //empty set is still the empty set, representing the empty fan.
      template <typename Decoration = BasicDecoration>
         class ComplexClosure : public BasicClosureOperator<Decoration> {
            public:
               typedef typename BasicClosureOperator<Decoration>::ClosureData ParentClosureData;
               class ClosureData : public ParentClosureData {
                  protected:
                     bool is_artificial;
                     bool is_maximal;
                  public:
                     ClosureData(const ComplexClosure<Decoration>& parent, const Set<int>& df) : ParentClosureData(parent,df), is_artificial(false), is_maximal(false) {}
                     template <typename TSet1, typename TSet2>
                        ClosureData(const GenericSet<TSet1,int>& f, const GenericSet<TSet2,int>& df) : ParentClosureData(f,df), is_artificial(false), is_maximal(false) {}
                     template <typename TSet>
                        ClosureData(const GenericSet<TSet,int> &df, const int index) :
                           ParentClosureData(scalar2set(index), df), is_artificial(false), is_maximal(true) {}
                     template <typename TSet>
                        ClosureData(const GenericSet<TSet,int> &df) : ParentClosureData( Set<int>(), df), is_artificial(true), is_maximal(false) {}
                     ClosureData(const ParentClosureData& p) : ParentClosureData(p) {}
                     const bool inline is_artificial_node() const { return is_artificial; } 
                     const bool inline is_maximal_face() const { return is_maximal; }
               };

               ComplexClosure() { }

               const ClosureData compute_closure_data(const Decoration &face) const {
                  return ClosureData( BasicClosureOperator<Decoration>::compute_closure_data(face));
               }
         };

      template <typename Decoration = BasicDecoration>
         class ComplexDualClosure : public ComplexClosure<Decoration> {
             public:
               typedef typename ComplexClosure<Decoration>::ClosureData ClosureData;
              
               ComplexDualClosure() {}
               ComplexDualClosure(const IncidenceMatrix<>& maximal_cones, const Array<IncidenceMatrix<> >& maximal_vifs, const FacetList& facet_data) : 
                  maximal_cones(maximal_cones), 
                  maximal_cones_as_list(maximal_cones.cols(), entire(rows(maximal_cones))),
                  non_redundant_facets(facet_data),
                  is_complete(non_redundant_facets.size() == 0),
                  maximal_vifs(maximal_vifs),
                  default_intersector(is_complete? maximal_cones_as_list: non_redundant_facets) {
                     BasicClosureOperator<Decoration>::total_size = maximal_cones.cols();
                     BasicClosureOperator<Decoration>::total_set = 
                        sequence(0,BasicClosureOperator<Decoration>::total_size); 
                     BasicClosureOperator<Decoration>::total_data = 
                        ClosureData(BasicClosureOperator<Decoration>::total_set, Set<int>());
                  }

               const ClosureData closure_of_empty_set() const {
                  return ClosureData(sequence(0, get_maximal_cones().cols()+1));
               }

               const ClosureData compute_closure_data(const Decoration& face) const {
                  if(face.face.contains(-1))
                     return closure_of_empty_set();
                  else return ComplexClosure<Decoration>::compute_closure_data(face);
               }

               complex_closures_above_iterator<ComplexDualClosure> get_closure_iterator(const ClosureData& face) const {
                  const int df_size = face.get_dual_face().size();
                  //Default iterator
                  if(__builtin_expect(!face.is_artificial_node() && (!face.is_maximal_face() || is_complete) && !(df_size <= 2),1))
                     return complex_closures_above_iterator<ComplexDualClosure>(*this,face, 
                           entire(default_intersector));
                  //Artificial node
                  if(__builtin_expect(face.is_artificial_node(),0))
                     return complex_closures_above_iterator<ComplexDualClosure>(*this);
                  //Iterator for rays and twodimensional cones
                  if(__builtin_expect(df_size <= 2,0))
                     return complex_closures_above_iterator<ComplexDualClosure>(*this,face.get_dual_face());
                  //Iterator for facets of maximal cones
                  return complex_closures_above_iterator<ComplexDualClosure>(*this,
                        maximal_vifs[face.get_face().front()]);
               }

               const IncidenceMatrix<>& get_maximal_cones() const { return maximal_cones; }
               const FacetList& get_non_redundant_facets() const { return non_redundant_facets; }

             protected:
               const IncidenceMatrix<> maximal_cones;
               FacetList maximal_cones_as_list;
               const FacetList& non_redundant_facets;
               const bool is_complete;
               const Array<IncidenceMatrix<> > maximal_vifs;
               const FacetList& default_intersector;
         };

      template <typename Decoration = BasicDecoration>
         class ComplexPrimalClosure : public ComplexClosure<Decoration> {
            public:
               typedef typename ComplexClosure<Decoration>::ClosureData ClosureData;
              
               ComplexPrimalClosure() {}

               ComplexPrimalClosure(const IncidenceMatrix<>& complete_incidence) {
                     BasicClosureOperator<Decoration>::facets = complete_incidence;
                     BasicClosureOperator<Decoration>::total_size = complete_incidence.rows();
                     BasicClosureOperator<Decoration>::total_set = 
                        sequence(0,BasicClosureOperator<Decoration>::total_size); 
                     BasicClosureOperator<Decoration>::total_data = 
                        ClosureData(BasicClosureOperator<Decoration>::total_set, Set<int>());
                  }

               const ClosureData closure_of_empty_set() const {
                  return ClosureData(Set<int>(), sequence(0,BasicClosureOperator<Decoration>::facets.rows() + 1));
               }

               const ClosureData compute_closure_data(const Decoration& face) const {
                  if(face.face.size() == 0)
                     return closure_of_empty_set();
                  else return ComplexClosure<Decoration>::compute_closure_data(face);
               }

               complex_closures_above_iterator<ComplexPrimalClosure> get_closure_iterator(const ClosureData& face) const {
                  return complex_closures_above_iterator<ComplexPrimalClosure>(*this,face, entire(cols(BasicClosureOperator<Decoration>::facets)));
               }
         };

      //Complex decorator
      class BasicComplexDecorator : public BasicDecorator<ComplexClosure<>::ClosureData> {
         protected:
            const int artificial_rank;
            Map< Set<int>, int> max_combinatorial_dims;
            const bool full_set_is_artificial;
            const int n_vertices;
            const bool is_pure;
         public:
            typedef ComplexClosure<>::ClosureData FaceData;
            typedef BasicDecorator<FaceData> ParentType;
            using ParentType::total_size;
            using ParentType::initial_rank;
            using ParentType::built_dually;
            using ParentType::artificial_set;

            //Primal version
            BasicComplexDecorator(int comb_dim, const Set<int> artificial, bool full_set_is_artificial,
                  int n_vertices) :
               ParentType(0, artificial), artificial_rank(comb_dim +2),
               full_set_is_artificial(full_set_is_artificial),n_vertices(n_vertices), is_pure(false) {}

            //Dual version
            BasicComplexDecorator(IncidenceMatrix<> maximal_cones,
                  int comb_dim,
                  const Array<int>& max_comb_dims,
                  const Set<int> artificial, bool is_pure)
               : ParentType(maximal_cones.cols(), comb_dim+2, artificial), artificial_rank(0),
               full_set_is_artificial(false), n_vertices(0), is_pure(is_pure) {
                  if(!is_pure) {
                     auto md = entire(max_comb_dims);
                     for(auto mc = entire(rows(maximal_cones)); !mc.at_end(); ++mc, ++md) {
                        max_combinatorial_dims[*mc] = *md;
                     }
                  }
               }

            const BasicDecoration compute_initial_decoration(const FaceData &face) const {
               BasicDecoration data;
               data.rank = initial_rank;
               data.face = built_dually? artificial_set : face.get_face();
               return data;
            }

            const BasicDecoration compute_decoration(const FaceData& face,
                  const BasicDecoration& predecessor_data) const {
               BasicDecoration data;
               data.face = built_dually? face.get_dual_face() : face.get_face();
               if(full_set_is_artificial && data.face.size() == n_vertices) {
                  data.face = artificial_set;
                  data.rank = artificial_rank;
                  return data;
               }
               if(predecessor_data.rank == initial_rank && built_dually && !is_pure) data.rank = max_combinatorial_dims[data.face] + 1;
               else data.rank = predecessor_data.rank + (built_dually? -1 : 1);
               return data;
            }

            const BasicDecoration compute_artificial_decoration(const NodeMap<Directed, BasicDecoration> &decor,
                  const std::list<int>& max_nodes) const {
               if(built_dually)
                  return BasicDecoration( Set<int>(),0);
               else
                  return ParentType::compute_artificial_decoration(decor,max_nodes);
            }

      };


      struct TopologicalType {
         bool is_pure;                 // All maximal cells have the same dimension
         bool is_complete;             // Essentially means coatomic: Every cell is an intersection of maximal cells..
         TopologicalType() : is_pure(false), is_complete(false) {}
         TopologicalType(bool p_arg, bool c_arg) : is_pure(p_arg), is_complete(c_arg) {}
      };

   }//END namespace lattice

   perl::Object lower_hasse_diagram(perl::Object fan, int boundary_rank, bool is_pure, bool is_complete);

   /*
    * @brief Computes the Hasse diagram of a fan, polyhedral complex or simplicial complex
    * @param IncidenceMatrix maximal_cones The maximal cells
    * @param Array<InicidenceMatrix> maximal_vifs The facets of each maximal cell. Can be empty, if tt.is_complete is true
    * @param int top_combinatorial_dim The combinatorial dim of the (artificial) top face. Needed if built dually.
    * @param Array<int> maximal_dims The ranks of the maximal cells. Can be empty if tt.pure = true
    * @param RankRestriction rr. Whether the hasse diagram should only partially be computed (upwards or downwards) up to a certain dimension.
    * @param TopologicalType tt. Indicates whether the complex is pure and/or complete (the latter meaning, that the intersections of the maximal cells generate the full Hasse diagram).
    * @param Set<int> far_vertices. If not trivial, only the faces not intersecting this set are computed.
    */
   template <typename IMatrix>
      graph::Lattice<graph::lattice::BasicDecoration> hasse_diagram_general(
            const GenericIncidenceMatrix<IMatrix>& maximal_cones,
            const Array<IncidenceMatrix<> >& maximal_vifs,
            const int top_combinatorial_dim,
            const Array<int>& maximal_dims,
            lattice::RankRestriction rr,
            lattice::TopologicalType tt,
            const Set<int>& far_vertices) {
         using namespace graph::lattice_builder;
         using namespace fan::lattice;

         //Detect trivial rank restriction
         if(rr.rank_restricted)
            if( (rr.rank_restriction_type == lattice::RankCutType::GreaterEqual && rr.boundary_rank <= 0) ||
                  (rr.rank_restriction_type == lattice::RankCutType::LesserEqual && rr.boundary_rank >= top_combinatorial_dim + 2))
               rr.rank_restricted = false;         
         
         const int n_vertices = maximal_cones.cols();

         FacetList non_redundant_facets(n_vertices);
         if(!tt.is_complete)
            for(auto mvf : maximal_vifs) 
               for(auto fct = entire(rows(mvf)); !fct.at_end(); ++fct)
                  non_redundant_facets.replaceMax(*fct);
         const bool is_dual = 
            !(rr.rank_restricted && rr.rank_restriction_type == lattice::RankCutType::LesserEqual)
            && far_vertices.size() == 0;
         
         RestrictedIncidenceMatrix<> building_matrix;
         if(!is_dual) {
            building_matrix /= maximal_cones;
            for(auto nrf = entire(non_redundant_facets); !nrf.at_end(); ++nrf)
               building_matrix /= *nrf;
         }

         const ComplexPrimalClosure<> primal_cop(IncidenceMatrix<>(std::move(building_matrix)));
         const ComplexDualClosure<> dual_cop(maximal_cones, maximal_vifs, non_redundant_facets);
         
         const Set<int> artificial_set = scalar2set(-1);
         const lattice::BasicComplexDecorator dec = is_dual?
            lattice::BasicComplexDecorator(maximal_cones, top_combinatorial_dim, maximal_dims, scalar2set(-1), tt.is_pure) :
            lattice::BasicComplexDecorator(top_combinatorial_dim, artificial_set, maximal_cones.rows() > 1 && far_vertices.size() == 0, n_vertices);





         //Plain version
         if(!rr.rank_restricted && far_vertices.size() == 0) {
            //For a complete complex in dual mode, we need an artificial final (i.e. bottom) node, if the
            //intersection of all cones is a vertex.
            //Note that in primal mode, if there are at least two maximal cones,
            //the artificial top node is created automatically as a closure.
            bool need_artificial_node = is_dual?
               tt.is_complete && !accumulate(rows(maximal_cones), operations::mul()).empty() :
               maximal_cones.rows() == 1;
            return is_dual? 
               compute_lattice_from_closure<lattice::BasicDecoration>(dual_cop,
                  lattice::TrivialCut<lattice::BasicDecoration>(),
                  dec, need_artificial_node, Dual()) :
               compute_lattice_from_closure<lattice::BasicDecoration>(primal_cop,
                     lattice::TrivialCut<lattice::BasicDecoration>(),
                     dec, need_artificial_node, Primal());
         }
         //Bounded version
         if(far_vertices.size() > 0) {
            typedef lattice::SetAvodiginCut<lattice::BasicDecoration> setcut;
            typedef lattice::RankCut<lattice::BasicDecoration, lattice::RankCutType::LesserEqual> rankcut;
            setcut bounded_cut(far_vertices);
            rankcut rank_cut(rr.boundary_rank);
            lattice::CutAnd<setcut, rankcut> combined_cut(bounded_cut, rank_cut);
            if(rr.rank_restricted)
               return compute_lattice_from_closure<lattice::BasicDecoration>(primal_cop,
                     combined_cut, dec, true, Primal());
            else
               return compute_lattice_from_closure<lattice::BasicDecoration>(primal_cop,
                     bounded_cut, dec, true, Primal());

         }
         //Rank-restricted version
			typedef lattice::NotFullSetCut<lattice::BasicDecoration> NFSCut;
			typedef lattice::RankCut<lattice::BasicDecoration, lattice::RankCutType::LesserEqual> LCut;
			typedef lattice::RankCut<lattice::BasicDecoration, lattice::RankCutType::GreaterEqual> GCut;
         NFSCut nfSetCut(n_vertices);
			LCut lesser_cut(rr.boundary_rank);
         GCut greater_cut(rr.boundary_rank);
         return rr.rank_restriction_type == lattice::RankCutType::GreaterEqual?
            compute_lattice_from_closure<lattice::BasicDecoration>(dual_cop,
                  lattice::CutAnd<NFSCut, GCut>(nfSetCut,greater_cut), dec, true, Dual()) :
            compute_lattice_from_closure<lattice::BasicDecoration>(primal_cop,
                  lattice::CutAnd<NFSCut, LCut>(nfSetCut,lesser_cut), dec, true, Primal());
      }

}}

#endif
