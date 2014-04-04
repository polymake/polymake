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

#ifndef _POLYMAKE_POSTSCRIPT_H
#define _POLYMAKE_POSTSCRIPT_H

#include <fstream>

using namespace polymake;

double transform(const double x, const double ll, const double scale, const double offset)
{
   return (x-ll)*scale+offset;
}

class PostScriptFile {
private:
   std::ofstream ps;

   double linewidth;
   double gray;
   char *dashpattern;

   char *dotcolor;
   double dotradius;

   double size, offset, llx, lly, scale;

public:
   PostScriptFile(const char *filename, const double _llx, const double _lly, const double urx, const double ury) :
      ps(filename,ios::out),
      linewidth(2.0), gray(0.0), dashpattern("[3 3] 1"),
      dotcolor("0.0 1.0 0.0"), dotradius(5.0),
      size(500.0), offset(40.0),     
      llx(_llx), lly(_lly), scale(size/std::max(urx-llx,ury-lly)) {

      ps << "%!PS-Adobe-3.0\n"
	 << "%%Creator: polymake"
	 << "%%Title: tropical polygon\n"
	 << "%%Pages: 1\n"
	 << "%%BoundingBox: " << transform(llx,llx,scale,0) << " " << transform(lly,lly,scale,0) << " "
	                      << transform(urx,llx,scale,2*offset) << " " << transform(ury,lly,scale,2*offset) << "\n"
	 << "%%EndComments\n"
	 << "%%BeginProlog\n"
	 << "%%EndProlog\n"
	 << "%%Page: 1 1\n"
	 << "%%BeginPageSetup\n"
	 << linewidth << " setlinewidth\n"
	 << gray << " setgray\n"
	 << "%%EndPageSetup\n";
   }

   ~PostScriptFile() {
      ps << "showpage\n"
	 << "%%EOF\n";
      ps.close();
   }

   void dot(double x, double y) {
      ps << "gsave "
	 << dotcolor << " setrgbcolor "
	 << "newpath "
	 << transform(x,llx,scale,offset) << " " << transform(y,lly,scale,offset) << " " << dotradius << " 0 360 arc "
	 << "closepath eofill grestore\n";
   }

   void line(double x1, double y1, double x2, double y2) {
      ps << transform(x1,llx,scale,offset) << " " << transform(y1,lly,scale,offset) << " moveto "
	 << transform(x2,llx,scale,offset) << " " << transform(y2,lly,scale,offset) << " lineto stroke\n";
   }

   void dashed_line(double x1, double y1, double x2, double y2) {
      ps << "gsave "
	 << dashpattern << " setdash "
	 << transform(x1,llx,scale,offset) << " " << transform(y1,lly,scale,offset) << " moveto "
	 << transform(x2,llx,scale,offset) << " " << transform(y2,lly,scale,offset) << " lineto stroke "
	 << "grestore\n";
   }
};

#endif // _POLYMAKE_POSTSCRIPT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// End:
