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

#include "polymake/internal/CharBuffer.h"

namespace pm {

int CharBuffer::matching_brace (std::streambuf *_buf, char opening, char closing, int offset)
{
   CharBuffer *buf=static_cast<CharBuffer*>(_buf);
   int level=1;
   int i_op=find_char_forward(buf,opening,offset), i_cl=find_char_forward(buf,closing,offset);
   while (i_cl>=0)
      if (i_op<0 || i_cl<i_op) {
         if (--level==0) break;
         i_cl=find_char_forward(buf,closing,i_cl+1);
      } else {
         ++level;
         i_op=find_char_forward(buf,opening,i_op+1);
      }
   return i_cl;
}

int CharBuffer::get_string(std::streambuf *_buf, std::string& s, char delim)
{
   CharBuffer *buf=static_cast<CharBuffer*>(_buf);
   int end=-1;
   if (delim!=0) {
      end=find_char_forward(buf,delim);
   } else if (skip_ws(buf) >= 0) {
      end=next_ws(buf,0,false);
   }
   if (end>=0) {
      s.assign(buf->gptr(),end);
      buf->gbump(end+(delim!=0));
   }
   return end;
}

OutCharBuffer::Slot::Slot(std::streambuf *b_arg, ptrdiff_t size_arg, ptrdiff_t width_arg)
   : b(b_arg), emerg_buf(0), out(0), size(size_arg), width(width_arg)
{
   OutCharBuffer *buf=static_cast<OutCharBuffer*>(b);
   ptrdiff_t s=size_arg, margin=0;
   if (width_arg >= s) {
      margin=width_arg+1-s;
      s=width_arg+1;
   }
   if (char *p=buf->pptr()) {
      // is buffered
      if (buf->epptr()-p >= s) {
         // enough space till the end of output area
         out=p;
      } else if (buf->epptr()-buf->pbase() >= s) {
         // the buffer is large enough, try to purge it
         buf->sync();
         if (buf->epptr()-(p=buf->pptr()) >= s) out=p;
      }
   }
   if (out) {
      if (margin>0) {
         std::memset(out, ' ', margin);
         out+=margin;
         width-=margin;
         buf->pbump(margin);
      }
   } else {
      out=emerg_buf=new char[size];
      if (margin>0) {
         width-=margin;
         do buf->sputc(' '); while (--margin>0);
      }
   }
}

OutCharBuffer::Slot::~Slot()
{
   OutCharBuffer *buf=static_cast<OutCharBuffer*>(b);
   if (size>2 && out[size-3]=='\0') size-=2;
   else if (size>1 && out[size-2]=='\0') --size;
   if (emerg_buf) {
      for (; width >= size; --width)
         buf->sputc(' ');
      buf->sputn(emerg_buf,size-1);
      delete[] emerg_buf;
   } else {
      if (width >= size) {
         ptrdiff_t addmargin=width+1-size;
         std::memmove(out+addmargin, out, size-1);
         std::memset(out, ' ', addmargin);
         size+=addmargin;
      }
      buf->pbump(size-1);
   }
}

} // end namespace pm

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
