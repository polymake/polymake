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

#include <vector>
#include <string>

#include "cone_property.h"
#include "normaliz_exception.h"

namespace libnormaliz {
using std::bitset;
using std::vector;
using std::string;


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

/* reset (=unset) properties */
ConeProperties& ConeProperties::reset(ConeProperty::Enum Property) {
    CPs.set(Property, false);
    return *this;
}
ConeProperties& ConeProperties::reset(const ConeProperties& ConeProps) {
    CPs &= ~ConeProps.CPs;
    return *this;
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
size_t ConeProperties::count () const {
    return CPs.count();
}


/* this method sets all fields that should be computed in that mode */
ConeProperties& ConeProperties::set(Mode::ComputationMode mode) {
    switch (mode) {
    case Mode::supportHyperplanes:
        set(ConeProperty::SupportHyperplanes, ConeProperty::ExtremeRays);
        break;
    case Mode::triangulationSize:
        set(ConeProperty::TriangulationSize);
        break;
    case Mode::triangulation:
        set(ConeProperty::Triangulation);
        break;
    case Mode::volumeTriangulation:
        set(ConeProperty::Triangulation, ConeProperty::Multiplicity);
        break;
    case Mode::volumeLarge:
        set(ConeProperty::Multiplicity);
        break;
    case Mode::degree1Elements:
        set(ConeProperty::Deg1Elements);
        break;
    case Mode::hilbertBasisTriangulation:
        set(ConeProperty::HilbertBasis, ConeProperty::Triangulation);
        break;
    case Mode::hilbertBasisMultiplicity:
        set(ConeProperty::HilbertBasis, ConeProperty::Multiplicity);
        break;
    case Mode::hilbertBasisLarge:
        set(ConeProperty::HilbertBasis);
        break;
    case Mode::hilbertSeries:
        set(ConeProperty::Triangulation);
    case Mode::hilbertSeriesLarge:
        set(ConeProperty::Deg1Elements, ConeProperty::HilbertSeries);
        break;
    case Mode::hilbertBasisSeries:
        set(ConeProperty::Triangulation);
    case Mode::hilbertBasisSeriesLarge:
        set(ConeProperty::HilbertSeries, ConeProperty::HilbertBasis);
        break;
    case Mode::dual:
        set(ConeProperty::DualMode);
        break;
    default:
        throw FatalException();
        break;
    }
    return *this;
}

/* conversion */
namespace { 
    // only to initialize the CPN in ConePropertyNames
    vector<string> initializeCPN() {
        vector<string> CPN(ConeProperty::EnumSize);
        if (ConeProperty::EnumSize != 21) { //to detect changes in size of Enum
            errorOutput() << "Fatal Error: ConeProperties Enum size does not fit!" << std::endl;
            throw FatalException();
        }
        CPN.at(ConeProperty::Generators) = "Generators";
        CPN.at(ConeProperty::ExtremeRays) = "ExtremeRays";
        CPN.at(ConeProperty::SupportHyperplanes) = "SupportHyperplanes";
        CPN.at(ConeProperty::TriangulationSize) = "TriangulationSize";
        CPN.at(ConeProperty::TriangulationDetSum) = "TriangulationDetSum";
        CPN.at(ConeProperty::Triangulation) = "Triangulation";
        CPN.at(ConeProperty::Multiplicity) = "Multiplicity";
        CPN.at(ConeProperty::HilbertBasis) = "HilbertBasis";
        CPN.at(ConeProperty::Deg1Elements) = "Deg1Elements";
        CPN.at(ConeProperty::HilbertSeries) = "HilbertSeries";
        CPN.at(ConeProperty::Grading) = "Grading";
        CPN.at(ConeProperty::IsPointed) = "IsPointed";
        CPN.at(ConeProperty::IsDeg1Generated) = "IsDeg1Generated";
        CPN.at(ConeProperty::IsDeg1ExtremeRays) = "IsDeg1ExtremeRays";
        CPN.at(ConeProperty::IsDeg1HilbertBasis) = "IsDeg1HilbertBasis";
        CPN.at(ConeProperty::IsIntegrallyClosed) = "IsIntegrallyClosed";
        CPN.at(ConeProperty::GeneratorsOfToricRing) = "GeneratorsOfToricRing";
        CPN.at(ConeProperty::ReesPrimary) = "ReesPrimary";
        CPN.at(ConeProperty::ReesPrimaryMultiplicity) = "ReesPrimaryMultiplicity";
        CPN.at(ConeProperty::StanleyDec) = "StanleyDec";
        CPN.at(ConeProperty::DualMode) = "DualMode";
        return CPN;
    }
 
    const vector<string>& ConePropertyNames() {
        static vector<string> CPN(initializeCPN());
        return CPN;
    }
}

ConeProperty::Enum toConeProperty(const std::string& s) {
    const vector<string>& CPN = ConePropertyNames();
    for (size_t i=0; i<ConeProperty::EnumSize; i++) {
        if (CPN[i] == s) return static_cast<ConeProperty::Enum>(i);
    }
    errorOutput() << "Unknown ConeProperty string \"" << s << "\"" << std::endl;
    throw BadInputException();
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
