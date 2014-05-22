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

package de.tuberlin.polymake.common.geometry;

public interface GeometryIf extends Cloneable {

	public void setPointCoords(int index, double[] coords);

	public void update();

	public GeometryIf copy();

	public String getName();
	
	public boolean getMarked(int i);
	
	public void setMarked(int i,boolean b);

	public int getNumVertices();

	public double[] getVertexCoords(int i);

}
