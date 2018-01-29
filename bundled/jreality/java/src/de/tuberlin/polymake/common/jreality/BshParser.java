/* Copyright (c) 1997-2018
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

package de.tuberlin.polymake.common.jreality;

import java.io.BufferedReader;
import java.io.IOException;
import java.util.StringTokenizer;

import de.jreality.reader.ReaderBSH;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.tuberlin.polymake.common.SelectorThread;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometry;
import de.tuberlin.polymake.common.io.GeometryParserIf;

public class BshParser implements GeometryParserIf {

    protected Transformation transform;

    public EmbeddedGeometries parse(java.io.BufferedReader in)
			throws IOException {
		SceneGraphComponent sgc;
		EmbeddedGeometry[] geoms = null;
		String title = "NoName";
		try {
			final ReaderBSH bshReader = new ReaderBSH();

			sgc = bshReader.getComponent();
			final BufferedReader pserver = in;
			Runnable runner = new Runnable() {
				public void run() {
					try {
						bshReader
								.processReader(pserver, System.out, System.err);
					} catch (Exception e) {
						e.printStackTrace(SelectorThread.newErr);
					}
				}
			};

			Thread bshThread = new Thread(runner,"BSH Reader thread");

			bshThread.start();
			bshThread.join();

			sgc = sgc.getChildComponent(0);
			title = sgc.getName();
			if (sgc.getChildComponentCount() == 0) {
				geoms = new EmbeddedGeometry[1];
				geoms[0] = new EmbeddedGeometry(null,
						new Geometry(sgc.getChildComponent(0)), false); 
			} else {
				geoms = new EmbeddedGeometry[sgc.getChildComponentCount()];

				for (int i = 0; i < sgc.getChildComponentCount(); ++i) {
					de.jreality.scene.PointSet ps = (de.jreality.scene.PointSet) sgc.getChildComponent(i)
							.getGeometry();
					String name = sgc.getChildComponent(i).getName();

					boolean dynamic = false;
					StringTokenizer st = new StringTokenizer(name, ":");
					if (st.nextToken().equals("dynamic")) {
						dynamic = true;
						name = st.nextToken();
						sgc.getChildComponent(i).setName(name);
					}
					int[] vertexList = null;

					if (dynamic) {
						vertexList = new int[ps.getNumPoints()];
						double[][] vertexCoords = ps.getVertexAttributes(
								Attribute.COORDINATES).toDoubleArrayArray(null);
						for (int j = 0; j < vertexList.length; ++j) {
							vertexList[j] = (int) (vertexCoords[j][0]);
						}
					}
					geoms[i] = new EmbeddedGeometry(vertexList,
							new Geometry(sgc.getChildComponent(i)), dynamic);
				}
			}
                        transform = sgc.getTransformation();

		} catch (InterruptedException e) {
			SelectorThread.newErr
					.println("Thread reading bsh-code from polymake-server interrupted.");
			e.printStackTrace(SelectorThread.newErr);
		} catch (Exception e) {
			SelectorThread.newErr
					.println("Exception occured while creating the ReaderBSH.");
			e.printStackTrace(SelectorThread.newErr);
		}
		return new EmbeddedGeometries(title, geoms);
	}

    public Transformation getTransformation() {
        return transform;
    }
}
