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

#ifndef CONE_H_
#define CONE_H_

#include <sys/stat.h>
#include <vector>
#include <map>
#include <utility> //for pair
#include <boost/dynamic_bitset.hpp>

#include <libnormaliz/libnormaliz.h>
#include <libnormaliz/cone_property.h>
#include <libnormaliz/sublattice_representation.h>
#include <libnormaliz/matrix.h>
#include <libnormaliz/HilbertSeries.h>

namespace libnormaliz {
using std::vector;
using std::map;
using std::pair;

template<typename Integer> class Full_Cone;
//template<typename Integer> class Matrix;

// type for simplex, short in contrast to class Simplex
template<typename Integer> struct SHORTSIMPLEX {
    vector<key_t> key;                // full key of simplex
    Integer height;                   // height of last vertex over opposite facet
    Integer vol;                      // volume if computed, 0 else
    vector<bool> Excluded;           // for disjoint decomposition of cone
                                      // true in position i indictate sthat the facet 
                                      // opposite of generator i must be excluded
};

template<typename Integer>
bool compareKeys(const SHORTSIMPLEX<Integer>& A, const SHORTSIMPLEX<Integer>& B){

    return(A.key < B.key);
}

struct STANLEYDATA_int { // for internal use
    vector<key_t> key;
    Matrix<long> offsets;
    vector<long> degrees; // degrees and classNr are used in nmz_integral.cpp
    size_t classNr;  // number of class of this simplicial cone
};

template<typename Integer> struct STANLEYDATA {
    vector<key_t> key;
    Matrix<Integer> offsets;
};

template<typename Integer>
class Cone {

//---------------------------------------------------------------------------
//                               public methods
//---------------------------------------------------------------------------
public:

//---------------------------------------------------------------------------
//                    Constructors, they preprocess the input
//---------------------------------------------------------------------------
    
    Cone(); //default constructor

    /* give up to 3 matrices as input
     * the types must be pairwise different
     */
    Cone(InputType type, const vector< vector<Integer> >& input_data);

    Cone(InputType type1, const vector< vector<Integer> >& input_data1,
         InputType type2, const vector< vector<Integer> >& input_data2);

    Cone(InputType type1, const vector< vector<Integer> >& input_data1,
         InputType type2, const vector< vector<Integer> >& input_data2,
         InputType type3, const vector< vector<Integer> >& input_data3);

    /* give multiple input */
    Cone(const map< InputType , vector< vector<Integer> > >& multi_input_data);
    
//-----------------------------------------------------------------------------
// the same for mpq_class
    
    Cone(InputType type, const vector< vector<mpq_class> >& input_data);

    Cone(InputType type1, const vector< vector<mpq_class> >& input_data1,
         InputType type2, const vector< vector<mpq_class> >& input_data2);

    Cone(InputType type1, const vector< vector<mpq_class> >& input_data1,
         InputType type2, const vector< vector<mpq_class> >& input_data2,
         InputType type3, const vector< vector<mpq_class> >& input_data3);

    /* give multiple input */
    Cone(const map< InputType , vector< vector<mpq_class> > >& multi_input_data);
    
//-----------------------------------------------------------------------------
// the same for nmz_float
    
    Cone(InputType type, const vector< vector<nmz_float> >& input_data);

    Cone(InputType type1, const vector< vector<nmz_float> >& input_data1,
         InputType type2, const vector< vector<nmz_float> >& input_data2);

    Cone(InputType type1, const vector< vector<nmz_float> >& input_data1,
         InputType type2, const vector< vector<nmz_float> >& input_data2,
         InputType type3, const vector< vector<nmz_float> >& input_data3);

    /* give multiple input */
    Cone(const map< InputType , vector< vector<nmz_float> > >& multi_input_data);

//-----------------------------------------------------------------------------
// Now with Matrix
    
    Cone(InputType type, const Matrix<Integer>& input_data);

    Cone(InputType type1, const Matrix<Integer>& input_data1,
         InputType type2, const Matrix<Integer>& input_data2);

