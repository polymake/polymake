/*
 * Normaliz
 * Copyright (C) 2007-2022  W. Bruns, B. Ichim, Ch. Soeger, U. v. d. Ohe
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#ifndef LIBNORMALIZ_CONE_PROPERTY_H_
#define LIBNORMALIZ_CONE_PROPERTY_H_

#include <bitset>
#include <ostream>

namespace libnormaliz {

/****************************************************************************
**
**  'START_ENUM_RANGE' and 'END_ENUM_RANGE' simplify creating "ranges" of
**  enum variables.
**
**  Usage example:
**    enum {
**      START_ENUM_RANGE(FIRST),
**        FOO,
**        BAR,
**      END_ENUM_RANGE(LAST)
**    };
**  is essentially equivalent to
**    enum {
**      FIRST,
**        FOO = FIRST,
**        BAR,
**      LAST = BAR
**    };
**  Note that if we add a value into the range after 'BAR', we must adjust
**  the definition of 'LAST', which is easy to forget. Also, reordering enum
**  values may require extra work. With the range macros, all of this is
**  taken care of automatically.
*/
#define START_ENUM_RANGE(id) id, _##id##_post = id - 1
#define END_ENUM_RANGE(id) _##id##_pre, id = _##id##_pre - 1

/* An enumeration of things, that can be computed for a cone.
 * The namespace prevents interfering with other names.
 * Remember to change also the string conversion if you change this enum!
 */
namespace ConeProperty {
enum Enum {
    // matrix valued
    START_ENUM_RANGE(FIRST_MATRIX),
    ExtremeRays,
    VerticesOfPolyhedron,
    SupportHyperplanes,
    HilbertBasis,
    ModuleGenerators,
    Deg1Elements,
    LatticePoints,
    ModuleGeneratorsOverOriginalMonoid,
    ExcludedFaces,
    OriginalMonoidGenerators,
    MaximalSubspace,
    Equations,
    Congruences,
    END_ENUM_RANGE(LAST_MATRIX),

    START_ENUM_RANGE(FIRST_MATRIX_FLOAT),
    ExtremeRaysFloat,
    SuppHypsFloat,
    VerticesFloat,
    END_ENUM_RANGE(LAST_MATRIX_FLOAT),

    // vector valued
    START_ENUM_RANGE(FIRST_VECTOR),
    Grading,
    Dehomogenization,
    WitnessNotIntegrallyClosed,
    GeneratorOfInterior,
    CoveringFace,
    AxesScaling,
    END_ENUM_RANGE(LAST_VECTOR),

    // integer valued
    START_ENUM_RANGE(FIRST_INTEGER),
    TriangulationDetSum,
    ReesPrimaryMultiplicity,
    GradingDenom,
    UnitGroupIndex,
    InternalIndex,
    END_ENUM_RANGE(LAST_INTEGER),

    START_ENUM_RANGE(FIRST_GMP_INTEGER),
    ExternalIndex = FIRST_GMP_INTEGER,
    END_ENUM_RANGE(LAST_GMP_INTEGER),

    // rational valued
    START_ENUM_RANGE(FIRST_RATIONAL),
    Multiplicity,
    Volume,
    Integral,
    VirtualMultiplicity,
    END_ENUM_RANGE(LAST_RATIONAL),

    // field valued
    START_ENUM_RANGE(FIRST_FIELD_ELEM),
    RenfVolume,
    END_ENUM_RANGE(LAST_FIELD_ELEM),

    // floating point valued
    START_ENUM_RANGE(FIRST_FLOAT),
    EuclideanVolume,
    EuclideanIntegral,
    END_ENUM_RANGE(LAST_FLOAT),

    // dimensions and cardinalities
    START_ENUM_RANGE(FIRST_MACHINE_INTEGER),
    TriangulationSize,
    NumberLatticePoints,
    RecessionRank,
    AffineDim,
    ModuleRank,
    Rank,
    EmbeddingDim,
    END_ENUM_RANGE(LAST_MACHINE_INTEGER),

    // boolean valued
    START_ENUM_RANGE(FIRST_BOOLEAN),
    IsPointed,
    IsDeg1ExtremeRays,
    IsDeg1HilbertBasis,
    IsIntegrallyClosed,
    IsReesPrimary,
    IsInhomogeneous,
    IsGorenstein,
    IsEmptySemiOpen,
    //
    // checking properties of already computed data
    // (cannot be used as a computation goal)
    //
    IsTriangulationNested,
    IsTriangulationPartial,
    END_ENUM_RANGE(LAST_BOOLEAN),

    // complex structures
    START_ENUM_RANGE(FIRST_COMPLEX_STRUCTURE),
    Triangulation,
    UnimodularTriangulation,
    LatticePointTriangulation,
    AllGeneratorsTriangulation,
    PlacingTriangulation,
    PullingTriangulation,
    StanleyDec,
    InclusionExclusionData,
    IntegerHull,
    ProjectCone,
    ConeDecomposition,
    //
    Automorphisms,
    CombinatorialAutomorphisms,
    RationalAutomorphisms,
    EuclideanAutomorphisms,
    AmbientAutomorphisms,
    InputAutomorphisms,
    //
    HilbertSeries,
    HilbertQuasiPolynomial,
    EhrhartSeries,
    EhrhartQuasiPolynomial,
    WeightedEhrhartSeries,
    WeightedEhrhartQuasiPolynomial,
    //
    FaceLattice,
    DualFaceLattice,
    FVector,
    DualFVector,
    Incidence,
    DualIncidence,
    Sublattice,
    //
    ClassGroup,
    END_ENUM_RANGE(LAST_COMPLEX_STRUCTURE),

