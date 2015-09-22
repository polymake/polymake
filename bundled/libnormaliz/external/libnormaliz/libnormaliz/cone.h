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

#include <vector>
#include <map>
#include <utility> //for pair
//#include <boost/dynamic_bitset.hpp>

#include "libnormaliz/libnormaliz.h"
#include "libnormaliz/cone_property.h"
#include "libnormaliz/sublattice_representation.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/HilbertSeries.h"

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

//---------------------------------------------------------------------------
//                          give additional data
//---------------------------------------------------------------------------

    /* Sets if the Cone prints verbose output.
     * The default value for the Cone is the global verbose.
     * returns the old value
     */
    bool setVerbose (bool v);

    void deactivateChangeOfPrecision();

    /* Sets the linear form which is used to grade.
     * It has to be an N-grading, i.e. all generators must have a value >=1.
     * If it is not, a subclass of NormalizException will be thrown at the
     * time of detection which can be in this method or later!
     * It will delete all data from the cone that depend on the grading!
     */
    void setGrading (const vector<Integer>& lf);
    void setDehomogenization (const vector<Integer>& lf);


//---------------------------------------------------------------------------
//                           make computations
//---------------------------------------------------------------------------

    // return what was NOT computed
    // ConeProperties compute(ComputationMode mode = Mode::hilbertBasisSeries); //default: everything
    ConeProperties compute(ConeProperties ToCompute);
    // special case for up to 3 CPs
    ConeProperties compute(ConeProperty::Enum);
    ConeProperties compute(ConeProperty::Enum, ConeProperty::Enum);
    ConeProperties compute(ConeProperty::Enum, ConeProperty::Enum, ConeProperty::Enum);

//---------------------------------------------------------------------------
//                         check what is computed
//---------------------------------------------------------------------------

    bool isComputed(ConeProperty::Enum prop) const;
    //returns true, when ALL properties in CheckComputed are computed
    bool isComputed(ConeProperties CheckComputed) const;

//---------------------------------------------------------------------------
//   get the results, these methods will start a computation if necessary
//   throws an NotComputableException if not succesful
//---------------------------------------------------------------------------

    // dimension and rank invariants
    size_t getEmbeddingDim() const { return dim; };   // is always known
    size_t getRank();                           // depends on ExtremeRays
    Integer getIndex();                         // depends on OriginalMonoidGenerators
    // only for inhomogeneous case:
    size_t getRecessionRank();
    long getAffineDim();
    size_t getModuleRank();

    const Matrix<Integer>& getGeneratorsMatrix();
    const vector< vector<Integer> >& getGenerators();
    size_t getNrGenerators();

    const Matrix<Integer>& getExtremeRaysMatrix();
    const vector< vector<Integer> >& getExtremeRays();
    size_t getNrExtremeRays();

    const Matrix<Integer>& getVerticesOfPolyhedronMatrix();
    const vector< vector<Integer> >& getVerticesOfPolyhedron();
    size_t getNrVerticesOfPolyhedron();

    const Matrix<Integer>& getSupportHyperplanesMatrix();
    const vector< vector<Integer> >& getSupportHyperplanes();
    size_t getNrSupportHyperplanes();

    // depends on the ConeProperty::s SupportHyperplanes and Sublattice
    map< InputType, vector< vector<Integer> > > getConstraints();

    const Matrix<Integer>& getExcludedFacesMatrix();
    const vector< vector<Integer> >& getExcludedFaces();
    size_t getNrExcludedFaces();

    size_t getTriangulationSize();
    Integer getTriangulationDetSum();

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

    bool isPointed();
    bool isInhomogeneous();
    bool isDeg1ExtremeRays();
    bool isDeg1HilbertBasis();
    bool isIntegrallyClosed();
    bool isReesPrimary();
    Integer getReesPrimaryMultiplicity();
    Integer getShift();
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
    const vector< pair<vector<key_t>, long> >& getInclusionExclusionData();
    const list< STANLEYDATA<Integer> >& getStanleyDec();

//---------------------------------------------------------------------------
//                          private part
//---------------------------------------------------------------------------

