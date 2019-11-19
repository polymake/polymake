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

#ifndef POLYMAKE_INTERNAL_PLAIN_PARSER_H
#define POLYMAKE_INTERNAL_PLAIN_PARSER_H

#include "polymake/internal/comparators_basic_defs.h"
#include "polymake/meta_list.h"

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

template <typename Options=mlist<>, typename Traits=std::ostream::traits_type>
class PlainPrinter
   : public GenericOutputImpl< PlainPrinter<Options, Traits> >
   , public GenericIOoptions< PlainPrinter<Options, Traits>, Options> {
public:
   typedef std::basic_ostream<char, Traits> ostream;
   typedef GenericOutputImpl<PlainPrinter> generic_impl;
protected:
   ostream* os;
public:
   explicit PlainPrinter(ostream& os_arg)
      : os(&os_arg) { }

   template <typename Data>
   void fallback(const Data& x) { *os << x; }

   void fallback(const char *s, size_t n) { os->write(s,n); }

   void finish() const { }

   template <typename ObjectRef>
   struct list_cursor {
      typedef typename deref<ObjectRef>::type Object;
      static const char null_char='\0';
      static const bool top_level=!mtagged_list_extract<Options, SeparatorChar>::is_specified;
      static const bool in_composite=tagged_list_extract_integral<Options, OpeningBracket>(null_char) == '(' ||
                                     tagged_list_extract_integral<Options, SeparatorChar>(null_char) == ' ' ||
                                     tagged_list_extract_integral<Options, SparseRepresentation>(false);
      static const char
         opening= object_traits<Object>::IO_separator==IO_sep_inherit && !object_traits<Object>::IO_ends_with_eol
                  ? '{'
                  : !top_level && (object_traits<Object>::IO_ends_with_eol || in_composite)
                  ? '<' : null_char,
         closing= object_traits<Object>::IO_separator==IO_sep_inherit && !object_traits<Object>::IO_ends_with_eol
                  ? '}'
                  : !top_level && (object_traits<Object>::IO_ends_with_eol || in_composite)
                  ? '>' : null_char,
         sep= object_traits<Object>::IO_separate_elements_with_eol ||
              object_traits<typename Object::value_type>::IO_ends_with_eol
              ? '\n' : ' ';

      typedef typename mtagged_list_replace<
                 typename mtagged_list_remove<Options, SparseRepresentation>::type,
                 OpeningBracket<char_constant<opening>>,
                 ClosingBracket<char_constant<closing>>,
                 SeparatorChar<char_constant<sep>> >::type
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
      static constexpr int
         total = list_length<typename object_traits<Object>::elements>::value,
         ignored = list_accumulate_unary<list_count, ignore_in_composite, typename object_traits<Object>::elements>::value;
      static constexpr bool
         top_level = !mtagged_list_extract<Options, SeparatorChar>::is_specified,
         compress = ignored > 0 && total-ignored <= 1;
      static constexpr char
         null_char = '\0',
         opening = top_level || compress ? null_char : '(',
         closing = top_level || compress ? null_char : ')',
         sep = compress ? null_char : object_traits<Object>::IO_ends_with_eol ? '\n' : ' ';

      using cursor_options = typename mtagged_list_replace<
                 typename mtagged_list_remove<Options, SparseRepresentation>::type,
                 OpeningBracket<char_constant<opening>>,
                 ClosingBracket<char_constant<closing>>,
                 SeparatorChar<char_constant<sep>> >::type
         ;
      using type = PlainPrinterCompositeCursor<cursor_options, Traits>;
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

   int choose_sparse_representation() const
   {
      return -os->width();  // fixed width per element -> show implicit 0's
   }
};

template <typename Traits> inline
PlainPrinter<mlist<>, Traits> wrap(std::basic_ostream<char, Traits>& os)
{
   return PlainPrinter<mlist<>, Traits>(os);
}

extern PlainPrinter<> cout;
extern PlainPrinter<> cerr;

#if defined(__INTEL_COMPILER)

// these compiler can't resolve the expression PlainPrinter << std::endl
enum std_manip { endl, ends };

template <typename Traits> inline
std::basic_ostream<char, Traits>& operator<< (std::basic_ostream<char, Traits>& os, std_manip manip)
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
PlainPrinter<Options, Traits>&
operator<< (PlainPrinter<Options, Traits>& os, std::basic_ostream<char, Traits>& (*manip)(std::basic_ostream<char, Traits>&))
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
   using base_t = PlainPrinter<Options, Traits>;
protected:
   char pending_sep;
   int width;
   static constexpr char
      null_char = '\0',
      sep = tagged_list_extract_integral<Options, SeparatorChar>(null_char),
      opening = tagged_list_extract_integral<Options, OpeningBracket>(null_char),
      closing = tagged_list_extract_integral<Options, ClosingBracket>(null_char);

public:
   PlainPrinterCompositeCursor(typename base_t::ostream& os_arg, bool no_opening_by_width=false)
      : base_t(os_arg)
      , pending_sep(null_char)
      , width(os_arg.width())
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
      if (pending_sep) {
         *this->os << pending_sep;
         pending_sep = null_char;
      }
      if (width) *this->os << std::setw(width);
      static_cast<base_t&>(*this) << x;
      if (sep=='\n') {
         if (! object_traits<T>::IO_ends_with_eol) *this->os << sep;
      } else {
         if (!width) pending_sep=sep;
      }
      return *this;
   }

   PlainPrinterCompositeCursor& non_existent()
   {
      return *this << "==UNDEF==";
   }

   PlainPrinterCompositeCursor& operator<< (const nothing&) { return *this; }

   void finish()
   {
      if (closing) {
         *this->os << closing;
         if (sep=='\n') *this->os << sep;
      }
      pending_sep=null_char;
   }
};

