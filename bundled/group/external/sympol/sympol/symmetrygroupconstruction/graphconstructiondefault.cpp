#include "graphconstructiondefault.h"
#include "matrixconstruction.h"
#include "../yal/logger.h"

#include <permlib/permlib_api.h>
#include <permlib/search/partition/matrix_automorphism_search.h>
#include <permlib/symmetric_group.h>

using namespace sympol;
using namespace yal;
using namespace permlib;

static LoggerPtr logger(Logger::getLogger("SymGraphD "));

boost::shared_ptr<sympol::PermutationGroup> GraphConstructionDefault::compute(const MatrixConstruction* matrix) const {
	YALLOG_DEBUG(logger, "start graph automorphism search with PermLib");
	SymmetricGroup<PERM> s_n(matrix->dimension());
	partition::MatrixAutomorphismSearch<SymmetricGroup<PERM>, TRANSVERSAL> mas(s_n, false);
	// set up search for matrix automorphism which map linearities onto linearities
	const std::set<unsigned int>& linearities = matrix->linearities();
	mas.construct(*matrix, linearities.begin(), linearities.end());
	
	BSGS<PERM,TRANSVERSAL>* K = new BSGS<PERM,TRANSVERSAL>(matrix->dimension());
	mas.search(*K);
	YALLOG_INFO(logger, "matrix automorphism search complete; found group of order " << K->order<mpz_class>());
	
	return boost::shared_ptr<PermutationGroup>(K);
}
