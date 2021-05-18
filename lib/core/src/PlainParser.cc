/* Copyright (c) 1997-2021
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

#include "polymake/internal/CharBuffer.h"
#include "polymake/GenericIO.h"
#include "polymake/Rational.h"
#include "polymake/socketstream.h"
#include <cstdlib>

namespace pm {

Int PlainParserCommon::count_words()
{
   std::streambuf* mybuf = is->rdbuf();
   if (!CharBuffer::skip_ws(mybuf)) return 0;
   Int cnt = 0;
   CharBuffer::size_type offset = 0;
   do {
      offset = CharBuffer::next_ws(mybuf, offset+1);
      ++cnt;
   } while (offset > 0  &&
            CharBuffer::seek_forward(mybuf, offset) != '\n'  && 
            (offset = CharBuffer::next_non_ws(mybuf, offset+1)) > 0);
   return cnt;
}

Int PlainParserCommon::count_lines()
{
   return CharBuffer::count_lines(is->rdbuf());
}

Int PlainParserCommon::count_all_lines()
{
   return dynamic_cast<streambuf_ext&>(*is->rdbuf()).lines();
}

Int PlainParserCommon::count_braced(const char opening, const char closing)
{
   std::streambuf* mybuf = is->rdbuf();
   if (!CharBuffer::skip_ws(mybuf)) return 0;
   Int cnt = 0;
   CharBuffer::size_type offset = 0;
   do {
      if (CharBuffer::get_ptr(mybuf)[offset] != opening) {
         is->setstate(is->failbit);
         return 0;
      }
      offset = CharBuffer::matching_brace(mybuf, opening, closing, offset+1);
      if (offset < 0) {
         is->setstate(is->failbit);
         return 0;
      }
      ++cnt;
   } while ((offset = CharBuffer::next_non_ws(mybuf, offset+1)) > 0);
   return cnt;
}

bool PlainParserCommon::lone_clause_on_line(const char opening, const char closing)
{
   std::streambuf* mybuf = is->rdbuf();
   CharBuffer::size_type offset = CharBuffer::next_non_ws(mybuf, 0);
   if (offset < 0 || CharBuffer::get_ptr(mybuf)[offset] != opening) return false;
   offset = CharBuffer::matching_brace(mybuf, opening, closing, offset+1);
   if (offset < 0) {
      is->setstate(is->failbit);
      return false;
   }
   return CharBuffer::seek_forward(mybuf, offset+1) == '\n';
}

Int PlainParserCommon::count_leading(char c)
{
   std::streambuf* mybuf = is->rdbuf();
   Int cnt = 0;
   CharBuffer::size_type offset = -1;
   for (;;) {
      if ((offset = CharBuffer::next_non_ws(mybuf, offset+1)) < 0)
         return -1;
      if (CharBuffer::get_ptr(mybuf)[offset] != c) break;
      ++cnt;
   }
   return cnt;
}

bool PlainParserCommon::at_end()
{
   return !CharBuffer::skip_ws(is->rdbuf());
}

void PlainParserCommon::skip_item()
{
   std::streambuf* mybuf = is->rdbuf();
   if (!CharBuffer::skip_ws(mybuf)) return;

   CharBuffer::size_type offset = 0;
   switch (mybuf->sbumpc()) {
   case '<':
      offset = CharBuffer::matching_brace(mybuf, '<', '>');  break;
   case '(':
      offset = CharBuffer::matching_brace(mybuf, '(', ')');  break;
   case '{':
      offset = CharBuffer::matching_brace(mybuf, '{', '}');  break;
   default:
      offset = CharBuffer::next_ws(mybuf, 0, false);
   }
   if (offset < 0)
      CharBuffer::skip_all(mybuf);
   else
      CharBuffer::get_bump(mybuf, offset+1);
}

void PlainParserCommon::skip_rest()
{
   std::streambuf* mybuf = is->rdbuf();
   CharBuffer::skip_all(mybuf);
}

char* PlainParserCommon::set_temp_range(const char opening, const char closing)
{
   std::streambuf* mybuf=is->rdbuf();

   if (!CharBuffer::skip_ws(mybuf)) {
      is->setstate(closing == '\n' ? is->eofbit : is->failbit | is->eofbit);
      return nullptr;
   }

   std::streamsize offset;
   if (closing == '\n') {
      offset = CharBuffer::find_char_forward(mybuf, '\n');
      if (offset < 0) return nullptr;
      ++offset;
   } else {
      if (*CharBuffer::get_ptr(mybuf) != opening) {
         is->setstate(is->failbit);
         return nullptr;
      }
      CharBuffer::get_bump(mybuf, 1);
      offset = CharBuffer::matching_brace(mybuf, opening, closing);
      if (offset < 0) {
         is->setstate(is->failbit);
         return nullptr;
      }
   }

   return set_input_range(offset);
}

char* PlainParserCommon::set_input_range(std::streamsize offset)
{
   streambuf_ext* mybuf = static_cast<streambuf_ext*>(is->rdbuf());
   char* egptr = CharBuffer::end_get_ptr(mybuf);
   if (CharBuffer::get_input_limit(mybuf)) {
      CharBuffer::set_end_get_ptr(mybuf, CharBuffer::get_ptr(mybuf) + offset);
   } else {
      mybuf->set_input_width(offset);
   }
   return egptr;
}

void PlainParserCommon::restore_input_range(char* egptr)
{
   streambuf_ext* mybuf = static_cast<streambuf_ext*>(is->rdbuf());
   if (egptr == CharBuffer::get_input_limit(mybuf)) {
      mybuf->reset_input_width(false);
   } else {
      CharBuffer::set_end_get_ptr(mybuf, egptr);
   }
}

void PlainParserCommon::discard_range(const char closing)
{
   std::streambuf* mybuf = is->rdbuf();
   if (is->eof())
      is->clear();
   else if (CharBuffer::skip_ws(mybuf) ||
            CharBuffer::get_ptr(mybuf) != CharBuffer::end_get_ptr(mybuf))
      is->setstate(is->failbit);
   if (is->good() && closing != '\n')
      CharBuffer::get_bump(mybuf, 1);
}

void PlainParserCommon::skip_temp_range(char* egptr)
{
   streambuf_ext* mybuf = static_cast<streambuf_ext*>(is->rdbuf());
   char* next = CharBuffer::end_get_ptr(mybuf)+1;
   if (egptr == CharBuffer::get_input_limit(mybuf)) {
      mybuf->reset_input_width(false);
      CharBuffer::get_bump(mybuf, next - CharBuffer::get_ptr(mybuf));
   } else {
      CharBuffer::set_get_and_end_ptr(mybuf, next, egptr);
   }
}

char* PlainParserCommon::save_read_pos()
{
   return CharBuffer::get_ptr(is->rdbuf());
}

void PlainParserCommon::restore_read_pos(char *pos)
{
   streambuf_ext* mybuf = static_cast<streambuf_ext*>(is->rdbuf());
   mybuf->rewind(CharBuffer::get_ptr(mybuf) - pos);
}

void PlainParserCommon::get_scalar(Rational& x)
{
   static std::string text;
   if (*is >> text) {
      if (text.find_first_of("eE") != std::string::npos) {
         char* end;
         x = strtod(text.c_str(), &end);
         if (*end) is->setstate(is->failbit);
      } else {
         x.set(text.c_str());
      }
   }
}

void PlainParserCommon::get_scalar(double& x)
{
   static std::string text;
   if (*is >> text) {
      if (text.find('/') != std::string::npos) {
         x = double(Rational(text.c_str()));
      } else {
         char* end;
         x = strtod(text.c_str(), &end);
         if (*end) is->setstate(is->failbit);
      }
   }
}

void PlainParserCommon::get_string(std::string& s, char delim)
{
   if (CharBuffer::get_string(is->rdbuf(), s, delim) < 0)
      is->setstate(is->eofbit | is->failbit);
}

int PlainParserCommon::probe_inf()
{
   std::streambuf* mybuf = is->rdbuf();
   if (!CharBuffer::skip_ws(mybuf)) return 0;
   CharBuffer::size_type offset = 0;
   int sgn = 1;
   switch (CharBuffer::seek_forward(mybuf, 0)) {
   case '-':
      sgn = -1;
      // FALLTHRU
   case '+':
      ++offset;
      break;
   case 'i':
      break;
   default:
      return 0;
   }
   if ((offset == 0 || CharBuffer::seek_forward(mybuf, offset) == 'i') &&
       CharBuffer::seek_forward(mybuf, offset+1) == 'n' &&
       CharBuffer::seek_forward(mybuf, offset+2) == 'f') {
      CharBuffer::get_bump(mybuf, offset+3);
      return sgn;
   }
   return 0;
}

PlainPrinter<> cout{std::cout};
PlainPrinter<> cerr{std::cerr};

} // end namespace pm

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
