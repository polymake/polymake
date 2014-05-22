/*
 * Normaliz
 * Copyright (C) 2007-2014  Winfried Bruns, Bogdan Ichim, Christof Soeger
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#ifndef FULL_CONE_H
#define FULL_CONE_H

#include <list>
#include <vector>
//#include <set>
#include <boost/dynamic_bitset.hpp>

#include "libnormaliz.h"
#include "cone_property.h"
#include "matrix.h"
#include "simplex.h"
#include "cone_dual_mode.h"
#include "HilbertSeries.h"
// #include "sublattice_representation.h"

namespace libnormaliz {
using std::list;
using std::vector;
//using std::set;
using std::pair;
using boost::dynamic_bitset;

template<typename Integer> class Cone;

template<typename Integer>
class Full_Cone {

    friend class Cone<Integer>;
    friend class SimplexEvaluator<Integer>;
    
    size_t dim;
    size_t level0_dim; // dim of cone in level 0 of the inhomogeneous case
    size_t module_rank;  // rank of solution module over level 0 monoid in the inhomogeneous case
    size_t nr_gen;
    // size_t hyp_size; // not used at present
    
    bool pointed;
    bool deg1_generated;
    bool deg1_extreme_rays;
    bool deg1_triangulation;
    bool deg1_hilbert_basis;
    bool integrally_closed;
    bool inhomogeneous; 
    
    // control of what to compute
    bool do_triangulation;
    bool do_partial_triangulation;
    bool do_determinants;
    bool do_multiplicity;
    bool do_Hilbert_basis;
    bool do_deg1_elements;
    bool do_h_vector;
    bool keep_triangulation;
    bool do_Stanley_dec;
    bool do_excluded_faces;
    bool do_approximation;
    bool do_default_mode;

    // internal helper control variables
    bool do_only_multiplicity;
    bool do_evaluation;
    bool do_all_hyperplanes;  // controls whether all support hyperplanes must be computed
    ConeProperties is_Computed;    

    // data of the cone (input or output)
    vector<Integer> Truncation;  //used in the inhomogeneous case to suppress vectors of level > 1
    vector<Integer> Grading;
    mpq_class multiplicity;
    Matrix<Integer> Generators;
    vector<bool> Extreme_Rays;
    list<vector<Integer> > Support_Hyperplanes;
    list<vector<Integer> > Hilbert_Basis;
    list<vector<Integer> > Candidates;   // for the Hilbert basis
    size_t CandidatesSize;
    list<vector<Integer> > Deg1_Elements;
    HilbertSeries Hilbert_Series;
    vector<long> gen_degrees;  // will contain the degrees of the generators
    Integer shift; // needed in the inhomogeneous case to make degrees positive
    vector<long> gen_levels;  // will contain the levels of the generators (in the inhomogeneous case)
    size_t TriangulationSize;          // number of elements in Triangulation, for efficiency
    list < SHORTSIMPLEX<Integer> > Triangulation; // triangulation of cone
    Integer detSum;                  // sum of the determinants of the simplices
    list< STANLEYDATA<Integer> > StanleyDec; // Stanley decomposition
    
    Matrix<Integer> ProjToLevel0Quot;  // projection matrix onto quotient modulo level 0 sublattice    

    // privare data controlling the computations
    vector<size_t> HypCounter; // counters used to give uniqoe number to jyperplane
                               // must be defined thread wise to avoid critical
                               
    vector<bool> in_triang;  // intriang[i]==true means that Generators[i] has been actively inserted
    vector<key_t> GensInCone;    // lists the generators completely built in
    size_t nrGensInCone;    // their number
        
    struct FACETDATA {
        vector<Integer> Hyp;               // linear form of the hyperplane
        boost::dynamic_bitset<> GenInHyp;  // incidence hyperplane/generators
        Integer ValNewGen;                 // value of linear form on the generator to be added
        size_t BornAt;                      // number of generator (in order of insertion) at which this hyperplane was added,, counting from 0
        size_t Ident;                      // unique number identifying the hyperplane (derived from HypCounter)
        size_t Mother;                     // Ident of positive mother if known, 0 if unknown
    };

    list<FACETDATA> Facets;  // contains the data for Fourier-Motzkin and extension of triangulation
    size_t old_nr_supp_hyps; // must be remembered since Facets gets extended before the current generators is finished 
        
    // data relating a pyramid to its ancestores
    Full_Cone<Integer>* Top_Cone; // reference to cone on top level
    vector<key_t> Top_Key;        // indices of generators w.r.t Top_Cone
    Full_Cone<Integer>* Mother;   // reference to the mother of the pyramid
    vector<key_t> Mother_Key;     // indices of generators w.r.t Mother
    size_t apex; // indicates which generator of mother cone is apex of pyramid
    int pyr_level;  // -1 for top cone, increased by 1 for each level of pyramids

    // control of pyramids, recusrion and parallelization
    bool is_pyramid; // false for top cone
    long last_to_be_inserted; // good to know in case of do_all_hyperplanes==false
    bool recursion_allowed;  // to allow or block recursive formation of pytamids
    bool multithreaded_pyramid; // indicates that this cone is computed in parallel threads
    bool tri_recursion; // true if we have gone to pyramids because of triangulation
    
    vector<size_t> Comparisons; // at index i we note the total number of comparisons 
                               // of positive and negative hyperplanes needed for the first i generators
    size_t nrTotalComparisons; // counts the comparisons in the current computation
   
    // storage for subpyramids
    size_t store_level; // the level on which daughters will be stored  
    vector< list<vector<key_t> > > Pyramids;  //storage for pyramids
    vector<size_t> nrPyramids; // number of pyramids on the various levels

    // data that can be used to go out of build_cone and return later (not done at present)
    // but also useful at other places
    long nextGen; // the next generator to be processed
    long lastGen; // the last generator processed
    
    // Helpers for triangulation and Fourier-Motzkin
    vector<typename list < SHORTSIMPLEX<Integer> >::iterator> TriSectionFirst;   // first simplex with lead vertex i
    vector<typename list < SHORTSIMPLEX<Integer> >::iterator> TriSectionLast;     // last simplex with lead vertex i
    list<FACETDATA> LargeRecPyrs; // storage for large recusive pyramids given by basis of pyramid in mother cone
    
    list< SHORTSIMPLEX<Integer> > FreeSimpl;           // list of short simplices already evaluated, kept for recycling
    vector<list< SHORTSIMPLEX<Integer> > > FS;         // the same per thread
    
    // helpers for evaluation
    vector< SimplexEvaluator<Integer> > SimplexEval; // one per thread
    vector<Integer> Order_Vector;  // vector for the disjoint decomposition of the cone
    
    // defining semiopen cones
    Matrix<Integer> ExcludedFaces;
    map<boost::dynamic_bitset<>, long> InExCollect;

    // statistics
    size_t totalNrSimplices;   // total number of simplices evaluated
    size_t nrSimplicialPyr;
    size_t totalNrPyr;

/* ---------------------------------------------------------------------------
 *              Private routines, used in the public routines
 * ---------------------------------------------------------------------------
 */
    void number_hyperplane(FACETDATA& hyp, const size_t born_at, const size_t mother);
    bool is_hyperplane_included(FACETDATA& hyp);
    void add_hyperplane(const size_t& new_generator, const FACETDATA & positive,const FACETDATA & negative,
                     list<FACETDATA>& NewHyps);
    void extend_triangulation(const size_t& new_generator);
    void find_new_facets(const size_t& new_generator);
    void process_pyramids(const size_t new_generator,const bool recursive);
    void process_pyramid(const vector<key_t>& Pyramid_key, 
                      const size_t new_generator, const size_t store_level, Integer height, const bool recursive,
                      typename list< FACETDATA >::iterator hyp, size_t start_level);
    void select_supphyps_from(const list<FACETDATA>& NewFacets, const size_t new_generator, 
                      const vector<key_t>& Pyramid_key);
    void evaluate_stored_pyramids(const size_t level);
    void match_neg_hyp_with_pos_hyps(const FACETDATA& hyp, size_t new_generator,list<FACETDATA*>& PosHyps, boost::dynamic_bitset<>& Zero_P);
    void collect_pos_supphyps(list<FACETDATA*>& PosHyps, boost::dynamic_bitset<>& Zero_P, size_t& nr_pos);
    void evaluate_rec_pyramids(const size_t level);
    void evaluate_large_rec_pyramids(size_t new_generator);

    void find_and_evaluate_start_simplex();
    Simplex<Integer> find_start_simplex() const;
    void store_key(const vector<key_t>&, const Integer& height, const Integer& mother_vol,
                                  list< SHORTSIMPLEX<Integer> >& Triangulation);
                                  
    Matrix<Integer> latt_approx(); // makes a cone over a lattice polytope approximating "this"
    void compute_deg1_elements_via_approx(); // uses the approximation
    void select_deg1_elements(const Full_Cone& C);
    
    void build_top_cone(); 
    void build_cone();   
    

    bool is_reducible(list<vector<Integer> *> & Irred, const vector<Integer> & new_element);
    void global_reduction();

    vector<Integer> compute_degree_function() const;
    
    Matrix<Integer> select_matrix_from_list(const list<vector<Integer> >& S,vector<size_t>& selection);

    bool contains(const vector<Integer>& v);
    bool contains(const Full_Cone& C);
    void extreme_rays_and_deg1_check();
    void find_grading();
    void find_grading_inhom();
    void disable_grading_dep_comp();
    void set_degrees();
    void set_levels(); // for truncation in the inhomogeneous case
    void find_module_rank(); // finds the module rank in the inhom case
    void find_level0_dim(); // ditto for the level 0 dimension 
    void sort_gens_by_degree();
    void compute_support_hyperplanes();
    bool check_evaluation_buffer();
    bool check_evaluation_buffer_size();
    void evaluate_triangulation();
    void transfer_triangulation_to_top();
    void primal_algorithm(); 

    void minimize_support_hyperplanes();   
    void compute_extreme_rays();
    void compute_extreme_rays_compare();
    void compute_extreme_rays_rank();
    void select_deg1_elements();

    void check_pointed();
    void deg1_check();
    void check_deg1_extreme_rays();
    void check_deg1_hilbert_basis();
    void check_integrally_closed();

    void compute_multiplicity();
    
    void prepare_inclusion_exclusion();

    void do_vars_check();
    void reset_tasks();
    void addMult(Integer& volume, const vector<key_t>& key, const int& tn); // multiplicity sum over thread tn