private:
    size_t dim;

    Sublattice_Representation<Integer> BasisChange;  //always use compose_basis_change() !
    bool BC_set;
    bool verbose;
    ConeProperties is_Computed;
    // Matrix<Integer> GeneratorsOfToricRing;
    Matrix<Integer> OriginalMonoidGenerators;
    Matrix<Integer> Generators;
    Matrix<Integer> ExtremeRays;
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
    vector< pair<vector<key_t>, long> > InExData;
    list< STANLEYDATA<Integer> > StanleyDec;
    mpq_class multiplicity;
    Matrix<Integer> HilbertBasis;
    Matrix<Integer> ModuleGeneratorsOverOriginalMonoid;
    Matrix<Integer> Deg1Elements;
    HilbertSeries HSeries;
    vector<Integer> Grading;
    vector<Integer> Dehomogenization;
    Integer GradingDenom;
    Integer index;

    bool pointed;
    bool inhomogeneous;
    bool deg1_extreme_rays;
    bool deg1_hilbert_basis;
    bool integrally_closed;
    bool rees_primary;
    Integer ReesPrimaryMultiplicity;
    Integer shift; // needed in the inhomogeneous case to make degrees positive
    int affine_dim; //dimension of polyhedron
    size_t recession_rank; // rank of recession monoid
    size_t module_rank; // for the inhomogeneous case
    Matrix<Integer> ModuleGenerators;
    vector<Integer> ClassGroup;

    Matrix<Integer> WeightsGrad;
    vector<bool> GradAbs;

    bool no_lattice_restriction; // true if cine generators are known to be in the relevant lattice
    bool normalization; // true if input type normalization is used

    // if this is true we allow to change to a smaller integer type in the computation
    bool change_integer_type;

    void compose_basis_change(const Sublattice_Representation<Integer>& SR); // composes SR

    // main input processing
    void process_multi_input(const map< InputType, vector< vector<Integer> > >& multi_input_data);
    void prepare_input_lattice_ideal(map< InputType, vector< vector<Integer> > >& multi_input_data);
    void prepare_input_constraints(const map< InputType, vector< vector<Integer> > >& multi_input_data,
            Matrix<Integer>& equations, Matrix<Integer>& congruence, Matrix<Integer>& Inequalities);
    void prepare_input_generators(map< InputType, vector< vector<Integer> > >& multi_input_data,
                     Matrix<Integer>& LatticeGenerators);
    void homogenize_input(map< InputType, vector< vector<Integer> > >& multi_input_data);
    void check_precomputed_support_hyperplanes();
    void check_excluded_faces();
    void process_lattice_data(const Matrix<Integer>& LatticeGenerators, Matrix<Integer>& Congruences, Matrix<Integer>& Equations);

    Matrix<Integer> prepare_input_type_2(const vector< vector<Integer> >& Input);
    Matrix<Integer> prepare_input_type_3(const vector< vector<Integer> >& Input);
    void prepare_input_type_4(Matrix<Integer>& Inequalities);

    /* only used by the constructors */
    void initialize();

    template<typename IntegerFC>
    void compute_inner(ConeProperties& ToCompute);

    /* compute the generators using the support hyperplanes */
    void compute_generators();
    template<typename IntegerFC>
    void compute_generators_inner();

    /* compute method for the dual_mode, used in compute(mode) */
    void compute_dual(ConeProperties& ToCompute);
    template<typename IntegerFC>
    void compute_dual_inner(ConeProperties& ToCompute);

    /* extract the data from Full_Cone, this may remove data from Full_Cone!*/
    template<typename IntegerFC>
    void extract_data(Full_Cone<IntegerFC>& FC);

    /* set OriginalMonoidGenerators */
    void set_original_monoid_generators(const Matrix<Integer>&);

    /* set ExtremeRays, in inhomogeneous case also VerticesOfPolyhedron */
    void set_extreme_rays(const vector<bool>&);

    void check_integrally_closed();

    /* set this object to the zero cone */
    void set_zero_cone();

    Integer compute_primary_multiplicity();
    template<typename IntegerFC>
    Integer compute_primary_multiplicity_inner();

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
