/*
 * Normaliz
 * Copyright (C) 2007-2019  Winfried Bruns, Bogdan Ichim, Christof Soeger
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

#ifndef LIBNORMALIZ_CONE_H_
#define LIBNORMALIZ_CONE_H_

#include <vector>
#include <map>
#include <set>
#include <string>
#include <utility>  // for pair

#include <libnormaliz/general.h>
#include "libnormaliz/input_type.h"
#include <libnormaliz/automorph.h>
#include <libnormaliz/sublattice_representation.h>
#include <libnormaliz/matrix.h>
#include <libnormaliz/HilbertSeries.h>
#include "libnormaliz/dynamic_bitset.h"

namespace libnormaliz {
using std::map;
using std::pair;
using std::vector;

template <typename Integer>
class Full_Cone;

template <typename Integer>
struct FACETDATA {
    vector<Integer> Hyp;      // linear form of the hyperplane
    dynamic_bitset GenInHyp;  // incidence hyperplane/generators
    Integer ValNewGen;        // value of linear form on the generator to be added
    size_t BornAt;            // number of generator (in order of insertion) at which this hyperplane was added,, counting from 0
    size_t Ident;             // unique number identifying the hyperplane (derived from HypCounter)
    size_t Mother;            // Ident of positive mother if known, 0 if unknown
    bool is_positive_on_all_original_gens;
    bool is_negative_on_some_original_gen;
    bool simplicial;  // indicates whether facet is simplicial
    bool neutral;
    bool positive;
    bool negative;
};

template <typename Integer>
struct CONVEXHULLDATA {
    Sublattice_Representation<Integer> SLR;  // identifies the version of BasisChangePointed with which the data were stored
    long nr_threads;

    bool is_primal;

    vector<size_t> HypCounter;  // counters used to give unique number to hyperplane
                                // must be defined thread wise to avoid critical

    vector<bool> in_triang;    // intriang[i]==true means that Generators[i] has been actively inserted
    vector<key_t> GensInCone;  // lists the generators completely built in
    size_t nrGensInCone;       // their number

    vector<size_t> Comparisons;  // at index i we note the total number of comparisons
                                 // of positive and negative hyperplanes needed for the first i generators
    size_t nrTotalComparisons;   // counts the comparisons in the current computation

    list<FACETDATA<Integer> > Facets;  // contains the data for Fourier-Motzkin and extension of triangulation
    size_t old_nr_supp_hyps;           // must be remembered since Facets gets extended before the current generators is finished
    Matrix<Integer> Generators;
};

// type for simplex, short in contrast to class Simplex
template <typename Integer>
struct SHORTSIMPLEX {
    vector<key_t> key;      // full key of simplex
    Integer height;         // height of last vertex over opposite facet
    Integer vol;            // volume if computed, 0 else
    Integer mult;           // used for renf_elem_class
    vector<bool> Excluded;  // for disjoint decomposition of cone
                            // true in position i indictate sthat the facet
                            // opposite of generator i must be excluded
};

template <typename Integer>
bool compareKeys(const SHORTSIMPLEX<Integer>& A, const SHORTSIMPLEX<Integer>& B) {
    return (A.key < B.key);
}

struct STANLEYDATA_int {  // for internal use
    vector<key_t> key;
    Matrix<long> offsets;
    vector<long> degrees;  // degrees and classNr are used in nmz_integral.cpp
    size_t classNr;        // number of class of this simplicial cone
};

template <typename Integer>
struct STANLEYDATA {
    vector<key_t> key;
    Matrix<Integer> offsets;
};

template <typename Integer>
class Cone {
    //---------------------------------------------------------------------------
    //                               public methods
    //---------------------------------------------------------------------------
   public:
    //---------------------------------------------------------------------------
    //                    Constructors, they preprocess the input
    //---------------------------------------------------------------------------

    typedef Integer elem_type;

    Cone() {
    }  // default constructor

    /* give up to 3 matrices as input
     * the types must be pairwise different
     */
    template <typename T>
    Cone(InputType type, const vector<vector<T> >& input_data) {
        // convert to a map
        map<InputType, vector<vector<T> > > multi_input_data;
        /*= {{type, input_data()}};*/
        multi_input_data[type] = input_data;
        process_multi_input(multi_input_data);
    }

    template <typename T>
    Cone(InputType type1, const vector<vector<T> >& input_data1, InputType type2, const vector<vector<T> >& input_data2) {
        if (type1 == type2) {
            throw BadInputException("Input types must be pairwise different!");
        }
        // convert to a map
        map<InputType, vector<vector<T> > > multi_input_data;
        /*= {
            {type1, input_data1},
            {type2, input_data2},
        };*/
        multi_input_data[type1] = input_data1;
        multi_input_data[type2] = input_data2;
        process_multi_input(multi_input_data);
    }

    template <typename T>
    Cone(InputType type1,
         const vector<vector<T> >& input_data1,
         InputType type2,
         const vector<vector<T> >& input_data2,
         InputType type3,
         const vector<vector<T> >& input_data3) {
        if (type1 == type2 || type1 == type3 || type2 == type3) {
            throw BadInputException("Input types must be pairwise different!");
        }
        // convert to a map
        map<InputType, vector<vector<T> > > multi_input_data;
         /*= {
            {type1, input_data1},
            {type2, input_data2},
            {type3, input_data3},
        };*/
        multi_input_data[type1] = input_data1;
        multi_input_data[type2] = input_data2;
        multi_input_data[type3] = input_data3;
        process_multi_input(multi_input_data);
    }

    /* give multiple input */
    template <typename T>
    Cone(const map<InputType, vector<vector<T> > >& multi_input_data) {
        process_multi_input(multi_input_data);
    }

    //-----------------------------------------------------------------------------
    // Now with Matrix

    template <typename T>
    Cone(InputType type, const Matrix<T>& input_data) {
        // convert to a map
        map<InputType, vector<vector<T> > > multi_input_data;
        // = {{type, input_data.get_elements()}};
        multi_input_data[type] = input_data.get_elements();
        process_multi_input(multi_input_data);
    }

    template <typename T>
    Cone(InputType type1, const Matrix<T>& input_data1, InputType type2, const Matrix<T>& input_data2) {
        if (type1 == type2) {
            throw BadInputException("Input types must be pairwise different!");
        }
        // convert to a map
        map<InputType, vector<vector<T> > > multi_input_data;
        /*= {
            {type1, input_data1.get_elements()},
            {type2, input_data2.get_elements()},
        };*/
        multi_input_data[type1] = input_data1.get_elements();
        multi_input_data[type2] = input_data2.get_elements();
        process_multi_input(multi_input_data);
    }

    template <typename T>
    Cone(InputType type1,
         const Matrix<T>& input_data1,
         InputType type2,
         const Matrix<T>& input_data2,
         InputType type3,
         const Matrix<T>& input_data3) {
        if (type1 == type2 || type1 == type3 || type2 == type3) {
            throw BadInputException("Input types must be pairwise different!");
        }
        // convert to a map
        map<InputType, vector<vector<T> > > multi_input_data;
        /*= {
            {type1, input_data1.get_elements()},
            {type2, input_data2.get_elements()},
            {type3, input_data3.get_elements()},
        };*/
        multi_input_data[type1] = input_data1.get_elements();
        multi_input_data[type2] = input_data2.get_elements();
        multi_input_data[type3] = input_data3.get_elements();
        process_multi_input(multi_input_data);
    }

    /* give multiple input */
    template <typename T>
    Cone(const map<InputType, Matrix<T> >& multi_input_data) {
        map<InputType, vector<vector<T> > > tmp;
        for (const auto& it : multi_input_data) {
            tmp[it.first] = it.second.get_elements();
        }
        process_multi_input(tmp);
    }

    //---------------------------------------------------------------------------
    //                                Destructor
    //---------------------------------------------------------------------------

    ~Cone();

    //---------------------------------------------------------------------------
    //                          give additional data
    //---------------------------------------------------------------------------

    /* Sets if the Cone prints verbose output.
     * The default value for the Cone is the global verbose.
     * returns the old value
     */
    bool setVerbose(bool v);

    void deactivateChangeOfPrecision();

    /* We allow the change of the cone by additional inequalities or generators
     * after the first computation for "dynamical" applications, in which
     * thecone is canged depending on previous computation results.
     *
     * Only one additional matrix can be added at a time. Nevertheless it is useful
     * to go through the map.
     */

    void modifyCone(const map<InputType, vector<vector<Integer> > >& add_multi_input);
    void modifyCone(const map<InputType, vector<vector<mpq_class> > >& add_multi_input);
    void modifyCone(const map<InputType, vector<vector<nmz_float> > >& add_multi_input);

    template<typename T>
    void modifyCone(InputType type, const vector<vector<T> >& input_data);

    template<typename T>
    void modifyCone(InputType type, const Matrix<T>& input_data);

    /* We must also transport data that cannot be conveyed by the constructors
     * or comute functions (in the present setting)
     */

    void setPolynomial(string poly);

    void setNumericalParams(const map<NumParam::Param, long>& num_params);
    void setNrCoeffQuasiPol(long nr_coeff);
    void setExpansionDegree(long degree);
    void setFaceCodimBound(long bound);
    void setAutomCodimBoundMult(long bound);
    void setAutomCodimBoundVectors(long bound);

