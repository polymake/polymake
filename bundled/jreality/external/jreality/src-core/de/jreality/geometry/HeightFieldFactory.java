/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.geometry;

import java.awt.geom.Rectangle2D;

import de.jreality.scene.IndexedFaceSet;

/**
 * A factory for generating instances of {@link IndexedFaceSet} which
 * are defined by a height field.  
 * <p>
 * Use the {@link QuadMeshFactory#setVertexCoordinates(double[][])} 
 * or some variation, to set the height field (with one entry per vector).
 * Then use {@link #setRegularDomain(Rectangle2D)} to specify the domain of
 * definition of the height field.  The resulting height field will
 * have <i>(x,y)</i> values given by appropriately interpolated position in the
 * domain, and z-value the appropriate element of the z-array.
 * <p>
 * The following code snippet illustrates a typical usage:
 * <code><b><pre>
			SceneGraphComponent theWorld = SceneGraphUtility.createFullSceneGraphComponent();
			double[][] verts = new double[200][1];
			for (int  i =0; i<20; ++i)	{
				for (int  j =0; j<10; ++j)	{
					verts[10*i+j][0] = 1.0 - (.25*(i-9.5)*(i-9.5)+(j-4.5)*(j-4.5))/50;
				}
			}
			HeightFieldFactory hff = new HeightFieldFactory();
			hff. setULineCount(10);
			hff.setVLineCount(20);
			hff.setClosedInUDirection(false);
			hff.setClosedInVDirection(false);
			hff.setVertexCoordinates(verts);
			hff.setGenerateVertexNormals(true);
			hff.setGenerateFaceNormals(true);
			Rectangle2D.Double domain = new Rectangle2D.Double(-2, -2, 4, 4);
			hff.setRegularDomain(domain);
			hff.update();
			IndexedFaceSet ifs = hff.getIndexedFaceSet();
			theWorld.setGeometry( ifs);
 * </pre></b></code>
 * <b>Warning</b>: Not all jReality backends can handle such height fields.  JOGL and PORTAL can.
 * @author Charles Gunn
 *
 */
public class HeightFieldFactory extends QuadMeshFactory {
	Rectangle2D theDomain = new Rectangle2D.Double(-1.0, -1.0, 2.0, 2.0);
	boolean domainHasChanged = true;
	public HeightFieldFactory() {
		
	}


	/**
	 * Set the domain for this height field. Default: (xmin, ymin, width, height) = (-1, -1, 2, 2).
	 * @param r
	 */
	public void setRegularDomain(Rectangle2D r)	{
		theDomain=r;
	}
	
	public Rectangle2D getRegularDomain()	{
		return theDomain;
	}
	
	protected void updateImpl() {
		super.updateImpl();

		if( domainHasChanged)
			ifs.setGeometryAttributes(GeometryUtility.HEIGHT_FIELD_SHAPE, theDomain);
		
		domainHasChanged= false;
	}

	double [][] generateFaceNormals() {
		int [][] fi = (int[][])faceIndices.getObject();
		double [][] vc = (double[][])vertexCoordinates.getObject();
		if( fi==null || vc==null)
			return null;
		return calculateFaceNormals( fi, vc, getMetric() );
		
	}
	
	private double[][] calculateFaceNormals(int[][] is, double[][] ds, int metric) {
		double[][] fullcoords = new double[ds.length][3];
		double[] foo = new double[2];
		int maxu = getULineCount();
		int maxv = getVLineCount();
		for (int i = 0; i<maxv; ++i)	{
			int k = i * maxu;
			for (int j = 0; j<maxu; ++j)	{
				getCoordinatesForUV(foo, theDomain, j, i, maxu, maxv);
				fullcoords[k+j][0] = foo[0];
				fullcoords[k+j][1] = foo[1];
				fullcoords[k+j][2] = ds[k+j][0];
			}
		}
		return IndexedFaceSetUtility.calculateFaceNormals(is, fullcoords, metric);
		
	}

	public static double[] getCoordinatesForUV(double[] store, Rectangle2D d, int u, int v, int uc, int vc)	{
		if (store == null) store = new double[2];
		store[0] = d.getMinX()+d.getWidth()*(u/(uc-1.0));
		store[1] = d.getMinY()+d.getHeight()*(v/(vc-1.0));
		return store;
	}
}
