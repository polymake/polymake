/*
 * Normaliz
 * Copyright (C) 2007-2013  Winfried Bruns, Bogdan Ichim, Christof Soeger
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
    size_t nr_gen;
    size_t hyp_size; // not used at present
    
    bool pointed;
    bool deg1_generated;
    bool deg1_extreme_rays;
    bool deg1_triangulation;
    bool deg1_hilbert_basis;
    bool integrally_closed;
    
    // control of what to compute
    bool do_triangulation;
    bool do_partial_triangulation;
    bool do_multiplicity;
    bool do_Hilbert_basis;
    bool do_deg1_elements;
    bool do_h_vector;
    bool keep_triangulation;
    bool do_Stanley_dec;

    // internal helper control variables
    bool do_only_multiplicity;
    bool do_evaluation;
    
    ConeProperties is_Computed;
    vector<Integer> Grading;
    mpq_class multiplicity;
    Matrix<Integer> Generators;
    vector<bool> Extreme_Rays;
    list<vector<Integer> > Support_Hyperplanes;
        
    vector<bool> in_triang;
    
    list<vector<Integer> > Hilbert_Basis;
    list<vector<Integer> > Candidates;   // for the Hilbert basis
    size_t CandidatesSize;
    list<vector<Integer> > Deg1_Elements;
    HilbertSeries Hilbert_Series;
    vector<long> gen_degrees;  // will contain the degrees of the generators
    
    // list< SHORTSIMPLEX<Integer> > CheckTri;
    list < SHORTSIMPLEX<Integer> > Triangulation; // triangulation of cone
    size_t TriangulationSize;          // number of elements in Triangulation, for efficiency
    Integer detSum;                  // sum of the det

    vector<typename list < SHORTSIMPLEX<Integer> >::iterator> TriSectionFirst;   // first simplex with lead vertex i
    vector<typename list < SHORTSIMPLEX<Integer> >::iterator> TriSectionLast;     // last simplex with lead vertex i
    vector<key_t> VertInTri;               // generators in the order in which they are inserted into the triangulation
    
    list< SHORTSIMPLEX<Integer> > FreeSimpl;           // list of short simplices already evaluated, kept for recycling
    vector<list< SHORTSIMPLEX<Integer> > > FS;         // the same per thread

    vector< SimplexEvaluator<Integer> > SimplexEval; // one per thread
           
    struct FACETDATA {
        vector<Integer> Hyp;               // linear form of the hyperplane
        boost::dynamic_bitset<> GenInHyp;  // incidence hyperplane/generators
        Integer ValNewGen;                 // value of linear form on the generator to be added                 // value on last generator added
    };
    
    list<FACETDATA> Facets;  // contains the data for Fourier-Motzkin and extension of triangulation
    
    vector<Integer> Order_Vector;  // vector for the disjoint decomposition of the cone 

    list< STANLEYDATA<Integer> > StanleyDec; // Stanley decomposition 

    Full_Cone<Integer>* Top_Cone;     // reference to cone on top level
    vector<key_t> Top_Key;  // indices of generators w.r.t Top_Cone
    
    // control of pyramids and recusrion
    int pyr_level;  // -1 for top cone, increased by 1 for each level of pyramids

    bool is_pyramid; // false for top cone
    bool do_all_hyperplanes;  // controls whether all support hyperplanes must be computed
    long last_to_be_inserted; // good to know in case of do_all_hyperplanes==false
    bool recursion_allowed;  // to allow or block recursive formation of pytamids
    bool parallel_inside_pyramid; // indicates that paralleization is taking place INSIDE the pyramid   

    bool supphyp_recursion; // true if we have gone to pyramids because of support hyperplanes
    bool tri_recursion; // true if we have gone to pyramids because of triangulation
    
    vector< list<vector<key_t> > > Pyramids;  //storage for pyramids
    vector<size_t> nrPyramids; // number of pyramids on the various levels

    long nextGen; // the next generator to be processed
    size_t old_nr_supp_hyps; // must be remembered since we may leave extend_cone 
                             // before discarding "negative" hyperplanes
    
    Full_Cone<Integer>* Mother;       // reference to the mother of the pyramid
    boost::dynamic_bitset<> in_Pyramid; // indicates which generators of the MOTHER are in pyramid
    size_t new_generator; // indicates which generator of mother cone is apex of pyramid 
       
    vector<list<Full_Cone<Integer> > > RecPyrs; // storage for recursive pyramids
    vector<size_t> nrRecPyrs;
    
    size_t nrRecPyramidsDue;  // number of recursive pyramids created from this at the current extension
    size_t nrRecPyramidsDone; // number of recursive pyramids that have returned supphyps
    bool allRecPyramidsBuilt; // indicates that all recursive pyramids from the current generator have been built    

    bool Done; // true if this cone has been finished

    // statistics
    size_t totalNrSimplices;   // total number of simplices evaluated
    size_t nrSimplicialPyr;
    size_t totalNrPyr;

/* ---------------------------------------------------------------------------
 *              Private routines, used in the public routines
 * ---------------------------------------------------------------------------
 */
    void add_hyperplane(const size_t& new_generator, const FACETDATA & positive,const FACETDATA & negative,
                     list<FACETDATA>& NewHyps);
    void extend_triangulation(const size_t& new_generator);
    void find_new_facets(const size_t& new_generator);
    void process_pyramids(const size_t new_generator,const bool recursive);
    void process_pyramid(const vector<key_t>& Pyramid_key, const boost::dynamic_bitset<>& in_Pyramid, 
                      const size_t new_generator, const size_t store_level, Integer height, const bool recursive);
    void select_supphyps_from(list<vector<Integer> >& NewFacets, const size_t new_generator, 
                      const boost::dynamic_bitset<>& in_Pyramid);
    void evaluate_stored_pyramids(const size_t level);
    void evaluate_rec_pyramids(const size_t level);

    void find_and_evaluate_start_simplex();
    Simplex<Integer> find_start_simplex() const;
    void store_key(const vector<key_t>&, const Integer& height, const Integer& mother_vol,
                                  list< SHORTSIMPLEX<Integer> >& Triangulation);
    
    void build_top_cone(); 
    void extend_cone();    
    

    bool is_reducible(list<vector<Integer> *> & Irred, const vector<Integer> & new_element);
    void global_reduction();

    vector<Integer> compute_degree_function() const;
    
    Matrix<Integer> select_matrix_from_list(const list<vector<Integer> >& S,vector<size_t>& selection);

    void extreme_rays_and_deg1_check();
    void set_degrees();
    void sort_gens_by_degree();
    void compute_support_hyperplanes();
    bool check_evaluation_buffer();
    bool check_evaluation_buffer_size();
    void evaluate_triangulation();
    void transfer_triangulation_to_top();
    void primal_algorithm(); 
     
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
    const Matrix<Integer>& getGenerators() const;
    vector<bool> getExtremeRays() const;
    Matrix<Integer> getSupportHyperplanes() const;
    void getTriangulation(list< vector<key_t> >& Triang, list<Integer>& TriangVol) const;
    Matrix<Integer> getHilbertBasis() const;
    Matrix<Integer> getDeg1Elements() const;
    vector<Integer> getHVector() const;
    
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

