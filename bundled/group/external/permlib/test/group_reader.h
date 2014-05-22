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


#include <list>
#include <string>
#include <fstream>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

namespace permlib { namespace test {

template<class PERM>
class GroupReader {
public:
	bool read(const std::string& s) {
		return read(s.c_str());
	}
	
	bool read(const char* filename) {
		m_n = 0;
		m_base.clear();
		m_generators.clear();
		
		std::ifstream file;
		file.open(filename);
		if (!file.is_open()) {
			std::cerr << "opening " << filename << " failed" << std::endl;
			return false;
		}
		
		std::string line;
		while (!file.eof()) {
			std::getline(file, line);
			if (line.length() < 2 || line[0] == '#')
				continue;
			
			if (line[0] == 'n') {
				m_n = boost::lexical_cast<unsigned int>(line.substr(1));
			} else if (line[0] == 'B') {
				typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
				boost::char_separator<char> sepBase(",");
				std::string sub = line.substr(1);
				tokenizer tokens(sub, sepBase);
				for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
					m_base.push_back( boost::lexical_cast<dom_int>(*tok_iter) - 1 );
					BOOST_ASSERT( m_base.back() < m_n );
				}
			} else {
				BOOST_ASSERT( m_n > 0 );
				typename PERM::ptr gen(new PERM(m_n, line));
				m_generators.push_back(gen);
			}
		}
		file.close();
		
		return m_n > 0;
	}
	
	unsigned int n() const { return m_n; }
	const std::list<dom_int>& base() const { return m_base; }
	const std::list<typename PERM::ptr>& generators() const { return m_generators; }
private:
	unsigned int m_n;
	std::list<dom_int> m_base;
	std::list<typename PERM::ptr> m_generators;
};

} } // end NS
