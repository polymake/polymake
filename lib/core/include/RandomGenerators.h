/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_RANDOM_GENERATORS_H
#define POLYMAKE_RANDOM_GENERATORS_H

#include "polymake/Rational.h"
#include "polymake/Bitset.h"
#include "polymake/AccurateFloat.h"
#include "polymake/Vector.h"

#include <cstdlib>
#include <memory>

namespace pm {

namespace perl { class Value; }

class RandomSeed {
public:
   RandomSeed() : data(64, Integer::Reserve()) { renew(); }
   RandomSeed(unsigned long x) : data(long(x)) {}
   RandomSeed(const Integer& x) : data(x) {}
   RandomSeed(perl::Value);

   const Integer& get() const { return data; }

   void renew();
private:
   static int rfd;
   Integer data;
};

class RandomState {
public:
   explicit RandomState(const RandomSeed& seed=RandomSeed())
   {
      gmp_randinit_default(state);
      gmp_randseed(state, seed.get().get_rep());
   }

   RandomState(const RandomState& s)
   {
      gmp_randinit_set(state, s.state);
   }

   RandomState& operator= (const RandomState&)
   {
      // do nothing
      return *this;
   }

   ~RandomState()
   {
      gmp_randclear(state);
   }

   gmp_randstate_t state;

#if GMP_LIMB_BITS==32
   void fix_for_mpfr();
   static void intercept_get_fn(gmp_randstate_t, mp_ptr, unsigned long int);
#endif
};

class SharedRandomState {
public:
   SharedRandomState() {}

   explicit SharedRandomState(const RandomSeed& seed)
      : state_ptr(new RandomState(seed)) {}

   void reset(const RandomSeed& seed=RandomSeed())
   {
      gmp_randseed(state(), seed.get().get_rep());
   }
protected:
   gmp_randstate_t& state() { return state_ptr->state; }

#if GMP_LIMB_BITS==32
   void fix_for_mpfr() { state_ptr->fix_for_mpfr(); }
#else
   static void fix_for_mpfr() {}
#endif

   std::shared_ptr<RandomState> state_ptr;
};

template <typename Generator, typename Eref>
class random_get_iterator {
protected:
   Generator* generator;
public:
   using iterator_category = input_iterator_tag;
   using value_type = typename deref<Eref>::type;
   using reference = Eref;
   using pointer = void*;
   using difference_type = ptrdiff_t;

   random_get_iterator(const Generator* gen_arg=nullptr)
      : generator(const_cast<Generator*>(gen_arg)) {}

   reference operator* () const { return generator->get(); }

