/*
 * Normaliz
 * Copyright (C) 2007-2014  Winfried Bruns, Bogdan Ichim, Christof Soeger
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#ifndef NORMALIZ_EXEPTION_H_
#define NORMALIZ_EXEPTION_H_

#include <exception>
#include <libnormaliz/libnormaliz.h>
#include <libnormaliz/cone_property.h>

namespace libnormaliz {

class NormalizException: public std::exception {
    public:
	virtual const char* what() const throw() = 0;
};

class ArithmeticException: public NormalizException {
    public:
	virtual const char* what() const throw() {
		return "Arithmetic Overflow detected, try a bigger integer type!";
	}
};

class NonpointedException: public NormalizException {
    public:
	virtual const char* what() const throw() {
		return "Cone is not pointed.";
	}
};

class NotIntegrallyClosedException: public NormalizException {
    public:
	virtual const char* what() const throw() {
		return "Original monoid is not integrally closed.";
	}
};

class BadInputException: public NormalizException {
    public:
	virtual const char* what() const throw() {
		return "Some error in the normaliz input data detected!";
	}
};


//class ConeProperties; // forward decl
class NotComputableException: public NormalizException {
    public:
    NotComputableException(){};
    NotComputableException(const ConeProperties& props){};
	virtual const char* what() const throw() {
		return "Could not compute: ...";
	}
};

class FatalException: public NormalizException {
    public:
	virtual const char* what() const throw() {
		return "Fatal error! This should not happen, please contact the developers.";
	}
};


} /* end namespace */

#endif /* LIBNORMALIZ_H_ */