    Cone(InputType type1, const Matrix<Integer>& input_data1,
         InputType type2, const Matrix<Integer>& input_data2,
         InputType type3, const Matrix<Integer>& input_data3);

    /* give multiple input */
    Cone(const map< InputType , Matrix<Integer> >& multi_input_data);
    
//-----------------------------------------------------------------------------
// Now with Matrix and mpq_class
    
    Cone(InputType type, const Matrix<mpq_class>& input_data);

    Cone(InputType type1, const Matrix<mpq_class>& input_data1,
         InputType type2, const Matrix<mpq_class>& input_data2);

    Cone(InputType type1, const Matrix<mpq_class>& input_data1,
         InputType type2, const Matrix<mpq_class>& input_data2,
         InputType type3, const Matrix<mpq_class>& input_data3);

    /* give multiple input */
    Cone(const map< InputType , Matrix<mpq_class> >& multi_input_data);
    
//-----------------------------------------------------------------------------
// Now with Matrix and nmz_float
    
    Cone(InputType type, const Matrix<nmz_float>& input_data);

    Cone(InputType type1, const Matrix<nmz_float>& input_data1,
         InputType type2, const Matrix<nmz_float>& input_data2);

    Cone(InputType type1, const Matrix<nmz_float>& input_data1,
         InputType type2, const Matrix<nmz_float>& input_data2,
         InputType type3, const Matrix<nmz_float>& input_data3);

    /* give multiple input */
    Cone(const map< InputType , Matrix<nmz_float> >& multi_input_data);
    
    
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
    bool setVerbose (bool v);

    void deactivateChangeOfPrecision();

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
    //returns true, when ALL properties in CheckComputed are computed
    bool isComputed(ConeProperties CheckComputed) const;
    
    void resetComputed(ConeProperty::Enum prop);

//---------------------------------------------------------------------------
//   get the results, these methods will start a computation if necessary
//   throws an NotComputableException if not succesful
//---------------------------------------------------------------------------

    // dimension and rank invariants
    size_t getEmbeddingDim() const { return dim; };   // is always known
    size_t getRank();                           // depends on ExtremeRays
    Integer getIndex(); // depends on OriginalMonoidGenerators
    Integer getInternalIndex(); // = getIndex()
    Integer getUnitGroupIndex(); // ditto
    // only for inhomogeneous case:
    size_t getRecessionRank();
    long getAffineDim();
    size_t getModuleRank();
    
    Cone<Integer>& getIntegerHullCone() const;
    Cone<Integer>& getSymmetrizedCone() const;
    Cone<Integer>& getProjectCone() const;

    const Matrix<Integer>& getGeneratorsMatrix();
    const vector< vector<Integer> >& getGenerators();
    size_t getNrGenerators();

    const Matrix<Integer>& getExtremeRaysMatrix();
    const vector< vector<Integer> >& getExtremeRays();
    size_t getNrExtremeRays();
    
    const Matrix<nmz_float>& getVerticesFloatMatrix();
    const vector< vector<nmz_float> >& getVerticesFloat();
    size_t getNrVerticesFloat();

    const Matrix<Integer>& getVerticesOfPolyhedronMatrix();
    const vector< vector<Integer> >& getVerticesOfPolyhedron();
    size_t getNrVerticesOfPolyhedron();

    const Matrix<Integer>& getSupportHyperplanesMatrix();
    const vector< vector<Integer> >& getSupportHyperplanes();
    size_t getNrSupportHyperplanes();
    
    const Matrix<Integer>& getMaximalSubspaceMatrix();
    const vector< vector<Integer> >& getMaximalSubspace();
    size_t getDimMaximalSubspace();

    // depends on the ConeProperty::s SupportHyperplanes and Sublattice
    map< InputType, vector< vector<Integer> > > getConstraints();

    const Matrix<Integer>& getExcludedFacesMatrix();
    const vector< vector<Integer> >& getExcludedFaces();
    size_t getNrExcludedFaces();

