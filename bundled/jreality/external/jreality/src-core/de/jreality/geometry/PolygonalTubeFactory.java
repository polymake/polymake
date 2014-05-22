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

import java.util.logging.Level;

import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.util.LoggingSystem;

/**
 * This subclass of {@link TubeFactory} implements a simple tubing strategy
 * based on fitting together cylindrical pieces around each segment of the underlying curve.
 * The cylindrical segment which tubes the segment joining  vertex <i>v<sub>i</sub></i> to vertex  <i>v<sub>i+1</sub></i> is
 * defined as follows. Let L be the line joining the two points, and let <i>P<sub>i</sub></i> 
 * be the angle bisector of the segments which meet at <i>v<sub>i</sub></i>. Then C is the segment
 * of the infinite cylinder around L cut out by the two planes 
 * <i>P<sub>i</sub></i> and <i>P<sub>i+1</sub></i>.
 * <p>
 * The tube factory attempts to detect that the curve is closed by inspecting the first and 
 * last points for equality (within a standard tolerance).  You can also force the
 * curve to be closed with the {@link de.jreality.geometry.TubeFactory#setClosed(boolean)} method.
 * Then the effect is as if a final point had been appended that is equal to the first one.
 * <p>
 * The cylinder's cross section is set by {@link TubeFactory#setCrossSection(double[][])}. 
 * Other aspects of the resulting polygonal tube are controlled by the other
 * set methods documented in {@link TubeFactory}.
 * <p>
 * The resulting geometry can be gotten after the call to {@link #update()}, via the
 * {@link #getTube()} method.
 * <code><b><pre>
 		IndexedLineSet ils = IndexedLineSetUtility.discreteTorusKnot(1,.25, 2, 9, 250)
		PolygonalTubeFactory ptf = new PolygonalTubeFactory(ils, 0);
		ptf.setClosed(true);
		ptf.setVertexColorsEnabled(true);
		ptf.setRadius(.04);
		ptf.setCrossSection(mysection);
		ptf.setTwists(6);
		ptf.update();
		IndexedFaceSet torus1Tubes = ptf.getTube();
		SceneGraphComponent sgc = new SceneGraphComponent();
		sgc.setGeometry(torus1Tubes); 
 * </pre></b></code>
 * 
 * @author Charles Gunn
 *
 */public class PolygonalTubeFactory extends TubeFactory {
	IndexedFaceSet theTube;
	QuadMeshFactory qmf;
	double[][] theTubeVertices;
	
	public PolygonalTubeFactory(double[][] curve) {
		super(curve);
	}
	
	public PolygonalTubeFactory(IndexedLineSet ils, int whichCurve) {
		this(IndexedLineSetUtility.extractCurve(null, ils, whichCurve));
	}
	

	/**
	 * Generate and return the points of a quad mesh gotten by sweeping the cross  section curve <i>xsec</i>
	 * along the line segments of <i>curve</i> in the manner described above.
	 * 
	 * @param polygon
	 * @param radius
	 * @param xsec
	 * @param type
	 * @param closed
	 * @param metric
	 * @return
	 */
	private double[][] polygon2, vals;
	
//	TODO This method is strange.  It kind of looks like a static method since it has so many parameters
//	but isn't. It's only called once in the workspace, from update().  But it doesn't include all
//	necessary parameters to work as a static method (no frames, for example).  
	protected  double[][] makeTube(double[][] curve, double[] radii, double[][] xsec, FrameFieldType type, boolean closed, int metric, int twists)	{

		int n = curve.length;
		int vl = xsec[0].length;
		// have to handle the situation here that the first and last points are the same but the closed flag isn't set.
		// We assume for now that the user wants to treat this as a closed curve so we have to ignore the last point
		// Here's how we do that
		boolean autoClosed = false;
		double d = Rn.euclideanDistance(curve[0], curve[n-1]);
		autoClosed =  d < 10E-8;
		if (autoClosed)	{
			closed = true;
			n = n-1;
		}
		int realLength = (closed ? n+1 : n)*xsec.length;
		if (vals == null || vals.length != realLength || vals[0].length != vl)
			vals = new double[realLength][vl];

		if (n <= 1) {
			throw new IllegalArgumentException("Can't tube a vertex list of length less than 2");
		}
		double[] radii2 = null;
		boolean hasRadii = radii.length > 1;
		int usedVerts = closed ? n+3 : n+2;
		if (polygon2 == null || polygon2.length != usedVerts)  {
			polygon2 = new double[usedVerts][];
			if (hasRadii) radii2 = new double[usedVerts];
		}
		for (int i = 0; i<n; ++i)	{ 
			polygon2[i+1] = curve[i]; 
			if (hasRadii) radii2[i+1] = radii[i]; 
			}
		if (closed)	{
			polygon2[0] = curve[n-1];
			polygon2[n+1] = curve[0];	
			polygon2[n+2] = curve[1];
			if (hasRadii) {
				radii2[0] = radii[n-1];
				radii2[n+1] = radii[0];
				radii2[n+2] = radii[1];				
			}
		} else {
			polygon2[0] = Rn.add(null, curve[0],  Rn.subtract(null, curve[0], curve[1]));
			polygon2[n+1] = Rn.add(null, curve[n-1], Rn.subtract(null, curve[n-1], curve[n-2]));
			if (hasRadii)	{
				radii2[0] = radii2[1];
				radii2[n+1] = radii2[n];				
			}
		}
//		FrameInfo[] 
		if (userFrames == null) frames = makeFrameField(polygon2, type, metric);
		else frames = userFrames;
		if (frames == null) 
			throw new NullPointerException("No frames!");
//		System.err.println("makeTube: sig = "+metric);
		double[] rad = Rn.identityMatrix(4);
		int nn = frames.length;
		double lastphi = frames[frames.length-1].phi;
		// calculation an angle per joint which will result in "matching" begin and end sections
		double correction = (closed && matchClosedTwist) ? 
				( lastphi > Math.PI ? (2*Math.PI-lastphi) : -lastphi) / nn : 0.0;
		for (int i = 0; i<nn; ++i)	{
			double sangle = Math.sin( frames[i].theta/2.0);
			double factor = 1.0;
			if (sangle != 0) factor = 1.0/sangle;
			double r = hasRadii ? radii2[i+1] : radii[0];
			rad[0] = r *factor;
			rad[5] = r;
			//System.err.println("frame is "+Rn.matrixToString(frames[i].frame));
			frames[i].phi = frames[i].phi + i*correction+ twists*2*Math.PI*frames[i].length;
			double[] zrot = P3.makeRotationMatrixZ(null,frames[i].phi);
			double[] scaledFrame = Rn.times(null, frames[i].frame, Rn.times(null, rad, zrot));
			//LoggingSystem.getLogger().log(Level.FINE,"Theta is "+frames[i].theta);
			int m = xsec.length;
			for (int j = 0; j < m; ++j) {
				int p = j; //m - j - 1;
				Rn.matrixTimesVector(vals[(i) * m + j], scaledFrame, xsec[p]);
			}
		}
		if (closed && matchClosedTwist)	{		// copy the last cross section over as the first
			System.err.println("Closing the tube");
			int m = xsec.length;
			for (int j = 0; j < m; ++j) {
				int p = j; //m - j - 1;
				vals[(nn-1)*m+j] = vals[j];
			}
			
		}
		return vals;
	}
	
	 /**
	  * Update the state of the output tube to reflect the current state of all settable variables.
	  */
	public void update() {
		super.update();
		if (radii == null) {
			radii = new double[1];
		}
		if (radii.length == 1) {
			radii[0] = radius;
		}
		theTubeVertices = makeTube(theCurve, radii, crossSection, frameFieldType, closedCurve, metric, twists);
//		System.err.println("PTF: metric is "+metric);
		qmf = new QuadMeshFactory();
		qmf.setMetric(metric);
//		System.err.println("PTF: sig = "+metric);
		qmf.setGenerateTextureCoordinates(generateTextureCoordinates && !arcLengthTextureCoordinates);
		qmf.setULineCount(crossSection.length);
		qmf.setVLineCount(theTubeVertices.length/crossSection.length);
		boolean closedInU = Rn.euclideanDistance(crossSection[0], crossSection[crossSection.length-1]) < 10E-8;
		qmf.setClosedInUDirection(closedInU);
		qmf.setClosedInVDirection(closedCurve);
		qmf.setVertexCoordinates(theTubeVertices);
		qmf.setGenerateFaceNormals(true);
		qmf.setGenerateVertexNormals(true);
		qmf.setEdgeFromQuadMesh(true);
		qmf.setGenerateEdgesFromFaces(generateEdges);
		qmf.update();
		theTube = qmf.getIndexedFaceSet();
		if (generateTextureCoordinates)	{
			if (!arcLengthTextureCoordinates) qmf.setGenerateTextureCoordinates(true);
			else {
				qmf.setVertexTextureCoordinates(arcLengthTextureCoordinates(theCurve, crossSection, metric));
			}
		}
		if (vertexColors != null || edgeColors != null)	{
		 	int numVerts = theTube.getNumPoints();
		 	int numFaces = theTube.getNumFaces();
		 	int xsLength = crossSection.length;
		 	if (edgeColors != null)	{
		 		int colorLength = edgeColors[0].length;
		 		double[][] faceColors = new double[numFaces][colorLength];
		 		int lim = numFaces/crossSection.length;
		 		for (int j = 0; j<lim; ++j)	{
		 			for (int k = 0; k<xsLength; ++k)	{
		 				for (int m = 0; m<colorLength; ++m)	{
		 					faceColors[j*xsLength+k][m] = edgeColors[j%edgeColors.length][m];
		 				}
		 			}
		 		}
		 		LoggingSystem.getLogger(this).log(Level.FINER,"Setting Face colors");
//		 		theTube.setFaceAttributes(Attribute.COLORS, StorageModel.DOUBLE_ARRAY.array(colorLength).createReadOnly(faceColors));
		 		qmf.setFaceColors(faceColors);
		 	}
		 	if (vertexColorsEnabled && vertexColors != null)	{
		 		int colorLength = vertexColors[0].length;
		 		double[][] vertColors = new double[numVerts][colorLength];
		 		int realNumVerts = numVerts/xsLength;
		 		for (int j = 0; j<realNumVerts; ++j)	{
		 			for (int k = 0; k<xsLength; ++k)	{
		 				for (int m = 0; m<colorLength; ++m)	{
		 					vertColors[j*xsLength+k][m] = vertexColors[j%vertexColors.length][m];
		 				}
		 			}
		 		}
		 		LoggingSystem.getLogger(this).log(Level.FINER,"Setting vertex colors");
//		 		theTube.setVertexAttributes(Attribute.COLORS, StorageModel.DOUBLE_ARRAY.array(colorLength).createReadOnly(vertColors));
		 		qmf.setVertexColors(vertColors);
		 	}
		}
		qmf.update();
		theTube = qmf.getIndexedFaceSet();
	}
	
	private double[][] arcLengthTextureCoordinates(double[][] theCurve, double[][] crossSection, int metric) {
			
			final int vLineCount = theCurve.length;
			final int uLineCount = crossSection.length;
			double[][] textureCoordinates = new double[uLineCount*vLineCount][2];
			int vLength = theCurve[0].length;			// 3 or 4?
			// create a list of v-parameter values parametrized by arc-length
			double[] lengths = new double[vLineCount];
			lengths[0] = 0.0;
			for (int i = 1; i<vLineCount; ++i)	{
				if (vLength == 3)		lengths[i] = lengths[i-1] + Rn.euclideanDistance(theCurve[i], theCurve[i-1]);
				else lengths[i] = lengths[i-1] + Pn.distanceBetween(theCurve[i], theCurve[i-1], metric);
			}
			final double du= 1.0 / (uLineCount - 1);
			
			double curveLength = lengths[vLineCount-1];
			for(int iv=0, firstIndexInULine=0;
			iv < vLineCount;
			iv++,  firstIndexInULine+=uLineCount) {
				double u=0;
				for(int iu=0; iu < uLineCount; iu++, u+=du) {
					final int indexOfUV=firstIndexInULine + iu;
					textureCoordinates[indexOfUV][0] = u;
					textureCoordinates[indexOfUV][1] = lengths[iv] / curveLength;
				}
			}
					
			return textureCoordinates;
		}

	/**
	 * This returns the current state of the tube geometry, as of the last call to {@link #update()}.
	 * @return
	 */
	public IndexedFaceSet getTube()	{
		return theTube;
	}

	@Override
	public void updateFrames() {
		// TODO Auto-generated method stub
		
	}


}
