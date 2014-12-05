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

#ifndef POLYMAKE_INTERNAL_PLAIN_PARSER_H
#define POLYMAKE_INTERNAL_PLAIN_PARSER_H

#include "polymake/internal/comparators_basic_defs.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <stdexcept>

namespace pm {

class Rational;

template <typename> class OpeningBracket;
template <typename> class SeparatorChar;
template <typename> class ClosingBracket;
template <typename Options, typename Traits> class PlainPrinterCompositeCursor;
template <typename Options, typename Traits> class PlainPrinterSparseCursor;

template <typename Options=void, typename Traits=std::ostream::traits_type>
class PlainPrinter
   : public GenericOutputImpl< PlainPrinter<Options,Traits> >,
     public GenericIOoptions< PlainPrinter<Options,Traits>, Options> {
public:
   typedef std::basic_ostream<char,Traits> ostream;
   typedef GenericOutputImpl<PlainPrinter> generic_impl;
protected:
   ostream* os;
public:
   PlainPrinter(ostream& os_arg) : os(&os_arg) { }

   template <typename Data>
   void fallback(const Data& x) { *os << x; }

   void fallback(const char *s, size_t n) { os->write(s,n); }

   void finish() const { }

   template <typename ObjectRef>
   struct list_cursor {
      typedef typename deref<ObjectRef>::type Object;
      static const bool top_level=!extract_type_param<Options, SeparatorChar>::specified;
      static const bool in_composite=extract_int_param<Options, OpeningBracket>::value == '(' ||
                                     extract_int_param<Options, SeparatorChar>::value == ' ' ||
                                     extract_bool_param<Options, SparseRepresentation>::value;
      static const int
         opening= object_traits<Object>::IO_separator==IO_sep_inherit && !object_traits<Object>::IO_ends_with_eol
                  ? '{'
                  : !top_level && (object_traits<Object>::IO_ends_with_eol || in_composite)
                  ? '<' : 0,
         closing= object_traits<Object>::IO_separator==IO_sep_inherit && !object_traits<Object>::IO_ends_with_eol
                  ? '}'
                  : !top_level && (object_traits<Object>::IO_ends_with_eol || in_composite)
                  ? '>' : 0,
         sep= object_traits<Object>::IO_separate_elements_with_eol ||
              object_traits<typename Object::value_type>::IO_ends_with_eol
              ? '\n' : ' ';

      typedef typename replace_params<
              typename remove_bool_param<Options, SparseRepresentation>::type,
                 cons< OpeningBracket< int2type<opening> >,
                 cons< ClosingBracket< int2type<closing> >,
                       SeparatorChar< int2type<sep> > > > >::type
         cursor_options;
      typedef PlainPrinterCompositeCursor<cursor_options, Traits> type;

#if POLYMAKE_DEBUG
      static void dump()
      {
         std::cout << "Object: " << typeid(Object).name()
                   << "\nOptions: " << typeid(Options).name()
                   << "\n  top_level=" << top_level
                   << "\n  in_composite=" << in_composite
                   << "\n  separator=" << int(object_traits<Object>::IO_separator)
                   << "\n  eol=" << object_traits<Object>::IO_ends_with_eol
                   << "\n-> opening=" << char(opening)
                   << "\n   closing=" << char(closing)
                   << "\n   sep=" << char(sep)
                   << std::endl;
      }
#endif
   };

   template <typename ObjectRef>
   struct sparse_cursor {
      typedef typename deref<ObjectRef>::type Object;
      typedef PlainPrinterSparseCursor<typename list_cursor<Object>::cursor_options, Traits> type;
   };

   template <typename ObjectRef>
   struct composite_cursor {
      typedef typename deref<ObjectRef>::type Object;
      static const int
         total= list_length<typename object_traits<Object>::elements>::value,
         ignored= list_accumulate_unary<list_count, ignore_in_composite, typename object_traits<Object>::elements>::value;
      static const bool
         top_level=!extract_type_param<Options, SeparatorChar>::specified,
         compress= ignored>0 && total-ignored<=1;
      static const int
         opening = top_level || compress ? 0 : '(',
         closing = top_level || compress ? 0 : ')',
         sep= compress ? 0 : object_traits<Object>::IO_ends_with_eol ? '\n' : ' ';

      typedef typename replace_params<
              typename remove_bool_param<Options, SparseRepresentation>::type,
                 cons< OpeningBracket< int2type<opening> >,
                 cons< ClosingBracket< int2type<closing> >,
                       SeparatorChar< int2type<sep> > > > >::type
         cursor_options;
      typedef PlainPrinterCompositeCursor<cursor_options, Traits> type;
   };

   template <typename Object>
   typename list_cursor<Object>::type begin_list(Object*) const
   {
      return typename list_cursor<Object>::type(*os);
   }

   template <typename Object>
   typename sparse_cursor<Object>::type begin_sparse(Object* x) const
   {
      return typename sparse_cursor<Object>::type(*os, x->dim());
   }

   template <typename Object>
   typename composite_cursor<Object>::type begin_composite(Object*) const
   {
      return typename composite_cursor<Object>::type(*os);
   }

   template <typename Object>
   bool prefer_sparse_representation(const Object& x) const
   {
      return os->width()>0 || generic_impl::prefer_sparse_representation(x);
   }
};

template <typename Traits> inline
PlainPrinter<void,Traits> wrap(std::basic_ostream<char,Traits>& os)
{
   return os;
}

extern PlainPrinter<> cout;
extern PlainPrinter<> cerr;

#if defined(__INTEL_COMPILER)

// these compiler can't resolve the expression PlainPrinter << std::endl
enum std_manip { endl, ends };

template <typename Traits> inline
std::basic_ostream<char,Traits>& operator<< (std::basic_ostream<char,Traits>& os, std_manip manip)
{
   switch (manip) {
   case endl:
      os << std::endl; break;
   case ends:
      os << std::ends; break;
   }
   return os;
}

#else

template <typename Options, typename Traits> inline
PlainPrinter<Options,Traits>&
operator<< (PlainPrinter<Options,Traits>& os, std::basic_ostream<char,Traits>& (*manip)(std::basic_ostream<char,Traits>&))
{
   os.fallback(manip);
   return os;
}

using std::endl;
using std::ends;
#endif

template <typename Options, typename Traits>
class PlainPrinterCompositeCursor
   : public PlainPrinter<Options, Traits> {
   typedef PlainPrinter<Options, Traits> super;
protected:
   char pending_sep;
   int width;
   static const char
      sep=extract_int_param<Options, SeparatorChar>::value,
      opening=extract_int_param<Options, OpeningBracket>::value,
      closing=extract_int_param<Options, ClosingBracket>::value;

public:
   PlainPrinterCompositeCursor(typename super::ostream& os_arg, bool no_opening_by_width=false)
      : super(os_arg), pending_sep(0), width(os_arg.width())
   {
      if (opening) {
         if (width) {
            if (!no_opening_by_width)
               *this->os << std::setw(0) << opening;
         } else {
            *this->os << opening;
         }
      }
   }

   template <typename T>
   PlainPrinterCompositeCursor& operator<< (const T& x)
   {
      if (pending_sep) *this->os << pending_sep;
      if (width) *this->os << std::setw(width);
      static_cast<super&>(*this) << x;
      if (sep=='\n') {
         if (! object_traits<T>::IO_ends_with_eol) *this->os << sep;
      } else {
         if (!width) pending_sep=sep;
      }
      return *this;
   }

   PlainPrinterCompositeCursor& operator<< (const nothing&) { return *this; }

   void finish()
   {
      if (closing) {
         *this->os << closing;
         if (sep=='\n') *this->os << sep;
      }
      pending_sep=0;
   }
};

template <typename Options, typename Traits>
class PlainPrinterSparseCursor
   : public PlainPrinterCompositeCursor<Options, Traits> {
   typedef PlainPrinterCompositeCursor<Options, Traits> super;
protected:
   int next_index, dim;

public:
   PlainPrinterSparseCursor(typename super::ostream& os_arg, int dim_arg)
      : super(os_arg,true), next_index(0), dim(dim_arg)
   {
      if (!this->width)  // print the dimension
         static_cast<super&>(*this) << item2composite(dim);
   }

   template <typename Iterator>
   PlainPrinterSparseCursor& operator<< (const Iterator& x)
   {
      if (this->width) {
         int i=x.index();
         while (next_index < i) {
            *this->os << std::setw(this->width) << '.';
            ++next_index;
         }
         *this->os << std::setw(this->width);
         static_cast<super&>(*this) << *x;
         ++next_index;
      } else {
         static_cast<super&>(*this) << reinterpret_cast<const indexed_pair<Iterator>&>(x);
      }
      return *this;
   }

   void finish()
   {
      if (this->width) {
         while (next_index < dim) {
            *this->os << std::setw(this->width) << '.';
            ++next_index;
         }
      } else {
         super::finish();
      }
   }
};

template <typename Value, typename Options> class PlainParserListCursor;
template <typename Options> class PlainParserCompositeCursor;

class PlainParserCommon {
protected:
   mutable std::istream* is;
   char *saved_egptr;