    size_t getTriangulationSize();
    Integer getTriangulationDetSum();

    vector<Integer> getWitnessNotIntegrallyClosed();
    vector<Integer> getGeneratorOfInterior();

    const Matrix<Integer>& getHilbertBasisMatrix();
    const vector< vector<Integer> >& getHilbertBasis();
    size_t getNrHilbertBasis();

    const Matrix<Integer>& getModuleGeneratorsOverOriginalMonoidMatrix();
    const vector< vector<Integer> >& getModuleGeneratorsOverOriginalMonoid();
    size_t getNrModuleGeneratorsOverOriginalMonoid();

    const Matrix<Integer>& getModuleGeneratorsMatrix();
    const vector< vector<Integer> >& getModuleGenerators();
    size_t getNrModuleGenerators();

    const Matrix<Integer>& getDeg1ElementsMatrix();
    const vector< vector<Integer> >& getDeg1Elements();
    size_t getNrDeg1Elements();

    // the actual grading is Grading/GradingDenom
    vector<Integer> getGrading();
    Integer getGradingDenom();

    vector<Integer> getDehomogenization();

    vector<Integer> getClassGroup();

    mpq_class getMultiplicity();
    mpq_class getVolume();
    nmz_float getEuclideanVolume();
    mpq_class getVirtualMultiplicity();
    mpq_class getIntegral();
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
    const vector< vector<Integer> >& getOriginalMonoidGenerators();
    size_t getNrOriginalMonoidGenerators();

    const Sublattice_Representation<Integer>& getSublattice();
    const HilbertSeries& getHilbertSeries(); //general purpose object

    // the following 2 methods give information about the last used triangulation
    // if no triangulation was computed so far they return false
    bool isTriangulationNested();
    bool isTriangulationPartial();
    const vector< pair<vector<key_t>, Integer> >& getTriangulation();
    const vector< vector<bool> >& getOpenFacets();
    const vector< pair<vector<key_t>, long> >& getInclusionExclusionData();
    const list< STANLEYDATA<Integer> >& getStanleyDec();
    list< STANLEYDATA_int >& getStanleyDec_mutable(); //allows us to erase the StanleyDec
                             // in order to save memeory for weighted Ehrhart
    
    void set_project(string name);
    void set_nmz_call(const string& path);
    void set_output_dir(string name);
    
    void setPolynomial(string poly);
    void setNrCoeffQuasiPol(long nr_coeff);
    void setExpansionDegree(long degree);
    
    bool get_verbose ();
    
    IntegrationData& getIntData();

//---------------------------------------------------------------------------
//                          private part
//---------------------------------------------------------------------------

private:
    
    string project;
    string output_dir;
    string nmz_call;
    size_t dim;
    
    // the following three matrices store the constraints of the input
    Matrix<Integer> Inequalities;
    Matrix<Integer> Equations;
    Matrix<Integer> Congruences;
    // we must register some information about thew input
    bool lattice_ideal_input;
    size_t nr_latt_gen, nr_cone_gen; // they count matrices in the input 

