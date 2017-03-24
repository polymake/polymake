# PermLib

## Overview

PermLib is a C++ library for permutation computations. 
Currently it features the following operations:

* base and strong generating set (BSGS) construction
* set stabilizers and are-in-same-orbit checks
* lexicographically least set of an orbit
* block systems and primitivity
* very basic group type recognitions
* specializations for direct products of symmetric groups

It is distributed under a BSD license.

## Usage

PermLib is implemented in C++ header files only and makes extensive use of [Boost](http://www.boost.org).
The unit tests and examples can be built with [CMake](http://www.cmake.org).

At the moment there are two different kinds of high-level APIs:
* function based, in *permlib_api.h* (example in *examples/api-example.cpp*)
* class based, in *abstract_permutation_group.h* (no example yet)

More information can be found at http://www.math.uni-rostock.de/~rehn/software/permlib.html
