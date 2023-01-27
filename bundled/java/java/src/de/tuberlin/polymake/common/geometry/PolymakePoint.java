/* Copyright (c) 1997-2023
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

package de.tuberlin.polymake.common.geometry;

public class PolymakePoint {
	
	/** the name of the pointset */
	protected String label;
	
	/** */
	protected double[] coords;
	
	protected boolean marked = false;
	
	public PolymakePoint( int dim ) {
		this.coords = new double[dim];
		label = null;
	}

	public PolymakePoint(double[] coords, String label) {
		this.coords = new double[coords.length];
		System.arraycopy(coords,0,this.coords,0,coords.length);
		this.label = label;
	}
	
	public PolymakePoint(double[] coords) {
		this.coords = new double[coords.length];
		System.arraycopy(coords,0,this.coords,0,coords.length);
		label = null;
	}
	
	public double[] getCoords() {
		return coords;
	}
	
	public String getLabel() {
		return label;
	}

	public void setCoords(double[] coords) {
		System.arraycopy(coords, 0, this.coords, 0, this.coords.length);
	}

	public void setMarked(boolean b) {
		marked = b;
	}
	
	public String toString() {
		String msg = new String();
		msg += "label:" + label + ",coords:";
		for (int i = 0; i < coords.length; i++) {
			msg += " " + coords[i];
		}
		msg += ", marked:" + marked +"\n";
		return msg;
	}
}