template <typename Options, typename Traits>
class PlainPrinterSparseCursor
   : public PlainPrinterCompositeCursor<Options, Traits> {
   using base_t = PlainPrinterCompositeCursor<Options, Traits>;
protected:
   int next_index, dim;

public:
   PlainPrinterSparseCursor(typename base_t::ostream& os_arg, int dim_arg)
      : base_t(os_arg, true)
      , next_index(0)
      , dim(dim_arg)
   {
      if (!this->width) {
         // print the dimension
         *this->os << '(' << dim << ')';
         this->pending_sep = base_t::sep;
      }
   }

   template <typename Iterator>
   PlainPrinterSparseCursor& operator<< (const Iterator& x)
   {
      if (this->width) {
         const int i = x.index();
         while (next_index < i) {
            *this->os << std::setw(this->width) << '.';
            ++next_index;
         }
         *this->os << std::setw(this->width);
         static_cast<base_t&>(*this) << *x;
         ++next_index;
      } else {
         static_cast<base_t&>(*this) << reinterpret_cast<const indexed_pair<Iterator>&>(x);
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
         base_t::finish();
      }
   }
};

template <typename Value, typename Options> class PlainParserListCursor;
template <typename Options> class PlainParserCompositeCursor;

class PlainParserCommon {
protected:
   std::istream* is;
   char *saved_egptr;

   PlainParserCommon(const PlainParserCommon& other) = delete;

   PlainParserCommon(PlainParserCommon&& other)
      : is(other.is)
      , saved_egptr(other.saved_egptr)
   {
      other.is = nullptr;
   }

   explicit PlainParserCommon(std::istream& is_arg)
      : is(&is_arg)
      , saved_egptr(nullptr) { }

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

   /// return ±1 if the input string equals "[+-]inf" (and consume that string)
   /// return 0 otherwise (and don't consume anything)
   int probe_inf();

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
   bool lone_clause_on_line(char opening, char closing);

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

template <typename Options=mlist<>>
class PlainParser
   : public PlainParserCommon
   , public GenericInputImpl< PlainParser<Options> >
   , public GenericIOoptions< PlainParser<Options>, Options > {
public:
   explicit PlainParser(std::istream& is_arg) : PlainParserCommon(is_arg) {}
   PlainParser(PlainParser&&) = default;

   template <typename ObjectRef>
   struct list_cursor {
      using type = PlainParserListCursor<typename deref<ObjectRef>::type::value_type,
                                         typename PlainPrinter<Options>::template list_cursor<ObjectRef>::cursor_options>;
   };

   template <typename ObjectRef>
   struct composite_cursor {
      using type = PlainParserCompositeCursor<typename PlainPrinter<Options>::template composite_cursor<ObjectRef>::cursor_options>;
   };

   template <typename Object>
   decltype(auto) begin_list(Object*) const
   {
      return typename list_cursor<Object>::type(*this->is);
   }

   template <typename Object>
   decltype(auto) begin_composite(Object*) const
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
   char* start_pos;

   static const char
      null_char = '\0',
      opening = tagged_list_extract_integral<Options, OpeningBracket>(null_char),
      closing = tagged_list_extract_integral<Options, ClosingBracket>(null_char),
      separator = tagged_list_extract_integral<Options, SeparatorChar>(null_char);
   static const bool
      is_temp = tagged_list_extract_integral<Options, LookForward>(false);

   PlainParser<Options>& sub_parser()
   {
      return static_cast<PlainParser<Options>&>(static_cast<PlainParserCommon&>(*this));
   }

public:
   PlainParserCursor(std::istream& is_arg)
      : PlainParserCommon(is_arg)
      , start_pos(nullptr)
   {
      if (is_temp)
         start_pos = save_read_pos();
      if (opening)
         set_range(opening, closing);
   }

   PlainParserCursor(PlainParserCursor&&) = default;

   ~PlainParserCursor()
   {
      if (is_temp)
         restore_read_pos(start_pos);
   }

   void finish()
   {
      if (closing)
         discard_range(closing);
   }

   bool at_end()
   {
      return PlainParserCommon::at_end() && (finish(), true);
   }
};

template <typename Options>
class PlainParserCompositeCursor
   : public PlainParserCursor<Options>
   , public GenericInputImpl< PlainParserCompositeCursor<Options> >
   , public GenericIOoptions< PlainParserCompositeCursor<Options>, Options > {
   using base_t = PlainParserCursor<Options>;
public:
   PlainParserCompositeCursor(std::istream& is_arg)
      : base_t(is_arg) { }

   template <typename Data>
   void operator>> (Data& data)
   {
      this->sub_parser() >> data;
   }
};

template <typename Value, typename Options>
class PlainParserListCursor
   : public PlainParserCursor<Options>
   , public GenericInputImpl< PlainParserListCursor<Value, Options> >
   , public GenericIOoptions< PlainParserListCursor<Value, Options>, Options, 1 > {
   using base_t = PlainParserCursor<Options>;

   template <typename, typename> friend class PlainParserListCursor;

   static const bool has_sparse_representation=tagged_list_extract_integral<Options, SparseRepresentation>(false);
protected:
   int size_;
   char* pair_egptr;

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
             base_t::opening
             ? this->count_lines()
             : this->count_all_lines();
   }

