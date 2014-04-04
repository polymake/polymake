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

import java.awt.Color;

import de.jreality.math.Rn;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.util.ColorGradient;
import de.jreality.util.LoggingSystem;
import de.jreality.util.Rectangle3D;

/**
 * Static methods for generating approximations to spheres. The approximations are based
 * either on subdividing a cube, or subdividing an icosahedron. These methods are used for
 * example in the package {@link de.jreality.jogl} for quick rendering of
 * spherical representations of points.
 * <p>
 * Warning: the approximation methods here have an upper limit on the fineness of the approximation
 * which they are prepared to calculate.  These limits should problably be removed.
 * 
 * @author Charles Gunn
 *
 */
public class SphereUtility {

	protected SphereUtility() {
		super();
	}

	protected static int numberOfTessellatedCubes = 16;
	protected static int numberOfTessellatedIcosahedra = 8;
	protected static IndexedFaceSet[] tessellatedIcosahedra = new IndexedFaceSet[numberOfTessellatedIcosahedra];
	protected static SceneGraphComponent[] tessellatedCubes = new SceneGraphComponent[numberOfTessellatedCubes];
	public static int SPHERE_COARSE=0, SPHERE_FINE=1, SPHERE_FINER=2, SPHERE_FINEST=3, SPHERE_SUPERFINE=4, SPHERE_WAYFINE=5;
	protected static IndexedFaceSet SPHERE_BOUND;
	protected static Rectangle3D sphereBB = null;
	protected static Transformation[] cubeSyms = null;
	protected static IndexedFaceSet[] cubePanels = new IndexedFaceSet[numberOfTessellatedCubes];
	
	// this method can be safely called at any time; it removes the precomputed geometry and SGC's,
	// subsequent calls will reproduce them as needed
	public static void dispose()	{
		   for(int i = 0; i < tessellatedIcosahedra.length; i++){ 
			      tessellatedIcosahedra[i] = null; 
			   } 

		   for(int i = 0; i < tessellatedCubes.length; i++){ 
			      tessellatedCubes[i].setGeometry(null); 
			      tessellatedCubes[i] = null; 
			   } 
			   for(int i = 0; i < cubePanels.length; i++){ 
			      cubePanels[i] = null; 
			   }	

	}
	public static IndexedFaceSet tessellatedIcosahedronSphere(int i)	{
		return tessellatedIcosahedronSphere(i, false);
	}
	
