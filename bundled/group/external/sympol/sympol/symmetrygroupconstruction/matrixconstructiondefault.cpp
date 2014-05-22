#include "matrixconstructiondefault.h"
#include "../matrix/invert.h"
#include "../matrix/rank.h"
#include "../yal/logger.h"

using namespace sympol;
using namespace yal;

static LoggerPtr logger(Logger::getLogger("SymMatrixD"));

bool MatrixConstructionDefault::construct(const Polyhedron& poly) {
	YALLOG_DEBUG(logger, "matrix construction with default");
	const ulong matrixRank = poly.dimension() - 1;
	const ulong matrixRows = poly.rows();
	typedef matrix::Matrix<mpq_class> QMatrix;
  
	// compute column defect of inequality matrix
	QMatrix* Acopy = new QMatrix(poly.rows(), matrixRank);
	{ulong j=0;
	BOOST_FOREACH(const QArray& row, poly.rowPair()) {
		for (ulong i=0; i<matrixRank; ++i) {
			mpq_set(Acopy->at(j,i).get_mpq_t(), row[i+1]);
		}
		++j;
	}}
	matrix::Rank<QMatrix> r(Acopy);
	// freeColumns will contain the column indices we may skip to get a full dimensional polyhedron
	std::set<uint> freeColumns;
	r.columnDefect(std::inserter(freeColumns, freeColumns.end()));
	delete Acopy;	
    
	typedef std::map<mpq_class,uint> WeightMap;
	WeightMap weights;
	mpq_t temp;
	mpq_init(temp);
	
	//
	// calculate vertex weights
	//
	QMatrix Q(matrixRank - freeColumns.size());
	ulong iQ = 0, jQ = 0;
	for (ulong i=0; i<matrixRank; ++i) {
		if (freeColumns.count(i)) {
			YALLOG_DEBUG3(logger, "free column #" << (i+1));
			continue;
		}
		jQ = 0;
		for (ulong j=0; j<matrixRank; ++j) {
			if (freeColumns.count(j))
				continue;
			BOOST_ASSERT(iQ < matrixRank - freeColumns.size());
			BOOST_ASSERT(jQ < matrixRank - freeColumns.size());
			BOOST_FOREACH(const QArray& row, poly.rowPair()) {
				mpq_mul(temp, row[i+1], row[j+1]);
				mpq_add(Q.at(iQ,jQ).get_mpq_t(), Q.at(iQ,jQ).get_mpq_t(), temp);
				//Q.at(i,j) += row[i] * row[j];
			}
			++jQ;
		}
		++iQ;
	}
	
	YALLOG_DEBUG2(logger, "supposed rank of Q = " << matrixRank << "; rank defect = " << freeColumns.size() << ";  Q = " << std::endl << Q);
	
	QMatrix Qinv(Q.rows());
	if (!matrix::Invert<QMatrix>(&Q).invert(&Qinv)) {
		YALLOG_ERROR(logger, "could not invert matrix");
		return false;
	}
	
	YALLOG_INFO(logger, "matrix inversion complete");
	YALLOG_DEBUG3(logger, "Qinv = " << std::endl << Qinv);
    
	m_zMatrix = new matrix::ZMatrix(matrixRows);
    
	uint weightIndex = 0;
	ulong i = 0, j = 0;
	BOOST_FOREACH(const QArray& row1, poly.rowPair()) {
		j = 0;
		BOOST_FOREACH(const QArray& row2, poly.rowPair()) {
			if (i < j)
				break;
			mpq_class newWeight;
			ulong kQ = 0, lQ = 0;
			for (uint k = 0; k < matrixRank; ++k) {
				if (freeColumns.count(k))
					continue;
				lQ = 0;
				for (uint l = 0; l < matrixRank; ++l) {
					if (freeColumns.count(l))
						continue;
					BOOST_ASSERT(kQ < matrixRank - freeColumns.size());
					BOOST_ASSERT(lQ < matrixRank - freeColumns.size());
					mpq_mul(temp, row1[k+1], row2[l+1]);
					mpq_mul(temp, temp, Qinv.at(kQ,lQ).get_mpq_t());
					mpq_add(newWeight.get_mpq_t(), newWeight.get_mpq_t(), temp);
					//newWeight += Qinv.at(k,l) * row1[k] * row2[l];
					++lQ;
				}
				++kQ;
			}
			std::pair<WeightMap::iterator, bool> suc = weights.insert(std::make_pair(newWeight, weightIndex));
			m_zMatrix->at(i,j) = suc.first->second;
			m_zMatrix->at(j,i) = suc.first->second;
			if (suc.second)
				++weightIndex;
			++j;
		}
		++i;
	}
	mpq_clear(temp);
	
	m_zMatrix->k() = weightIndex;
	
	YALLOG_DEBUG(logger, "zMatrix = " << std::endl << *m_zMatrix);
	
	initData(poly, weightIndex);
	return true;
}


unsigned int MatrixConstructionDefault::weightAt(unsigned int i, unsigned int j) const {
	BOOST_ASSERT(m_zMatrix != 0);
	return m_zMatrix->at(i,j);
}
