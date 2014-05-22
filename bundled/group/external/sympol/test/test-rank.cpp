#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE matrix rank computation
#include <boost/test/unit_test.hpp>

#include <gmpxx.h>

typedef unsigned int uint;
typedef unsigned long ulong;
#include "matrix/matrix.h"
#include "matrix/rank.h"

using namespace sympol::matrix;
typedef Matrix<mpq_class> QMatrix;

void checkMatrix(unsigned int dataCols, unsigned int dataRows, const int* data, const unsigned int* expectedRanks) {
	for (unsigned int r = 0; r <= dataRows; ++r) {
		QMatrix q(dataRows, dataCols);
		for (unsigned int i = 0; i < r; ++i) {
			for (unsigned int j = 0; j < dataCols; ++j) {
				q.at(i,j) = data[i * dataCols + j];
			}
		}
		Rank<QMatrix> rankQ(&q);
		BOOST_CHECK_EQUAL(rankQ.rank(), expectedRanks[r]);
		
		q.transpose();
		Rank<QMatrix> rankQT(&q);
		BOOST_CHECK_EQUAL(rankQT.rank(), expectedRanks[r]);
	}
	
	for (unsigned int r = 0; r <= dataRows; ++r) {
		QMatrix q(dataRows, dataCols);
		for (unsigned int i = 0; i < r; ++i) {
			for (unsigned int j = 0; j < dataCols; ++j) {
				q.at(dataRows - 1 - i,j) = data[i * dataCols + j];
			}
		}
		Rank<QMatrix> rankQ(&q);
		BOOST_CHECK_EQUAL(rankQ.rank(), expectedRanks[r]);
		
		q.transpose();
		Rank<QMatrix> rankQT(&q);
		BOOST_CHECK_EQUAL(rankQT.rank(), expectedRanks[r]);
	}
}



BOOST_AUTO_TEST_CASE( matrix_rank )
{
	const unsigned int dataCols = 5;
	const unsigned int dataRows = 5;
	const int data[] = {
		1, 2, 4, 8, 16,
		2, 4, 8, 1, 16,
		4, 8, 1, 2, 32,
		0, 0, 12, 0, 0,
		3, 1, 4, 1, 5
	};
	const unsigned int expectedRanks[] = {
		0, 1, 2, 3, 3, 4
	};
	
	checkMatrix(dataCols, dataRows, data, expectedRanks);
}


BOOST_AUTO_TEST_CASE( matrix_rank2 )
{
	const unsigned int dataCols = 5;
	const unsigned int dataRows = 6;
	const int data[] = {
		1, -1, 0, 0, 0,
		0, 1, -1, 0, 0,
		0, 0, 1, -1, 0,
		3, 2, -5, 0, 0,
		0, 0, 0, 1, -1,
		7, 7, 7, 7, 7
	};
	const unsigned int expectedRanks[] = {
		0, 1, 2, 3, 3, 4, 5
	};
	
	checkMatrix(dataCols, dataRows, data, expectedRanks);
}
