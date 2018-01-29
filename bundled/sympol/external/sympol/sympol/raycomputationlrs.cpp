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

#include "configuration.h"
#include "raycomputationlrs.h"
#include "polyhedron.h"

extern "C" {
  #include <lrslib.h>
}

#include <ctime>
#include <cstdio>

using namespace yal;
using namespace sympol;

//
// static members
//
FILE* RayComputationLRS::ms_fIn = NULL;
FILE* RayComputationLRS::ms_fOut = NULL;
bool RayComputationLRS::ms_bInitialized = false;
const char* RayComputationLRS::ms_chName = "LRS";
LoggerPtr RayComputationLRS::logger(Logger::getLogger("RayCompLRS"));


bool RayComputationLRS::initialize() {
    if (!RayComputationLRS::ms_bInitialized) {
        // take some pseudo-devices that lrs can operate on
        RayComputationLRS::ms_fIn = std::fopen("/dev/null","r");
        RayComputationLRS::ms_fOut = std::fopen("/dev/null","w");
        // init lrs MP arithmetic
        if (!lrs_mp_init(0, RayComputationLRS::ms_fIn, RayComputationLRS::ms_fOut)) {
            return false;
        }
        
        RayComputationLRS::ms_bInitialized = true;       
        return true;
    }
    
    return true;
}

bool RayComputationLRS::finish() {
    if (!RayComputationLRS::ms_bInitialized) {
        return true;
    }

    if (RayComputationLRS::ms_fIn != NULL) {
        if (std::fclose(RayComputationLRS::ms_fIn)) {
            return false;
        }
    }
    if (RayComputationLRS::ms_fOut != NULL) {
        if (std::fclose(RayComputationLRS::ms_fOut)) {
            return false;
        }
    }
    
    RayComputationLRS::ms_bInitialized = false;
    
    return true;
}


/**
 * computes dual description of data into rays
 */
bool RayComputationLRS::dualDescription(const Polyhedron & data, std::vector<FaceWithDataPtr> & rays) const {
	lrs_dic *P;   /* structure for holding current dictionary and indices  */
	lrs_dat *Q;   /* structure for holding static problem data             */
	lrs_mp_vector output; /* one line of output:ray,vertex,facet,linearity */

	if (!initLRS(data, P, Q)) {
		return false;
	}
	output = lrs_alloc_mp_vector (Q->n);

	do {
		for (int col = 0; col <= P->d; ++col) {
			if (lrs_getsolution (P, Q, output, col)) {
				QArrayPtr qRay(new QArray(data.dimension()));
				qRay->initFromArray(output);
				qRay->normalizeArray();

				const Face f = data.faceDescription(*qRay);
				FaceWithDataPtr fdPtr(new FaceWithData(f, qRay, data.incidenceNumber(f)));
				rays.push_back(fdPtr);
			}
		}
	}
	while (lrs_getnextbasis (&P, Q, FALSE));
	//lrs_printtotals (P, Q);    /* print final totals */

	lrs_clear_mp_vector (output, Q->n);
	lrs_free_dic (P,Q);           /* deallocate lrs_dic */
	lrs_free_dat (Q);             /* deallocate lrs_dat */

	return true;
}


double RayComputationLRS::estimate(const Polyhedron & data, std::list<FaceWithData> & rays) const {
    lrs_dic *P;   /* structure for holding current dictionary and indices  */
    lrs_dat *Q;   /* structure for holding static problem data             */
    lrs_mp_matrix Lin=nullptr;
    lrs_mp_vector output; /* one line of output:ray,vertex,facet,linearity */
    
    if (!initLRS(data, P, Q, Lin,
				Configuration::getInstance().lrsEstimates, 
				Configuration::getInstance().lrsEstimateMaxDepth))
		{
        return -1.0;
    }
    output = lrs_alloc_mp_vector (Q->n);

    // TODO: time-measurement thread-safe?

    std::clock_t start, end;
    start = std::clock();
    do {
        for (int col = 0; col <= P->d; ++col) {
            if (lrs_getsolution (P, Q, output, col)) {
            	QArrayPtr qRay(new QArray(data.dimension()));
							qRay->initFromArray(output);
							qRay->normalizeArray();
							rays.push_back(FaceWithData(data.faceDescription(*qRay), qRay));
              YALLOG_DEBUG3(logger, "estimate stumbled upon " << data.faceDescription(*qRay) << " <=> " << *qRay);
            }
        }
    }
    while (lrs_getnextbasis (&P, Q, FALSE));
    end = std::clock();
    YALLOG_DEBUG3(logger, "Estimate " << Q->cest[0] << " " << Q->cest[1] << " " << Q->cest[2] << " " << Q->cest[3] << " " << Q->cest[4]);
    double est = 0.0;
    if (Q->cest[2] > 0) {
        est = (Q->count[2]+Q->cest[2])/Q->totalnodes*((double)(end-start) / CLOCKS_PER_SEC );
    }
    
    if (Lin) lrs_clear_mp_matrix(Lin, Q->nredundcol, Q->n);
    lrs_clear_mp_vector (output, Q->n);
    lrs_free_dic (P,Q);           /* deallocate lrs_dic */
    lrs_free_dat (Q);             /* deallocate lrs_dat */

    return est;
}


