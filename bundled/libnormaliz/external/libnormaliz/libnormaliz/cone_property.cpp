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

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute(push, target(mic))
#endif

#include <vector>
#include <string>
#include <assert.h>

#include "libnormaliz/general.h"

namespace libnormaliz {
using std::bitset;
using std::endl;
using std::string;
using std::vector;

/* Constructors */
ConeProperties::ConeProperties() {
    CPs = bitset<ConeProperty::EnumSize>();
}
ConeProperties::ConeProperties(ConeProperty::Enum p1) {
    CPs = bitset<ConeProperty::EnumSize>();
    CPs.set(p1);
}
ConeProperties::ConeProperties(ConeProperty::Enum p1, ConeProperty::Enum p2) {
    CPs = bitset<ConeProperty::EnumSize>();
    CPs.set(p1);
    CPs.set(p2);
}
ConeProperties::ConeProperties(ConeProperty::Enum p1, ConeProperty::Enum p2, ConeProperty::Enum p3) {
    CPs = bitset<ConeProperty::EnumSize>();
    CPs.set(p1);
    CPs.set(p2);
    CPs.set(p3);
}
ConeProperties::ConeProperties(const bitset<ConeProperty::EnumSize>& props) {
    CPs = props;
}

/* set Properties */
ConeProperties& ConeProperties::set(ConeProperty::Enum p1, bool value) {
    CPs.set(p1, value);
    return *this;
}
ConeProperties& ConeProperties::set(ConeProperty::Enum p1, ConeProperty::Enum p2) {
    CPs.set(p1);
    CPs.set(p2);
    return *this;
}
ConeProperties& ConeProperties::set(const ConeProperties& ConeProps) {
    CPs ^= ConeProps.CPs;
    return *this;
}

ConeProperties& ConeProperties::set(const std::string s, bool value) {
    CPs.set(toConeProperty(s), value);
    return *this;
}

/* reset (=unset) properties */
ConeProperties& ConeProperties::reset(ConeProperty::Enum Property) {
    CPs.set(Property, false);
    return *this;
}
ConeProperties& ConeProperties::reset(const ConeProperties& ConeProps) {
    CPs &= ~ConeProps.CPs;
    return *this;
}

ConeProperties& ConeProperties::reset_compute_options() {
    CPs.set(ConeProperty::Projection, false);
    CPs.set(ConeProperty::ProjectionFloat, false);
    CPs.set(ConeProperty::NoProjection, false);
    CPs.set(ConeProperty::Approximate, false);
    CPs.set(ConeProperty::BottomDecomposition, false);
    CPs.set(ConeProperty::NoBottomDec, false);
    CPs.set(ConeProperty::DefaultMode, false);
    CPs.set(ConeProperty::DualMode, false);
    CPs.set(ConeProperty::PrimalMode, false);
    CPs.set(ConeProperty::KeepOrder, false);
    CPs.set(ConeProperty::HSOP, false);
    CPs.set(ConeProperty::Symmetrize, false);
    CPs.set(ConeProperty::NoSymmetrization, false);
    CPs.set(ConeProperty::BigInt, false);
    CPs.set(ConeProperty::NoSubdivision, false);
    CPs.set(ConeProperty::NoNestedTri, false);
    CPs.set(ConeProperty::NoPeriodBound, false);
    CPs.set(ConeProperty::NoLLL, false);
    CPs.set(ConeProperty::NoRelax, false);
    CPs.set(ConeProperty::NakedDual, false);
    CPs.set(ConeProperty::Descent, false);
    CPs.set(ConeProperty::NoDescent, false);
    CPs.set(ConeProperty::NoGradingDenom, false);
    CPs.set(ConeProperty::GradingIsPositive, false);
    CPs.set(ConeProperty::Dynamic, false);
    CPs.set(ConeProperty::Static, false);
    return *this;
}

/* return a new ConeProperties object with only the goals/options set,
 * which are set in this object
 */
ConeProperties ConeProperties::goals() {
    ConeProperties ret(*this);
    ret.reset_compute_options();
    return ret;
}
ConeProperties ConeProperties::options() {
    ConeProperties ret;
    ret.set(ConeProperty::Projection, CPs.test(ConeProperty::Projection));
    ret.set(ConeProperty::ProjectionFloat, CPs.test(ConeProperty::ProjectionFloat));
    ret.set(ConeProperty::NoProjection, CPs.test(ConeProperty::NoProjection));
    ret.set(ConeProperty::Approximate, CPs.test(ConeProperty::Approximate));
    ret.set(ConeProperty::BottomDecomposition, CPs.test(ConeProperty::BottomDecomposition));
    ret.set(ConeProperty::NoBottomDec, CPs.test(ConeProperty::NoBottomDec));
    ret.set(ConeProperty::DefaultMode, CPs.test(ConeProperty::DefaultMode));
    ret.set(ConeProperty::DualMode, CPs.test(ConeProperty::DualMode));
    ret.set(ConeProperty::KeepOrder, CPs.test(ConeProperty::KeepOrder));
    ret.set(ConeProperty::HSOP, CPs.test(ConeProperty::HSOP));
    ret.set(ConeProperty::Symmetrize, CPs.test(ConeProperty::Symmetrize));
    ret.set(ConeProperty::NoSymmetrization, CPs.test(ConeProperty::NoSymmetrization));
    ret.set(ConeProperty::PrimalMode, CPs.test(ConeProperty::PrimalMode));
    ret.set(ConeProperty::NoSubdivision, CPs.test(ConeProperty::NoSubdivision));
    ret.set(ConeProperty::NoNestedTri, CPs.test(ConeProperty::NoNestedTri));
    ret.set(ConeProperty::BigInt, CPs.test(ConeProperty::BigInt));
    ret.set(ConeProperty::NoPeriodBound, CPs.test(ConeProperty::NoPeriodBound));
    ret.set(ConeProperty::NoLLL, CPs.test(ConeProperty::NoLLL));
    ret.set(ConeProperty::NoRelax, CPs.test(ConeProperty::NoRelax));
    ret.set(ConeProperty::NakedDual, CPs.test(ConeProperty::NakedDual));
    ret.set(ConeProperty::Descent, CPs.test(ConeProperty::Descent));
    ret.set(ConeProperty::NoDescent, CPs.test(ConeProperty::NoDescent));
    ret.set(ConeProperty::NoGradingDenom, CPs.test(ConeProperty::NoGradingDenom));
    ret.set(ConeProperty::GradingIsPositive, CPs.test(ConeProperty::GradingIsPositive));
    return ret;
}

/* test which/how many properties are set */
bool ConeProperties::test(ConeProperty::Enum Property) const {
    return CPs.test(Property);
}
bool ConeProperties::any() const {
    return CPs.any();
}
bool ConeProperties::none() const {
    return CPs.none();
}
size_t ConeProperties::count() const {
    return CPs.count();
}

/* add preconditions */
void ConeProperties::set_preconditions(bool inhomogeneous, bool numberfield) {
    if ((CPs.test(ConeProperty::ExploitAutomsMult) || CPs.test(ConeProperty::ExploitAutomsVectors)) ||
        CPs.test(ConeProperty::AmbientAutomorphisms)) {
        errorOutput() << *this << endl;
        throw BadInputException("At least one of the listed computation goals not yet implemernted");
    }

    if ((CPs.test(ConeProperty::ExploitAutomsMult) || CPs.test(ConeProperty::ExploitAutomsVectors)) &&
        !CPs.test(ConeProperty::AmbientAutomorphisms))
        CPs.set(ConeProperty::Automorphisms);

    if (CPs.test(ConeProperty::RenfVolume)) {
        CPs.set(ConeProperty::Volume);
        CPs.reset(ConeProperty::RenfVolume);
    }

    if (CPs.test(ConeProperty::HilbertQuasiPolynomial))
        CPs.set(ConeProperty::HilbertSeries);

    if (CPs.test(ConeProperty::EhrhartQuasiPolynomial))
        CPs.set(ConeProperty::EhrhartSeries);

    if (CPs.test(ConeProperty::EhrhartSeries) && !inhomogeneous) {
        CPs.set(ConeProperty::HilbertSeries);
        CPs.set(ConeProperty::NoGradingDenom);
        CPs.reset(ConeProperty::EhrhartSeries);
    }

    if (CPs.test(ConeProperty::EuclideanVolume))
        CPs.set(ConeProperty::Volume);

    if (CPs.test(ConeProperty::EuclideanIntegral))
        CPs.set(ConeProperty::Integral);

    if (inhomogeneous && CPs.test(ConeProperty::LatticePoints)) {
        if (!numberfield) {
            CPs.set(ConeProperty::HilbertBasis);
        }
        else {
            CPs.set(ConeProperty::ModuleGenerators);
        }
        CPs.reset(ConeProperty::LatticePoints);
    }

    if (CPs.test(ConeProperty::ModuleGenerators) && !numberfield) {
        CPs.set(ConeProperty::HilbertBasis);
        CPs.reset(ConeProperty::ModuleGenerators);
    }

    if (!inhomogeneous && CPs.test(ConeProperty::LatticePoints)) {
        CPs.set(ConeProperty::NoGradingDenom);
        CPs.set(ConeProperty::Deg1Elements);
        CPs.reset(ConeProperty::LatticePoints);
    }

    if (inhomogeneous && CPs.test(ConeProperty::HilbertBasis)) {
        CPs.reset(ConeProperty::NumberLatticePoints);
    }

    if (!inhomogeneous && CPs.test(ConeProperty::Deg1Elements)) {
        CPs.reset(ConeProperty::NumberLatticePoints);
    }

    if (CPs.test(ConeProperty::NumberLatticePoints)) {
        CPs.set(ConeProperty::NoGradingDenom);
    }

    if (!inhomogeneous && CPs.test(ConeProperty::Volume) && !numberfield) {
        CPs.set(ConeProperty::Multiplicity);
    }

    if (CPs.test(ConeProperty::VerticesFloat)) {
        CPs.set(ConeProperty::SupportHyperplanes);
        if (!inhomogeneous)
            CPs.set(ConeProperty::Grading);
    }

    if (CPs.test(ConeProperty::SuppHypsFloat)) {
        CPs.set(ConeProperty::SupportHyperplanes);
    }

    if (CPs.test(ConeProperty::ProjectionFloat))
        CPs.set(ConeProperty::Projection);

    if (CPs.test(ConeProperty::GeneratorOfInterior))
        CPs.set(ConeProperty::IsGorenstein);

    if (CPs.test(ConeProperty::IsGorenstein))
        CPs.set(ConeProperty::SupportHyperplanes);

    if (CPs.test(ConeProperty::NoNestedTri))
        CPs.set(ConeProperty::NoSubdivision);

    if (CPs.test(ConeProperty::WitnessNotIntegrallyClosed))
        CPs.set(ConeProperty::IsIntegrallyClosed);

    if (CPs.test(ConeProperty::IsDeg1HilbertBasis)) {
        CPs.set(ConeProperty::HilbertBasis);
        CPs.set(ConeProperty::Grading);
    }
    if (CPs.test(ConeProperty::IsDeg1ExtremeRays)) {
        CPs.set(ConeProperty::ExtremeRays);
        CPs.set(ConeProperty::Grading);
    }
    if (CPs.test(ConeProperty::Grading))
        CPs.set(ConeProperty::Generators);

    if (CPs.test(ConeProperty::IsPointed))
        CPs.set(ConeProperty::ExtremeRays);

    if (CPs.test(ConeProperty::VerticesOfPolyhedron))
        CPs.set(ConeProperty::ExtremeRays);

    if (CPs.test(ConeProperty::ExtremeRays))
        CPs.set(ConeProperty::SupportHyperplanes);

    /*if (CPs.test(ConeProperty::HSOP) && !inhomogeneous){
        CPs.set(ConeProperty::SupportHyperplanes);
        CPs.set(ConeProperty::HilbertSeries);
    }*/

    if (CPs.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid))
        CPs.set(ConeProperty::HilbertBasis);