public:
/*---------------------------------------------------------------------------
 *                      Constructors
 *---------------------------------------------------------------------------
 */
    Full_Cone(Matrix<Integer> M);            //main constructor
    Full_Cone(const Cone_Dual_Mode<Integer> &C);
    Full_Cone(Full_Cone<Integer>& C, const vector<key_t>& Key); // for pyramids

/*---------------------------------------------------------------------------
 *                      Data access
 *---------------------------------------------------------------------------
 */
    void print() const;             //to be modified, just for tests
    size_t getDimension() const;       
    size_t getNrGenerators() const;    
    bool isPointed() const;
    bool isDeg1ExtremeRays() const;
    bool isDeg1HilbertBasis() const;
    bool isIntegrallyClosed() const;
    vector<Integer> getGrading() const; 
    mpq_class getMultiplicity() const;
    Integer getShift()const;
    size_t getModuleRank()const;
    const Matrix<Integer>& getGenerators() const;
    vector<bool> getExtremeRays() const;
    Matrix<Integer> getSupportHyperplanes() const;
    void getTriangulation(list< vector<key_t> >& Triang, list<Integer>& TriangVol) const;
    Matrix<Integer> getHilbertBasis() const;
    Matrix<Integer> getDeg1Elements() const;
    vector<Integer> getHVector() const;
    Matrix<Integer> getExcludedFaces()const;
    
    bool isComputed(ConeProperty::Enum prop) const; 


/*---------------------------------------------------------------------------
 *              Computation Methods
 *---------------------------------------------------------------------------
 */
    void dualize_cone();
    void support_hyperplanes();

    void compute();

    /* computes the multiplicity of the ideal in case of a Rees algebra
     * (not the same as the multiplicity of the semigroup) */
    Integer primary_multiplicity() const;

    void dual_mode();

    void error_msg(string s) const;
};
//class end *****************************************************************
//---------------------------------------------------------------------------

}

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