bool RayComputationLRS::firstVertex(const Polyhedron & data, Face & f, QArray & q, bool requireRay) const {
    lrs_dic *P;   /* structure for holding current dictionary and indices  */
    lrs_dat *Q;   /* structure for holding static problem data             */
    lrs_mp_vector output; /* one line of output:ray,vertex,facet,linearity */
    
    if (!initLRS(data, P, Q)) {
        return false;
    }
    output = lrs_alloc_mp_vector (Q->n);
    
    bool found = false;
    
    do {
        for (int col = 0; col <= P->d; ++col) {
            if (lrs_getsolution (P, Q, output, col)) {
                q.initFromArray(output);
                f = data.faceDescription(q);

                if (!requireRay || (requireRay && q.isRay())) {
                    found = true;
                    q.normalizeArray();
                    break;
                }
            }
        }
    }
    while (!found && lrs_getnextbasis (&P, Q, FALSE));

    if (found)
        YALLOG_DEBUG3(logger, "found first vertex " << q);
    //lrs_printtotals (P, Q);    /* print final totals */
    
    lrs_clear_mp_vector (output, Q->n);
    lrs_free_dic (P,Q);           /* deallocate lrs_dic */
    lrs_free_dat (Q);             /* deallocate lrs_dat */
    
    return found;
}

/**
 * determines redundant inequalities of given polyhedron
 * indices of redundant inequalities are stored in the list lstRedundancies
 * if the computation yields rays, these are stored in the list myRays
 * @return computation success
 */
bool RayComputationLRS::determineRedundancies(Polyhedron & data, std::list<FaceWithData> & myRays) const {
    lrs_dic *P;   /* structure for holding current dictionary and indices  */
    lrs_dat *Q;   /* structure for holding static problem data             */
    lrs_mp_vector output; /* one line of output:ray,vertex,facet,linearity */
    
    if (!initLRS(data, P, Q)) {
        return false;
    }
    output = lrs_alloc_mp_vector (Q->n);
    
    // the following code is mostly copied from LRS redund_main
    
    /* note some of these may have been changed in getting initial dictionary        */
    ulong m = P->m_A;
    ulong d = P->d;
    ulong nlinearity = Q->nlinearity;
    ulong lastdv = Q->lastdv;
    ulong* redineq = new ulong[m+1];
    ulong ineq;
    
    /* linearities are not considered for redundancy */

    for (ulong i = 0; i < nlinearity; i++) {
        redineq[Q->linearity[i]] = 2L;
    }

    /* rows 0..lastdv are cost, decsion variables, or linearities  */
    /* other rows need to be tested                                */
    for (ulong index = lastdv + 1; index <= m + d; index++) {
        ineq = Q->inequality[index - lastdv]; /* the input inequality number corr. to this index */

        redineq[ineq] = checkindex (P, Q, index);
    }  /* end for index ..... */
    
    std::list<ulong> redundancies;
    for (ulong i=0; i < m; ++i) {
        if (redineq[i+1] == ONE) {
            // no typo here - we have to map (see above)
            redundancies.push_back(i);
        }
    }
    data.addRedundancies(redundancies);
    
    // look for vertices that we get from redundancy check
    for (int col = 0; col <= P->d; ++col) {
        if (lrs_getsolution (P, Q, output, col)) {
						QArrayPtr qRay(new QArray(data.dimension()));
            qRay->initFromArray(output);
            myRays.push_back(FaceWithData(data.faceDescription(*qRay), qRay));
            YALLOG_DEBUG3(logger, "redund stumbled upon " << data.faceDescription(*qRay) << " <=> " << *qRay);
        }
    }
    lrs_clear_mp_vector (output, Q->n);
    
    lrs_free_dic (P,Q);           /* deallocate lrs_dic */
    lrs_free_dat (Q);             /* deallocate lrs_dat */
    delete[] redineq;

    return true;
}