    if (CPs.test(ConeProperty::MaximalSubspace))
        CPs.set(ConeProperty::SupportHyperplanes);

    if (CPs.test(ConeProperty::ConeDecomposition))
        CPs.set(ConeProperty::Triangulation);

    if (CPs.test(ConeProperty::GradingDenom))
        CPs.reset(ConeProperty::Grading);

    if (CPs.test(ConeProperty::UnitGroupIndex))
        CPs.set(ConeProperty::HilbertBasis);

    if (CPs.test(ConeProperty::Equations) || CPs.test(ConeProperty::Congruences) || CPs.test(ConeProperty::ExternalIndex))
        CPs.set(ConeProperty::Sublattice);

    if (CPs.test(ConeProperty::Rank))
        CPs.set(ConeProperty::Sublattice);

    /* if(CPs.test(ConeProperty::Multiplicity) || CPs.test(ConeProperty::HilbertSeries))
        CPs.set(ConeProperty::SupportHyperplanes);  // to meke them computed if Symmetrize is used
    */

    if (CPs.test(ConeProperty::Integral)) {
        // CPs.set(ConeProperty::Multiplicity);
        CPs.set(ConeProperty::Triangulation);
    }

    if (CPs.test(ConeProperty::VirtualMultiplicity)) {
        // CPs.set(ConeProperty::Multiplicity);
        CPs.set(ConeProperty::Triangulation);
    }

