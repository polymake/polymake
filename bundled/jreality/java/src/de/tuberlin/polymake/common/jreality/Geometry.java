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

package de.tuberlin.polymake.common.jreality;

import java.awt.Color;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.PointDragEvent;
import de.jreality.tools.PointDragListener;
import de.tuberlin.polymake.common.geometry.GeometryIf;

public class Geometry implements GeometryIf {

	protected SceneGraphComponent sgc;

	protected boolean[] marked;

	// original colors of the pointset
	final protected double[][] colors;

	DragEventTool markTool = new DragEventTool("PrimaryAction");

	public Geometry(SceneGraphComponent i_sgc) {

		sgc = new SceneGraphComponent();
		sgc.setGeometry(i_sgc.getGeometry());
		sgc.getGeometry().setName(i_sgc.getName());
		Appearance app = i_sgc.getAppearance();
		sgc.setAppearance(app);
		sgc.setName(i_sgc.getName());
		// FIXME: Clone the SceneGraphComponent?
//		Utils.copy(i_sgc);

		de.jreality.scene.PointSet pset = (de.jreality.scene.PointSet) sgc
				.getGeometry();
		marked = new boolean[pset.getNumPoints()];

		if (pset.getVertexAttributes(Attribute.COLORS) == null) {
			colors = new double[pset.getNumPoints()][3];
			Color defaultColor = (Color) sgc.getAppearance().getAttribute(
					CommonAttributes.POINT_SHADER + "."
							+ CommonAttributes.DIFFUSE_COLOR);
			for (int i = 0; i < colors.length; i++) {
				float[] rgb = new float[3];
				defaultColor.getRGBColorComponents(rgb);
				colors[i][0] = rgb[0];
				colors[i][1] = rgb[1];
				colors[i][2] = rgb[2];
			}
		} else {
			colors = new double[pset.getNumPoints()][];
			((DoubleArrayArray) pset.getVertexAttributes(Attribute.COLORS))
					.toDoubleArrayArray(colors);
		}
		markTool.addPointDragListener(new PDMarker());
		sgc.addTool(markTool);
		sgc.getAppearance().setAttribute(
						CommonAttributes.POINT_SHADER + "."
								+ CommonAttributes.PICKABLE, true);
		sgc.getAppearance().setAttribute(
				CommonAttributes.POLYGON_SHADER + "."
						+ CommonAttributes.PICKABLE, false);
		sgc.getAppearance().setAttribute(
				CommonAttributes.LINE_SHADER + "."
						+ CommonAttributes.PICKABLE, false);
	}

	@Override
	public GeometryIf copy() {
		return new Geometry(sgc);
	}

	@Override
	public boolean getMarked(int i) {
		return marked[i];
	}

	@Override
	public String getName() {
		return sgc.getName();
	}

	@Override
	public int getNumVertices() {
		return marked.length;
	}

	@Override
	public double[] getVertexCoords(int i) {
		de.jreality.scene.PointSet ps = (de.jreality.scene.PointSet) sgc
				.getGeometry();
		double[][] newCoords = ((DoubleArrayArray) ps
				.getVertexAttributes(Attribute.COORDINATES))
				.toDoubleArrayArray(null);
		return newCoords[i];
	}

	@Override
	public void setMarked(int i, boolean b) {
		marked[i] = b;
	}

	@Override
	public void setPointCoords(int index, double[] coords) {
		de.jreality.scene.PointSet ps = (de.jreality.scene.PointSet) sgc
				.getGeometry();
		double[][] newCoords = ((DoubleArrayArray) ps
				.getVertexAttributes(Attribute.COORDINATES))
				.toDoubleArrayArray(null);
		System.arraycopy(coords, 0, newCoords[index], 0, coords.length);
		ps.setVertexAttributes(Attribute.COORDINATES,
				new DoubleArrayArray.Array(newCoords));
	}

	@Override
	public void update() {
		if (sgc.getGeometry() instanceof de.jreality.scene.IndexedFaceSet) {
		    IndexedFaceSetUtility.calculateAndSetFaceNormals((IndexedFaceSet) sgc.getGeometry());
		} 
	}
	
	public SceneGraphComponent getSceneGraphComponent() {
		return sgc;
	}

	public void updateColor() {
		de.jreality.scene.PointSet pset = (de.jreality.scene.PointSet) sgc
				.getGeometry();
		double[][] newcls = new double[pset.getNumPoints()][3];

		for (int j = 0; j < pset.getNumPoints(); ++j) {
			if (marked[j]) {
				newcls[j][0] = 1;
				newcls[j][1] = 1;
				newcls[j][2] = 1;
			} else {
				newcls[j][0] = colors[j][0];
				newcls[j][1] = colors[j][1];
				newcls[j][2] = colors[j][2];
			}
		}
		pset.setVertexAttributes(Attribute.COLORS, new DoubleArrayArray.Array(
				newcls));
	}

	private class PDMarker implements PointDragListener {
		@Override
		public void pointDragStart(PointDragEvent e) {
			setMarked(e.getIndex(), !getMarked(e.getIndex()));
			updateColor();
		}

		@Override
		public void pointDragged(PointDragEvent e) {
			// do nothing
		}

		@Override
		public void pointDragEnd(PointDragEvent e) {
			// do nothing
		}
	}

}