/**
 * computes a list of indices of redundant columns in the inequalities matrix defining the polyhedron
 * @return computation success
 */
bool RayComputationLRS::determineRedundantColumns(const Polyhedron & data, std::set<ulong> & redundantColumns) const {
	lrs_dic *P;   /* structure for holding current dictionary and indices  */
	lrs_dat *Q;   /* structure for holding static problem data             */

	if (!initLRS(data, P, Q)) {
		return false;
	}

	if (Q->homogeneous) {
		redundantColumns.insert(0);
	} else if (Q->nredundcol > 0) {
		for (int k=0; k<Q->nredundcol; ++k) {
			redundantColumns.insert(Q->redundcol[k]);
		}
	}

	lrs_free_dic (P,Q);           /* deallocate lrs_dic */
	lrs_free_dat (Q);             /* deallocate lrs_dat */

	return true;
}

bool RayComputationLRS::getLinearities(const Polyhedron & data, std::list<QArrayPtr>& linearities) const {
	lrs_dic *P;   /* structure for holding current dictionary and indices  */
	lrs_dat *Q;   /* structure for holding static problem data             */
	lrs_mp_matrix Lin=nullptr;

	if (!initLRS(data, P, Q, Lin, 0, 0)) {
		return false;
	}

	for (unsigned int i = 0; i < Q->nredundcol; ++i) {
		QArrayPtr row(new QArray(data.dimension()));
		row->initFromArray(Lin[i]);
		linearities.push_back(row);
	}

        if (Lin) lrs_clear_mp_matrix(Lin, Q->nredundcol, Q->n);
	return true;
}

// --------------------------------------
//
//  HELPER METHODS start below
//
//
// --------------------------------------

bool RayComputationLRS::initLRS(const Polyhedron & data, lrs_dic* & P, lrs_dat* & Q) const {
    lrs_mp_matrix Lin=nullptr;
    const bool result=initLRS(data, P, Q, Lin, 0, 0);
    if (result && Lin) lrs_clear_mp_matrix(Lin, Q->nredundcol, Q->n);
    return result;
}

bool RayComputationLRS::initLRS(const Polyhedron & data, lrs_dic* & P, lrs_dat* & Q, lrs_mp_matrix& Lin, int estimates, int maxDepth) const {
    /* allocate and init structure for static problem data */
    Q = lrs_alloc_dat ("LRS globals");
    if (Q == NULL) {
        return false;
    }
    
    /* now flags in lrs_dat can be set */
    
    Q->n = data.dimension();           /* number of input columns         (dimension + 1 )  */
    Q->m = data.rows();         /* number of input rows = number of inequalities     */

    // setup estimate mode
    if (estimates > 0) {
        Q->runs = estimates;
        Q->maxdepth = maxDepth;
    }
    
    P = lrs_alloc_dic (Q);   /* allocate and initialize lrs_dic      */
    if (P == NULL) {
        return false;
    }
    
    // prepare polyhedron data for use in LRS
    fillModelLRS(data, P, Q);

    if (!lrs_getfirstbasis (&P, Q, &Lin, FALSE)) {
        if (Lin) lrs_clear_mp_matrix(Lin, Q->nredundcol, Q->n);
        lrs_free_dic (P,Q);           /* deallocate lrs_dic */
        lrs_free_dat (Q);             /* deallocate lrs_dat */
        return false;
    }
    
    return true;
}





void RayComputationLRS::fillModelLRS(const Polyhedron & data, lrs_dic *P, lrs_dat *Q) const {
    unsigned long i = 1, j = 0;
    unsigned long n = Q->n;
    
    YALLOG_DEBUG3(logger, "LRS polyhedron " << data);
    
    lrs_mp_vector num = lrs_alloc_mp_vector (n);
    lrs_mp_vector den = lrs_alloc_mp_vector (n);
    
    const unsigned int modeEQ = 0; // EQ for Equal
    const unsigned int modeGE = 1; // GE for Greater Equal
    BOOST_FOREACH(const QArray& row, data.rowPair()) {
      for (j=0; j<n; ++j) {
        mpq_get_num(num[j], row[j]);
        mpq_get_den(den[j], row[j]);
      }
      // NOTE lrs_set_row_mp index i starts at 1
      lrs_set_row_mp(P, Q, i, num, den, 
                     (data.isLinearity(row)) ? modeEQ : modeGE);
      
      ++i;
    }
    
    lrs_clear_mp_vector (num, n);
    lrs_clear_mp_vector (den, n);
}