    if (CPs.test(ConeProperty::WeightedEhrhartQuasiPolynomial))
        CPs.set(ConeProperty::WeightedEhrhartSeries);

    if (CPs.test(ConeProperty::WeightedEhrhartSeries)) {
        // CPs.set(ConeProperty::Multiplicity);
        CPs.set(ConeProperty::StanleyDec);
    }

    if (CPs.test(ConeProperty::Volume) || CPs.test(ConeProperty::Integral) || CPs.test(ConeProperty::Volume)) {
        CPs.set(ConeProperty::NoGradingDenom);
    }

    if (CPs.test(ConeProperty::IntegerHull)) {
        if (inhomogeneous) {
            if (!numberfield)
                CPs.set(ConeProperty::HilbertBasis);
            else
                CPs.set(ConeProperty::ModuleGenerators);
        }
        else {
            CPs.set(ConeProperty::Deg1Elements);
        }
    }

    // -d without -1 means: compute Hilbert basis in dual mode
    if (CPs.test(ConeProperty::DualMode) && !CPs.test(ConeProperty::Deg1Elements)) {
        CPs.set(ConeProperty::HilbertBasis);
    }

    if (CPs.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid))  // can't be computed in dual mode
        CPs.reset(ConeProperty::DualMode);

    // dual mode has priority, approximation and projection make no sense if HB is computed, except possibly with inhomogeneous
    // data
    if (CPs.test(ConeProperty::DualMode) || (CPs.test(ConeProperty::HilbertBasis) && !inhomogeneous)) {
        CPs.reset(ConeProperty::Approximate);
        CPs.reset(ConeProperty::Projection);
    }

    if ((CPs.test(ConeProperty::DualMode) || CPs.test(ConeProperty::Approximate) || CPs.test(ConeProperty::Projection)) &&
        (CPs.test(ConeProperty::HilbertSeries) || CPs.test(ConeProperty::StanleyDec)) && !CPs.test(ConeProperty::HilbertBasis)) {
        CPs.reset(ConeProperty::DualMode);     // it makes no sense to compute only deg 1 elements in dual mode
        CPs.reset(ConeProperty::Approximate);  // or by approximation or projection if the
        CPs.reset(ConeProperty::Projection);   // Stanley decomposition must be computed anyway
    }

    if (inhomogeneous && CPs.test(ConeProperty::SupportHyperplanes))
        CPs.set(ConeProperty::AffineDim);

    if (CPs.test(ConeProperty::SupportHyperplanes))
        CPs.set(ConeProperty::ExtremeRays);

    if (!CPs.test(ConeProperty::DefaultMode))
        return;

    // below only DefaultMode

    if (!numberfield) {
        CPs.set(ConeProperty::HilbertBasis);
        CPs.set(ConeProperty::HilbertSeries);
        if (!inhomogeneous)
            CPs.set(ConeProperty::ClassGroup);
        CPs.set(ConeProperty::SupportHyperplanes);
    }
    else {
        CPs.set(ConeProperty::SupportHyperplanes);
    }

    if (CPs.test(ConeProperty::SupportHyperplanes))
        CPs.set(ConeProperty::ExtremeRays);
}