#ifdef ENFNORMALIZ
    void setRenf(renf_class* renf);
#endif

    template <typename InputNumber>
    void check_add_input(const map<InputType, vector<vector<InputNumber> > >& multi_add_data);
    template <typename InputNumber>
    void check_consistency_of_dimension(const map<InputType, vector<vector<InputNumber> > >& multi_add_data);

    map<InputType, vector<vector<Integer> > > mpqclass_input_to_integer(
        const map<InputType, vector<vector<mpq_class> > >& multi_input_data_const);

    //---------------------------------------------------------------------------
    //                           make computations
    //---------------------------------------------------------------------------

    // return what was NOT computed
    // special cases for up to 3 CPs
    ConeProperties compute(ConeProperties ToCompute);
    ConeProperties compute(ConeProperty::Enum);
    ConeProperties compute(ConeProperty::Enum, ConeProperty::Enum);
    ConeProperties compute(ConeProperty::Enum, ConeProperty::Enum, ConeProperty::Enum);

    //---------------------------------------------------------------------------
    //                         check what is computed
    //---------------------------------------------------------------------------

    bool isComputed(ConeProperty::Enum prop) const;
    // returns true, when ALL properties in CheckComputed are computed
    bool isComputed(ConeProperties CheckComputed) const;

    void resetComputed(ConeProperty::Enum prop);

    //---------------------------------------------------------------------------
    //   get the results, these methods will start a computation if necessary
    //   throws an NotComputableException if not succesful
    //---------------------------------------------------------------------------

    // dimension and rank invariants
    size_t getEmbeddingDim() const {
        return dim;
    };                            // is always known
    size_t getRank();             // depends on ExtremeRays
    Integer getIndex();           // depends on OriginalMonoidGenerators
    Integer getInternalIndex();   // = getIndex()
    Integer getUnitGroupIndex();  // ditto
    // only for inhomogeneous case:
    size_t getRecessionRank();
    long getAffineDim();
    size_t getModuleRank();

    Cone<Integer>& getIntegerHullCone() const;
    Cone<Integer>& getSymmetrizedCone() const;
    Cone<Integer>& getProjectCone() const;

    const Matrix<Integer>& getGeneratorsMatrix();
    const vector<vector<Integer> >& getGenerators();
    size_t getNrGenerators();

    const Matrix<Integer>& getExtremeRaysMatrix();
    const vector<vector<Integer> >& getExtremeRays();
    size_t getNrExtremeRays();

    const Matrix<nmz_float>& getVerticesFloatMatrix();
    const vector<vector<nmz_float> >& getVerticesFloat();
    size_t getNrVerticesFloat();

    const Matrix<nmz_float>& getSuppHypsFloatMatrix();
    const vector<vector<nmz_float> >& getSuppHypsFloat();
    size_t getNrSuppHypsFloat();

    const Matrix<Integer>& getVerticesOfPolyhedronMatrix();
    const vector<vector<Integer> >& getVerticesOfPolyhedron();
    size_t getNrVerticesOfPolyhedron();

    const Matrix<Integer>& getSupportHyperplanesMatrix();
    const vector<vector<Integer> >& getSupportHyperplanes();
    size_t getNrSupportHyperplanes();

    const Matrix<Integer>& getMaximalSubspaceMatrix();
    const vector<vector<Integer> >& getMaximalSubspace();
    size_t getDimMaximalSubspace();

    // depends on the ConeProperty::s SupportHyperplanes and Sublattice
    map<InputType, vector<vector<Integer> > > getConstraints();

    const Matrix<Integer>& getExcludedFacesMatrix();
    const vector<vector<Integer> >& getExcludedFaces();
    size_t getNrExcludedFaces();

    size_t getTriangulationSize();
    Integer getTriangulationDetSum();

    vector<Integer> getWitnessNotIntegrallyClosed();
    vector<Integer> getGeneratorOfInterior();

    const Matrix<Integer>& getHilbertBasisMatrix();
    const vector<vector<Integer> >& getHilbertBasis();
    size_t getNrHilbertBasis();

    const Matrix<Integer>& getModuleGeneratorsOverOriginalMonoidMatrix();
    const vector<vector<Integer> >& getModuleGeneratorsOverOriginalMonoid();
    size_t getNrModuleGeneratorsOverOriginalMonoid();

    const Matrix<Integer>& getModuleGeneratorsMatrix();
    const vector<vector<Integer> >& getModuleGenerators();
    size_t getNrModuleGenerators();

    const Matrix<Integer>& getDeg1ElementsMatrix();
    const vector<vector<Integer> >& getDeg1Elements();
    size_t getNrDeg1Elements();

    size_t getNumberLatticePoints();

    const Matrix<Integer>& getLatticePointsMatrix();
    const vector<vector<Integer> >& getLatticePoints();
    size_t getNrLatticePoints();

    const map<dynamic_bitset, int>& getFaceLattice();
    vector<size_t> getFVector();
    const vector<dynamic_bitset>& getIncidence();

    // the actual grading is Grading/GradingDenom
    vector<Integer> getGrading();
    Integer getGradingDenom();

    vector<Integer> getDehomogenization();

    vector<Integer> getClassGroup();

    const AutomorphismGroup<Integer>& getAutomorphismGroup(ConeProperty::Enum quality);
    const AutomorphismGroup<Integer>& getAutomorphismGroup();

    mpq_class getMultiplicity();
    mpq_class getVolume();
