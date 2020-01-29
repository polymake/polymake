/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <gmpxx.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "polymake/Integer.h"

namespace libnormaliz {

bool try_convert(double& a, const pm::Integer& b)
{
   a = static_cast<double>(b);
   return true;
}

bool try_convert(long& a, const pm::Integer& b)
{
  if (b.fits_into_Int()) {
    a = static_cast<long>(b);
    return true;
  }
  return false;
}

bool try_convert(long long& a, const pm::Integer& b)
{
  if (b.fits_into_long_long()) {
    a = static_cast<long long>(b);
    return true;
  }
  return false;
}

bool try_convert(mpz_class& a, const pm::Integer& b)
{
  a = mpz_class(b.get_rep());
  return true;
}

bool try_convert(pm::Integer& a, const mpz_class& b)
{
  a = pm::Integer(b);
  return true;
}

bool try_convert(pm::Integer& a, const long& b)
{
  a = b;
  return true;
}

bool try_convert(pm::Integer& a, const long long& b)
{
  a = b;
  return true;
}

bool try_convert(pm::Integer& a, const double& b)
{
  a = b;
  return true;
}

double convert_to_double(const pm::Integer& a)
{
  return double(a);
}

pm::Integer operator%(size_t a, const pm::Integer& b)
{
  if (a <= (unsigned long)std::numeric_limits<long>::max())
    return long(a) % b;
  else
    return pm::Integer(a) % b;
}

pm::Integer operator*(unsigned long a, const pm::Integer& b)
{
  if (a <= (unsigned long)std::numeric_limits<long>::max())
    return long(a) * b;
  else
    return pm::Integer(a) * b;
}

pm::Integer operator*(unsigned int a, const pm::Integer& b)
{
  return static_cast<unsigned long>(a) * b;
}

inline bool int_quotient(pm::Integer& Quot, const pm::Integer& Num, const pm::Integer& Den){
   Quot = abs(Num)/abs(Den);
   return Quot*abs(Den)!=abs(Num);
}

inline bool int_quotient(long long& Quot, const pm::Integer& Num, const pm::Integer& Den){
   pm::Integer QI = abs(Num)/abs(Den);
   Quot = (long long) QI;
   return QI*abs(Den)!=abs(Num);
}

}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wreturn-type"
#if __clang_major__ >= (defined(__APPLE__) ? 9 : 4)
#pragma clang diagnostic ignored "-Winstantiation-after-specialization"
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wreturn-type"
#if __GNUC__ >= 6
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wcatch-value=0"
#endif // gcc8
#endif // gcc6
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include "libnormaliz/linear_algebra.cpp"
#include "libnormaliz/cone_and_control.cpp"
#include "libnormaliz/enumeration.cpp"
#include "libnormaliz/other_algorithms.cpp"
#include "libnormaliz/nmz_nauty.cpp"
#include "libnormaliz/primal.cpp"
#include "libnormaliz/output.cpp"


#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace libnormaliz {

template<>
pm::Integer int_max_value_dual<pm::Integer>()
{
  assert(false);
  return 0;
}

template<>
pm::Integer int_max_value_primary<pm::Integer>()
{
  assert(false);
  return 0;
}

template<>
bool using_GMP<pm::Integer>()
{
  return true;
}

template<>
bool check_range<pm::Integer>(const pm::Integer& m)
{
  return true;
}

// avoid issue with libc++ and libgmpxx
template<>
ArithmeticException::ArithmeticException(const mpz_class& convert_number) {
   std::stringstream stream;
   stream << "Could not convert " << pm::Integer(convert_number) << ".\n";
   stream << "Overflow detected. A fatal size excess or  a computation overflow.\n If Normaliz has terminated and you are "
      "using LongLong, rerun without it.";
   msg = stream.str();
}

template class Cone<pm::Integer>;
template class Matrix<pm::Integer>;
template class Sublattice_Representation<pm::Integer>;
template class AutomorphismGroup<pm::Integer>;
}
