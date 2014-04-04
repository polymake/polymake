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

#ifndef POLYMAKE_COMMON_SIMPLE_GEOMETRY_PARSER_H
#define POLYMAKE_COMMON_SIMPLE_GEOMETRY_PARSER_H

#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/socketstream.h"
#include <cctype>

namespace polymake { namespace common {

// A simple line parser
class SimpleGeometryParser {
public:
   typedef Map<std::string, double> param_map;
   typedef Map<std::string, bool> iparam_map;

protected:
   void _print_name(std::ostream& os, const std::string& this_geom_name) const
   {
      if (!os) throw std::runtime_error("communication error");
      os << "n " << this_geom_name << '\n';
   }

   template <typename Window>
   void _print_points(std::ostream& os, const Window& W, pm::False) const
   {
      const Matrix<double>& P=W.get_points();
      os << "# " << P.rows() << '\n';

      for (Entire< Rows< Matrix<double> > >::const_iterator p_i=entire(rows(P)); !p_i.at_end(); ++p_i)
         wrap(os) << "p " << *p_i << '\n';
   }

   template <typename Window>
   void _print_points(std::ostream& os, const Window& W, pm::True) const
   {
      os << "P " << W.get_shared_matrix_id() << '\n';
   }

   template <typename Window>
   void _print_points(std::ostream& os, const Window& W) const
   {
      _print_points(os, W, bool2type<Window::has_shared_matrix>());
   }

   template <typename Window>
   void _print_params(std::ostream& os, const Window& W) const
   {
      const param_map& params = W.get_params();
      const iparam_map& iparams = W.get_iparams();
      for (Entire<param_map>::const_iterator pi = entire(params); !pi.at_end(); ++pi) {
         os << "s " << pi->first << " " << pi->second << '\n';
         iparam_map::const_iterator ipi=iparams.find(pi->first);
         if (!ipi.at_end())
            os << "i " << pi->first << " " << ipi->second << '\n';
      }
   }

   template <typename Params> static
   typename pm::object_traits<Params>::model* param_model(const Params&) { return 0; }

   static pm::is_scalar* param_model(const std::string&) { return 0; }

   template <typename Window, typename Params>
   void _print_params(std::ostream& os, const Window& W, const Params& params) const
   {
      _print_params(os, W, params, param_model(params));
   }

   template <typename Window, typename Params>
   void _print_params(std::ostream& os, const Window& W, const Params& param_name, pm::is_scalar*) const
   {
      const param_map& params = W.get_params();
      const iparam_map& iparams = W.get_iparams();
      os << "s " << param_name << " " << params[param_name] << '\n';
      iparam_map::const_iterator ipi=iparams.find(param_name);
      if (!ipi.at_end())
         os << "i " << param_name << " " << ipi->second << '\n';
   }

   template <typename Window, typename Params>
   void _print_params(std::ostream& os, const Window& W, const Params& param_name, pm::is_container*) const
   {
      const param_map& params = W.get_params();
      const iparam_map& iparams = W.get_iparams();
      for (typename Entire<Params>::const_iterator param_name=entire(params); !param_name.at_end(); ++param_name) {
         os << "s " << *param_name << " " << params[*param_name] << '\n';
         iparam_map::const_iterator ipi=iparams.find(param_name);
         if (!ipi.at_end())
            os << "i " << *param_name << " " << ipi->second << '\n';
      }
   }

   void _print_end(std::ostream& os) const
   {
      os << 'x' << endl;
   }

public:
   std::istringstream line;

   template <typename Window>
   void print_short(std::ostream& os, const Window& W) const;

   template <typename Window, typename Params>
   void print_short(std::ostream& os, const Window& W, const Params& params) const;

   template <typename Window>
   void print_long(std::ostream& os, const Window& W) const;

   void print_empty(std::ostream& os) { _print_end(os); }

   void print_warning(std::ostream& os, const std::string& this_geom_name, const std::string& message) const
   {
      _print_name(os, this_geom_name);
      os << "w " << message << '\n';
      _print_end(os);
   }

   template <typename Window, typename Params>
   void print_error(std::ostream& os, const Window& W, const Params& params, const std::string& message) const;

   template <typename Window>
   void loop(pm::socketstream& js, Window& W);

   std::pair<int, Vector<double> > get_point();
};

template <typename Window>
void SimpleGeometryParser::print_short(std::ostream& os, const Window& W) const
{
   _print_name(os, W.get_name());
   _print_points(os, W);
   _print_end(os);
}

template <typename Window, typename Params>
void SimpleGeometryParser::print_short(std::ostream& os, const Window& W, const Params& params) const
{
   _print_name(os, W.get_name());
   _print_points(os, W);
   _print_params(os, W, params);
   _print_end(os);
}

template <typename Window>
void SimpleGeometryParser::print_long(std::ostream& os, const Window& W) const
{
   _print_name(os, W.get_name());
   _print_points(os, W);
   _print_params(os, W);
   _print_end(os);
}

template <typename Window, typename Params>
void SimpleGeometryParser::print_error(std::ostream& os, const Window& W, const Params& params, const std::string& message) const
{
   _print_name(os, W.get_name());
   _print_points(os, W);
   _print_params(os, W, params);
   os << "e " << message << '\n';
   _print_end(os);
}


template <typename Window>
void SimpleGeometryParser::loop(pm::socketstream& js, Window& W)
{
   char command=0, c;
   std::string param;
   double value;

   while (js >> command) {
      switch (command) {
      case 'n':
         js.skip('\n');
         break;
      case 'p': {
         int p_index;
         if (!(js >> p_index)) return;
         W.set_point(p_index);
         break;
      }
      case 'P': {
         W.set_points(); js.skip('\n');
         break;
      }
      case 's':
         if (!(js >> param)) return;
         while (isspace(c=js.peek())) js.get();
         if (c=='n') {
            js >> param;
            if (param=="null") break;
            throw std::runtime_error("invalid option value: "+param);
         } else if (c!='-' && !isdigit(c)) {
            js >> param;
            throw std::runtime_error("invalid option value: "+param);
         }
         if (!(js >> value)) return;
         W.set_param(param,value);
         break;
      case 'f': {
         pm::PlainParserListCursor<int, pm::TrustedValue<pm::False> > reader(js);
         Set<int> f;
         int i;
         while (js >> i) {
            f+=i;
         }
         js.clear();
         W.set_facet(f);
         break;
      }
      case 'x':
         js.skip('\n');
         W.restart(*this);
         break;
      default: 
         throw std::runtime_error(std::string("unknown command: ")+command);
      }
   }
}

} }

#endif // POLYMAKE_COMMON_SIMPLE_GEOMETRY_PARSER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