void ConeProperties::check_Q_permissible(bool after_implications) {
    ConeProperties copy(*this);
    copy.reset(ConeProperty::SupportHyperplanes);
    copy.reset(ConeProperty::ExtremeRays);
    copy.reset(ConeProperty::VerticesOfPolyhedron);
    copy.reset(ConeProperty::KeepOrder);
    copy.reset(ConeProperty::Triangulation);
    copy.reset(ConeProperty::ConeDecomposition);
    copy.reset(ConeProperty::DefaultMode);
    copy.reset(ConeProperty::Generators);
    copy.reset(ConeProperty::Sublattice);
    copy.reset(ConeProperty::MaximalSubspace);
    copy.reset(ConeProperty::Equations);
    copy.reset(ConeProperty::Dehomogenization);
    copy.reset(ConeProperty::Rank);
    copy.reset(ConeProperty::EmbeddingDim);
    copy.reset(ConeProperty::IsPointed);
    copy.reset(ConeProperty::IsInhomogeneous);
    copy.reset(ConeProperty::AffineDim);
    copy.reset(ConeProperty::ModuleGenerators);
    copy.reset(ConeProperty::Deg1Elements);
    copy.reset(ConeProperty::Volume);
    copy.reset(ConeProperty::RenfVolume);
    copy.reset(ConeProperty::IntegerHull);
    copy.reset(ConeProperty::TriangulationDetSum);
    copy.reset(ConeProperty::LatticePoints);
    copy.reset(ConeProperty::TriangulationSize);
    copy.reset(ConeProperty::NoGradingDenom);
    copy.reset(ConeProperty::NumberLatticePoints);
    copy.reset(ConeProperty::EuclideanVolume);
    copy.reset(ConeProperty::RecessionRank);
    copy.reset(ConeProperty::ProjectCone);
    copy.reset(ConeProperty::NoBottomDec);
    copy.reset(ConeProperty::BottomDecomposition);
    copy.reset(ConeProperty::GradingIsPositive);
    copy.reset(ConeProperty::VerticesFloat);
    copy.reset(ConeProperty::SuppHypsFloat);
    copy.reset(ConeProperty::FaceLattice);
    copy.reset(ConeProperty::FVector);
    copy.reset(ConeProperty::Incidence);
    copy.reset(ConeProperty::AmbientAutomorphisms);
    copy.reset(ConeProperty::Automorphisms);
    copy.reset(ConeProperty::CombinatorialAutomorphisms);
    copy.reset(ConeProperty::EuclideanAutomorphisms);
    copy.reset(ConeProperty::Dynamic);
    copy.reset(ConeProperty::Static);

    if (after_implications) {
        copy.reset(ConeProperty::Multiplicity);
        copy.reset(ConeProperty::Grading);
    }

    // bvverboseOutput() << copy << endl;
    if (copy.any()) {
        errorOutput() << copy << endl;
        throw BadInputException("Cone Property in last line not allowed for field coefficients");
    }
}

