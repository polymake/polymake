/* Copyright (c) 1997-2022
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

package de.tuberlin.polymake.common.javaview;

import jv.geom.PgPointSet;
import jv.object.PsObject;
import jv.project.PgGeometry;
import de.tuberlin.polymake.common.geometry.GeometryIf;

public class Geometry implements GeometryIf {

	protected PgGeometry geometry;

	public Geometry(PgGeometry geom) {
		this.geometry = (PgGeometry) geom.clone();
	}

	public void setPointCoords(int index, double[] coords) {
		geometry.getVertex(index).copy(coords, coords.length);
	}

	public void update() {
		geometry.update(geometry);
	}

	public GeometryIf copy() {
		return new Geometry(geometry);
	}

	// ATTENTION: no cloning here!
	public PgGeometry getGeometry() {
		return geometry;
	}

	public String getName() {
		return geometry.getName();
	}

	public boolean getMarked(int i) {
		return geometry.getVertex(i).hasTag(PsObject.IS_SELECTED);
	}

	public int getNumVertices() {
		PgPointSet pset = (PgPointSet)geometry;
		return pset.getNumVertices();
	}

	public double[] getVertexCoords(int i) {
		return geometry.getVertex(i).getEntries();
	}

	public void setMarked(int i, boolean b) {
		geometry.getVertex(i).setTag(PsObject.IS_SELECTED);
	}
	
}
