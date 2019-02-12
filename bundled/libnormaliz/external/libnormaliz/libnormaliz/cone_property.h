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

#ifndef CONE_PROPERTY_H_
#define CONE_PROPERTY_H_

#include <bitset>
#include <ostream>

namespace libnormaliz {

/* An enumeration of things, that can be computed for a cone.
 * The namespace prevents interfering with other names.
 * Remember to change also the string conversion if you change this enum!
 */
namespace ConeProperty {
    enum Enum {
        FIRST_MATRIX,
        Generators = ConeProperty::FIRST_MATRIX,
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
        LAST_MATRIX = ConeProperty::Congruences,
        FIRST_MATRIX_FLOAT,
        SuppHypsFloat = ConeProperty::FIRST_MATRIX_FLOAT,
        VerticesFloat,
        LAST_MATRIX_FLOAT = ConeProperty::VerticesFloat,
        // Vector values
        FIRST_VECTOR,
        Grading = ConeProperty::FIRST_VECTOR,
        Dehomogenization,
        WitnessNotIntegrallyClosed,
        GeneratorOfInterior,
        ClassGroup,
        LAST_VECTOR = ConeProperty::ClassGroup,
        // Integer valued,
        FIRST_INTEGER,
        TriangulationDetSum = ConeProperty::FIRST_INTEGER,
        ReesPrimaryMultiplicity,
        GradingDenom,
        UnitGroupIndex,
        InternalIndex,
        LAST_INTEGER = ConeProperty::InternalIndex,
        FIRST_GMP_INTEGER,
        ExternalIndex = FIRST_GMP_INTEGER,
        LAST_GMP_INTEGER = ConeProperty::ExternalIndex,
        // rational valued
        FIRST_RATIONAL,
        Multiplicity = ConeProperty::FIRST_RATIONAL,
        Volume,
        Integral,
        VirtualMultiplicity,
        LAST_RATIONAL = ConeProperty::VirtualMultiplicity,
        // floating point valued
        FIRST_FLOAT,
        EuclideanVolume = ConeProperty::FIRST_FLOAT,
        EuclideanIntegral,
        LAST_FLOAT = ConeProperty::EuclideanIntegral,
        // dimensions
        FIRST_MACHINE_INTEGER,
        TriangulationSize = ConeProperty::FIRST_MACHINE_INTEGER,
        RecessionRank,
        AffineDim,
        ModuleRank,
        Rank,
        EmbeddingDim,
        LAST_MACHINE_INTEGER = ConeProperty::EmbeddingDim,
        // boolean valued 
        FIRST_BOOLEAN,
        IsPointed = ConeProperty::FIRST_BOOLEAN,
        IsDeg1ExtremeRays,
        IsDeg1HilbertBasis,
        IsIntegrallyClosed,
        IsReesPrimary,
        IsInhomogeneous,
        IsGorenstein,
        LAST_BOOLEAN = ConeProperty::IsGorenstein,
        // complex structures
        FIRST_COMPLEX_STRUCTURE,
        Triangulation = ConeProperty::FIRST_COMPLEX_STRUCTURE,
        StanleyDec,
        InclusionExclusionData,
        IntegerHull,
        ProjectCone,
        ConeDecomposition,
        HilbertSeries,
        HilbertQuasiPolynomial,
        EhrhartSeries,
        EhrhartQuasiPolynomial,
        WeightedEhrhartSeries,
        WeightedEhrhartQuasiPolynomial,
        Sublattice,
        LAST_COMPLEX_STRUCTURE = ConeProperty::Sublattice,
        //
        // integer type for computations
        //
        FIRST_PROPERTY,
        BigInt = ConeProperty::FIRST_PROPERTY,
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
        NoNestedTri, // synonym for NoSubdivision
        KeepOrder,
        HSOP,
        NoPeriodBound,
        SCIP,
        NoLLL,
        NoRelax,
        Descent,
        NoDescent,
        NoGradingDenom,
        GradingIsPositive,
        //
        // checking properties of already computed data
        // (cannot be used as a computation goal)
        //
        IsTriangulationNested,
        IsTriangulationPartial,
        //
        // ONLY FOR INTERNAL CONTROL
        //
        ExplicitHilbertSeries,
        NakedDual,
        EnumSize,
        LAST_PROPERTY = ConeProperty::EnumSize // this has to be the last entry, to get the number of entries in the enum
    }; // remember to change also the string conversion function if you change this enum
}

namespace OutputType{
    enum Enum {
        Matrix,
        MatrixFloat,
        Vector,
        Integer,
        GMPInteger,
        Rational,
        Float,
        MachineInteger,
        Bool,
        Complex,
        Void
    };
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
    ConeProperties& set(ConeProperty::Enum, bool value=true);
    ConeProperties& set(const std::string s, bool value=true);
    ConeProperties& set(ConeProperty::Enum, ConeProperty::Enum);
    ConeProperties& set(ConeProperty::Enum, ConeProperty::Enum, ConeProperty::Enum);
    ConeProperties& set(const ConeProperties&);

    /* reset (=unset) properties */
    ConeProperties& reset(ConeProperty::Enum Property);
    ConeProperties& reset(const ConeProperties&);
    ConeProperties& reset_compute_options();

    /* test which/how many properties are set */
    bool test(ConeProperty::Enum Property) const;
    bool any() const;
    bool none() const;
    size_t count () const;

    /* return the restriction of this to the goals / options */
    ConeProperties goals();
    ConeProperties options();

    /* the following methods are used internally */
    void set_preconditions(bool inhomogeneous);    // activate properties which are needed implicitly
    void prepare_compute_options(bool inhomogeneous);
    void check_sanity(bool inhomogeneous);
    void check_conflicting_variants();

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

}

#endif /* CONE_PROPERTY_H_ */