   random_get_iterator& operator++() { return *this; }
   random_get_iterator& operator++(int) { return *this; }
   bool operator== (const random_get_iterator& it) const { return generator==it.generator; }
   bool operator!= (const random_get_iterator& it) const { return !operator==(it); }
};

/*
 *  The classes defined below comply with the STL container concept.
 *  To obtain the random numbers, you should either call the get() method,
 *  or create an iterator and dereference it.
 */

template <typename T> class UniformlyRandom;
template <typename T> class UniformlyRandomRanged;

template <typename Top, typename Eref=typename Top::reference>
class GenericRandomGenerator
   : public Generic<Top> {
public:
   using value_type = typename deref<Eref>::type;
   using reference = Eref;
   using const_reference = reference;
   using generic_type = GenericRandomGenerator;

   using iterator = random_get_iterator<Top, Eref>;
   using const_iterator = iterator;
   iterator begin() const { return iterator(static_cast<const Top*>(this)); }
   iterator end() const { return iterator(); }
};

template <>
class UniformlyRandom<long>
   : public SharedRandomState
   , public GenericRandomGenerator<UniformlyRandom<long>, long> {
public:
   explicit UniformlyRandom(const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed) {}

   explicit UniformlyRandom(const SharedRandomState& s)
      : SharedRandomState(s) {}

   long get() { return gmp_urandomb_ui(state(), 32); }
};

template <>
class UniformlyRandomRanged<long>
   : public SharedRandomState
   , public GenericRandomGenerator< UniformlyRandomRanged<long>, long> {
public:
   explicit UniformlyRandomRanged(long max_arg, const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed)
      , max(max_arg) {}

   UniformlyRandomRanged(long max_arg, const SharedRandomState& s)
      : SharedRandomState(s)
      , max(max_arg) {}

   long get()
   {
      return gmp_urandomm_ui(state(), max);
   }

   long upper_limit() const { return max; }
   long& upper_limit() { return max; }
protected:
   long max;
};

template <>
class UniformlyRandom<Integer>
   : public SharedRandomState
   , public GenericRandomGenerator<UniformlyRandom<Integer>, Integer> {
protected:
   static const unsigned long default_bits=48;
public:
   explicit UniformlyRandom(unsigned long bitlength_arg=default_bits, const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed)
      , bitlength(bitlength_arg) {}

   explicit UniformlyRandom(const RandomSeed& seed)
      : SharedRandomState(seed)
      , bitlength(default_bits) {}

   UniformlyRandom(unsigned long bitlength_arg, const SharedRandomState& s)
      : SharedRandomState(s)
      , bitlength(bitlength_arg) {}

   explicit UniformlyRandom(const SharedRandomState& s)
      : SharedRandomState(s)
      , bitlength(default_bits) {}

   Integer get() { return Integer(state(), bitlength); }

protected:
   const unsigned long bitlength;
};

template <>
class UniformlyRandomRanged<Integer>
   : public SharedRandomState
   , public GenericRandomGenerator<UniformlyRandomRanged<Integer>, Integer> {
public:
   explicit UniformlyRandomRanged(const Integer& max_arg, const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed)
      , max(max_arg) {}

   UniformlyRandomRanged(const Integer& max_arg, const SharedRandomState& s)
      : SharedRandomState(s)
      , max(max_arg) {}

   Integer get()
   {
      return Integer(state(), max);
   }

   const Integer& upper_limit() const { return max; }
   Integer& upper_limit() { return max; }
protected:
   Integer max;
};

/** Generator of random rational numbers uniformly distributed in [0, 1).

    Only the numerators of the numbers are really random.
    The denominators are always some powers of 2.
*/
template <>
class UniformlyRandom<Rational>
   : public SharedRandomState
   , public GenericRandomGenerator<UniformlyRandom<Rational>, Rational> {
protected:
   static const unsigned long default_bits=48;
public:
   explicit UniformlyRandom(unsigned long bitlength_arg=default_bits, const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed)
      , bitlength(bitlength_arg) {}

   explicit UniformlyRandom(const RandomSeed& seed)
      : SharedRandomState(seed)
      , bitlength(default_bits) {}

   UniformlyRandom(unsigned long bitlength_arg, const SharedRandomState& s)
      : SharedRandomState(s)
      , bitlength(bitlength_arg) {}

   explicit UniformlyRandom(const SharedRandomState& s)
      : SharedRandomState(s)
      , bitlength(default_bits) {}

   Rational get()
   {
      return Rational(state(), bitlength);
   }
protected:
   unsigned long bitlength;
};

/// Generator of random Bitset of a given maximal cardinality
template <>
class UniformlyRandom<Bitset>
   : public SharedRandomState
   , public GenericRandomGenerator<UniformlyRandom<Bitset>, Bitset> {
public:
   explicit UniformlyRandom(int max_arg, const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed), max_elem(max_arg) {}

   UniformlyRandom(int max_arg, const SharedRandomState& s)
      : SharedRandomState(s), max_elem(max_arg) {}

   Bitset get()
   {
      return Bitset(state(), max_elem);
   }
protected:
   int max_elem;
};

/// Generator of random AccurateFloat numbers from [0, 1)
template <>
class UniformlyRandom<AccurateFloat>
   : public SharedRandomState
   , public GenericRandomGenerator<UniformlyRandom<AccurateFloat>, AccurateFloat> {
public:
   explicit UniformlyRandom(const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed)
   {
      fix_for_mpfr();
   }

   explicit UniformlyRandom(const SharedRandomState& s)
      : SharedRandomState(s)
   {
      fix_for_mpfr();
   }

   AccurateFloat get()
   {
      return AccurateFloat(state());
   }
};

template <>
class UniformlyRandom<double>
   : public SharedRandomState
   , public GenericRandomGenerator<UniformlyRandom<double>, double> {
public:
   explicit UniformlyRandom(const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed)
   {
      fix_for_mpfr();
   }

   explicit UniformlyRandom(const SharedRandomState& s)
      : SharedRandomState(s)
   {
      fix_for_mpfr();
   }

   double get()
   {
      x.set_random(state());
      return mpfr_get_d(x.get_rep(), MPFR_RNDZ);
   }
protected:
   AccurateFloat x;
};

template <>
class UniformlyRandomRanged<double>
   : public SharedRandomState
   , public GenericRandomGenerator<UniformlyRandomRanged<double>, double> {
public:
   explicit UniformlyRandomRanged(double max_arg, const RandomSeed& seed=RandomSeed())
      : SharedRandomState(seed)
      , max(max_arg)
   {
      fix_for_mpfr();
   }

   UniformlyRandomRanged(double max_arg, const SharedRandomState& s)
      : SharedRandomState(s)
      , max(max_arg)
   {
      fix_for_mpfr();
   }

   double get()
   {
      x.set_random(state());
      return max * mpfr_get_d(x.get_rep(), MPFR_RNDZ);
   }

   double upper_limit() const { return max; }
   double& upper_limit() { return max; }
protected:
   AccurateFloat x;
   double max;
};

class DiscreteRandom
   : public GenericRandomGenerator<DiscreteRandom, int> {
public:
   template <typename Container, typename=std::enable_if_t<isomorphic_to_container_of<Container,double>::value>>
   DiscreteRandom(const Container& distrib_src, const RandomSeed& seed=RandomSeed())
      : rg(seed), distribution(distrib_src)
   {
      normalize();
   }

   template <typename Container, typename=std::enable_if_t<isomorphic_to_container_of<Container,double>::value>>
   DiscreteRandom(const Container& distrib_src, const SharedRandomState& s)
      : rg(s), distribution(distrib_src)
   {
      normalize();
   }

   int get();
protected:
   void normalize();

   mutable UniformlyRandom<double> rg;
   Vector<double> distribution;
};


template <typename Generator, typename E>
struct check_iterator_feature<random_get_iterator<Generator, E>, unlimited> : std::true_type {};

template <typename Top, typename Eref>
struct spec_object_traits< GenericRandomGenerator<Top, Eref> > :
      spec_or_model_traits<Top,is_container> {};

} // end namespace pm

namespace polymake {

using pm::UniformlyRandom;
using pm::UniformlyRandomRanged;
using pm::DiscreteRandom;
using pm::RandomSeed;

}

#endif // POLYMAKE_RANDOM_GENERATORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