    Sublattice_Representation<Integer> BasisChange;  //always use compose_basis_change() !
    Sublattice_Representation<Integer> BasisChangePointed; // to the pointed cone
    bool BC_set;
    bool verbose;
    ConeProperties is_Computed;
    // Matrix<Integer> GeneratorsOfToricRing;
    Matrix<Integer> OriginalMonoidGenerators;
    Matrix<Integer> Generators;
    Matrix<Integer> ExtremeRays;
    Matrix<nmz_float> VerticesFloat;
    vector<bool> ExtremeRaysIndicator;
    Matrix<Integer> VerticesOfPolyhedron;
    Matrix<Integer> SupportHyperplanes;
    Matrix<Integer> ExcludedFaces;
    Matrix<Integer> PreComputedSupportHyperplanes;
    size_t TriangulationSize;
    Integer TriangulationDetSum;
    bool triangulation_is_nested;
    bool triangulation_is_partial;
    vector< pair<vector<key_t>, Integer> > Triangulation;
    vector<vector<bool> > OpenFacets;
    vector<bool> projection_coord_indicator;
    vector< pair<vector<key_t>, long> > InExData;
    list< STANLEYDATA_int > StanleyDec;
    list< STANLEYDATA<Integer> > StanleyDec_export;
    mpq_class multiplicity;
    mpq_class volume;
    nmz_float euclidean_volume;
    mpq_class Integral;
    mpq_class VirtualMultiplicity;
    vector<Integer> WitnessNotIntegrallyClosed;
    vector<Integer> GeneratorOfInterior;
    Matrix<Integer> HilbertBasis;
    Matrix<Integer> BasisMaxSubspace;
    Matrix<Integer> ModuleGeneratorsOverOriginalMonoid;
    Matrix<Integer> Deg1Elements;
    HilbertSeries HSeries;
    IntegrationData IntData;
    vector<Integer> Grading;
    vector<Integer> Dehomogenization;
    Integer GradingDenom;
    Integer index;  // the internal index
    Integer unit_group_index;
    
    vector<boost::dynamic_bitset<> > Pair; // for indicator vectors in project-and_lift
    vector<boost::dynamic_bitset<> > ParaInPair; // if polytope is a parallelotope
    bool check_parallelotope();

    bool pointed;
    bool inhomogeneous;
    bool gorensetin;
    bool deg1_extreme_rays;
    bool deg1_hilbert_basis;
    bool integrally_closed;
    bool Gorenstein;
    bool rees_primary;
    Integer ReesPrimaryMultiplicity;
    int affine_dim; //dimension of polyhedron
    size_t recession_rank; // rank of recession monoid
    size_t module_rank; // for the inhomogeneous case
    Matrix<Integer> ModuleGenerators;
    vector<Integer> ClassGroup;
    
    bool is_approximation;
    Cone* ApproximatedCone;

    Matrix<Integer> WeightsGrad;
    vector<bool> GradAbs;

    bool no_lattice_restriction; // true if cine generators are known to be in the relevant lattice
    bool normalization; // true if input type normalization is used

    // if this is true we allow to change to a smaller integer type in the computation
    bool change_integer_type;
    
    Cone<Integer>* IntHullCone; // cone containing data of integer hull
    Cone<Integer>* SymmCone;    // cone containing symmetrized data
    Cone<Integer>* ProjCone;    // cone containing projection to selected coordinates 
    
    // In cone based algorithms we use the following information
    bool Grading_Is_Coordinate; // indicates that the grading or dehomogenization is a coordinate
    key_t GradingCoordinate;  // namely this one

    void compose_basis_change(const Sublattice_Representation<Integer>& SR); // composes SR

    // main input processing
    void process_multi_input(const map< InputType, vector< vector<Integer> > >& multi_input_data);
    void process_multi_input_inner(map< InputType, vector< vector<Integer> > >& multi_input_data);
    void process_multi_input(const map< InputType, vector< vector<mpq_class> > >& multi_input_data);
    void process_multi_input(const map< InputType, vector< vector<nmz_float> > >& multi_input_data);
    
    void prepare_input_lattice_ideal(map< InputType, vector< vector<Integer> > >& multi_input_data);
    void prepare_input_constraints(const map< InputType, vector< vector<Integer> > >& multi_input_data);
    void prepare_input_generators(map< InputType, vector< vector<Integer> > >& multi_input_data,
                     Matrix<Integer>& LatticeGenerators);
    void homogenize_input(map< InputType, vector< vector<Integer> > >& multi_input_data);
    void check_precomputed_support_hyperplanes();
    void check_excluded_faces();
    
    void setGrading (const vector<Integer>& lf);
    void setWeights ();
    void setDehomogenization (const vector<Integer>& lf);
    void checkGrading();
    void checkDehomogenization();
    void check_vanishing_of_grading_and_dehom();
    void process_lattice_data(const Matrix<Integer>& LatticeGenerators, Matrix<Integer>& Congruences, Matrix<Integer>& Equations);
    
