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

import de.tuberlin.polymake.common.SelectorThread;
import de.tuberlin.polymake.common.SharedMemoryMatrix;
import de.tuberlin.polymake.common.SharedMemoryMatrixException;

public class PointSet {

	/** the name of the pointset */
	protected String name;

	/** the coordinates of the pointset */
	protected PolymakePoint[] points;

	/** dimension */
	int dim = 3;

	protected SharedMemoryMatrix smm = null;

	public PointSet() {
		name = null;
		points = new PolymakePoint[0];
	}

	public PointSet(String name, int nPoints) {
		this.name = name;
		points = new PolymakePoint[nPoints];
	}

	// / Copy constructor
	public PointSet(PointSet ps) {
		this.name = ps.getName();
		this.points = ps.getPoints();
		this.smm = ps.getSMM(); // FIXME???
	}

	public void setName(String name) {
		this.name = name;
	}

	public void setPoint(int index, PolymakePoint pt) {
		points[index] = pt;
	}

	public void setPoints(PolymakePoint[] points) {
		if (this.points == null || this.points.length == 0)
			this.points = new PolymakePoint[points.length];
		System.arraycopy(points, 0, this.points, 0, points.length);
	}

	
	public void resize(int n_points, int new_dim) {
            dim = new_dim;
		points = new PolymakePoint[n_points];
		for (int i = 0; i < points.length; i++) {
			points[i] = new PolymakePoint(dim);
		}
	}
	
	/*
	 * public void setPoint(int index, double[] coords, String label) {
	 * points[index] = new Point(coords,label); }
	 * 
	 * public void setPoint(int index, double[] coords) { points[index] = new
	 * Point(coords); }
	 */

	public String getName() {
		return name;
	}

	public PolymakePoint getPoint(int index) {
		return points[index];
	}

	public PolymakePoint[] getPoints() {
		PolymakePoint[] pts = new PolymakePoint[points.length];
		System.arraycopy(points, 0, pts, 0, points.length);
		return pts;
	}

	public int getNPoints() {
		return points.length;
	}

	/**
	 * @return Returns the dim.
	 */
	public int getDim() {
		return dim;
	}

	/**
	 * @param dim
	 *            The dim to set.
	 */
	public void setDim(int dim) {
		this.dim = dim;
	}

	public void setPointCoords(int vertexIndex, double[] coords) {
		if (points[vertexIndex] == null) {
                    points[vertexIndex] = new PolymakePoint(coords);
		} else {
                    points[vertexIndex].setCoords(coords);
		}
		if(smm != null) {
			try {
				smm.setCoords(vertexIndex, points[vertexIndex]);
			} catch (SharedMemoryMatrixException e) {
				e.printStackTrace(SelectorThread.newErr);
			}
		}
	}

	public void setNPoints(int npoints) {
		PolymakePoint[] newPoints = new PolymakePoint[npoints];
		System.arraycopy(points, 0, newPoints, 0, Math.min(npoints,
				points.length));
		points = newPoints;
	}

	public void setMarkedPoint(int i, boolean b) {
		points[i].setMarked(b);
	}

	public String toString() {
		String msg = new String();
		msg += "name:" + name + "\npoints:\n";
		for (int i = 0; i < points.length; i++) {
			msg += "\t" + points[i].toString();
		}
 	
		return msg;
	}

	public SharedMemoryMatrix getSMM() {
		return smm;
	}

	public void initSMM(int smmKey) throws SharedMemoryMatrixException{
		smm = new SharedMemoryMatrix(smmKey);
	}

	public void readFromSMM() throws SharedMemoryMatrixException {
		if(getNPoints() == 0) {
			resize(smm.getNPoints(), smm.getDim());
		}
		smm.copyCoords(this);
	}

	public void writeToSMM() throws SharedMemoryMatrixException {
		smm.setCoords(this);
	}	
}
