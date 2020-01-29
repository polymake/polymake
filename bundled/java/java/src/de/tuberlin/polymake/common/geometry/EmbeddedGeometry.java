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

package de.tuberlin.polymake.common.geometry;

/**
 * This class implements the base geometry structure of the polymake JavaView
 * visualisation. It is based on the JavaView class jv.geom.PgGeometry.
 * 
 * @author Thilo Rörig
 */
public class EmbeddedGeometry {

	/** The JavaView geometry containing the geometric data. */
	protected GeometryIf geometry;
	
	/** An array containing the vertex indices */
	protected int[] vertexList;
	/** 
	 * If a geometry is dynamic it depends on the vertices of a second geometry
	 * and is updated if this geometry is changed.
	 */
	protected boolean dynamic = false;

	/**
	 * Creates a new <code>EmbeddedGeometry</code> instance.
	 * 
	 * @param vertexList
	 *            an array of vertex indices corresponding to the vertices of
	 *            the geometry this geometry is depending on
	 * @param geom
	 *            the geometric data, e.g. polygons, lines.
	 * @param dynamic
	 *            if true, this geometry is embedded on the vertices of some
	 *            other geometry.
	 */
	public EmbeddedGeometry(int[] vertexList, GeometryIf geom, boolean dynamic) {
		if (vertexList != null) {
			this.vertexList = new int[vertexList.length];
			System.arraycopy(vertexList, 0, this.vertexList, 0,
					vertexList.length);
		}
		geometry = geom.copy();
		this.dynamic = dynamic;
	}

	/**
	 * Creates a new <code>EmbeddedGeometry</code> instance.
	 * 
	 * @param vertexList
	 *            an <code>int[]</code> value
	 */
	public EmbeddedGeometry(int[] vertexList) {
		System.arraycopy(vertexList, 0, this.vertexList, 0, vertexList.length);
	}

	/**
	 * This methods updates the coordinates of the EmbeddedGeometry.
	 * 
	 * @param ps
	 *            A <code>PgPointSet</code> containing the new vertices
	 * @param clearTag
	 *            if true, the tags (e.g. marked) of the vertices will be
	 *            removed
	 */
	public void update(PointSet ps, boolean clearTag) {
		for (int i = 0; i < vertexList.length; ++i) {
			double[] coords = (ps.getPoint(vertexList[i])).getCoords();
			geometry.setPointCoords(i,coords);
		}
		geometry.update();
	}

	/**
	 * Set the geometric structure of the EmbeddedGeometry
	 * 
	 * @param g
	 *            the geometric structure
	 */
	public void setGeometry(GeometryIf g) {
		geometry = g.copy();
	}

	/**
	 * Get the geometric structure of the EmbeddedGeometry
	 * 
	 * @return the geometric structure as <code>PgGeometry</code>
	 */
	public GeometryIf getGeometry() {
		return geometry;
	}

	// @Override
	public Object clone() {
		return new EmbeddedGeometry(vertexList, geometry, dynamic);
	}

	public String getName() {
		return geometry.getName();
	}

	// @Override
	public String toString() {
		String msg = new String();
		msg += "geometry=" + geometry.toString() + "\n";
		msg += "vertexList=";
		if(vertexList!=null) {
			for(int i = 0; i < vertexList.length; ++i) {
				msg += vertexList[i] + " ";
			}
		} else {
			msg += "null";
		}
		msg += "\ndynamic = " + dynamic +"\n";
		return msg;
	}

	/**
	 * Each vertex of an EmbeddedGeometry corresponds to a vertex of the embedding.
	 * This method returns the indices of the vertices of the EmbeddedGeometry.
	 *
	 * @return the indices of the vertices of the underlying geometry
	 */
	public int[] getVertexList() {
		int[] vl = new int[vertexList.length];;
		System.arraycopy(this.vertexList,0,vl,0,vl.length);
		return vl;
	}

	/**
	 * Get the index of the <code>i</code>'s vertex.
	 *
	 * @param i the number of the vertex of the EmbeddedGeometry
	 * @return  the corresponding index of the underlying geometry.
	 */
	public int getVertexIndex(int i) {
		
		return vertexList[i];
	}

	/**
	 * Is the geometry dynamically embedded on an underlying geometry?
	 *
	 * @return true, if dynamic
	 */
	public boolean isDynamic() {
		return dynamic;
	}
}