void ConeProperties::check_conflicting_variants() {
    if ((CPs.test(ConeProperty::BottomDecomposition) && CPs.test(ConeProperty::NoBottomDec)) ||
        (CPs.test(ConeProperty::DualMode) && CPs.test(ConeProperty::PrimalMode)) ||
        (CPs.test(ConeProperty::Symmetrize) && CPs.test(ConeProperty::NoSymmetrization)) ||
        (CPs.test(ConeProperty::Projection) && CPs.test(ConeProperty::NoProjection)) ||
        (CPs.test(ConeProperty::Projection) && CPs.test(ConeProperty::ProjectionFloat)) ||
        (CPs.test(ConeProperty::NoProjection) && CPs.test(ConeProperty::ProjectionFloat)) ||
        (CPs.test(ConeProperty::NoDescent) && CPs.test(ConeProperty::Descent)) ||
        (CPs.test(ConeProperty::Symmetrize) && CPs.test(ConeProperty::Descent)) ||
        (CPs.test(ConeProperty::Dynamic) && CPs.test(ConeProperty::Static)))
        throw BadInputException("Contradictory algorithmic variants in options.");

    /* if((CPs.test(ConeProperty::HilbertSeries) || CPs.test(ConeProperty::HilbertQuasiPolynomial))
               && (CPs.test(ConeProperty::EhrhartSeries) || CPs.test(ConeProperty::EhrhartQuasiPolynomial)) )
        throw BadInputException("Only one of HilbertSeries or EhrhartSeries allowed.");*/

    size_t nr_var = 0;
    if (CPs.test(ConeProperty::DualMode))
        nr_var++;
    if (CPs.test(ConeProperty::PrimalMode))
        nr_var++;
    if (CPs.test(ConeProperty::Projection))
        nr_var++;
    if (CPs.test(ConeProperty::ProjectionFloat))
        nr_var++;
    if (CPs.test(ConeProperty::Approximate))
        nr_var++;
    if (nr_var > 1)
        throw BadInputException("Only one of DualMode, PrimalMode, Approximate, Projection, ProjectionFloat allowed.");
}

