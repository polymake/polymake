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

#ifndef POLYMAKE_INTERNAL_CHARBUFFER_H
#define POLYMAKE_INTERNAL_CHARBUFFER_H

#include <sys/types.h>
#include <cctype>
#include <cstring>
#include <cstddef>
#include <limits>
#include "polymake/internal/streambuf_ext.h"

namespace pm {

class CharBuffer : public streambuf_ext {
private:
   CharBuffer() = delete;
   ~CharBuffer();
public:
   using std::streambuf::traits_type;
   using std::streambuf::int_type;

   void skip_all()
   {
      setg(eback(), egptr(), egptr());
   }

   static void skip_all(std::streambuf* buf_)
   {
      static_cast<CharBuffer*>(buf_)->skip_all();
   }

   static int_type seek_forward(std::streambuf* buf_, size_type offset)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      const int_type eof_char = traits_type::eof();
      if (buf->gptr() + offset >= buf->egptr() && buf->underflow() == eof_char)
         return eof_char;
      return buf->gptr()[offset];
   }

   static size_type next_ws(std::streambuf* buf_, size_type offset = 0, bool report_eof = true)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      const int_type eof_char = traits_type::eof();
      for (int_type c; (c = seek_forward(buf, offset)) != eof_char; ++offset)
         if (isspace(c)) return offset;
      return report_eof ? -1 : offset;
   }

   static size_type next_non_ws(std::streambuf *buf_, size_type offset = 0)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      const int_type eof_char = traits_type::eof();
      for (int_type c; (c = seek_forward(buf, offset)) != eof_char; ++offset)
         if (!isspace(c)) return offset;
      return -1;
   }

   static bool skip_ws(std::streambuf* buf_)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      const size_type i = next_non_ws(buf);
      if (i < 0) {
         buf->skip_all();
         return false;
      } else {
         buf->gbump(i);
         return true;
      }
   }

   static size_type find_char_forward(std::streambuf* buf_, const char c, size_type offset = 0)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      const int_type eof_char = traits_type::eof();
      if (seek_forward(buf, offset) != eof_char) {
         do {
            if (char* found = (char*)memchr(buf->gptr()+offset, c, buf->egptr() - (buf->gptr() + offset)))
               return found - buf->gptr();
            offset = buf->egptr() - buf->gptr();
         } while (buf->underflow() != eof_char);
      }
      return -1;
   }

   static int_type ignore(std::streambuf* buf_, const char c)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      const int_type eof_char = traits_type::eof();
      int_type next_c;
      while ((next_c = seek_forward(buf, 0)) != eof_char) {
         if (next_c == c) {
            buf->gbump(1);
            break;
         }
         if (char* found = (char*)memchr(buf->gptr(), c, buf->egptr() - buf->gptr())) {
            buf->gbump(found - buf->gptr()+1);
            return c;
         }
         buf->skip_all();
      }
      return next_c;
   }

   static size_type find_string_forward(std::streambuf* buf_, const char* c, size_type len, size_type offset)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      const int_type eof_char = traits_type::eof();
      while ((offset = find_char_forward(buf, c[0], offset)) >= 0  &&
             seek_forward(buf, offset+len-1) != eof_char) {
         if (!memcmp(buf->gptr()+offset, c, len)) return offset;
         ++offset;
      }
      return -1;
   }

   static size_type count_char(std::streambuf *buf_, const char c)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      char* start = buf->gptr(), *end = buf->egptr();
      size_type cnt = 0;
      while ((start = (char*)memchr(start, c, end - start)) != nullptr) {
         ++cnt; ++start;
      }
      return cnt;
   }

   static size_type count_lines(std::streambuf *buf)
   {
      if (!skip_ws(buf)) return 0;
      return count_char(buf, '\n');
   }

   static size_type matching_brace(std::streambuf* buf, char opening, char closing, size_type offset = 0);
   static size_type get_string(std::streambuf* buf, std::string&, char delim);

   static char* get_buf_start(std::streambuf* buf_)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      return buf->eback();
   }
   static char* get_ptr(std::streambuf* buf_)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      return buf->gptr();
   }
   static char* end_get_ptr(std::streambuf* buf_)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      return buf->egptr();
   }
   static void get_bump(std::streambuf *buf_, size_type offset)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      buf->gbump(offset);
   }
   static void set_end_get_ptr(std::streambuf* buf_, char* end)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      buf->setg(buf->eback(), buf->gptr(), end);
   }
   static void set_end_get_ptr(std::streambuf* buf_, size_type offset)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      buf->setg(buf->eback(), buf->gptr(), buf->gptr() + offset);
   }
   static void set_get_and_end_ptr(std::streambuf* buf_, char* get, char* end)
   {
      CharBuffer* buf = static_cast<CharBuffer*>(buf_);
      buf->setg(buf->eback(), get, end);
   }
   static const char* get_input_limit(std::streambuf* buf_)
   {
      return static_cast<CharBuffer*>(buf_)->input_limit;
   }
};

class OutCharBuffer : public std::streambuf {
private:
   OutCharBuffer() = delete;
   ~OutCharBuffer();
public:
   using size_type = std::streamsize;

   class Slot {
      friend class OutCharBuffer;
   protected:
      std::streambuf* b;
      char* emerg_buf;
      char* out;
      size_type size;
      size_type width;

      Slot(std::streambuf* b_arg, size_type size_arg, size_type width_arg);
   public:
      operator char* () const { return out; }
      ~Slot();
   };

   template <typename Traits>
   static Slot reserve(std::basic_ostream<char, Traits>& os, size_type size)
   {
      size_type w = os.width();
      if (w > 0) os.width(0);
      return Slot(os.rdbuf(), size, w);
   }

   void pbump(size_type n)
   {
      while (__builtin_expect(n > std::numeric_limits<int>::max(), 0)) {
         std::streambuf::pbump(std::numeric_limits<int>::max());
         n -= std::numeric_limits<int>::max();
      }
      std::streambuf::pbump(static_cast<int>(n));
   }
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_CHARBUFFER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
