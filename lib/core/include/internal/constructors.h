/* Copyright (c) 1997-2014
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

#ifndef POLYMAKE_INTERNAL_CONSTRUCTORS_H
#define POLYMAKE_INTERNAL_CONSTRUCTORS_H

namespace pm {

template <typename Target>
class constructor< Target() > {
public:
   typedef Target value_type;

   Target* operator() (void *p) const
   {
      return new(p) Target();
   }

   Target operator() () const
   {
      return Target();
   }
};

template <typename Target, typename Arg1>
class constructor< Target(Arg1) > : public constructor< Target() > {
   typedef constructor< Target() > _super;
protected:
   typedef alias<Arg1> al1;
   mutable al1 arg1;
public:
   constructor(typename al1::arg_type a1)
      : arg1(a1) { }

   Target* operator() (void *p) const
   {
      return new(p) Target(*arg1);
   }

   Target operator() () const
   {
      return Target(*arg1);
   }
};

template <typename Target, typename Arg1, typename Arg2>
class constructor< Target(Arg1,Arg2) > : public constructor< Target(Arg1) > {
   typedef constructor< Target(Arg1) > _super;
protected:
   typedef alias<Arg2> al2;
   mutable al2 arg2;
public:
   constructor(typename _super::al1::arg_type a1, typename al2::arg_type a2)
      : _super(a1), arg2(a2) { }

   Target* operator() (void *p) const
   {
      return new(p) Target(*this->arg1, *arg2);
   }

   Target operator() () const
   {
      return Target(*this->arg1, *arg2);
   }
};

template <typename Target, typename Arg1, typename Arg2, typename Arg3>
class constructor< Target(Arg1,Arg2,Arg3) > : public constructor< Target(Arg1,Arg2) > {
   typedef constructor< Target(Arg1,Arg2) > _super;
protected:
   typedef alias<Arg3> al3;
   mutable al3 arg3;
public:
   constructor(typename _super::al1::arg_type a1, typename _super::al2::arg_type a2, typename al3::arg_type a3)
      : _super(a1,a2), arg3(a3) { }

   Target* operator() (void *p) const
   {
      return new(p) Target(*this->arg1, *this->arg2, *arg3);
   }

   Target operator() () const
   {
      return Target(*this->arg1, *this->arg2, *arg3);
   }
};

template <typename Target, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class constructor< Target(Arg1,Arg2,Arg3,Arg4) > : public constructor< Target(Arg1,Arg2,Arg3) > {
   typedef constructor< Target(Arg1,Arg2,Arg3) > _super;
protected:
   typedef alias<Arg4> al4;
   mutable al4 arg4;
public:
   constructor(typename _super::al1::arg_type a1, typename _super::al2::arg_type a2, typename _super::al3::arg_type a3,
               typename al4::arg_type a4)
      : _super(a1,a2,a3), arg4(a4) { }

   Target* operator() (void *p) const
   {
      return new(p) Target(*this->arg1, *this->arg2, *this->arg3, *arg4);
   }

   Target operator() () const
   {
      return Target(*this->arg1, *this->arg2, *this->arg3, *arg4);
   }
};

template <typename Target, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
class constructor< Target(Arg1,Arg2,Arg3,Arg4,Arg5) > : public constructor< Target(Arg1,Arg2,Arg3,Arg4) > {
   typedef constructor< Target(Arg1,Arg2,Arg3,Arg4) > _super;
protected:
   typedef alias<Arg5> al5;
   mutable al5 arg5;
public:
   constructor(typename _super::al1::arg_type a1, typename _super::al2::arg_type a2, typename _super::al3::arg_type a3,
               typename _super::al4::arg_type a4, typename al5::arg_type a5)
      : _super(a1,a2,a3,a4), arg5(a5) { }

   Target* operator() (void *p) const
   {
      return new(p) Target(*this->arg1, *this->arg2, *this->arg3, *this->arg4, *arg5);
   }

   Target operator() () const
   {
      return Target(*this->arg1, *this->arg2, *this->arg3, *this->arg4, *arg5);
   }
};

template <typename Target> inline
constructor< Target() >
make_constructor(Target*)
{
   return constructor< Target() >();
}

template <typename Target, typename Arg1> inline
constructor< Target(Arg1&) >
make_constructor(Arg1& a1, Target*)
{
   return constructor< Target(Arg1&) >(a1);
}

template <typename Target, typename Arg1> inline
constructor< Target(const Arg1&) >
make_constructor(const Arg1& a1, Target*)
{
   return constructor< Target(const Arg1&) >(a1);
}

template <typename Target, typename Arg1, typename Arg2> inline
constructor< Target(Arg1&, Arg2&) >
make_constructor(Arg1& a1, Arg2& a2, Target*)
{
   return constructor< Target(Arg1&, Arg2&) >(a1,a2);
}

template <typename Target, typename Arg1, typename Arg2> inline
constructor< Target(const Arg1&, Arg2&) >
make_constructor(const Arg1& a1, Arg2& a2, Target*)
{
   return constructor< Target(const Arg1&, Arg2&) >(a1,a2);
}

template <typename Target, typename Arg1, typename Arg2> inline
constructor< Target(Arg1&, const Arg2&) >
make_constructor(Arg1& a1, const Arg2& a2, Target*)
{
   return constructor< Target(Arg1&, const Arg2&) >(a1,a2);
}

template <typename Target, typename Arg1, typename Arg2> inline
constructor< Target(const Arg1&, const Arg2&) >
make_constructor(const Arg1& a1, const Arg2& a2, Target*)
{
   return constructor< Target(const Arg1&, const Arg2&) >(a1,a2);
}

template <typename Target, typename Arg1, typename Arg2, typename Arg3> inline
constructor< Target(const Arg1&, const Arg2&, const Arg3&) >
make_constructor(const Arg1& a1, const Arg2& a2, const Arg3& a3, Target*)
{
   return constructor< Target(const Arg1&, const Arg2&, const Arg3&) >(a1,a2,a3);
}

template <typename Target, typename Arg1, typename Arg2, typename Arg3, typename Arg4> inline
constructor< Target(const Arg1&, const Arg2&, const Arg3&, const Arg4&) >
make_constructor(const Arg1& a1, const Arg2& a2, const Arg3& a3, const Arg4& a4, Target*)
{
   return constructor< Target(const Arg1&, const Arg2&, const Arg3&, const Arg4&) >(a1,a2,a3,a4);
}

template <typename Target, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> inline
constructor< Target(const Arg1&, const Arg2&, const Arg3&, const Arg4&, const Arg5&) >
make_constructor(const Arg1& a1, const Arg2& a2, const Arg3& a3, const Arg4& a4, const Arg5& a5, Target*)
{
   return constructor< Target(const Arg1&, const Arg2&, const Arg3&, const Arg4&, const Arg5&) >(a1,a2,a3,a4,a5);
}

} // end namespace pm

namespace polymake {
   using pm::make_constructor;
}

#endif // POLYMAKE_INTERNAL_CONSTRUCTORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