void ConeProperties::check_sanity(bool inhomogeneous) {  //, bool input_automorphisms) {

    ConeProperty::Enum prop;

    if (CPs.test(ConeProperty::IsTriangulationNested) || CPs.test(ConeProperty::IsTriangulationPartial))
        throw BadInputException("ConeProperty not allowed in compute().");

    if ((CPs.test(ConeProperty::Approximate) || CPs.test(ConeProperty::DualMode)) && CPs.test(ConeProperty::NumberLatticePoints))
        throw BadInputException("NumberLatticePoints not compuiable with DualMode or Approximate.");

    size_t automs = 0;
    if (CPs.test(ConeProperty::Automorphisms))
        automs++;
    if (CPs.test(ConeProperty::CombinatorialAutomorphisms))
        automs++;
    if (CPs.test(ConeProperty::AmbientAutomorphisms))
        automs++;
    if (CPs.test(ConeProperty::RationalAutomorphisms))
        automs++;
    if (CPs.test(ConeProperty::EuclideanAutomorphisms))
        automs++;
    if (automs > 1)
        throw BadInputException("Only one type of automorphism group allowed.");

    for (size_t i = 0; i < ConeProperty::EnumSize; i++) {
        if (CPs.test(i)) {
            prop = static_cast<ConeProperty::Enum>(i);
            if (inhomogeneous) {
                if (prop == ConeProperty::Deg1Elements
                    // || prop == ConeProperty::StanleyDec
                    // || prop == ConeProperty::Triangulation           // now allowed
                    // || prop == ConeProperty::ConeDecomposition
                    || prop == ConeProperty::IsIntegrallyClosed || prop == ConeProperty::WitnessNotIntegrallyClosed ||
                    prop == ConeProperty::ClassGroup || prop == ConeProperty::Symmetrize ||
                    prop == ConeProperty::NoSymmetrization || prop == ConeProperty::InclusionExclusionData ||
                    prop == ConeProperty::ExcludedFaces || prop == ConeProperty::UnitGroupIndex ||
                    prop == ConeProperty::ReesPrimaryMultiplicity || prop == ConeProperty::IsReesPrimary ||
                    prop == ConeProperty::IsDeg1HilbertBasis || prop == ConeProperty::IsDeg1ExtremeRays ||
                    prop == ConeProperty::Integral || prop == ConeProperty::IsGorenstein ||
                    prop == ConeProperty::GeneratorOfInterior) {
                    throw BadInputException(toString(prop) + " not computable in the inhomogeneous case.");
                }
            }
            else {  // homgeneous
                if (prop == ConeProperty::VerticesOfPolyhedron || prop == ConeProperty::ModuleRank ||
                    prop == ConeProperty::ModuleGenerators || prop == ConeProperty::AffineDim ||
                    prop == ConeProperty::RecessionRank) {
                    throw BadInputException(toString(prop) + " only computable in the inhomogeneous case.");
                }
            }
        }  // end if test(i)
    }
}