#ifdef ENFNORMALIZ
    renf_elem_class getRenfVolume();
#endif
    nmz_float getEuclideanVolume();
    mpq_class getVirtualMultiplicity();
    mpq_class getIntegral();
    nmz_float getEuclideanIntegral();

    const pair<HilbertSeries, mpz_class>& getWeightedEhrhartSeries();

    string getPolynomial() const;

    bool inequalities_present;

    bool isPointed();
    bool isInhomogeneous();
    bool isDeg1ExtremeRays();
    bool isDeg1HilbertBasis();
    bool isIntegrallyClosed();
    bool isGorenstein();
    bool isReesPrimary();
    Integer getReesPrimaryMultiplicity();
    const Matrix<Integer>& getOriginalMonoidGeneratorsMatrix();
    const vector<vector<Integer> >& getOriginalMonoidGenerators();
    size_t getNrOriginalMonoidGenerators();

    const Sublattice_Representation<Integer>& getSublattice();
    const HilbertSeries& getHilbertSeries();  // general purpose object
    const HilbertSeries& getEhrhartSeries();  // general purpose object

    // the following 2 methods give information about the last used triangulation
    // if no triangulation was computed so far they return false
    bool isTriangulationNested();
    bool isTriangulationPartial();
    const vector<pair<vector<key_t>, Integer> >& getTriangulation();
    const vector<vector<bool> >& getOpenFacets();
    const vector<pair<vector<key_t>, long> >& getInclusionExclusionData();
    const list<STANLEYDATA<Integer> >& getStanleyDec();
    list<STANLEYDATA_int>& getStanleyDec_mutable();  // allows us to erase the StanleyDec
                                                     // in order to save memeory for weighted Ehrhart

    bool get_verbose();
    void write_cone_output(const string& output_file);

    IntegrationData& getIntData();

    void resetGrading(vector<Integer> lf);

    const Matrix<Integer>& getMatrixConePropertyMatrix(ConeProperty::Enum property);
    const vector<vector<Integer> >& getMatrixConeProperty(ConeProperty::Enum property);

    const Matrix<nmz_float>& getFloatMatrixConePropertyMatrix(ConeProperty::Enum property);
    const vector<vector<nmz_float> >& getFloatMatrixConeProperty(ConeProperty::Enum property);

    vector<Integer> getVectorConeProperty(ConeProperty::Enum property);

    Integer getIntegerConeProperty(ConeProperty::Enum property);

    mpz_class getGMPIntegerConeProperty(ConeProperty::Enum property);

    mpq_class getRationalConeProperty(ConeProperty::Enum property);

    nmz_float getFloatConeProperty(ConeProperty::Enum property);

