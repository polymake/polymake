#include "graphconstructionbliss.h"
#include "matrixconstruction.h"
#include "../yal/logger.h"

#include <permlib/permlib_api.h>
#include <permlib/construct/known_bsgs_construction.h>

#if HAVE_BLISS

#include <bliss/graph.hh>

using namespace sympol;
using namespace yal;
using namespace permlib;

static LoggerPtr logger(Logger::getLogger("SymGraphB "));


/// data structure used for the bliss callback
struct BlissData {
	unsigned int T;
	std::list<Permutation::ptr> generators;
};


/// bliss callback for graph automorphism generators
static
void blisshook(
	void*                 user_param,
	unsigned int          n,
	const unsigned int*   aut
	)
{
	BOOST_ASSERT( user_param != 0 );
	BlissData* bliss = reinterpret_cast<BlissData*>(user_param);
	
	BOOST_ASSERT( n % bliss->T == 0 );
	Permutation::perm proj(n / bliss->T);
	for (unsigned int i = 0; i < proj.size(); ++i) {
		proj[i] = aut[i];
		BOOST_ASSERT( aut[i] < proj.size() );
	}
	Permutation::ptr p(new Permutation(proj));
	bliss->generators.push_back(p);
}


boost::shared_ptr<sympol::PermutationGroup> GraphConstructionBliss::compute(const MatrixConstruction* matrix) const {
	const unsigned int T = static_cast<unsigned int>(std::ceil( log2((double) matrix->k() + 1.0) ));
	const unsigned int matrixRows = matrix->dimension();
	bliss::Graph G;
	
	for (unsigned int j = 0; j < T; ++j) {
		for (unsigned int i = 0; i < matrixRows; ++i) {
			const unsigned int vindex =  G.add_vertex(2 * j + (matrix->isEquation(i) ? 1 : 0));
			if (j > 0)
				G.add_edge(vindex, vindex-matrixRows);
		}
	}
	
	for (unsigned int i = 0; i < matrixRows; ++i) {
		for (unsigned int j = i; j < matrixRows; ++j) {
			for (unsigned int c = 0; c < T; ++c) {
				if ((1 << c) & matrix->at(i,j)) {
					G.add_edge(i + c * matrixRows, j + c * matrixRows);
				}
			}
		}
	}
	
	YALLOG_DEBUG(logger, "start graph automorphism search with bliss");
	
	bliss::Stats stats;
	BlissData data;
	data.T = T;
	/* Prefer splitting partition cells corresponding to nodes with color 0 or 1,
	 * so that we obtain a group basis beginning with them. */
	G.set_splitting_heuristic(bliss::Graph::shs_f);
	// disable component recursion as advised by Tommi Junttila from bliss
	G.set_component_recursion(false);
	G.find_automorphisms(stats, blisshook, &data);
	if (yal::DEBUG <= yal::ReportLevel::get())
		stats.print(stdout);
	else 
		YALLOG_INFO(logger, "bliss found a symmetry group of order " << stats.get_group_size_approx() );
	
	// deactivate the following code until bliss officially exposes base information
#if 0	
	// set up a BSGS data structure for the symmetry group, using BSGS information from bliss
	
	std::vector<bliss::PathInfo> basePath = G.get_first_path_info();
	std::list<unsigned int> baseVars;
	bool expectBaseVars = true;
	for (unsigned int i = 0; i < basePath.size(); ++i) {
		const unsigned int b = basePath[i].splitting_element;
		if (expectBaseVars && b < matrixRows)
			baseVars.push_back(b);
		else if (b < matrixRows)
			BOOST_ASSERT(b >= matrixRows);
		if (b >= matrixRows)
			expectBaseVars = false;
	}
	
	KnownBSGSConstruction<PERMUTATION, TRANSVERSAL> bsgsSetup(matrixRows);
	boost::shared_ptr<PermutationGroup> group(new PermutationGroup(
		bsgsSetup.construct(data.generators.begin(), data.generators.end(), baseVars.begin(), baseVars.end())
	));
#else
	SchreierSimsConstruction<PERMUTATION, TRANSVERSAL> bsgsSetup(matrixRows);
	boost::shared_ptr<PermutationGroup> group(new PermutationGroup(
		bsgsSetup.construct(data.generators.begin(), data.generators.end())
	));
#endif

	return group;
}

#endif // HAVE_BLISS
