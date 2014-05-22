// ---------------------------------------------------------------------------
//
//  This file is part of PermLib.
//
// Copyright (c) 2009-2012 Thomas Rehn <thomas@carmen76.de>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------------------------------------------------------------------

#include "group_reader.h"

namespace permlib { namespace test {

struct GroupInformation {
	std::string filename;
	unsigned int n;
	unsigned int order;
	
	GroupInformation(const std::string &f, unsigned int _n, unsigned int _order) : filename(f), n(_n), order(_order) {}
    
	bool operator<(const GroupInformation& info) const {
		return n < info.n || filename < info.filename;
	}
};

const GroupInformation info_trivial("../data/G_trivial42", 42, 1);
const GroupInformation info_test33("../data/G_test33", 33, 163680);
const GroupInformation info_psu4_3("../data/G_psu4_3", 820, 3265920);
const GroupInformation info_1997("../data/G_test1997", 13, 13*12*9*4);
const GroupInformation info_testThesis("../data/G_testThesis", 10, 28800);
const GroupInformation info_e6("../data/G_e6", 36, 51840);
const GroupInformation info_e7("../data/G_e7", 63, 1451520);
const GroupInformation info_e8("../data/G_e8", 120, 348364800);
const GroupInformation info_metric5("../data/G_metric5", 40, 1920);
//const GroupInformation info_metric6("../data/G_metric6", 80, 0);
//const GroupInformation info_metric7("../data/G_metric7", 140, 0);
const GroupInformation info_cyclic500("../data/G_cyclic500", 500, 500);
const GroupInformation info_cyclic10("../data/G_cyclic10", 10, 10);
const GroupInformation info_cyclic37_2("../data/G_cyclic37-2", 74, 37);
const GroupInformation info_myciel3("../data/G_myciel3", 33, 60);
// group is transitive and isomorphic to S_4 x D_10
const GroupInformation info_myciel4sub("../data/G_myciel4sub", 20, 240);
const GroupInformation info_cov1075("../data/G_cov1075", 120, 3628800L);
const GroupInformation info_tanglegram2("../data/G_tanglegram2", 4714, 0);
const GroupInformation info_rout("../data/G_rout", 556, 120);
// wreath product of S_3 and S_5
const GroupInformation info_S3wrS5("../data/G_S3wrS5", 15, 933120L);
// wreath product of S_5 and S_3
const GroupInformation info_S5wrS3("../data/G_S5wrS3", 15, 10368000L);
const GroupInformation info_S6("../data/G_S6", 6, 720L);
const GroupInformation info_A9("../data/G_A9", 9, 181440L);
const GroupInformation info_A12("../data/G_A12", 12, 239500800L);
const GroupInformation info_S6_3("../data/G_S6-3", 18, 720L);
// Klein four-group ~ C_2 x C_2
const GroupInformation info_Klein4("../data/G_klein4", 4, 4L);
const GroupInformation info_lexminTest1("../data/G_lexminTest1", 6, 36L);
const GroupInformation info_lexminTest2("../data/G_lexminTest2", 8, 48L);

template<class PERM>
unsigned int readGroup(const std::string &filename, std::list<typename PERM::ptr> &groupGenerators) {
	GroupReader<PERM> reader;
	reader.read(filename);
	groupGenerators = reader.generators();
	return reader.n();
}

} } // end NS
