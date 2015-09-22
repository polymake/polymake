/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>
	*/

#ifndef POLYMAKE_ATINT_LOGGING_PRINTER_H
#define POLYMAKE_ATINT_LOGGING_PRINTER_H

/**
  @brief This class defines a logger that can be turned on or off in the following way: Put "using namespace xxx;" in your code and replace xxx with atintlog::donotlog, atintlog::dolog, atintlog::dotrace, depending on what you want. Under donotlog, all  calls to dbglog or dbgtrace have no effect. Unter dolog, only dbglog produces output and under dbgtrace, both commands produce output (to pm::cout). The commands are used as cout, i.e. dbglog << ... and dbgtrace << ...
  */

#include "polymake/client.h"

namespace polymake { namespace tropical{

	/**
	  @brief A dummy streambuffer class that does absolutely nothing. Used for deactivating logging.
	  */
	class DummyBuffer : public std::streambuf{

	};

	static DummyBuffer dummybf;
	static std::ostream dbgstream(&dummybf);


}}

namespace atintlog {

	namespace donotlog{
		/**
		  @brief A logger printer that outputs nothing
		  */
		static pm::PlainPrinter<> dbglog(polymake::tropical::dbgstream); 
		/**
		  @brief A logger printer that outputs nothing
		  */
		static pm::PlainPrinter<> dbgtrace(polymake::tropical::dbgstream);
	}
	namespace dolog{
		/**
		  @brief A logger printer, that prints to pm::cout
		  */
		static pm::PlainPrinter<> dbglog(pm::cout);
		/**
		  @brief A logger printer that outputs nothing
		  */
		static pm::PlainPrinter<> dbgtrace(polymake::tropical::dbgstream);
	}
	namespace dotrace{
		/**
		  @brief A logger printer, that prints to pm::cout
		  */
		static pm::PlainPrinter<> dbglog(pm::cout);
		/**
		  @brief A logger printer that prints to pm::cout
		  */
		static pm::PlainPrinter<> dbgtrace(pm::cout);
	}
}
#endif