#ifdef ENFNORMALIZ
    renf_elem_class getFieldElemConeProperty(ConeProperty::Enum property);
#endif

    size_t getMachineIntegerConeProperty(ConeProperty::Enum property);

    bool getBooleanConeProperty(ConeProperty::Enum property);

    nmz_float euclidean_corr_factor();

    //---------------------------------------------------------------------------
    //                          private part
    //---------------------------------------------------------------------------

   private:
    size_t dim;
    bool inhom_input;

    bool keep_convex_hull_data;  // indicates that data computed in Full_Cone and other data are preserved and can be used again
    CONVEXHULLDATA<Integer> ConvHullData;
    bool conversion_done;  // indicates that generators have been converted to inequalities

    // the following matrices store the constraints of the input
    Matrix<Integer> Inequalities;
    Matrix<Integer> AddInequalities;  // for inequalities added later on
    Matrix<Integer> AddGenerators;    // for inequalities added later on
    Matrix<Integer> Equations;
    Matrix<Integer> Congruences;
    // we must register some information about thew input
    bool lattice_ideal_input;
    size_t nr_latt_gen, nr_cone_gen;  // they count matrices in the input

    Sublattice_Representation<Integer> BasisChange;         // always use compose_basis_change() !
    Sublattice_Representation<Integer> BasisChangePointed;  // to the pointed cone
    bool BC_set;
    bool verbose;
    ConeProperties is_Computed;
    // Matrix<Integer> GeneratorsOfToricRing;
    Matrix<Integer> OriginalMonoidGenerators;
    Matrix<Integer> Generators;
    Matrix<Integer> ReferenceGenerators;
    Matrix<Integer> ExtremeRays;         // of the homogenized cone
    Matrix<Integer> ExtremeRaysRecCone;  // of the recession cone, = ExtremeRays in the homogeneous case
    Matrix<nmz_float> VerticesFloat;
    vector<bool> ExtremeRaysIndicator;
    Matrix<Integer> VerticesOfPolyhedron;
    Matrix<Integer> SupportHyperplanes;
    Matrix<nmz_float> SuppHypsFloat;
    Matrix<Integer> ExcludedFaces;
    Matrix<Integer> PreComputedSupportHyperplanes;
    size_t TriangulationSize;
    Integer TriangulationDetSum;
    bool triangulation_is_nested;
    bool triangulation_is_partial;
    vector<pair<vector<key_t>, Integer> > Triangulation;
    vector<vector<bool> > OpenFacets;
    vector<bool> projection_coord_indicator;
    vector<pair<vector<key_t>, long> > InExData;
    list<STANLEYDATA_int> StanleyDec;
    list<STANLEYDATA<Integer> > StanleyDec_export;
    mpq_class multiplicity;
    mpq_class volume;
    nmz_float euclidean_volume;
    nmz_float euclidean_height;  // for volume computations wuth renf_elem_class