   PlainParserCommon(const PlainParserCommon& other)
      : is(other.is), saved_egptr(other.saved_egptr)
   {
      other.is=NULL;
   }

   explicit PlainParserCommon(std::istream& is_arg)
      : is(&is_arg), saved_egptr(0) { }
public:
   ~PlainParserCommon()
   {
      if (is && saved_egptr) restore_input_range(saved_egptr);
   }

   template <typename Data>
   void fallback(Data& x) { *is >> x; }

   void finish() const { }
   bool at_end();

   void get_scalar(double&);
   void get_scalar(Rational&);
   void get_string(std::string&, char delim);

   int count_lines();
   int count_all_lines();
protected:
   char* set_temp_range(char opening, char closing);
   void set_range(char opening, char closing)
   {
      saved_egptr=set_temp_range(opening,closing);
   }

   void discard_range(char closing);
   void discard_temp_range(char closing, char *egptr)
   {
      discard_range(closing);
      restore_input_range(egptr);
   }

   int count_words();
   int count_braced(char opening, char closing);
   int count_leading(char);

   char* set_input_range(int offset);
   void restore_input_range(char *egptr);
   void skip_temp_range(char *egptr);

   char* save_read_pos();
   void restore_read_pos(char *pos);
public:
   void skip_item();
   void skip_rest();
};

template <typename Object, typename Model=typename object_traits<Object>::model>
struct composite_depth {
   static const int value=0;
};

template <typename Object>
struct composite_depth<Object, is_composite> {
   static const int value= composite_depth<typename n_th<typename object_traits<Object>::elements, 0>::type>::value + 1;
};

template <typename Options=void>
class PlainParser
   : public PlainParserCommon,
     public GenericInputImpl< PlainParser<Options> >,
     public GenericIOoptions< PlainParser<Options>, Options > {
public:
   PlainParser(std::istream& is_arg) : PlainParserCommon(is_arg) { }

   template <typename ObjectRef>
   struct list_cursor {
      typedef PlainParserListCursor<typename deref<ObjectRef>::type::value_type,
                                    typename PlainPrinter<Options>::template list_cursor<ObjectRef>::cursor_options>
         type;
   };

   template <typename ObjectRef>
   struct composite_cursor {
      typedef PlainParserCompositeCursor<typename PlainPrinter<Options>::template composite_cursor<ObjectRef>::cursor_options>
         type;
   };

   template <typename Object>
   typename list_cursor<Object>::type begin_list(Object*) const
   {
      return typename list_cursor<Object>::type(*this->is);
   }

   template <typename Object>
   typename composite_cursor<Object>::type begin_composite(Object*) const
   {
      return typename composite_cursor<Object>::type(*this->is);
   }

   operator void* () const { return this->is->good() ? (void*)this : 0; }
   bool operator! () const { return !this->is->good(); }
};

template <typename Options> inline
PlainParser<Options>&
operator>> (GenericInput< PlainParser<Options> >& is, double& x)
{
   is.top().get_scalar(x);
   return is.top();
}

template <typename Options> inline
PlainParser<Options>&
operator>> (GenericInput< PlainParser<Options> >& is, Rational& x)
{
   is.top().get_scalar(x);
   return is.top();
}

template <typename Options> inline
PlainParser<Options>&
getline (PlainParser<Options>& is, std::string& s, char delim='\n')
{
   is.get_string(s,delim);
   return is;
}

template <typename Options> inline
PlainParser<Options>&
operator>> (GenericInput< PlainParser<Options> >& is, std::string& s)
{
   is.top().get_string(s,0);
   return is.top();
}

template <typename Options>
class PlainParserCursor : public PlainParserCommon {
protected:
   char *start_pos;

