/* Copyright (c) 1997-2015
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

#ifndef POLYMAKE_INTERNAL_CHARBUFFER_H
#define POLYMAKE_INTERNAL_CHARBUFFER_H

#include <sys/types.h>
#include <cctype>
#include <cstring>
#include <cstddef>
#include "polymake/socketstream.h"

namespace pm {

class CharBuffer : public streambuf_with_input_width {
private:
   // never create
   CharBuffer();
   ~CharBuffer();
public:
   typedef std::streambuf::traits_type traits_type;
   typedef std::streambuf::int_type int_type;

   static void skip_all(std::streambuf *_buf)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      PM_SET_BUF_GET_CUR_END(buf);
   }

   static int seek_forward(std::streambuf *_buf, int offset)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      const int_type eof_char=traits_type::eof();
      if (buf->gptr()+offset >= buf->egptr() && buf->underflow()==eof_char)
         return eof_char;
      return buf->gptr()[offset];
   }

   static int next_ws(std::streambuf *_buf, int offset=0, bool report_eof=true)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      const int_type eof_char=traits_type::eof();
      for (int_type c; (c=seek_forward(buf, offset))!=eof_char; ++offset)
         if (isspace(c)) return offset;
      return report_eof ? -1 : offset;
   }

   static int next_non_ws(std::streambuf *_buf, int offset=0)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      const int_type eof_char=traits_type::eof();
      for (int_type c; (c=seek_forward(buf, offset))!=eof_char; ++offset)
         if (!isspace(c)) return offset;
      return -1;
   }

   static int skip_ws(std::streambuf *_buf)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      int i=next_non_ws(buf);
      if (i<0) {
         skip_all(buf);
         return -1;
      } else {
         buf->gbump(i);
         return 0;
      }
   }

   static int find_char_forward(std::streambuf *_buf, char c, int offset=0)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      const int_type eof_char=traits_type::eof();
      if (seek_forward(buf, offset) != eof_char) {
         do {
            if (char *found=(char*)memchr(buf->gptr()+offset, c, buf->egptr()-(buf->gptr()+offset)))
               return found-buf->gptr();
            offset=buf->egptr()-buf->gptr();
         } while (buf->underflow() != eof_char);
      }
      return -1;
   }

   static int_type ignore(std::streambuf *_buf, char c)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      const int_type eof_char=traits_type::eof();
      int_type next_c;
      while ((next_c=seek_forward(buf,0)) != eof_char) {
         if (next_c==c) {
            buf->gbump(1);
            break;
         }
         if (char *found=(char*)memchr(buf->gptr(), c, buf->egptr()-buf->gptr())) {
            buf->gbump(found-buf->gptr()+1);
            return c;
         }
         PM_SET_BUF_GET_CUR_END(buf);
      }
      return next_c;
   }

   static int find_string_forward(std::streambuf *_buf, const char *c, int len, int offset)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      const int_type eof_char=traits_type::eof();
      while ((offset=find_char_forward(buf, c[0], offset)) >= 0  &&
             seek_forward(buf, offset+len-1) != eof_char) {
         if (!memcmp(buf->gptr()+offset, c, len)) return offset;
         ++offset;
      }
      return -1;
   }

   static int count_char(std::streambuf *_buf, char c)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      char *start=buf->gptr(), *end=buf->egptr();
      int cnt=0;
      while ((start=(char*)memchr(start, c, end-start)) != 0) {
         ++cnt; ++start;
      }
      return cnt;
   }

   static int count_lines(std::streambuf *buf)
   {
      if (skip_ws(buf)<0) return 0;
      return count_char(buf,'\n');
   }

   static int matching_brace (std::streambuf *buf, char opening, char closing, int offset=0);
   static int get_string(std::streambuf *buf, std::string&, char delim);

   static char* get_buf_start(std::streambuf *_buf)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      return buf->eback();
   }
   static char* get_ptr(std::streambuf *_buf)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      return buf->gptr();
   }
   static char* end_get_ptr(std::streambuf *_buf)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      return buf->egptr();
   }
   static void get_bump(std::streambuf *_buf, int offset)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      buf->gbump(offset);
   }
   static void set_end_get_ptr(std::streambuf *_buf, char *end)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      PM_SET_BUF_GET_END(buf,end);
   }
   static void set_end_get_ptr(std::streambuf *_buf, int offset)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      PM_SET_BUF_GET_END_OFF(buf,offset);
   }
   static void set_get_and_end_ptr(std::streambuf *_buf, char *get, char *end)
   {
      CharBuffer *buf=static_cast<CharBuffer*>(_buf);
      PM_SET_BUF_GET_CUR(buf,get,end);
   }
   static const char* get_input_limit(std::streambuf *_buf)
   {
      return static_cast<CharBuffer*>(_buf)->input_limit;
   }
};

class OutCharBuffer : public std::streambuf {
private:
   // never create
   OutCharBuffer();
   ~OutCharBuffer();
public:
   class Slot {
      friend class OutCharBuffer;
   protected:
      std::streambuf *b;
      char *emerg_buf;
      char *out;
      ptrdiff_t size;
      int width;

      Slot(std::streambuf *b_arg, ptrdiff_t size_arg, ptrdiff_t width_arg);
   public:
      operator char* () const { return out; }
      ~Slot();
   };

   template <typename Traits>
   static Slot reserve(std::basic_ostream<char,Traits>& os, size_t size)
   {
      ptrdiff_t w=os.width();
      if (w>0) os.width(0);
      return Slot(os.rdbuf(),size,w);
   }
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_CHARBUFFER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