   int size(is_composite)
   {
      typedef typename PlainPrinter<Options>::template composite_cursor<Value> defs;
      return defs::opening
             ? this->count_braced(defs::opening, defs::closing) :
             base_t::separator=='\n'
             ? base_t::opening
               ? this->count_lines()
               : this->count_all_lines()
             : this->count_words();
   }

public:
   using value_type = Value;

   explicit PlainParserListCursor(std::istream& is_arg)
      : base_t(is_arg)
      , size_(-1)
      , pair_egptr(0)
   {
      if (!base_t::opening && object_traits<Value>::total_dimension==0)
         this->set_range(0, '\n');
   }

   PlainParserListCursor(PlainParserListCursor&&) = default;

   int size()
   {
      if (size_<0)
         size_=size(typename object_traits<Value>::model());
      return size_;
   }

protected:
   template <typename Model>
   static constexpr int missing_parens(Model) { return 0; }

   static constexpr int missing_parens(is_composite)
   {
      typedef typename PlainPrinter<Options>::template composite_cursor<Value> defs;
      return defs::opening == 0;
   }

   template <typename Model>
   static constexpr bool recognize_own_dimension(Model) { return true; }

   bool recognize_own_dimension(is_container)
   {
      // try to distinguish a sequence of sparse vectors from a sparse sequence of vectors
      // in the latter case the dimension will appear on a separate line
      if (!object_traits<Value>::allow_sparse) return true;
      typedef typename PlainPrinter<Options>::template list_cursor<Value> defs;
      return defs::sep=='\n' && this->lone_clause_on_line('(', ')');
   }

   bool detect_sparse_representation()
   {
      constexpr int own_missing = missing_parens(typename object_traits<Value>::model());
      constexpr int expect_open_parens = composite_depth<Value>::value - own_missing + 1;
      if (this->count_leading('(') == expect_open_parens)
      {
         return expect_open_parens != 1 || recognize_own_dimension(typename object_traits<Value>::model());
      }
      return false;
   }
public:
   int index(const int index_bound)
   {
      if (!ignore_in_composite<Value>::value) {
         if (has_sparse_representation)
            pair_egptr = this->set_temp_range('(', ')');
         else
            this->is->setstate(std::ios::failbit);
      }
      int i = -1;
      *this->is >> i;
      if (!this->get_option(TrustedValue<std::true_type>()) && (i < 0 || i >= index_bound))
         this->is->setstate(std::ios::failbit);
      return i;
   }

   bool sparse_representation()
   {
      return mtagged_list_extract<Options, SparseRepresentation>::is_specified
             ? has_sparse_representation
             : detect_sparse_representation();
   }

   bool is_ordered() const { return true; }

   int cols(bool tell_size_if_dense)
   {
      return this->sub_parser().set_option(LookForward<std::true_type>()).begin_list((Value*)0).get_dim(tell_size_if_dense);
   }

   int get_dim(bool tell_size_if_dense)
   {
      return (!ignore_in_composite<Value>::value && sparse_representation())
             ? this->set_option(SparseRepresentation<std::true_type>()).get_dim() :
             tell_size_if_dense
             ? size() : -1;
   }

private:
   int get_dim()
   {
      int d = index(std::numeric_limits<int>::max());
      if (PlainParserCommon::at_end()) {
         this->discard_temp_range(')', pair_egptr);
      } else {
         d = -1;  // it's not the dimension but the index of the first entry
         this->skip_temp_range(pair_egptr);
      }
      pair_egptr = nullptr;
      return d;
   }

   template <typename Anything>
   void cleanup(type2type<Anything>)
   {
      if (has_sparse_representation) {
         this->discard_temp_range(')', pair_egptr);
         pair_egptr = nullptr;
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
         pair_egptr = nullptr;
      } else {
         base_t::skip_item();
      }
   }
};

namespace perl {

// loop through perl STDOUT
extern std::ostream cout;

}

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
