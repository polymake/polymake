#include "matrixconstructioneigen.h"
#include "../yal/logger.h"

#if HAVE_EIGEN

#include <Eigen/QR>

using namespace sympol;
using namespace yal;
using namespace permlib;

static LoggerPtr logger(Logger::getLogger("SymMatrixE"));

bool MatrixConstructionEigen::construct(const Polyhedron& poly) {
	YALLOG_DEBUG(logger, "matrix construction with Eigen");
	const ulong matrixDimension = poly.dimension() - 1;
	const ulong matrixRows = poly.rows();
	
	m_Acopy = FloatMatrix(matrixDimension, matrixRows);
	unsigned int j = 0;
	BOOST_FOREACH(const QArray& row, poly.rowPair()) {
		for (unsigned int i = 0; i < matrixDimension; ++i) {
			m_Acopy(i, j) = mpq_get_d(row[i+1]);
		}
		++j;
	}
	
	Eigen::FullPivHouseholderQR<FloatMatrix>* qrA = new Eigen::FullPivHouseholderQR<FloatMatrix>(m_Acopy);
	const unsigned int rank = qrA->rank();
	
	// find linear independent rows (columns in m_Acopy) that span a basis
	m_rowBasis.resize(rank);
	for (unsigned int i = 0; i < rank; ++i)
		m_rowBasis[i] = qrA->colsPermutation().indices()[i];
	
	YALLOG_DEBUG(logger, "A has rank " << rank << ", expected " << matrixDimension);
	if (rank < matrixDimension) {
		std::vector<unsigned int> fullDimensionalRows(matrixDimension);
		for (unsigned int i = 0; i < matrixDimension; ++i) {
			fullDimensionalRows[i] = i;
		}
		for (unsigned int i = 0; i < m_Acopy.rows() ; ++i) {
			std::swap(fullDimensionalRows[i], fullDimensionalRows[ qrA->rowsTranspositions().coeff(i) ]);
		}
		std::sort(fullDimensionalRows.begin(), fullDimensionalRows.begin() + rank);
		
		m_Acopy = FloatMatrix(rank, matrixRows);
		YALLOG_DEBUG2(logger, "refill matrix A");
		unsigned int j = 0;
		BOOST_FOREACH(const QArray& row, poly.rowPair()) {
			for (unsigned int i=0; i < rank; ++i) {
				m_Acopy(i, j) = mpq_get_d( row[ fullDimensionalRows[i] + 1 ] );
			}
			++j;
		}
	}
	delete qrA;
	
	YALLOG_DEBUG2(logger, "filled matrix A");
	
	FloatMatrix Q(rank, rank);
	for (unsigned int j = 0; j < rank; ++j) {
		YALLOG_DEBUG2(logger, " j = " << j );
		for (unsigned int k = 0; k < rank; ++k) {
			Q(j,k) = 0;
			for (unsigned int i = 0; i < matrixRows; ++i) {
				Q(j,k) += m_Acopy(j,i) * m_Acopy(k,i);
			}
		}
	}
	
	YALLOG_DEBUG(logger, "start matrix calculation");
	
	Eigen::FullPivHouseholderQR<FloatMatrix> lu(Q);
	BOOST_ASSERT( lu.isInvertible() );
	FloatMatrix Qinv = lu.inverse();
	m_W = m_Acopy.transpose() * Qinv * m_Acopy;
	
	BOOST_ASSERT( static_cast<unsigned int>(m_W.rows()) == matrixRows );
	BOOST_ASSERT( static_cast<unsigned int>(m_W.cols()) == matrixRows );
	
	YALLOG_DEBUG(logger, "start graph construction");
	
	unsigned int weightIndex = 0;
	for (unsigned int i = 0; i < matrixRows; ++i) {
		for (unsigned int j = i; j < matrixRows; ++j) {
			WeightMap::const_iterator it = m_weights.find( m_W(i,j) );
			if (it == m_weights.end()) {
				m_weights[ m_W(i,j) ] = weightIndex++;
				YALLOG_DEBUG2(logger, "found weight " << m_W(i,j) );
			}
		}
	}
	
	YALLOG_DEBUG(logger, "use " << weightIndex << " weights");
	
	initData(poly, weightIndex);
	return true;
}

unsigned int MatrixConstructionEigen::weightAt(unsigned int i, unsigned int j) const {
	BOOST_ASSERT(m_W.rows() > 0 && m_W.cols() > 0);
	WeightMap::const_iterator it = m_weights.find( m_W(i,j) );
	BOOST_ASSERT( it != m_weights.end() );
	return (*it).second;
}

bool MatrixConstructionEigen::checkSymmetries(boost::shared_ptr<PermutationGroup>& group) const {
	// check that permutations are symmetries of the polyhedron
	YALLOG_DEBUG(logger, "checkSymmetries");
	
	const unsigned int rank = m_Acopy.rows();
	FloatMatrix Abase(rank, rank);
	for (unsigned int i = 0; i < rank; ++i)
		for (unsigned int j = 0; j < rank; ++j)
			Abase(j, i) = m_Acopy(j, m_rowBasis[i]);
	FloatMatrix AbaseInverse = Abase.inverse();
	
	BOOST_FOREACH(const Permutation::ptr& gen, group->S) {
		FloatMatrix AbaseImage(rank, rank);
		for (unsigned int i = 0; i < rank; ++i)
			for (unsigned int j = 0; j < rank; ++j)
				AbaseImage(j, i) = m_Acopy(j, gen->at(m_rowBasis[i]));
		
		//std::cout << "AbaseImage\n" << Abase << std::endl;
		FloatMatrix transform = AbaseImage * AbaseInverse;
		for (unsigned int i = 0; i < rank; ++i) {
			for (unsigned int j = 0; j < rank; ++j) {
				if (fabs(transform(i, j)) < zeroTolerance)
					transform(i, j) = 0;
			}
		}
		
		Eigen::Matrix<int, Eigen::Dynamic, 1> genIndices(gen->size());
		for (unsigned int i = 0; i < gen->size(); ++i) {
			genIndices(i) = gen->at(i);
			// check that permutation respects equations
			BOOST_ASSERT( ! ((isEquation(i) > 0) ^ (isEquation(gen->at(i)) > 0) ) );
		}
		Eigen::PermutationMatrix<Eigen::Dynamic> genMatrix(genIndices);
		FloatMatrix Adiff = m_Acopy * genMatrix - transform * m_Acopy;
		YALLOG_DEBUG2(logger, "transformation/permutation gap is " << Adiff.squaredNorm());
		if (Adiff.squaredNorm() - zeroTolerance > 0) {
			YALLOG_WARNING(logger, "permutation " << (*gen) << " is not a symmetry; aborting");
			return false;
		}
		
		/*
		if (transform.colwise().sum().maxCoeff() > 1.5 || transform.rowwise().sum().maxCoeff() > 1.5) {
			std::cout << "non GL_n(ZZ) matrix transformation:\n" << transform << std::endl;
		}
		*/
	}
	return true;
}

#endif // HAVE_EIGEN
