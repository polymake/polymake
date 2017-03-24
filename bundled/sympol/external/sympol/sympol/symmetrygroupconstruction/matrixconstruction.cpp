#include "matrixconstruction.h"

using namespace sympol;
using namespace yal;

static LoggerPtr logger(Logger::getLogger("SymMatrix "));

void MatrixConstruction::initData(const Polyhedron& poly, unsigned int numberOfWeights) {
	m_dimension = poly.rows();
	m_numberOfWeights = numberOfWeights;
	const std::list<unsigned long>& linearities = poly.linearities();
	m_linearities.insert(linearities.begin(), linearities.end());
}