    void try_symmetrization(ConeProperties& ToCompute);
    void try_approximation_or_projection (ConeProperties& ToCompute);

    Matrix<Integer> prepare_input_type_2(const vector< vector<Integer> >& Input);
    Matrix<Integer> prepare_input_type_3(const vector< vector<Integer> >& Input);
    void prepare_input_type_4(Matrix<Integer>& Inequalities);

    /* only used by the constructors */
    void initialize();

    template<typename IntegerFC>
    void compute_full_cone(ConeProperties& ToCompute);

    /* compute the generators using the support hyperplanes */
    void compute_generators();
    template<typename IntegerFC>
    void compute_generators_inner();

    /* compute method for the dual_mode, used in compute(mode) */
    void compute_dual(ConeProperties& ToCompute);
    template<typename IntegerFC>
    void compute_dual_inner(ConeProperties& ToCompute);
    
    void set_implicit_dual_mode(ConeProperties& ToCompute);

    /* extract the data from Full_Cone, this may remove data from Full_Cone!*/
    template<typename IntegerFC>
    void extract_data(Full_Cone<IntegerFC>& FC);
    template<typename IntegerFC>
    void extract_supphyps(Full_Cone<IntegerFC>& FC);
    
    void extract_supphyps(Full_Cone<Integer>& FC);


    /* set OriginalMonoidGenerators */
    void set_original_monoid_generators(const Matrix<Integer>&);

    /* set ExtremeRays, in inhomogeneous case also VerticesOfPolyhedron */
    void set_extreme_rays(const vector<bool>&);

    /* If the Hilbert basis and the original monoid generators are computed,
     * use them to check whether the original monoid is integrally closed. */
    void check_integrally_closed();
    void compute_unit_group_index();
    /* try to find a witness for not integrally closed in the Hilbert basis */
    void find_witness();
    
    void check_Gorenstein (ConeProperties&  ToCompute);

    Integer compute_primary_multiplicity();
    template<typename IntegerFC>
    Integer compute_primary_multiplicity_inner();
    
    void compute_integer_hull();
    void complete_sublattice_comp(ConeProperties& ToCompute); // completes the sublattice computations
    void complete_HilbertSeries_comp(ConeProperties& ToCompute);
    
    void compute_integral (ConeProperties& ToCompute);
    void compute_virt_mult (ConeProperties& ToCompute);
    void compute_weighted_Ehrhart(ConeProperties& ToCompute);
    
    void compute_vertices_float(ConeProperties& ToCompute);
    
    void make_StanleyDec_export();
    
    void NotComputable (string message); // throws NotComputableException if default_mode = false
    
    void set_parallelization();
    
    template<typename IntegerFC>
    void give_data_of_approximated_cone_to(Full_Cone<IntegerFC>& FC);
    
    void project_and_lift(ConeProperties& ToCompute, Matrix<Integer>& Deg1, const Matrix<Integer>& Gens, Matrix<Integer>& Supps, bool float_projection);

    void compute_volume(ConeProperties& ToCompute);
    void compute_euclidean_volume(const vector<Integer>& Grad);
    
    void compute_projection(ConeProperties& ToCompute);
    void compute_projection_from_gens(const vector<Integer>& GradOrDehom);
    void compute_projection_from_constraints(const vector<Integer>& GradOrDehom);
 
    //in order to avoid getRank fromm inside compute
    size_t get_rank_internal();
    const Sublattice_Representation<Integer>& get_sublattice_internal();
};

// helpers

template<typename Integer>
vector<vector<Integer> > find_input_matrix(const map< InputType, vector< vector<Integer> > >& multi_input_data,
                               const InputType type);

template<typename Integer>
void insert_zero_column(vector< vector<Integer> >& mat, size_t col);

template<typename Integer>
void insert_column(vector< vector<Integer> >& mat, size_t col, Integer entry);


}  //end namespace libnormaliz

#endif /* CONE_H_ */