   static const char
      opening=extract_int_param<Options, OpeningBracket>::value,
      closing=extract_int_param<Options, ClosingBracket>::value,
      separator=extract_int_param<Options, SeparatorChar>::value;
   static const bool
      is_temp=extract_bool_param<Options, LookForward>::value;

   PlainParser<Options>& sub_parser()
   {
      return static_cast<PlainParser<Options>&>(static_cast<PlainParserCommon&>(*this));
   }

public:
   PlainParserCursor(std::istream& is_arg)
      : PlainParserCommon(is_arg), start_pos(0)
   {
      if (is_temp)
         start_pos=save_read_pos();
      if (opening) set_range(opening, closing);
   }

   ~PlainParserCursor()
   {
      if (is_temp)
         restore_read_pos(start_pos);
   }

   void finish()
   {
      if (closing) discard_range(closing);
   }

   bool at_end()
   {
      return PlainParserCommon::at_end() && (finish(), true);
   }
};

template <typename Options>
class PlainParserCompositeCursor
   : public PlainParserCursor<Options>,
     public GenericInputImpl< PlainParserCompositeCursor<Options> >,
     public GenericIOoptions< PlainParserCompositeCursor<Options>, Options > {
   typedef PlainParserCursor<Options> super;
public:
   PlainParserCompositeCursor(std::istream& is_arg)
      : super(is_arg) { }

   template <typename Data>
   void operator>> (Data& data)
   {
      this->sub_parser() >> data;
   }
};

template <typename Value, typename Options>
class PlainParserListCursor
   : public PlainParserCursor<Options>,
     public GenericInputImpl< PlainParserListCursor<Value,Options> >,
     public GenericIOoptions< PlainParserListCursor<Value,Options>, Options, 1 > {
   typedef PlainParserCursor<Options> super;

   template <typename, typename> friend class PlainParserListCursor;

   static const bool has_sparse_representation=extract_bool_param<Options, SparseRepresentation, false>::value;
protected:
   int _size;
   char *pair_egptr;

   int size(is_scalar)
   {
      return this->count_words();
   }

   int size(is_opaque)
   {
      return this->count_words();
   }

   int size(is_container)
   {
      typedef typename PlainPrinter<Options>::template list_cursor<Value> defs;
      return defs::opening
             ? this->count_braced(defs::opening, defs::closing) :
             super::opening
             ? this->count_lines()
             : this->count_all_lines();
   }

   int size(is_composite)
   {
      typedef typename PlainPrinter<Options>::template composite_cursor<Value> defs;
      return defs::opening
             ? this->count_braced(defs::opening, defs::closing) :
             super::separator=='\n'
             ? super::opening
               ? this->count_lines()
               : this->count_all_lines()
             : this->count_words();
   }

public:
   typedef Value value_type;

   PlainParserListCursor(std::istream& is_arg)
      : super(is_arg), _size(-1), pair_egptr(0)
   {
      if (!super::opening && object_traits<Value>::total_dimension==0)
         this->set_range(0, '\n');
   }

   int size()
   {
      if (_size<0)
         _size=size(typename object_traits<Value>::model());
      return _size;
   }

protected:
   template <typename Model>
   static int missing_parens(Model) { return 0; }

   static int missing_parens(is_composite)
   {
      typedef typename PlainPrinter<Options>::template composite_cursor<Value> defs;
      return defs::opening == 0;
   }

   bool _sparse_representation()
   {
      const int own_missing=missing_parens(typename object_traits<Value>::model());
      return this->count_leading('(') == composite_depth<Value>::value - own_missing + 1;
   }
public:
   int index()
   {
      if (!ignore_in_composite<Value>::value) {
         if (has_sparse_representation)
            pair_egptr=this->set_temp_range('(', ')');
         else
            this->is->setstate(std::ios::failbit);
      }
      int i=-1;
      *this->is >> i;
      return i;
   }

   bool sparse_representation()
   {
      return extract_type_param<Options,SparseRepresentation>::specified
             ? has_sparse_representation
             : _sparse_representation();
   }

   template <typename ElementType>
   int lookup_lower_dim(bool tell_size_if_dense)
   {
      return this->sub_parser().set_option(LookForward<True>()).begin_list((ElementType*)0).lookup_dim(tell_size_if_dense);
   }

   int lookup_dim(bool tell_size_if_dense)
   {
      return (!ignore_in_composite<Value>::value && sparse_representation())
         ? this->set_option(SparseRepresentation<True>()).get_dim() :
             tell_size_if_dense
             ? size() : -1;
   }

private:
   int get_dim()
   {
      int d=index();
      if (PlainParserCommon::at_end()) {
         this->discard_temp_range(')', pair_egptr);
      } else {
         d=-1;  // it's not the dimension but the index of the first entry
         this->skip_temp_range(pair_egptr);
      }
      pair_egptr=NULL;
      return d;
   }

   template <typename Anything>
   void cleanup(type2type<Anything>)
   {
      if (has_sparse_representation) {
         this->discard_temp_range(')', pair_egptr);
         pair_egptr=NULL;
      }
   }

   void cleanup(type2type<nothing>) { }

public:
   template <typename Data>
   void operator>> (Data& x)
   {
      this->sub_parser() >> x;
      cleanup(type2type<Value>());
   }
      
   void skip_item()
   {
      if (has_sparse_representation && pair_egptr) {
         this->skip_temp_range(pair_egptr);
         pair_egptr=NULL;
      } else {
         super::skip_item();
      }
   }
};

template <typename T>
class convToString {
public:
   typedef T argument_type;
   typedef const std::string result_type;

   result_type operator() (typename function_argument<T>::type x) const
   {
      std::ostringstream out;
      wrap(out) << x;
      return out.str();
   }
};

template <> class conv<int, std::string> : public convToString<int> {};
template <> class conv<long, std::string> : public convToString<long> {};
template <> class conv<float, std::string> : public convToString<float> {};
template <> class conv<double, std::string> : public convToString<double> {};

} // end namespace pm

namespace polymake {

using pm::cout;
using pm::cerr;
using pm::endl;
using pm::ends;
using std::ios;
using pm::wrap;

}

#endif // POLYMAKE_INTERNAL_PLAIN_PARSER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