/* conversion */
namespace {
// only to initialize the CPN in ConePropertyNames
vector<string> initializeCPN() {
    vector<string> CPN(ConeProperty::EnumSize);
    CPN.at(ConeProperty::Generators) = "Generators";
    CPN.at(ConeProperty::ExtremeRays) = "ExtremeRays";
    CPN.at(ConeProperty::VerticesFloat) = "VerticesFloat";
    CPN.at(ConeProperty::VerticesOfPolyhedron) = "VerticesOfPolyhedron";
    CPN.at(ConeProperty::SupportHyperplanes) = "SupportHyperplanes";
    CPN.at(ConeProperty::SuppHypsFloat) = "SuppHypsFloat";
    CPN.at(ConeProperty::TriangulationSize) = "TriangulationSize";
    CPN.at(ConeProperty::TriangulationDetSum) = "TriangulationDetSum";
    CPN.at(ConeProperty::Triangulation) = "Triangulation";
    CPN.at(ConeProperty::Multiplicity) = "Multiplicity";
    CPN.at(ConeProperty::Volume) = "Volume";
    CPN.at(ConeProperty::RenfVolume) = "RenfVolume";
    CPN.at(ConeProperty::EuclideanVolume) = "EuclideanVolume";
    CPN.at(ConeProperty::EuclideanIntegral) = "EuclideanIntegral";
    CPN.at(ConeProperty::RecessionRank) = "RecessionRank";
    CPN.at(ConeProperty::AffineDim) = "AffineDim";
    CPN.at(ConeProperty::ModuleRank) = "ModuleRank";
    CPN.at(ConeProperty::HilbertBasis) = "HilbertBasis";
    CPN.at(ConeProperty::ModuleGenerators) = "ModuleGenerators";
    CPN.at(ConeProperty::Deg1Elements) = "Deg1Elements";
    CPN.at(ConeProperty::LatticePoints) = "LatticePoints";
    CPN.at(ConeProperty::HilbertSeries) = "HilbertSeries";
    CPN.at(ConeProperty::Grading) = "Grading";
    CPN.at(ConeProperty::IsPointed) = "IsPointed";
    CPN.at(ConeProperty::IsDeg1ExtremeRays) = "IsDeg1ExtremeRays";
    CPN.at(ConeProperty::IsDeg1HilbertBasis) = "IsDeg1HilbertBasis";
    CPN.at(ConeProperty::IsIntegrallyClosed) = "IsIntegrallyClosed";
    CPN.at(ConeProperty::WitnessNotIntegrallyClosed) = "WitnessNotIntegrallyClosed";
    CPN.at(ConeProperty::OriginalMonoidGenerators) = "OriginalMonoidGenerators";
    CPN.at(ConeProperty::IsReesPrimary) = "IsReesPrimary";
    CPN.at(ConeProperty::ReesPrimaryMultiplicity) = "ReesPrimaryMultiplicity";
    CPN.at(ConeProperty::StanleyDec) = "StanleyDec";
    CPN.at(ConeProperty::ExcludedFaces) = "ExcludedFaces";
    CPN.at(ConeProperty::Dehomogenization) = "Dehomogenization";
    CPN.at(ConeProperty::InclusionExclusionData) = "InclusionExclusionData";
    CPN.at(ConeProperty::Sublattice) = "Sublattice";
    CPN.at(ConeProperty::ClassGroup) = "ClassGroup";
    CPN.at(ConeProperty::ModuleGeneratorsOverOriginalMonoid) = "ModuleGeneratorsOverOriginalMonoid";
    CPN.at(ConeProperty::Approximate) = "Approximate";
    CPN.at(ConeProperty::BottomDecomposition) = "BottomDecomposition";
    CPN.at(ConeProperty::DefaultMode) = "DefaultMode";
    CPN.at(ConeProperty::DualMode) = "DualMode";
    CPN.at(ConeProperty::KeepOrder) = "KeepOrder";
    CPN.at(ConeProperty::IntegerHull) = "IntegerHull";
    CPN.at(ConeProperty::ProjectCone) = "ProjectCone";
    CPN.at(ConeProperty::MaximalSubspace) = "MaximalSubspace";
    CPN.at(ConeProperty::ConeDecomposition) = "ConeDecomposition";

    CPN.at(ConeProperty::Automorphisms) = "Automorphisms";
    CPN.at(ConeProperty::AmbientAutomorphisms) = "AmbientAutomorphisms";
    CPN.at(ConeProperty::RationalAutomorphisms) = "RationalAutomorphisms";
    CPN.at(ConeProperty::EuclideanAutomorphisms) = "EuclideanAutomorphisms";
    CPN.at(ConeProperty::CombinatorialAutomorphisms) = "CombinatorialAutomorphisms";
    CPN.at(ConeProperty::ExploitAutomsVectors) = "ExploitAutomsVectors";
    CPN.at(ConeProperty::ExploitAutomsMult) = "ExploitAutomsMult";

    CPN.at(ConeProperty::HSOP) = "HSOP";
    CPN.at(ConeProperty::NoBottomDec) = "NoBottomDec";
    CPN.at(ConeProperty::PrimalMode) = "PrimalMode";
    CPN.at(ConeProperty::Symmetrize) = "Symmetrize";
    CPN.at(ConeProperty::NoSymmetrization) = "NoSymmetrization";
    CPN.at(ConeProperty::EmbeddingDim) = "EmbeddingDim";
    CPN.at(ConeProperty::Rank) = "Rank";
    CPN.at(ConeProperty::InternalIndex) = "InternalIndex";
    CPN.at(ConeProperty::IsInhomogeneous) = "IsInhomogeneous";
    CPN.at(ConeProperty::UnitGroupIndex) = "UnitGroupIndex";
    CPN.at(ConeProperty::GradingDenom) = "GradingDenom";
    CPN.at(ConeProperty::Equations) = "Equations";
    CPN.at(ConeProperty::Congruences) = "Congruences";
    CPN.at(ConeProperty::ExternalIndex) = "ExternalIndex";
    CPN.at(ConeProperty::HilbertQuasiPolynomial) = "HilbertQuasiPolynomial";
    CPN.at(ConeProperty::IsTriangulationNested) = "IsTriangulationNested";
    CPN.at(ConeProperty::IsTriangulationPartial) = "IsTriangulationPartial";
    CPN.at(ConeProperty::BigInt) = "BigInt";
    CPN.at(ConeProperty::NoSubdivision) = "NoSubdivision";
    CPN.at(ConeProperty::Projection) = "Projection";
    CPN.at(ConeProperty::ProjectionFloat) = "ProjectionFloat";
    CPN.at(ConeProperty::NoProjection) = "NoProjection";
    CPN.at(ConeProperty::NoNestedTri) = "NoNestedTri";
    CPN.at(ConeProperty::Integral) = "Integral";
    CPN.at(ConeProperty::VirtualMultiplicity) = "VirtualMultiplicity";
    CPN.at(ConeProperty::WeightedEhrhartSeries) = "WeightedEhrhartSeries";
    CPN.at(ConeProperty::WeightedEhrhartQuasiPolynomial) = "WeightedEhrhartQuasiPolynomial";
    CPN.at(ConeProperty::EhrhartSeries) = "EhrhartSeries";
    CPN.at(ConeProperty::EhrhartQuasiPolynomial) = "EhrhartQuasiPolynomial";
    CPN.at(ConeProperty::IsGorenstein) = "IsGorenstein";
    CPN.at(ConeProperty::NoPeriodBound) = "NoPeriodBound";
    CPN.at(ConeProperty::NoLLL) = "NoLLL";
    CPN.at(ConeProperty::NoRelax) = "NoRelax";
    CPN.at(ConeProperty::GeneratorOfInterior) = "GeneratorOfInterior";
    CPN.at(ConeProperty::NakedDual) = "NakedDual";
    CPN.at(ConeProperty::Descent) = "Descent";
    CPN.at(ConeProperty::NoDescent) = "NoDescent";
    CPN.at(ConeProperty::NoGradingDenom) = "NoGradingDenom";
    CPN.at(ConeProperty::GradingIsPositive) = "GradingIsPositive";
    CPN.at(ConeProperty::NumberLatticePoints) = "NumberLatticePoints";
    CPN.at(ConeProperty::FaceLattice) = "FaceLattice";
    CPN.at(ConeProperty::FVector) = "FVector";
    CPN.at(ConeProperty::Incidence) = "Incidence";
    CPN.at(ConeProperty::Dynamic) = "Dynamic";
    CPN.at(ConeProperty::Static) = "Static";

    // detect changes in size of Enum, to remember to update CPN!
    static_assert(ConeProperty::EnumSize == 99, "ConeProperties Enum size does not fit! Update cone_property.cpp!");
    // assert all fields contain an non-empty string
    for (size_t i = 0; i < ConeProperty::EnumSize; i++) {
        assert(CPN.at(i).size() > 0);
    }
    return CPN;
}

const vector<string>& ConePropertyNames() {
    static const vector<string> CPN(initializeCPN());
    return CPN;
}
}  // namespace