#ifdef ENFNORMALIZ
    renf_elem_class renf_volume;
#endif
    mpq_class Integral;
    mpq_class VirtualMultiplicity;
    vector<Integer> WitnessNotIntegrallyClosed;
    vector<Integer> GeneratorOfInterior;
    Matrix<Integer> HilbertBasis;
    Matrix<Integer> HilbertBasisRecCone;
    Matrix<Integer> BasisMaxSubspace;
    Matrix<Integer> ModuleGeneratorsOverOriginalMonoid;
    Matrix<Integer> Deg1Elements;
    HilbertSeries HSeries;
    HilbertSeries EhrSeries;
    IntegrationData IntData;
    vector<Integer> Grading;
    vector<Integer> Dehomogenization;
    vector<Integer> Norm;  // used by v_standardize in the numberfield case
    Integer GradingDenom;
    Integer index;  // the internal index
    Integer unit_group_index;
    size_t number_lattice_points;
    vector<size_t> f_vector;

    vector<dynamic_bitset> Pair;        // for indicator vectors in project-and_lift
    vector<dynamic_bitset> ParaInPair;  // if polytope is a parallelotope
    bool check_parallelotope();
    bool is_parallelotope;

    map<dynamic_bitset, int> FaceLattice;
    vector<dynamic_bitset> SuppHypInd;  // incidemnce vectors of the support hyperplanes

    bool pointed;
    bool inhomogeneous;

    bool input_automorphisms;

    bool polytope_in_input;
    bool gorensetin;

    bool deg1_extreme_rays;
    bool deg1_hilbert_basis;
    bool integrally_closed;
    bool Gorenstein;
    bool rees_primary;
    bool dual_original_generators;  // true means: dual cone has original generators
    Integer ReesPrimaryMultiplicity;
    int affine_dim;         // dimension of polyhedron
    size_t recession_rank;  // rank of recession monoid
    size_t module_rank;     // for the inhomogeneous case
    Matrix<Integer> ModuleGenerators;
    vector<Integer> ClassGroup;

    bool is_approximation;
    Cone* ApproximatedCone;

    Matrix<Integer> WeightsGrad;
    vector<bool> GradAbs;

    bool normalization;  // true if input type normalization is used
    bool general_no_grading_denom;

