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

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute (push, target(mic))
#endif

#include <vector>
#include <string>
#include <assert.h>

#include "libnormaliz/cone_property.h"
#include "libnormaliz/libnormaliz.h"
#include "libnormaliz/normaliz_exception.h"

namespace libnormaliz {
using std::bitset;
using std::vector;
using std::string;
using std::endl;


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
ConeProperties::ConeProperties(ConeProperty::Enum p1, ConeProperty::Enum p2,
                               ConeProperty::Enum p3) {
    CPs = bitset<ConeProperty::EnumSize>();
    CPs.set(p1);
    CPs.set(p2);
    CPs.set(p3);
}
ConeProperties::ConeProperties(const bitset<ConeProperty::EnumSize>& props){
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
    CPs.set(ConeProperty::Approximate, false);
    CPs.set(ConeProperty::BottomDecomposition, false);
    CPs.set(ConeProperty::DefaultMode, false);
    CPs.set(ConeProperty::DualMode, false);
    CPs.set(ConeProperty::KeepOrder, false);
    CPs.set(ConeProperty::HSOP, false);
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
    ret.set(ConeProperty::Approximate, CPs.test(ConeProperty::Approximate));
    ret.set(ConeProperty::BottomDecomposition, CPs.test(ConeProperty::BottomDecomposition));
    ret.set(ConeProperty::DefaultMode, CPs.test(ConeProperty::DefaultMode));
    ret.set(ConeProperty::DualMode, CPs.test(ConeProperty::DualMode));
    ret.set(ConeProperty::KeepOrder, CPs.test(ConeProperty::KeepOrder));
    ret.set(ConeProperty::HSOP, CPs.test(ConeProperty::HSOP));
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
void ConeProperties::set_preconditions() {
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

    if (CPs.test(ConeProperty::ExtremeRays))
        CPs.set(ConeProperty::SupportHyperplanes);
        
    if (CPs.test(ConeProperty::HSOP)){
        CPs.set(ConeProperty::SupportHyperplanes);
        CPs.set(ConeProperty::HilbertSeries);
    }
    // inhomogenous preconditions
    if (CPs.test(ConeProperty::VerticesOfPolyhedron))
        CPs.set(ConeProperty::ExtremeRays);
    
    if(CPs.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid))
        CPs.set(ConeProperty::HilbertBasis);

    if (CPs.test(ConeProperty::ModuleGenerators))
        CPs.set(ConeProperty::HilbertBasis);
    
    if (CPs.test(ConeProperty::MaximalSubspace))
        CPs.set(ConeProperty::SupportHyperplanes);
    
    // always
    
    if (CPs.test(ConeProperty::ExtremeRays))
        CPs.set(ConeProperty::SupportHyperplanes);
}

/* removes ignored compute options and sets implications */
void ConeProperties::prepare_compute_options(bool inhomogeneous) {
    if (CPs.test(ConeProperty::IntegerHull)){
        if(inhomogeneous){
            CPs.set(ConeProperty::HilbertBasis);
        }
        else{
            CPs.set(ConeProperty::Deg1Elements);
        }
    }
    // -d without -1 means: compute Hilbert basis in dual mode
    if (CPs.test(ConeProperty::DualMode) && !CPs.test(ConeProperty::Deg1Elements)){
        CPs.set(ConeProperty::HilbertBasis);
    }
    
    if(CPs.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) // can't be computed in dual mode
        CPs.reset(ConeProperty::DualMode);

    // dual mode has priority, approximation makes no sense if HB is computed
    if(CPs.test(ConeProperty::DualMode) || CPs.test(ConeProperty::HilbertBasis))
        CPs.reset(ConeProperty::Approximate);

    if ((CPs.test(ConeProperty::DualMode) || CPs.test(ConeProperty::Approximate))
        && (CPs.test(ConeProperty::HilbertSeries) || CPs.test(ConeProperty::StanleyDec))
         && !CPs.test(ConeProperty::HilbertBasis)){
        CPs.reset(ConeProperty::DualMode); //it makes no sense to compute only deg 1 elements in dual mode
        CPs.reset(ConeProperty::Approximate); // or by approximation if the
    }                                            // Stanley decomposition must be computed anyway
    if (CPs.test(ConeProperty::Approximate)
            && !CPs.test(ConeProperty::Deg1Elements)) {
        errorOutput() << "WARNING: Approximate is ignored since Deg1Elements is not set."<< std::endl;
    }
    if (CPs.test(ConeProperty::ConeDecomposition))
        CPs.reset(ConeProperty::Triangulation);    
    
    if(inhomogeneous && CPs.test(ConeProperty::SupportHyperplanes))
        CPs.set(ConeProperty::AffineDim);
        
}


void ConeProperties::check_sanity(bool inhomogeneous) {
    ConeProperty::Enum prop;
    for (size_t i=0; i<ConeProperty::EnumSize; i++) {
        if (CPs.test(i)) {
            prop = static_cast<ConeProperty::Enum>(i);
            if (inhomogeneous) {
                if ( prop == ConeProperty::Deg1Elements
                  || prop == ConeProperty::StanleyDec
                  || prop == ConeProperty::Triangulation
                  || prop == ConeProperty::ConeDecomposition
                  || prop == ConeProperty::IsIntegrallyClosed
                  || prop == ConeProperty::WitnessNotIntegrallyClosed
                  || prop == ConeProperty::Approximate
                  || prop == ConeProperty::ClassGroup
                 // || prop == ConeProperty::ModuleGeneratorsOverOriginalMonoid
                ) {
                    throw BadInputException(toString(prop) + " not computable in the inhomogeneous case.");
                }
            } else { // homgeneous
                if ( prop == ConeProperty::VerticesOfPolyhedron
                  || prop == ConeProperty::ModuleRank
                  || prop == ConeProperty::ModuleGenerators ) {
                    throw BadInputException(toString(prop) + " only computable in the inhomogeneous case.");
                }
            }
        }  //end if test(i)
    }
}


/* conversion */
namespace {
    // only to initialize the CPN in ConePropertyNames
    vector<string> initializeCPN() {
        vector<string> CPN(ConeProperty::EnumSize);
        CPN.at(ConeProperty::Generators) = "Generators";
        CPN.at(ConeProperty::ExtremeRays) = "ExtremeRays";
        CPN.at(ConeProperty::VerticesOfPolyhedron) = "VerticesOfPolyhedron";
        CPN.at(ConeProperty::SupportHyperplanes) = "SupportHyperplanes";
        CPN.at(ConeProperty::TriangulationSize) = "TriangulationSize";
        CPN.at(ConeProperty::TriangulationDetSum) = "TriangulationDetSum";
        CPN.at(ConeProperty::Triangulation) = "Triangulation";
        CPN.at(ConeProperty::Multiplicity) = "Multiplicity";
        CPN.at(ConeProperty::RecessionRank) = "RecessionRank";
        CPN.at(ConeProperty::AffineDim) = "AffineDim";
        CPN.at(ConeProperty::ModuleRank) = "ModuleRank";
        CPN.at(ConeProperty::HilbertBasis) = "HilbertBasis";
        CPN.at(ConeProperty::ModuleGenerators) = "ModuleGenerators";
        CPN.at(ConeProperty::Deg1Elements) = "Deg1Elements";
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
        // the following are more compute options than real properties of the cone
        CPN.at(ConeProperty::Approximate) = "Approximate";
        CPN.at(ConeProperty::BottomDecomposition) = "BottomDecomposition";
        CPN.at(ConeProperty::DefaultMode) = "DefaultMode";
        CPN.at(ConeProperty::DualMode) = "DualMode";
        CPN.at(ConeProperty::KeepOrder) = "KeepOrder";
        CPN.at(ConeProperty::IntegerHull) = "IntegerHull";
        CPN.at(ConeProperty::MaximalSubspace) = "MaximalSubspace";
        CPN.at(ConeProperty::ConeDecomposition) = "ConeDecomposition";
        CPN.at(ConeProperty::HSOP) = "HSOP";
        // detect changes in size of Enum, to remember to update CPN!
        static_assert (ConeProperty::EnumSize == 40,
            "ConeProperties Enum size does not fit! Update cone_property.cpp!");
        // assert all fields contain an non-empty string
        for (size_t i=0;  i<ConeProperty::EnumSize; i++) {
            assert(CPN.at(i).size() > 0);
        }
        return CPN;
    }

    const vector<string>& ConePropertyNames() {
        static const vector<string> CPN(initializeCPN());
        return CPN;
    }
}

bool isConeProperty(ConeProperty::Enum& cp, const std::string& s) {
    const vector<string>& CPN = ConePropertyNames();
    for (size_t i=0; i<ConeProperty::EnumSize; i++) {
        if (CPN[i] == s) {
            cp = static_cast<ConeProperty::Enum>(i);
            return true;
        }
    }
    return false;
}

ConeProperty::Enum toConeProperty(const std::string& s) {
    ConeProperty::Enum cp;
    if (isConeProperty(cp, s)) return cp;
    throw BadInputException("Unknown ConeProperty string \"" + s + "\"");
}

const std::string& toString(ConeProperty::Enum cp) {
    return ConePropertyNames()[cp];
}

/* print it in a nice way */
std::ostream& operator<< (std::ostream& out, const ConeProperties& CP){
    for (size_t i=0; i<ConeProperty::EnumSize; i++) {
        if (CP.CPs.test(i)) out << toString(static_cast<ConeProperty::Enum>(i)) << " ";
    }
    return out;
}


} /* end namespace libnormaliz */

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute (pop)
#endif