bool isConeProperty(ConeProperty::Enum& cp, const std::string& s) {
    const vector<string>& CPN = ConePropertyNames();
    for (size_t i = 0; i < ConeProperty::EnumSize; i++) {
        if (CPN[i] == s) {
            cp = static_cast<ConeProperty::Enum>(i);
            return true;
        }
    }
    return false;
}

ConeProperty::Enum toConeProperty(const std::string& s) {
    ConeProperty::Enum cp;
    if (isConeProperty(cp, s))
        return cp;
    throw BadInputException("Unknown ConeProperty string \"" + s + "\"");
}

const std::string& toString(ConeProperty::Enum cp) {
    return ConePropertyNames()[cp];
}

/* print it in a nice way */
std::ostream& operator<<(std::ostream& out, const ConeProperties& CP) {
    for (size_t i = 0; i < ConeProperty::EnumSize; i++) {
        if (CP.CPs.test(i))
            out << toString(static_cast<ConeProperty::Enum>(i)) << " ";
    }
    return out;
}

OutputType::Enum output_type(ConeProperty::Enum property) {
    if (property >= ConeProperty::FIRST_MATRIX && property <= ConeProperty::LAST_MATRIX)
        return OutputType::Matrix;
    if (property >= ConeProperty::FIRST_MATRIX_FLOAT && property <= ConeProperty::LAST_MATRIX_FLOAT)
        return OutputType::MatrixFloat;
    if (property >= ConeProperty::FIRST_VECTOR && property <= ConeProperty::LAST_VECTOR)
        return OutputType::Vector;
    if (property >= ConeProperty::FIRST_INTEGER && property <= ConeProperty::LAST_INTEGER)
        return OutputType::Integer;
    if (property >= ConeProperty::FIRST_GMP_INTEGER && property <= ConeProperty::LAST_GMP_INTEGER)
        return OutputType::GMPInteger;
    if (property >= ConeProperty::FIRST_RATIONAL && property <= ConeProperty::LAST_RATIONAL)
        return OutputType::Rational;
    if (property >= ConeProperty::FIRST_FIELD_ELEM && property <= ConeProperty::LAST_FIELD_ELEM)
        return OutputType::FieldElem;
    if (property >= ConeProperty::FIRST_FLOAT && property <= ConeProperty::LAST_FLOAT)
        return OutputType::Float;
    if (property >= ConeProperty::FIRST_MACHINE_INTEGER && property <= ConeProperty::LAST_MACHINE_INTEGER)
        return OutputType::MachineInteger;
    if (property >= ConeProperty::FIRST_BOOLEAN && property <= ConeProperty::LAST_BOOLEAN)
        return OutputType::Bool;
    if (property >= ConeProperty::FIRST_COMPLEX_STRUCTURE && property <= ConeProperty::LAST_COMPLEX_STRUCTURE)
        return OutputType::Complex;
    return OutputType::Void;
}

} /* end namespace libnormaliz */

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute(pop)
#endif
