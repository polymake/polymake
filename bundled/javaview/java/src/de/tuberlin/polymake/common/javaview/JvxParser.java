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

package de.tuberlin.polymake.common.javaview;

import java.io.IOException;
import java.util.StringTokenizer;

import jv.geom.PgElementSet;
import jv.geom.PgPointSet;
import jv.geom.PgPolygon;
import jv.geom.PgPolygonSet;
import jv.loader.PgJvxLoader;
import jv.object.PsDebug;
import jv.project.PgJvxSrc;
import jv.project.PvGeometryIf;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometry;
import de.tuberlin.polymake.common.io.GeometryParserIf;

/**
 * This class parses JavaView jvx files and puts them into one
 * EmbeddedGeometries class. It parses the name of the JavaView geometries to
 * detect dynamic geometries. The name of a dynamic geometry is "dynamic:<name>".
 * The first geometry of the jvx file is the embedding of the
 * <code>EmdeddedGeometries</code>.
 * 
 * @author Thilo Rörig
 */
public class JvxParser implements GeometryParserIf{

	/**
	 * Parse a jvx-data into an EmbeddedGeometries object. The first geometry of
	 * the jvx-data is the basis of all the following geometries. The geometries
	 * that have to change if the embedding changes must have the name "dynamic:<name>".
	 * 
	 * @param in
	 *            BufferedReader for textual input which will be parsed
	 * @return EmbeddedGeometries instance containing
	 * @exception IOException
	 *                if problems reading from in occur
	 */
	public EmbeddedGeometries parse(java.io.BufferedReader in)
			throws IOException {

		PgJvxLoader jvxLoader = new PgJvxLoader();

		if (!jvxLoader.load(in))
			throw new IOException("Error reading JVX-Data.");

		PgJvxSrc[] i_geoms = jvxLoader.getGeometries();

		EmbeddedGeometry[] geoms = new EmbeddedGeometry[i_geoms.length];
		for (int i = 0; i < i_geoms.length; ++i) {
			String name = i_geoms[i].getName();
			boolean dynamic = false;
			StringTokenizer st = new StringTokenizer(name, ":");
			if (st.nextToken().equals("dynamic")) {
				dynamic = true;
				name = st.nextToken();
				i_geoms[i].setName(name);
			}
			int[] vertexList = null;

			if (dynamic) {
				vertexList = new int[i_geoms[i].getNumVertices()];
				for (int j = 0; j < vertexList.length; ++j) {
					vertexList[j] = (int) (i_geoms[i].getVertex(j).getEntry(0));
				}
			}
			switch (i_geoms[i].getType()) {
			case PvGeometryIf.GEOM_POINT_SET:
				PgPointSet pointSet = new PgPointSet();
				pointSet.setJvx(i_geoms[i]);
				geoms[i] = new EmbeddedGeometry(vertexList, new Geometry(pointSet), dynamic);
				break;
			case PvGeometryIf.GEOM_POLYGON_SET:
				PgPolygonSet polygonSet = new PgPolygonSet();
				polygonSet.setJvx(i_geoms[i]);
				polygonSet.showPolygonColors(true);
				geoms[i] = new EmbeddedGeometry(vertexList, new Geometry(polygonSet), dynamic);
				break;
			case PvGeometryIf.GEOM_ELEMENT_SET:
				PgElementSet elementSet = new PgElementSet();
				elementSet.setJvx(i_geoms[i]);
				geoms[i] = new EmbeddedGeometry(vertexList, new Geometry(elementSet), dynamic);
				break;
			case PvGeometryIf.GEOM_POLYGON:
				PgPolygon polygon = new PgPolygon();
				polygon.setJvx(i_geoms[i]);
				geoms[i] = new EmbeddedGeometry(vertexList, new Geometry(polygon), dynamic);
				break;
			default:
				PsDebug.warning("geometry[" + i + "] has unknown type = "
						+ i_geoms[i].getType());
			}
		}
		String title = i_geoms[0].getTitle();
		StringTokenizer st = new StringTokenizer(title, ":");
		if (st.nextToken().equals("dynamic")) {
			title = st.nextToken();
		}
		return new EmbeddedGeometries(title, geoms);
	}
}
