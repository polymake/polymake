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

#ifndef POLYMAKE_RANDOM_SPHERE_POINTS_H
#define POLYMAKE_RANDOM_SPHERE_POINTS_H

#include "polymake/RandomGenerators.h"
#include <cmath>

namespace pm {

using std::log;

/** Generator of floating-point numbers normally distributed in (-1,1).
 *  The algorithm is taken from Donald E. Knuth, The Art of Computer Programming, vol. II, section 3.4.1.E.6
 */
template <typename Num=AccurateFloat>
class NormalRandom
   : public GenericRandomGenerator<NormalRandom<Num>, const Num&> {
public:
   static_assert(is_among<Num, AccurateFloat, double>::value, "wrong number type");

   explicit NormalRandom(const RandomSeed& seed=RandomSeed())
      : uni_src(seed)
   {
      fill();
   }

   explicit NormalRandom(const SharedRandomState& s)
      : uni_src(s)
   {
      fill();
   }

   const Num& get()
   {
      if (++index==2) fill();
      return x[index];
   }

protected:
   Num x[2];
   UniformlyRandom<Num> uni_src;
   int index;

   void fill()
   {
      Num v, u, s;
      do {
         v = 2*uni_src.get()-1;
         u = 2*uni_src.get()-1;
         s = u*u + v*v;
      } while (s >= 1);

      const Num scale = sqrt( (-2*log(s)) / s );
      x[0]=v * scale;
      x[1]=u * scale;
      index=0;
   }
};


/// Generator of uniformly distributed random points on the unit sphere in R^d
template <typename Num=AccurateFloat>
class RandomSpherePoints
   : public GenericRandomGenerator<RandomSpherePoints<Num>, const Vector<Num>&> {
public:
   explicit RandomSpherePoints(int dim, const RandomSeed& seed=RandomSeed())
      : point(dim), norm_src(seed) {}

   RandomSpherePoints(int dim, const SharedRandomState& s)
      : point(dim), norm_src(s) {}

   const Vector<Num>& get()
   {
      fill_point();
      return point;
   }

   void set_precision(int precision)
   {
      static_assert(std::is_same<Num,AccurateFloat>::value, "RandomSpherePoints.set_precision is defined only for AccurateFloat");
      for(auto&& x: point)
         x.set_precision(precision);
   }

protected:
   Vector<Num> point;
   NormalRandom<Num> norm_src;

   void fill_point()
   {
      Num norm;
      do {
         copy_range(norm_src.begin(), entire(point));
         norm = sqr(point);
      } while (norm == 0);        // this occurs with very low probability
      point /= sqrt(norm);
   }
};

template <>
class RandomSpherePoints<pm::Rational>
   : public GenericRandomGenerator<RandomSpherePoints<Rational>, const Vector<Rational> &> {
public:
   explicit RandomSpherePoints(int dim, const RandomSeed& seed = RandomSeed())
       : point(dim), rand_sphere_float(dim, seed) {}

   RandomSpherePoints(int dim, const SharedRandomState& s)
       : point(dim), rand_sphere_float(dim, s) {}

   const Vector<Rational>& get()
   {
      fill_point();
      return point;
   }

   void set_precision(int precision)
   {
      rand_sphere_float.set_precision(precision);
   }

 protected:
   Vector<Rational> point;
   RandomSpherePoints<AccurateFloat> rand_sphere_float;

   void fill_point()
   {
      Vector<AccurateFloat> pt_float = rand_sphere_float.get();

      // pick the coordinate with maximal abs value:
      AccurateFloat max_val {abs(pt_float[0])};
      int max_idx {0};
      for (int i = 1; i != pt_float.size(); ++i)
      {
         if (abs(pt_float[i]) > max_val)
         {
            max_idx = i;
            max_val = pt_float[i];
         };
      };
      std::swap(pt_float[0], pt_float[max_idx]);
      pt_float[0] *= -1;
      // the distinguished point for the projection is oposite to the argmax;
      // the angle of projecion will not be greater than +-pi/4
      stereographic_projection<AccurateFloat>(pt_float);
      // the first coordinate of pt_float is 0.0 now;

      // approximate rationally;
      // TODO #1138: bound the height of point based on precision
      for (int i=0; i!= pt_float.size(); ++i){
         point[i] = Rational(pt_float[i]);
      };

      inv_stereographic_projection(point);
      point[0] *= -1; // not really necessary
      std::swap(point[0], point[max_idx]);
   }

   template <typename Num>
   void stereographic_projection(Vector<Num> &pt)
   {
      for (int i = 1; i != pt.size(); ++i)
      {
         pt[i] = pt[i] / (1 - pt[0]);
      };
      pt[0] = 0;
   }

   template <typename Num>
   void inv_stereographic_projection(Vector<Num> &pt)
   {
      // we assume that the first coordinate is 0;
      Num norm2 {sqr(pt)};

      for (int i = 1; i != pt.size(); ++i)
      {
         pt[i] = 2 * pt[i] / (norm2 + 1);
      }
      pt[0] = (norm2 - 1) / (norm2 + 1);
   }
};

} // end namespace pm

namespace polymake {

using pm::NormalRandom;
using pm::RandomSpherePoints;

}

#endif // POLYMAKE_RANDOM_SPHERE_POINTS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
