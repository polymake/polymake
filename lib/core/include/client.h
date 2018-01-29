/* Copyright (c) 1997-2018
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

#ifndef POLYMAKE_CLIENT_H
#define POLYMAKE_CLIENT_H

#include "polymake/perl/constants.h"
#include "polymake/perl/Value.h"
#include "polymake/perl/types.h"
#include "polymake/perl/macros.h"
#include "polymake/perl/wrappers.h"
#include "polymake/perl/calls.h"
#include "polymake/perl/Object.h"

namespace polymake { namespace perl {

using pm::perl::Value;
using pm::perl::OptionSet;
using pm::perl::Scalar;
using pm::perl::Array;
using pm::perl::Hash;
using pm::perl::ListReturn;
using pm::perl::undefined;
using pm::perl::temporary;
using pm::perl::Canned;
using pm::perl::TryCanned;
using pm::perl::Enum;
using pm::perl::Object;
using pm::perl::ObjectType;
using pm::perl::load_data;
using pm::perl::save_data;
using pm::perl::get_custom;
using pm::perl::get_debug_level;
using pm::perl::cout;

} }

#ifdef POLYMAKE_APPNAME

namespace polymake { namespace POLYMAKE_APPNAME {

# ifdef POLYMAKE_BUNDLED_EXT
namespace bundled { namespace POLYMAKE_BUNDLED_EXT {
# endif

class GlueRegistratorTag;

# ifdef POLYMAKE_BUNDLED_EXT
} }

using PolymakeGlueRegistratorTag = bundled::POLYMAKE_BUNDLED_EXT::GlueRegistratorTag;

#  define EmbeddedItemsKey4perl(app, bundled) FirstArgAsString(app) ":" FirstArgAsString(bundled)
# else

using PolymakeGlueRegistratorTag = GlueRegistratorTag;

#  define EmbeddedItemsKey4perl(app, bundled) FirstArgAsString(app)
# endif

template <typename Tag, pm::perl::RegistratorQueue::Kind kind>
const pm::perl::RegistratorQueue& get_registrator_queue(mlist<Tag>, std::integral_constant<pm::perl::RegistratorQueue::Kind, kind>)
{
   static pm::perl::RegistratorQueue queue(EmbeddedItemsKey4perl(POLYMAKE_APPNAME, POLYMAKE_BUNDLED_EXT), kind);
   return queue;
};

namespace {

template <typename Fptr>
class IndirectFunctionWrapper
   : public pm::perl::FunctionTemplateBase {
   IndirectFunctionWrapper() = delete;
public:
   void add__me(const AnyString& file, int line) const
   {
      register_it(reinterpret_cast<pm::perl::wrapper_type>(&call), ".wrp", file, line,
                  pm::perl::TypeListUtils<Fptr>::get_type_names());
   }

   using fptr_type = Fptr*;
   static SV* call(void*, SV**);
};

template <typename What, int id>
class QueueingRegistrator4perl {
public:
   template <typename... Args>
   explicit QueueingRegistrator4perl(Args&&... args)
   {
      static_cast<const What&>(get_registrator_queue(mlist<PolymakeGlueRegistratorTag>(), std::integral_constant<pm::perl::RegistratorQueue::Kind, What::kind>()))
                              .add__me(std::forward<Args>(args)...);
   }

   static QueueingRegistrator4perl r;
};

template <typename What, int id>
class StaticRegistrator4perl {
public:
   template <typename... Args>
   explicit StaticRegistrator4perl(Args&&... args)
   {
      What::add__me(std::forward<Args>(args)...);
   }

   static StaticRegistrator4perl r;
};

} } }
#endif // POLYMAKE_APPNAME

namespace polymake { namespace perl_bindings {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T, typename T0>
   RecognizeType4perl("Polymake::common::Serialized", (T0), pm::Serialized<T0>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::Pair", (T0,T1), std::pair<T0,T1>)

///==== Automatically generated contents end here.  Please do not delete this line. ====
} }

#endif // POLYMAKE_CLIENT_POLYMAKE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