    //
    // integer type for computations
    //
    START_ENUM_RANGE(FIRST_PROPERTY),
    BigInt,
    //
    // algorithmic variants
    //
    DefaultMode,
    Approximate,
    BottomDecomposition,
    NoBottomDec,
    DualMode,
    PrimalMode,
    Projection,
    ProjectionFloat,
    NoProjection,
    Symmetrize,
    NoSymmetrization,
    NoSubdivision,
    NoNestedTri,  // synonym for NoSubdivision
    KeepOrder,
    HSOP,
    NoPeriodBound,
    NoLLL,
    NoRelax,
    Descent,
    NoDescent,
    NoGradingDenom,
    GradingIsPositive,
    ExploitAutomsVectors,
    ExploitIsosMult,
    StrictIsoTypeCheck,
    SignedDec,
    NoSignedDec,
    FixedPrecision,
    DistributedComp,
    //
    Dynamic,
    Static,
    //
    WritePreComp,
    //
    END_ENUM_RANGE(LAST_PROPERTY),
    //
    // ONLY FOR INTERNAL CONTROL
    //
    START_ENUM_RANGE(FIRST_INTERNAL),
    BasicTriangulation,
    BasicStanleyDec,
    NakedDual,
    FullConeDynamic,
    Generators,
    PullingTriangulationInternal,
    //
    // ONLY FOR E§XTENDED TESTS
    //
    TestArithOverflowFullCone,
    TestArithOverflowDualMode,
    TestArithOverflowDescent,
    TestArithOverflowProjAndLift,
    TestSmallPyramids,
    TestLargePyramids,
    TestLinearAlgebraGMP,
    TestSimplexParallel,
    TestLibNormaliz,
    END_ENUM_RANGE(LAST_INTERNAL),

    EnumSize  // this has to be the last entry, to get the number of entries in the enum

};  // remember to change also the string conversion function if you change this enum
}

namespace OutputType {
enum Enum { Matrix, MatrixFloat, Vector, Integer, GMPInteger, Rational, FieldElem, Float, MachineInteger, Bool, Complex, Void };
}

class ConeProperties {
   public:
    /* Constructors */
    ConeProperties();
    ConeProperties(ConeProperty::Enum);
    ConeProperties(ConeProperty::Enum, ConeProperty::Enum);
    ConeProperties(ConeProperty::Enum, ConeProperty::Enum, ConeProperty::Enum);
    ConeProperties(const std::bitset<ConeProperty::EnumSize>&);

    /* set properties */
    ConeProperties& set(bool value = true);  // set ALL to value;
    ConeProperties& set(ConeProperty::Enum, bool value = true);
    ConeProperties& set(const std::string s, bool value = true);
    ConeProperties& set(ConeProperty::Enum, ConeProperty::Enum);
    ConeProperties& set(ConeProperty::Enum, ConeProperty::Enum, ConeProperty::Enum);
    ConeProperties& set(const ConeProperties&);

    /* reset (=unset) properties */
    ConeProperties& reset();  // reset ALL
    ConeProperties& reset(ConeProperty::Enum Property);
    ConeProperties& reset(const ConeProperties&);
    ConeProperties& reset_compute_options();

    // does not change *this
    ConeProperties intersection_with(const ConeProperties& ConeProps) const;

    /* test which/how many properties are set */
    bool test(ConeProperty::Enum Property) const;
    bool any() const;
    bool none() const;
    size_t count() const;

    /* return the restriction of this to the goals / options */
    ConeProperties goals() const;
    ConeProperties options() const;
    ConeProperties full_cone_goals(bool renf) const;
    ConeProperties goals_using_grading(bool inhomogeneous) const;

    /* the following methods are used internally */
    void set_preconditions(bool inhomogeneous, bool numberfield);  // activate properties which are needed implicitly
    // void prepare_compute_options(bool inhomogeneous, bool numberfield);
    void check_sanity(bool inhomogeneous);

    void check_conflicting_variants();
    void check_Q_permissible(bool after_implications);
    // void set_default_goals(bool inhomogeneous, bool numberfield);

    /* print it in a nice way */
    friend std::ostream& operator<<(std::ostream&, const ConeProperties&);

   private:
    std::bitset<ConeProperty::EnumSize> CPs;
};

// conversion to/from strings
bool isConeProperty(ConeProperty::Enum& cp, const std::string& s);
ConeProperty::Enum toConeProperty(const std::string&);
const std::string& toString(ConeProperty::Enum);
std::ostream& operator<<(std::ostream&, const ConeProperties&);
OutputType::Enum output_type(ConeProperty::Enum);

ConeProperties all_options();                                // returns cps with the options set
ConeProperties all_goals();                                  // returns cps with the options set
ConeProperties all_full_cone_goals(bool renf);               // returns the goals controlling compute_full_cone()
ConeProperties all_goals_using_grading(bool inhomogeneous);  // returns the goals which depend on grading
ConeProperties only_homogeneous_props();
ConeProperties only_inhomogeneous_props();
ConeProperties treated_as_hom_props();
ConeProperties all_automorphisms();
ConeProperties all_triangulations();

}  // namespace libnormaliz

#endif /* CONE_PROPERTY_H_ */
