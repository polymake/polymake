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

#ifndef RECURSIONSTRATEGY_H
#define RECURSIONSTRATEGY_H

#include "facesuptosymmetrylist.h"
#include "raycomputation.h"
#include "symmetrycomputationdirect.h"
#include "symmetrycomputationadm.h"
#include "symmetrycomputationidm.h"
#include "yal/logger.h"
#include "symmetrycomputationmemento.h"
#include "symmetrycomputationadmmemento.h"
#include "symmetrycomputationidmmemento.h"

namespace sympol {

class RecursionStrategy {
	public:
		RecursionStrategy();
		virtual ~RecursionStrategy();
		
		bool enumerateRaysUpToSymmetry(const RayComputation* rayComp, const Polyhedron & data, 
						const PermutationGroup & permGroup, FacesUpToSymmetryList& rays);
		bool resumeComputation(const RayComputation* rayComp, const Polyhedron & data, 
						const PermutationGroup & permGroup, FacesUpToSymmetryList& rays);
		void setDumpfile(const std::string& dumpFile);

	protected:
		virtual SymmetryComputation* devise(const RayComputation* rayComp, const Polyhedron & data, 
						const PermutationGroup & permGroup, FacesUpToSymmetryList& rays) = 0;
		
		static uint ms_instanceCounter;
		static yal::LoggerPtr logger;
		uint recursionDepth() const { return m_recursionDepth; }
	private:
		char* m_dumpFilename;
		
		typedef std::list<SymmetryComputationMemento*> SClist;
		SClist m_usedComputations;
		SClist::iterator m_compIt;
		uint m_recursionDepth;
		
		SymmetryComputation* symmetryComputationFactory(SymmetryComputationMethod method, const RayComputation* rayComp,
								const Polyhedron& data, const PermutationGroup & permGroup, FacesUpToSymmetryList& rays) {
			switch (method) {
				case DIRECT:
					return new SymmetryComputationDirect(this, rayComp, data, permGroup, rays);
				case ADM:
					return new SymmetryComputationADM(this, rayComp, data, permGroup, rays);
				case IDM:
					return new SymmetryComputationIDM(this, rayComp, data, permGroup, rays);
				default:
					return NULL;
			}
		}
};


class RecursionStrategyDirect : public RecursionStrategy {
  protected:
    virtual SymmetryComputation* devise(const RayComputation* rayComp, const Polyhedron & data, 
            const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
    {
      return new SymmetryComputationDirect(this, rayComp, data, permGroup, rays);
    }
};

class RecursionStrategyADM : public RecursionStrategy {
  public:
    RecursionStrategyADM(double threshold) : m_estimateThreshold(threshold) {}
  protected:
    virtual SymmetryComputation* devise(const RayComputation* rayComp, const Polyhedron & data, 
            const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
    {
      std::list<FaceWithData> dummy;
      SymmetryComputationDirect* sd = new SymmetryComputationDirect(this, rayComp, data, permGroup, rays);
      const double est = sd->probe(data, permGroup, dummy);
      YALLOG_INFO(logger, "estimation: " << est << " <? " << m_estimateThreshold);
      if (est < m_estimateThreshold)
        return sd;
      delete sd;
      return new SymmetryComputationADM(this, rayComp, data, permGroup, rays);
    }
  private:
    const double m_estimateThreshold;
};

class RecursionStrategyADMDimension : public RecursionStrategy {
  public:
    RecursionStrategyADMDimension(uint threshold) : m_dimensionThreshold(threshold) {}
  protected:
    virtual SymmetryComputation* devise(const RayComputation* rayComp, const Polyhedron & data, 
            const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
    {
      const uint dim = data.workingDimension();
      YALLOG_INFO(logger, "working dim: " << dim << " <=? " << m_dimensionThreshold);
      if (dim <= m_dimensionThreshold)
        return new SymmetryComputationDirect(this, rayComp, data, permGroup, rays);
      return new SymmetryComputationADM(this, rayComp, data, permGroup, rays);
    }
  private:
    const uint m_dimensionThreshold;
};

class RecursionStrategyADMIncidence : public RecursionStrategy {
  public:
    RecursionStrategyADMIncidence(uint threshold) : m_incidenceThreshold(threshold) {}
  protected:
    virtual SymmetryComputation* devise(const RayComputation* rayComp, const Polyhedron & data, 
            const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
    {
      const uint rows = data.rows();
      YALLOG_INFO(logger, "number of rows: " << rows << " <=? " << m_incidenceThreshold);
      if (rows <= m_incidenceThreshold)
        return new SymmetryComputationDirect(this, rayComp, data, permGroup, rays);
      return new SymmetryComputationADM(this, rayComp, data, permGroup, rays);
    }
  private:
    const uint m_incidenceThreshold;
};

}
#endif
