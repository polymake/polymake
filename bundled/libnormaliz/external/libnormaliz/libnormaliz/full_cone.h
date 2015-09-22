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
#include <deque>
//#include <set>
#include <boost/dynamic_bitset.hpp>

#include "libnormaliz/libnormaliz.h"
#include "libnormaliz/cone_property.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/simplex.h"
#include "libnormaliz/cone_dual_mode.h"
#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/reduction.h"
// #include "libnormaliz/sublattice_representation.h"
#include "libnormaliz/offload_handler.h"

namespace libnormaliz {
using std::list;
using std::vector;
//using std::set;
using std::pair;
using boost::dynamic_bitset;

template<typename Integer> class Cone;
template<typename Integer> class SimplexEvaluator;
template<typename Integer> class CandidateList;
template<typename Integer> class Candidate;
template<typename Integer> class Simplex;
template<typename Integer> class Collector;

template<typename Integer>
class Full_Cone {

    friend class Cone<Integer>;
    friend class SimplexEvaluator<Integer>;
    friend class CandidateList<Integer>;
    friend class Candidate<Integer>;
    friend class Collector<Integer>;
    
public:
    size_t dim;
    size_t level0_dim; // dim of cone in level 0 of the inhomogeneous case
    size_t module_rank;  // rank of solution module over level 0 monoid in the inhomogeneous case
    size_t nr_gen;
    // size_t hyp_size; // not used at present
    Integer index; // index of full lattice over lattice of generators
    
    bool verbose;
    
    bool pointed;
    bool is_simplicial;
    bool deg1_generated_computed;
    bool deg1_generated;
    bool deg1_extreme_rays;
    bool deg1_triangulation;
    bool deg1_hilbert_basis;
    bool inhomogeneous; 
    
    // control of what to compute
    bool do_triangulation;
    bool explicit_full_triang; // indicates whether full triangulation is asked for without default mode
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
	bool do_bottom_dec;
	bool keep_order;
    bool do_class_group;
    bool do_module_gens_intcl;
    
    bool do_extreme_rays;
    bool do_pointed;

    // internal helper control variables
    bool do_only_multiplicity;
    bool do_evaluation;
    bool do_all_hyperplanes;  // controls whether all support hyperplanes must be computed
    bool use_bottom_points;
    ConeProperties is_Computed;    
    bool triangulation_is_nested;
    bool triangulation_is_partial;
    bool has_generator_with_common_divisor;

    // data of the cone (input or output)
    vector<Integer> Truncation;  //used in the inhomogeneous case to suppress vectors of level > 1
    Integer TruncLevel; // used for approximation of simplicial cones
    vector<Integer> Grading;
    vector<Integer> Sorting;
    mpq_class multiplicity;
    Matrix<Integer> Generators;
    vector<key_t> PermGens;  // stores the permutation of the generators created by sorting
    vector<bool> Extreme_Rays;
    Matrix<Integer> Support_Hyperplanes;
    size_t nrSupport_Hyperplanes;
    list<vector<Integer> > Hilbert_Basis;
    list<vector<Integer> > ModuleGeneratorsOverOriginalMonoid;
    CandidateList<Integer> OldCandidates,NewCandidates;   // for the Hilbert basis
    size_t CandidatesSize;
    list<vector<Integer> > Deg1_Elements;
    HilbertSeries Hilbert_Series;
    vector<long> gen_degrees;  // will contain the degrees of the generators
    Integer shift; // needed in the inhomogeneous case to make degrees positive
    vector<long> gen_levels;  // will contain the levels of the generators (in the inhomogeneous case)
    size_t TriangulationSize;          // number of elements in Triangulation, for efficiency
    list < SHORTSIMPLEX<Integer> > Triangulation; // triangulation of cone
    list< SimplexEvaluator<Integer> > LargeSimplices; // Simplices for internal parallelization
    Integer detSum;                  // sum of the determinants of the simplices
    list< STANLEYDATA<Integer> > StanleyDec; // Stanley decomposition
    vector<Integer> ClassGroup;  // the class group as a vector: ClassGroup[0]=its rank, then the orders of the finite cyclic summands
    
    Matrix<Integer> ProjToLevel0Quot;  // projection matrix onto quotient modulo level 0 sublattice    

    // privare data controlling the computations
    vector<size_t> HypCounter; // counters used to give unique number to hyperplane
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
    deque< list<vector<key_t> > > Pyramids;  //storage for pyramids
    deque<size_t> nrPyramids; // number of pyramids on the various levels

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
    vector< Matrix<Integer> > RankTest;                // helper matrices for rank test
    
    // helpers for evaluation
    vector< SimplexEvaluator<Integer> > SimplexEval; // one per thread
    vector< Collector<Integer> > Results; // one per thread
    vector<Integer> Order_Vector;  // vector for the disjoint decomposition of the cone
#ifdef NMZ_MIC_OFFLOAD
    MicOffloader<long long> mic_offloader;
#endif

    // defining semiopen cones
    Matrix<Integer> ExcludedFaces;
    map<boost::dynamic_bitset<>, long> InExCollect;

    // statistics
    size_t totalNrSimplices;   // total number of simplices evaluated
    size_t nrSimplicialPyr;
    size_t totalNrPyr;
    