#ifdef ENFNORMALIZ
    renf_class* Renf;
#endif

    long renf_degree;
    long face_codim_bound;

    // if this is true we allow to change to a smaller integer type in the computation
    bool change_integer_type;

    long autom_codim_vectors;
    long autom_codim_mult;

    Cone<Integer>* IntHullCone;  // cone containing data of integer hull
    Cone<Integer>* SymmCone;     // cone containing symmetrized data
    Cone<Integer>* ProjCone;     // cone containing projection to selected coordinates

    // In cone based algorithms we use the following information
    bool Grading_Is_Coordinate;  // indicates that the grading or dehomogenization is a coordinate
    key_t GradingCoordinate;     // namely this one

    void compose_basis_change(const Sublattice_Representation<Integer>& SR);  // composes SR

    // main input processing
    void process_multi_input(const map<InputType, vector<vector<Integer> > >& multi_input_data);
    void process_multi_input_inner(map<InputType, vector<vector<Integer> > >& multi_input_data);
    void process_multi_input(const map<InputType, vector<vector<mpq_class> > >& multi_input_data);
    void process_multi_input(const map<InputType, vector<vector<nmz_float> > >& multi_input_data);

    void prepare_input_lattice_ideal(map<InputType, vector<vector<Integer> > >& multi_input_data);
    void prepare_input_constraints(const map<InputType, vector<vector<Integer> > >& multi_input_data);
    void prepare_input_generators(map<InputType, vector<vector<Integer> > >& multi_input_data,
                                  Matrix<Integer>& LatticeGenerators);
    template <typename InputNumber>
    void homogenize_input(map<InputType, vector<vector<InputNumber> > >& multi_input_data);
    void check_precomputed_support_hyperplanes();
    void check_excluded_faces();
    bool check_lattice_restrictions_on_generators(bool& cone_sat_cong);

    void check_gens_vs_reference();  // to make sure that newly computed generators agrre with the previously computed

    void setGrading(const vector<Integer>& lf);
    void setWeights();
    void setDehomogenization(const vector<Integer>& lf);
    void checkGrading();
    void checkDehomogenization();
    void check_vanishing_of_grading_and_dehom();
    void process_lattice_data(const Matrix<Integer>& LatticeGenerators, Matrix<Integer>& Congruences, Matrix<Integer>& Equations);

    void try_symmetrization(ConeProperties& ToCompute);
    void try_approximation_or_projection(ConeProperties& ToCompute);

    void try_Hilbert_Series_from_lattice_points(const ConeProperties& ToCompute);
    void make_Hilbert_series_from_pos_and_neg(const vector<num_t>& h_vec_pos, const vector<num_t>& h_vec_neg);

    void make_face_lattice(const ConeProperties& ToCompute);
    void compute_combinatorial_automorphisms(const ConeProperties& ToCompute);
    void compute_euclidean_automorphisms(const ConeProperties& ToCompute);

    AutomorphismGroup<Integer> Automs;

    Matrix<Integer> prepare_input_type_2(const vector<vector<Integer> >& Input);
    Matrix<Integer> prepare_input_type_3(const vector<vector<Integer> >& Input);
    void insert_default_inequalities(Matrix<Integer>& Inequalities);

    /* only used by the constructors */
    void initialize();

    template <typename IntegerFC>
    void compute_full_cone(ConeProperties& ToCompute);

    /* compute the generators using the support hyperplanes */
    void compute_generators(ConeProperties& ToCompute);
    template <typename IntegerFC>
    void compute_generators_inner(ConeProperties& ToCompute);

    /* compute method for the dual_mode, used in compute(mode) */
    void compute_dual(ConeProperties& ToCompute);
    template <typename IntegerFC>
    void compute_dual_inner(ConeProperties& ToCompute);

    void set_implicit_dual_mode(ConeProperties& ToCompute);

    /* extract the data from Full_Cone, this may remove data from Full_Cone!*/
    template <typename IntegerFC>
    void extract_data(Full_Cone<IntegerFC>& FC, ConeProperties& ToCompute);

    template <typename IntegerFC>
    void extract_convex_hull_data(Full_Cone<IntegerFC>& FC, bool primal);
    template <typename IntegerFC>
    void push_convex_hull_data(Full_Cone<IntegerFC>& FC, bool primal);

    template <typename IntegerFC>
    void extract_supphyps(Full_Cone<IntegerFC>& FC, Matrix<Integer>& ret, bool dual = true);
    void extract_supphyps(Full_Cone<Integer>& FC, Matrix<Integer>& ret, bool dual = true);

    void norm_dehomogenization(size_t FC_dim);

    /* set OriginalMonoidGenerators */
    void set_original_monoid_generators(const Matrix<Integer>&);

    /* set ExtremeRays, in inhomogeneous case also VerticesOfPolyhedron and ExtremeRaysRecCone*/
    void set_extreme_rays(const vector<bool>&);

    /* If the Hilbert basis and the original monoid generators are computed,
     * use them to check whether the original monoid is integrally closed. */
    void check_integrally_closed();
    void compute_unit_group_index();
    /* try to find a witness for not integrally closed in the Hilbert basis */
    void find_witness();

    void check_Gorenstein(ConeProperties& ToCompute);

    Integer compute_primary_multiplicity();
    template <typename IntegerFC>
    Integer compute_primary_multiplicity_inner();

    void compute_integer_hull();
    void complete_sublattice_comp(ConeProperties& ToCompute);  // completes the sublattice computations
    void complete_HilbertSeries_comp(ConeProperties& ToCompute);

    void treat_polytope_as_being_hom_defined(ConeProperties ToCompute);

    void compute_integral(ConeProperties& ToCompute);
    void compute_virt_mult(ConeProperties& ToCompute);
    void compute_weighted_Ehrhart(ConeProperties& ToCompute);

    void compute_vertices_float(ConeProperties& ToCompute);
    void compute_supp_hyps_float(ConeProperties& ToCompute);

    void make_StanleyDec_export();

    void NotComputable(string message);  // throws NotComputableException if default_mode = false

    void set_parallelization();

    void handle_dynamic(const ConeProperties& ToCompute);

    template <typename IntegerFC>
    void give_data_of_approximated_cone_to(Full_Cone<IntegerFC>& FC);

    void project_and_lift(const ConeProperties& ToCompute,
                          Matrix<Integer>& Deg1,
                          const Matrix<Integer>& Gens,
                          const Matrix<Integer>& Supps,
                          const Matrix<Integer>& Congs,
                          const vector<Integer> GradingOnPolytope);

    void compute_volume(ConeProperties& ToCompute);

    void try_multiplicity_by_descent(ConeProperties& ToCompute);
    void try_multiplicity_of_para(ConeProperties& ToCompute);

    void compute_projection(ConeProperties& ToCompute);
    void compute_projection_from_gens(const vector<Integer>& GradOrDehom);
    void compute_projection_from_constraints(const vector<Integer>& GradOrDehom, ConeProperties& ToCompute);

    // in order to avoid getRank fromm inside compute
    size_t get_rank_internal();
    const Sublattice_Representation<Integer>& get_sublattice_internal();

    void compute_lattice_points_in_polytope(ConeProperties& ToCompute);
    void prepare_volume_computation(ConeProperties& ToCompute);

    bool set_quality_of_automorphisms(ConeProperties& ToCompute, AutomParam::Quality& quality_of_automorphisms);

    template <typename IntegerFC>
    vector<vector<key_t> > extract_permutations(const vector<vector<key_t> >& FC_Permutations,
                                                Matrix<IntegerFC>& FC_Vectors,
                                                const Matrix<Integer>& ConeVectors,
                                                bool primal,
                                                vector<key_t>& Key);

    vector<vector<key_t> > extract_subsets(const vector<vector<key_t> >& FC_Subsets, size_t max_index, const vector<key_t>& Key);
};