	/**
	 * Return a tessellated icosahedron of order <i>i</i>. That is, the triangular faces of an
	 * icosahedron are binary subdivided <i>i</i> times, the vertices are projected onto
	 * the unit sphere, and the result is returned.  If <i>sharedInstance</i>
	 * is true, then the returned copy is a shared instance which should not be written on.
	 * The resulting polyhedra has 20*(4^i) faces. If <i>i>7</i>, it is clamped to 7 and the
	 * result is returned.
	 * @param i
	 * @param sharedInstance
	 * @return
	 */
	public static IndexedFaceSet tessellatedIcosahedronSphere(int i, boolean sharedInstance)	{
		if (i<0 || i >= numberOfTessellatedIcosahedra) {
			LoggingSystem.getLogger(SphereUtility.class).warning("Invalid index");
			if (i<0) i = 0; 
			else i = numberOfTessellatedIcosahedra-1;
		}
		if (tessellatedIcosahedra[i] == null)	{
			if (i == 0)	{
				tessellatedIcosahedra[i] = Primitives.icosahedron();
			} else {
				tessellatedIcosahedra[i] = IndexedFaceSetUtility.binaryRefine(tessellatedIcosahedronSphere(i-1, true));
				double[][] verts = tessellatedIcosahedra[i].getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
				int vlength = GeometryUtility.getVectorLength(tessellatedIcosahedra[i]);
				Rn.normalize(verts, verts);
				tessellatedIcosahedra[i].setVertexAttributes(Attribute.COORDINATES, StorageModel.DOUBLE_ARRAY.array(vlength).createReadOnly(verts));
			}
			tessellatedIcosahedra[i].setVertexAttributes(Attribute.NORMALS, tessellatedIcosahedra[i].getVertexAttributes(Attribute.COORDINATES)); 
			IndexedFaceSetUtility.calculateAndSetFaceNormals(tessellatedIcosahedra[i]);
			IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(tessellatedIcosahedra[i]);			
		}
		if (sharedInstance) return tessellatedIcosahedra[i];
		// TODO need a method to copy IndexedFaceSets
		IndexedFaceSet ifs = tessellatedIcosahedra[i];
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setFaceCount(ifs.getNumFaces());
		ifsf.setFaceIndices(ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null));
		ifsf.setVertexCount(ifs.getNumPoints());
		ifsf.setVertexCoordinates(ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null));
		ifsf.setVertexNormals(ifs.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null));
		ifsf.setGenerateEdgesFromFaces(true);
		ifsf.setGenerateFaceNormals(true);
		ifsf.update();
		return ifsf.getIndexedFaceSet();
	}
	
	/**
	 * Return a tessellated cube of order <i>i</i>. That is, the square faces of an
	 * cube are evenly subdivided into <i>i<sup>2</sup></i> smaller squares, and the vertices are
	 * projected onto the unit sphere.  If <i>sharedInstance</i>
	 * is true, then the returned copy is a shared instance which should not be written on.
	 * The resulting polyhedra has 6*(i<sup>2</sup>) faces.If <i>i>15</i>, 
	 * it is clamped to 15 and the
	 * result is returned.

	 * @param i
	 * @return
	 */
	public static SceneGraphComponent tessellatedCubeSphere(int i)	{
		return tessellatedCubeSphere(i, false);
	}
	public static SceneGraphComponent tessellatedCubeSphere(int i, boolean sharedInstance)	{
		/*
		 * TODO add a flag to allow a non-shared copy of the geometry
		 */
		if (sharedInstance) {
			if (i<0 || i >= numberOfTessellatedCubes) {
				LoggingSystem.getLogger(SphereUtility.class).warning("Invalid index");
				if (i<0) i = 0; 
				else i = numberOfTessellatedCubes-1;
			}
		}
		if (cubeSyms == null)	{
			cubeSyms = new Transformation[2];
			cubeSyms[0] = new Transformation();
			cubeSyms[1] = new Transformation( new double[] {-1,0,0,0,  0,0,1,0,  0,1,0,0, 0,0,0,1});
		}
		if (sharedInstance && tessellatedCubes[i] != null)	{
			return tessellatedCubes[i];
		}
		IndexedFaceSet hemisphere = oneHalfSphere(2*i+2);//uvPanel(0.0, 0.0,270.0, 90.0, 6*i+4, 2*i+2, 1.0);
		SceneGraphComponent parent = new SceneGraphComponent();
		for (int j = 0; j<2; ++j)	{
			SceneGraphComponent sgc = new SceneGraphComponent();
			sgc.setTransformation(cubeSyms[j]);
			sgc.setGeometry(hemisphere);
			parent.addChild(sgc);
		}
		if (sharedInstance)	{
			cubePanels[i] = hemisphere;//uvPanel(0.0, 0.0,270.0, 90.0, 6*i+4, 2*i+2, 1.0);
			tessellatedCubes[i] = parent;	
		}
		return parent;
	}
	
	/**
	 * Return a standard bounding box for a unit sphere.
	 * @return
	 */
	 public static Rectangle3D getSphereBoundingBox()	{
		if (sphereBB == null)	{
			double[][] bnds = {{-1d,-1d,-1d},{1d,1d,1d}};
			sphereBB = new Rectangle3D();
			sphereBB.setBounds(bnds);	
		}
		return sphereBB;
	}
	

	public static IndexedFaceSet sphericalPatch(double cU, double cV, double uSize, double vSize, int xDetail, int yDetail, double radius)	{
		return sphericalPatchFactory(cU,cV,uSize,vSize,xDetail,yDetail,radius).getIndexedFaceSet();
	}
	/**
	 * Generate a spherical patch. <i>(cU, cV)</i> specify the center of the patch in
	 * spherical angles (longitude, latitude) in radians. 
	 * @param cU
	 * @param cV
	 * @param uSize		wdith of the patch	(longitude)
	 * @param vSize		height of the patch (latitude)
	 * @param n			number of sample points in u
	 * @param m			number of sample points in v
	 * @param r			radius of the sphere
	 * @return
	 */
	public static QuadMeshFactory sphericalPatchFactory( double cU, double cV, double uSize, double vSize, int xDetail, int yDetail, double radius)	{
		double factor = Math.PI/180.0;
		double uH = uSize/2.0; double vH = vSize/2.0;
		//Globe qms = new Globe(n, m, false, false, factor*(cU-uH), factor*(cU+uH), factor*(cV-vH), factor*(cV+vH), r);
		//Globe qms = new Globe(n, m, false, false, 
		double umin = factor*(cU-uH), umax = factor*(cU+uH), vmin = factor*(cV-vH), vmax= factor*(cV+vH);
		QuadMeshFactory qmf = new QuadMeshFactory();
		qmf.setClosedInUDirection(false);
		qmf.setClosedInVDirection(false);
		qmf.setULineCount(xDetail);
		qmf.setVLineCount(yDetail);
		//xDetail, yDetail, false, false);
		double du = umax - umin;
		double dv = vmax - vmin;
		du = du/(xDetail-1.0);
		dv = dv/(yDetail-1.0);
		double[] points = new double[xDetail*yDetail*3];
		double x,y, cu, cv, su, sv;
		int index;
		for (int i = 0; i< yDetail; ++i)	{
			y = vmin + i*dv;
			for (int j = 0; j<xDetail; ++j)	{
				index  = 3*(i*xDetail + j);
				x = umin+j*du;
				cu = Math.cos(x);
				su = Math.sin(x);
				cv = Math.cos(-y);
				sv = Math.sin(-y);
				points[index] = radius * cu * cv;
				points[index+1] = radius * su*cv;
				points[index+2] = radius * sv;
			}
		}
		qmf.setVertexCoordinates(points);
		qmf.setVertexNormals(points);
		qmf.setGenerateEdgesFromFaces(true);
		qmf.setGenerateFaceNormals(true);
		qmf.setGenerateTextureCoordinates(true);
		qmf.update();
		return qmf;
	}
	
	/**
	 * Generate half of a sphere: the half covered by three adjacent faces of a cube!
	 * Two such pieces fit together to cover the sphere! (like a baseball). This is
	 * an optimal rendering solution since it can be expressed as quadmeshes, while the
	 * icosahedral subdivision consists of triangles ... and I can't find a good
	 * decomposition as triangle mesh.
	 * @param n
	 * @return
	 */
	static IndexedFaceSet oneHalfSphere( int n)	{
		AbstractQuadMeshFactory qmf = new AbstractQuadMeshFactory(3*n-2,n,false, false);
		double[][] verts = new double[n*(3*n-2)][3];
		for (int i = 0; i<n; ++i)	{
			double y = 1.0 - 2 * (i/(n-1.0));
			for (int j = 0 ; j<n ; ++j)	{
				double x = -1.0 + 2 * (j/(n-1.0));
				double[] v = {x,y,1.0};
				Rn.normalize(v,v);
				System.arraycopy(v,0,verts[i*(3*n-2)+j], 0, 3);
				double tmp = v[2];v[2] = -v[0];v[0] = tmp;
				System.arraycopy(v,0,verts[i*(3*n-2)+j + (n-1)], 0, 3);
				tmp = v[2];v[2] = -v[0];v[0] = tmp;
				System.arraycopy(v,0,verts[i*(3*n-2)+j + (2*n-2)], 0, 3);
			}
		}
		qmf.setVertexCoordinates(verts);
		qmf.setVertexNormals(verts);
		qmf.setGenerateEdgesFromFaces(true);
		qmf.setGenerateFaceNormals(true);
		qmf.setGenerateTextureCoordinates(true);
		qmf.update();
		return qmf.getIndexedFaceSet();
//		qms.setVertexAttributes(Attribute.COORDINATES, StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(verts));
//		qms.setVertexAttributes(Attribute.NORMALS, qms.getVertexAttributes(Attribute.COORDINATES));
//		GeometryUtility.calculateAndSetFaceNormals(qms);
//		GeometryUtility.calculateAndSetTextureCoordinates(qms);
//		return  qms;
	}

	/**
	 * Calculated the distance from center ( [0,0,0] if center is null ) for each vertex and sets vertex
	 * colors from the given Color Gradient (d_min->0, d_max-_1).
	 * @param ps the PointSet to set vertex colors
	 * @param center optional center
	 * @param cg optional color color gradient
	 * 
	 * TODO: adapt to homogenious coordinates
	 */
	public static void colorizeSphere(PointSet ps, double[] center, ColorGradient cg) {
		if (cg==null) cg=new ColorGradient();
		double[][] colors = ps.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		
		// calculate min/max
		double min=Double.MAX_VALUE, max=0;
		for (int i=0; i<colors.length; i++) {
			if (center != null) Rn.subtract(colors[i], colors[i], center);
			double n = Rn.euclideanNorm(colors[i]);
			if (n<min) min=n;
			if (n>max) max=n;
		}
		
		// calculate colors
		for (int i=0; i<colors.length; i++) {
			double n = Rn.euclideanNorm(colors[i]);
			double cc = (n-min)/(max-min);
			Color c = cg.getColor(cc);
			colors[i][0]=c.getRed()/255.;
			colors[i][1]=c.getGreen()/255.;
			colors[i][2]=c.getBlue()/255.;
		}
		ps.setVertexAttributes(Attribute.COLORS, new DoubleArrayArray.Array(colors));
	}

	public static void assignSphericalUVs(PointSet ps, double[] center) {
		double[][] points = ps.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		double [] tc = new double[2*points.length];
		int i=0;
		for (double[] p : points) {
			if (center != null) Rn.subtract(p, p, center);
			Rn.normalize(p, p);
		    tc[i++] = 0.5+Math.atan2(p[1], p[0])/(Math.PI*2.);
		    tc[i++] = Math.acos(p[2])/Math.PI;
		}
		ps.setVertexAttributes(Attribute.TEXTURE_COORDINATES, new DoubleArrayArray.Inlined(tc, 2));
	}
	
}
