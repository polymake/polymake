// ---------------------------------------------------------------------------
//
// This file is part of SymPol
//
// Copyright (C) 2006-2010  Thomas Rehn <thomas@carmen76.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// ---------------------------------------------------------------------------

#include "symmetrycomputation.h"
#include "symmetrycomputationmemento.h"
#include "configuration.h"

#include <permlib/change/conjugating_base_change.h>
#include <permlib/change/deterministic_base_transpose.h> 
#include <permlib/search/classic/set_stabilizer_search.h>
#include <permlib/search/classic/set_image_search.h>

using namespace permlib;
using namespace sympol;
using namespace yal;

// empty set we can use as a reference
const SymmetryComputation::FaceOrbit2 SymmetryComputation::ms_setEmpty = SymmetryComputation::FaceOrbit2();
LoggerPtr SymmetryComputation::logger(Logger::getLogger("SymComp   "));

SymmetryComputation::SymmetryComputation(SymmetryComputationMethod method, RecursionStrategy* const recursionStrategy, const RayComputation* rayCompDefault,
                                         const Polyhedron & data, const PermutationGroup & permGroup, FacesUpToSymmetryList & rays)
	: m_recursionStrategy(recursionStrategy), m_rayCompDefault(rayCompDefault), m_data(data), 
		m_permGroup(permGroup), m_rays(rays), m_loadedFromMemento(false), m_method(method)
{ }


bool SymmetryComputation::equivalentFaces(const PermutationGroup & permGroup, const Face & f1, const Face & f2) const {
    return equivalentFaces(permGroup, f1, f2, ms_setEmpty);
}

bool SymmetryComputation::equivalentFaces(const PermutationGroup & permGroup, const Face & f1, const Face & f2, const FaceOrbit2 & orbit) const {
  // we need equal number of equalities
  if (f1.count() != f2.count()) {
    return false;
  }
  if (f1 == f2) {
    return true;
  }

  // if we have a complete orbit, we use it
  if (!orbit.empty()) {
    return orbit.contains(f2);
  }
  
  unsigned int i;
  
  std::list<ulong> setF1;
  std::list<ulong> setF2;
  for (i=0; i<f1.size(); ++i) {
    if (f1[i])
      setF1.push_back(i);
    if (f2[i])
      setF2.push_back(i);
  }
    
  PermutationGroup myGroup(permGroup);
    
  // change the base so that is prefixed by the set
  ConjugatingBaseChange<PERM,TRANSVERSAL,
    DeterministicBaseTranspose<PERM,TRANSVERSAL> > baseChange(myGroup);
  baseChange.change(myGroup, setF1.begin(), setF1.end());

  // prepare search without DCM pruning
  classic::SetImageSearch<PermutationGroup,TRANSVERSAL> backtrackSearch(myGroup, 0);
  backtrackSearch.construct(setF1.begin(), setF1.end(), setF2.begin(), setF2.end());
  boost::shared_ptr<PERM> repr = backtrackSearch.searchCosetRepresentative();
    
  return repr.get() != NULL;
}






PermutationGroup SymmetryComputation::stabilizer(const PermutationGroup & permGroup, const Face & f) {
  unsigned int i;
  std::list<ulong> setF;
  for (i=0; i<f.size(); ++i) {
    if (f[i])
      setF.push_back(i);
  }
    
  PermutationGroup myGroup(permGroup);
    
  //std::cout << "my group " << myGroup;
  //print_iterable(setF.begin(), setF.end(), 1, "new base");

  // change the base so that is prefixed by the set
  ConjugatingBaseChange<PERM,TRANSVERSAL,
    DeterministicBaseTranspose<PERM,TRANSVERSAL> > baseChange(myGroup);
  baseChange.change(myGroup, setF.begin(), setF.end());

  // prepare search without DCM pruning
  classic::SetStabilizerSearch<PermutationGroup,TRANSVERSAL> backtrackSearch(myGroup, 0);
  backtrackSearch.construct(setF.begin(), setF.end());

  PermutationGroup stab(f.size());
  backtrackSearch.search(stab);
  YALLOG_DEBUG2(logger, "Stab #B = " << stab.B.size() << " // #S = " << stab.S.size());
  return stab;
}


SymmetryComputationMemento* SymmetryComputation::rememberMe() const { 
	SymmetryComputationMemento* memo = new SymmetryComputationMemento(); 
	initRememberMe(memo);
	return memo;
}
void SymmetryComputation::initRememberMe(SymmetryComputationMemento* memo) const {
	memo->m_method = method();
}
void SymmetryComputation::rememberMe(SymmetryComputationMemento* memo) {
	m_method = memo->m_method;
	m_loadedFromMemento = true;
}

//TODO MIGRATE ME
/*
PermutationGroup SymmetryComputation::extendGroup(const Face & f, const PermutationGroup & bigPermGroup, const PermutationGroup & smallPermGroup) {
    PermutationGroup extendedGroup;
    PermutationGroup stab = stabilizer(bigPermGroup, f);
    
    std::stringstream ss;
    //ss << extendedGroup.name() << " := Group(SmallGeneratingSet(Group(Union(GeneratorsOfGroup(" << stab.name() << "),GeneratorsOfGroup(" << smallPermGroup.name() << ")))));;\n";
    ss << "if IsNonTrivial(" << stab.name() << ") then " << extendedGroup.name() << " := Group(SmallGeneratingSet(Group(Union(GeneratorsOfGroup(" << stab.name() << "),GeneratorsOfGroup(" << smallPermGroup.name() << "))))); else " << extendedGroup.name() << " := Group(()); fi;;\n";
 
    char buffer[ulGapBufferLength];
    
    dout() << "issuing " << ss.str();
    for (unsigned int i=0; i<ulGapBufferLength; ++i) buffer[i] = 0;
    GAP::getInstance().execute(ss.str(), buffer, ulGapBufferLength);
    dout() << buffer << std::endl;
    
    m_setKnownGroups.insert(extendedGroup.name());
    
    return extendedGroup;
}
*/