// helpers

template <typename Integer>
vector<vector<Integer> > find_input_matrix(const map<InputType, vector<vector<Integer> > >& multi_input_data,
                                           const InputType type);

template <typename Integer>
void insert_zero_column(vector<vector<Integer> >& mat, size_t col);

template <typename Integer>
void insert_column(vector<vector<Integer> >& mat, size_t col, Integer entry);

// computes approximating lattice simplex using the A_n dissection of the unit cube
// q is a rational vector with the denominator in the FIRST component q[0]
template <typename Integer>
inline void approx_simplex(const vector<Integer>& q, std::list<vector<Integer> >& approx, const long approx_level) {
    // cout << "approximate the point " << q;
    long dim = q.size();
    long l = approx_level;
    // if (approx_level>q[0]) l=q[0]; // approximating on level q[0](=grading) is the best we can do
    // TODO in this case, skip the rest and just approximate on q[0]
    Matrix<Integer> quot = Matrix<Integer>(l, dim);
    Matrix<Integer> remain = Matrix<Integer>(l, dim);
    for (long j = 0; j < approx_level; j++) {
        for (long i = 0; i < dim; ++i) {
            quot[j][i] = (q[i] * (j + 1)) / q[0];  // write q[i]=quot*q[0]+remain
            // quot[j][0] = 1;
            remain[j][i] = (q[i] * (j + 1)) % q[0];  // with 0 <= remain < q[0]
            if (remain[j][i] < 0) {
                remain[j][i] += q[0];
                quot[j][i]--;
            }
        }
        v_make_prime(quot[j]);
        remain[j][0] = q[0];  // helps to avoid special treatment of i=0
    }
    // choose best level
    // cout << "this is the qout matrix" << endl;
    // quot.pretty_print(cout);
    // cout << "this is the remain matrix" << endl;
    // remain.pretty_print(cout);
    long best_level = l - 1;
    vector<long> nr_zeros(l);
    for (long j = l - 1; j >= 0; j--) {
        for (long i = 0; i < dim; ++i) {
            if (remain[j][i] == 0)
                nr_zeros[j]++;
        }
        if (nr_zeros[j] > nr_zeros[best_level])
            best_level = j;
    }
    // cout << "the best level is " << (best_level+1) << endl;
    // now we proceed as before
    vector<pair<Integer, size_t> > best_remain(dim);
    for (long i = 0; i < dim; i++) {
        best_remain[i].first = remain[best_level][i];
        best_remain[i].second = i;  // after sorting we must lnow where elements come from
    }

    sort(best_remain.begin(), best_remain.end());
    reverse(best_remain.begin(), best_remain.end());  // we sort remain into descending order

    /*for(long i=0;i<dim;++i){
        cout << remain[i].first << " " << remain[i].second << endl;
    } */

    for (long i = 1; i < dim; ++i) {
        if (best_remain[i].first < best_remain[i - 1].first) {
            approx.push_back(quot[best_level]);
            // cout << "add the point " << quot[best_level];
            // cout << i << " + " << remain[i].first << " + " << quot << endl;
        }
        quot[best_level][best_remain[i].second]++;
    }
    if (best_remain[dim - 1].first > 0) {
        // cout << "E " << quot << endl;
        approx.push_back(quot[best_level]);
        // cout << "add the point " << quot[best_level];
    }
}

#ifdef ENFNORMALIZ
template <>
inline void approx_simplex(const vector<renf_elem_class>& q,
                           std::list<vector<renf_elem_class> >& approx,
                           const long approx_level) {
    assert(false);
}
#endif

// Doubly templated functions

template <typename Integer>
template <typename T>
void Cone<Integer>::modifyCone(InputType input_type, const vector<vector<T> >& Input) {
    // convert to a map
    map<InputType, vector<vector<T> > > multi_add_input;
    multi_add_input[input_type] = Input;
    modifyCone(multi_add_input);
}
//---------------------------------------------------------------------------

template <typename Integer>
template <typename T>
void Cone<Integer>::modifyCone(InputType input_type, const Matrix<T>& Input) {
    // convert to a map
    map<InputType, vector<vector<T> > > multi_add_input;
    multi_add_input[input_type] = Input.get_elements();
    modifyCone(multi_add_input);
}

}  // end namespace libnormaliz

#endif /* LIBNORMALIZ_CONE_H_ */