    bool use_existing_facets;  // in order to avoid duplicate computation of already computed facets
    size_t start_from;
    
    size_t AdjustedReductionBound;
    
    long approx_level;
    bool is_approximation;

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
    bool check_pyr_buffer(const size_t level);
    void evaluate_stored_pyramids(const size_t level);
    void match_neg_hyp_with_pos_hyps(const FACETDATA& hyp, size_t new_generator,list<FACETDATA*>& PosHyps, boost::dynamic_bitset<>& Zero_P);
    void collect_pos_supphyps(list<FACETDATA*>& PosHyps, boost::dynamic_bitset<>& Zero_P, size_t& nr_pos);
    void evaluate_rec_pyramids(const size_t level);
    void evaluate_large_rec_pyramids(size_t new_generator);

    void find_and_evaluate_start_simplex();
    // Simplex<Integer> find_start_simplex() const;
    vector<key_t>  find_start_simplex() const;
    void store_key(const vector<key_t>&, const Integer& height, const Integer& mother_vol,
                                  list< SHORTSIMPLEX<Integer> >& Triangulation);
	void find_bottom_facets();                                  
    Matrix<Integer> latt_approx(); // makes a cone over a lattice polytope approximating "this"
    void convert_polyhedron_to_polytope();
    void compute_elements_via_approx(list<vector<Integer> >& elements_from_approx); // uses the approximation
	void compute_deg1_elements_via_approx_global(); // deg 1 elements from the approximation
    void compute_deg1_elements_via_approx_simplicial(const vector<key_t>& key); // the same for a simplicial subcone
    void compute_sub_div_elements(const Matrix<Integer>& gens,list<vector<Integer> >& sub_div_elements); //computes subdividing elements via approximation
    void select_deg1_elements(const Full_Cone& C);
    void select_Hilbert_Basis(const Full_Cone& C);
    
    void build_top_cone(); 
    void build_cone();
    void get_supphyps_from_copy(bool from_scratch);   // if evealuation starts before support hyperplanes are fully computed
    void update_reducers(bool forced=false);   // update list of reducers after evaluation of simplices
    

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
    void find_module_rank_from_HB();
    void find_module_rank_from_proj();  // used if Hilbert basis is not computed
    void find_level0_dim(); // ditto for the level 0 dimension 
    void sort_gens_by_degree(bool triangulate);
    // void compute_support_hyperplanes(bool do_extreme_rays=false);
    bool check_evaluation_buffer();
    bool check_evaluation_buffer_size();
    void evaluate_triangulation();
    void evaluate_large_simplices();
    void evaluate_large_simplex(size_t j, size_t lss);
    void transfer_triangulation_to_top();
    void primal_algorithm();
    void primal_algorithm_initialize();
    void primal_algorithm_finalize();
    void primal_algorithm_set_computed();
    void remove_duplicate_ori_gens_from_HB();
    void compute_class_group();
    void compose_perm_gens(const vector<key_t>& perm);

    void minimize_support_hyperplanes();   
    void compute_extreme_rays();
    void compute_extreme_rays_compare();
    void compute_extreme_rays_rank();
    void select_deg1_elements();

    void check_pointed();
    void deg1_check();
    void check_deg1_extreme_rays();
    void check_deg1_hilbert_basis();

    void compute_multiplicity();
    
    void prepare_inclusion_exclusion();

    void do_vars_check(bool with_default);
    void reset_tasks();
    void addMult(Integer& volume, const vector<key_t>& key, const int& tn); // multiplicity sum over thread tn
    
    void start_message();
    void end_message();


#ifdef NMZ_MIC_OFFLOAD
    void try_offload(size_t max_level);
#else
    void try_offload(size_t max_level) {};
#endif


/*---------------------------------------------------------------------------
 *                      Constructors
 *---------------------------------------------------------------------------
 */
Full_Cone(Matrix<Integer> M, bool do_make_prime=true);            //main constructor
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
    vector<Integer> getGrading() const; 
    mpq_class getMultiplicity() const;
    Integer getShift()const;
    size_t getModuleRank()const;
    const Matrix<Integer>& getGenerators() const;
    vector<bool> getExtremeRays() const;
    Matrix<Integer> getSupportHyperplanes() const;
    void getTriangulation(list< vector<key_t> >& Triang, list<Integer>& TriangVol) const;
    Matrix<Integer> getHilbertBasis() const;
    Matrix<Integer> getModuleGeneratorsOverOriginalMonoid()const;
    Matrix<Integer> getDeg1Elements() const;
    vector<Integer> getHVector() const;
    Matrix<Integer> getExcludedFaces()const;
    
    bool isComputed(ConeProperty::Enum prop) const; 


/*---------------------------------------------------------------------------
 *              Computation Methods
 *---------------------------------------------------------------------------
 */
    void dualize_cone(bool print_message=true);
    void support_hyperplanes();

    void compute();

    /* adds generators, they have to lie inside the existing cone */
    void add_generators(const Matrix<Integer>& new_points);

    /* computes the multiplicity of the ideal in case of a Rees algebra
     * (not the same as the multiplicity of the semigroup) */
    // Integer primary_multiplicity() const;  // replaced by function in cone.cpp

    void dual_mode();

    void error_msg(string s) const;
};
//class end *****************************************************************
//---------------------------------------------------------------------------

}

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

