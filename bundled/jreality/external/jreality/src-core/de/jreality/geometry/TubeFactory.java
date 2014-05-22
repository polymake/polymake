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
import java.util.Vector;
import java.util.logging.Level;

import de.jreality.geometry.TubeUtility.FrameInfo;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.LoggingSystem;

/**
 * This class calculates tubes around curves. A tube of radius <i>r</i>
 * around a curve <i>C</i> can be defined as the locus of points
 * which lie at distant <i>r</i> from the curve (ignoring endpoints).
 * We are interested only in imbedded tubes, that is, tubes which do not intersect themselves.
 * <p>
 * Example: in euclidean space, a tube around a line is an infinite cylinder. And, a tube
 * around a line segment is a finite cylinder.
 * <p>
 * Complications arise in calculating tubes in various ways, some of which 
 * this class attempts to take into account:
 * <ul>
 * <li>The ambient space may be hyperbolic or elliptic/spherical space, not euclidean space.</li>
 * <li>A curve represented by a finite set of points can be thought of as truly a discrete curve,
 * or as an approximation to a hypothetical smooth curve.</li>
 * </ul>
 * <p>
 * If the curve is considered to be discrete, then the tubing problem is not well-posed near the
 * joints of the curve, since there the curvature approaches infinity and any finite tube will intersect 
 * itself. 
 * <p>
 * This  factory provides the basic foundation. It does not provide any geometry output;
 * it is useful mainly on account of the method {@link #makeFrameField(double[][], int, int)}. See its 
 * documentation for a more precise statement of the mathematical implementation.
 * Subclassses which generate actual geometric tubes can take different strategies
 * to the challenges posed above, working with the same basic data provided in this class.
 * <p>
 * The basic data which determine the tube are:
 * <ul>
 * <li>A curve of points in 3-space.</li>
 * <li>Whether the curve is closed, </li>
 * <li>The radius of the tube,</li>
 * <li>A cross-section curve in 3-space. </li>
 * <li>An ambient metric (specified by a metric (see {@link Pn})</li>
 * <li>A list of edge colors (optional), </li>
 * <li>A list of vertex colors (optional), </li>
 * <li>The type of frame family to use (Frenet or parallel), </li>
 * <li>Whether to generate texture coordinates, </li>
 * <li>Whether to parametrize the texture coordinates to constant speed, </li>
  * </ul>
 * <p>
 * These data are set using the instance methods below.
 * <p>
 * Calling {@link #update()} will update the current state of the factory to the
 * values set since the last call to {@link #update()}. This is mostly of interest in the subclasses.
 * <p>
  * TODO adjust the parameter which determines how the profile at the vertices of the curve
 * are "pulled back" towards the mid-segment profiles.
 * @author Charles Gunn
 *
 */
public  class TubeFactory {

		static int debug = 0; //255;
//		static Logger LoggingSystem.getLogger(this) = LoggingSystem.getLogger(TubeFactory.class);
		
		public double[][] theCurve, 
			userTangents = null,
			userBinormals = null,
			vertexColors, 
			edgeColors, 
			crossSection = TubeUtility.octagonalCrossSection;
		public double[] radii = null;
		public double radius = .05;
		public FrameFieldType frameFieldType = FrameFieldType.PARALLEL;
		public int metric = Pn.EUCLIDEAN;
		public int twists = 0;
		public boolean generateTextureCoordinates = false;
		boolean arcLengthTextureCoordinates = false;
		boolean generateEdges = false;
		boolean matchClosedTwist = false;
		public boolean extendAtEnds = false;
		boolean removeDuplicates = false;
		boolean duplicatesRemoved = false;
		public boolean framesDirty = true;
		protected FrameInfo[] frames = null, userFrames = null;
		protected double[] radiiField = null;
		
		public boolean closedCurve = false,
			vertexColorsEnabled = false;
		
		public TubeFactory()	{
			this(null);
		}
		
		public TubeFactory(double[][] curve)	{
			super();
			theCurve = curve;
		}
		
		public  void updateFrames() {
			
		}
		
		public FrameInfo[] getFrameField()	{
			return frames;
		}
		/**
		 * Set whether the curve should be considered a closed loop. Default: false.
		 * @param closedCurve
		 */
		public void setClosed(boolean closedCurve) {
			this.closedCurve = closedCurve;
			framesDirty = true;
		}

		/**
		 * A 2D curve in the (x,y) plane (with 3D coordinates!) to be used as the 
		 * cross section of the tube. For flexibility: unless the first and last points are equal, the
		 * curve is considered to be open and the resulting tube will not close up.
		 * Default: an octagon lying on the unit circle.
		 * @param crossSection
		 */
		public void setCrossSection(double[][] crossSection) {
			this.crossSection = crossSection;
		}

		public double[][] getTangents() {
			return userTangents;
		}

		public void setTangents(double[][] tangents) {
			this.userTangents = tangents;
		}

		public double[][] getUserBinormals() {
			return userBinormals;
		}

		public void setUserBinormals(double[][] userBinormals) {
			this.userBinormals = userBinormals;
		}

		public void setFrameField(FrameInfo[] frames)	{
			userFrames = frames;
		}
		
		/**
		 * Should the underlying frame field be generated by {@link TubeUtility#PARALLEL} or
		 * {@link TubeUtility#FRENET} displacement?  Default: {@link TubeUtility#PARALLEL}
		 * @param frameFieldType
		 */public void setFrameFieldType(FrameFieldType frameFieldType) {
			this.frameFieldType = frameFieldType;
		}

		/**
		 * Set the radius of the tube.  To be exact, this is applied as a scale factor to the
		 * cross section before it is swept out to make the tube.
		 * @param radius
		 */
		 public void setRadius(double radius) {
			this.radius = radius;
		}

		 public void setRadii( double[] radii)	{
			 this.radii = radii;
		 }
		/**
		 * Set the metric of the ambient space. See {@link Pn}.
		 * @param metric
		 */
		 public void setMetric(int metric) {
			this.metric = metric;
			framesDirty = true;
		}

		/**
		 * Set an integer number of twists to apply to the cross section as it is
		 * swept along the curve.  This twisting is done proportional to the arc length of
		 * the curve. For creating exotic shapes. Default: 0
		 * @param twists
		 */
		 public void setTwists(int twists) {
			this.twists = twists;
		}
		
		/**
		 * Whether to apply vertex colors to the output tube. See {@link #setVertexColors(double[][])}.
		 * @param vertexColorsEnabled
		 */
		 public void setVertexColorsEnabled(boolean vertexColorsEnabled) {
			this.vertexColorsEnabled = vertexColorsEnabled;
		}

		/**
		 * Apply colors to faces of output tube, one for each tube segment. 
		 * @param edgeColors	double[n][]	  where n is number of segments in curve
		 */
		public void setEdgeColors(double[][] edgeColors) {
			this.edgeColors = edgeColors;
		}

		/**
		 * Apply colors to vertices of output tube, one for each tube cross section.
		 * @param vertexColors double[n][]	  where n is number of vertices in curve
		 */
		public void setVertexColors(double[][] vertexColors) {
			this.vertexColors = vertexColors;
		}


		public boolean isGenerateTextureCoordinates() {
			return generateTextureCoordinates;
		}

		/**
		 * Whether the output geometry should have automatic texture coordinates. If true,
		 * texture coordinates will be based on
		 * the tubing parameters: u is the cross section parameter, v is the length of the tube.
		 * 
		 * @return
		 * @see #setArcLengthTextureCoordinates(boolean)
		 */
		public void setGenerateTextureCoordinates(boolean generateTextureCoordinates) {
			this.generateTextureCoordinates = generateTextureCoordinates;
		}

		public boolean isArcLengthTextureCoordinates() {
			return arcLengthTextureCoordinates;
		}

		/**
		 * If true, force the generated v- texture coordinates to run from 0 to 1 proportional
		 * to the (discrete) arc length of the curve.
		 * @param arcLengthTextureCoordinates
		 */
		public void setArcLengthTextureCoordinates(boolean arcLengthTextureCoordinates) {
			this.arcLengthTextureCoordinates = arcLengthTextureCoordinates;
		}
			
		public boolean isExtendAtEnds() {
			return extendAtEnds;
		}

		public void setExtendAtEnds(boolean extendAtEnds) {
			this.extendAtEnds = extendAtEnds;
			framesDirty = true;
		}

		public boolean isRemoveDuplicates() {
			return removeDuplicates;
		}

		public void setRemoveDuplicates(boolean removeDuplicates) {
			this.removeDuplicates = removeDuplicates;
		}

		public boolean isGenerateEdges() {
			return generateEdges;
		}

		public void setGenerateEdges(boolean generateEdges) {
			this.generateEdges = generateEdges;
		}

		public boolean isMatchClosedTwist() {
			return matchClosedTwist;
		}

		public void setMatchClosedTwist(boolean matchClosedTwist) {
			this.matchClosedTwist = matchClosedTwist;
		}

		public SceneGraphComponent getFramesSceneGraphRepresentation()	{
			return TubeFactory.getSceneGraphRepresentation(frames);
		}
		
		public  void update()		{
			if (removeDuplicates && !duplicatesRemoved)	{
				theCurve = removeDuplicates(theCurve);
				duplicatesRemoved = true;
			}
//			System.err.println("tube style = "+frameFieldType);
		}
		
		/**
		 * A hack to deal with a situation where vertices were repeated.
		 * @param cc
		 * @return
		 */
		protected static double[][] removeDuplicates(double[][] cc)	{
			int n = cc.length;
			Vector<double[]> v = new Vector<double[]>();
			double[] currentPoint = cc[0], nextPoint;
			v.add(currentPoint);
			int i = 1;
			double d;
			do {
				do {
					nextPoint = cc[i];
					d = Rn.euclideanDistance(currentPoint, nextPoint);
					i++;
				} while (d < 10E-16 && i < n);
				if (i==n) break;
				currentPoint = nextPoint;
				v.add(currentPoint);
			} while(i<n);
			double[][] newcurve = new double[v.size()][];
			v.toArray(newcurve);
			return newcurve;
		}
		static double[] px1 = {0,0,-.5,1};
		static double[] px2 = {0,0,.5,1};
		private double[][] tangentField;
		private double[][] frenetNormalField;
		private double[][] parallelNormalField;
		private double[][] binormalField;
		private FrameInfo[] frameInfo;
		/**
		 * The primary method in the tube-generating process.  
		 * <p>
		 * This is an instance method, since there is quite a bit of state which is
		 * expensive to compute and so can be saved in the instance.
		 * <p>
		 * The code is complicated by dealing with euclidean, hyperbolic, and elliptic cases simultaneously and
		 * But at the same time the code is a interesting example of how euclidean objects
		 * can be generalized to non-euclidean setting.
		 *  <p>
		 * Explanation of the algorithm:
		 *     Assume that <i> polygon </i> is an array of length <i>n</i> 
		 * <p>
		 * The input curve <i>polygon</i> is assumed to have an initial and terminal point with the following properties:
		 *     1) if the curve is closed, <i>polygon[0]=polygon[n-2]</i> and <i>polygon[n-1]=polygon[1]</i>
		 *     2) if the curve is not closed, then polygon[0] is the mirror of polygon[2] wrt polygon[1], similarly for polygon[n-1]. 
		 *  The tubing factories perform this setup automatically before calling this method, 
		 *  but if you want to use this method directly, you'll
		 *  need to do this setup yourself.
		 * <p>
		 * The returned frame array (of type {@link TubeUtility.FrameInfo} is of length <i>n-2</i>.
		 * <p>
		 * The input curve can consist of either 3- or 4-d vectors; in the latter case they are assumed 
		 * to be homogeneous projective coordinates.
		 * <p>
		 * The basic idea is: first calculate the Frenet frame for the curve. That is, in the best case,
		 * a simple matter at each point <i>P[i]</i> of the curve:
		 * <ul>
		 * <li>
		 * 	 Calculate the polar ("tangent") plane at P. </li>
		 * <ul>
		 * <li> In the euclidean case this is the plane at infinity (w=0), i.e., tangent vectors are distinguished by 
		 * having fourth coordinate 0. </li>
		 * <li> In non-euclidean case, the polar plane is uniquely defined by P. </li>
		 * <li> All vectors in the frame for P belong to this tangent plane.</li> 
		 * </ul>
		 * <li>Calculate the osculating plane for the curve at P.  This is the plane spanned by 
		 * P<sub>i-1</sub, P<sub>i</sub>, P<sub>i+1</sub>. 
		 * When polarized, this gives the binormal vector</li>
		 * <ul>
		 * <li>In the euclidean case, polarizing a plane means "set the 4th coordinate to 0".  
		 * This yields the binormal vector. </li>
		 * <li>In the non-euclidean case, with full symmetric duality,
		 * one also gets the binormal vector.</li>
		 * </ul>
		 * <li>Calculate the tangent vector at P<sub>i</sub>. (Geometrically the tangent is not defined since 
		 *  the curve is not differentiable here. What follows is a reasonable guess.)</li>
		 * <ul>
		 * <li>First calculate the angle bisector or mid-plane of the curve at P<sub>i</sub>: that is 
		 * the plane spanned by the directions at P<sub>i</sub> which lie at an equal
		 * angle to the two curve segments meeting there.</li>
		 * <li>Define the tangent direction at P<sub>i</sub>< to be the direction
		 *  orthogonal to the angle bisector </li>
		 *  <li> The tangent vector is then the polar point of the mid-plane.</li>
		 * </ul>
		 * <li>Calculate the normal vector.
		 * <ul>
		 * <li>Calculate the plane spanned by P<sub>i</sub>, the binormal, and the normal vector.</li>
		 * <li>The normal vector is the polar point of this plane.</li>
		 * </ul>
		 * </ul>
		 * This algorithm calculates the Frenet frame. To get the parallel frame, first
		 * calculate the Frenet frame. Then the 
		 * normal vector N has to be parallel transported along the curve:</li>
		 * <ul>
		 * <li>At the first point P<sub>0</sub></i>, 
		 * define the parallel field to be the same as the Frenet field.</li>
		 * <li>At each further point P<sub>i</sub>, calculate the plane spanned by P<sub>i-1</sub>,
		 * P<sub>i</sub>, and the previous parallel normal vector. </li>
		 * <li>Calculate the point of intersection of this plane, the polar plane (tangent plane) of P<sub>i</sub>,
		 * and the mid-plane of P<sub>i</sub> (both the latter two have already been calculated above).</li>
		 * <li>The result is the desired parallel normal vector N.</li>
		 * <li>The rest of the frame at P<sub>i</sub> is gotten by rotating the Frenet frame so
		 * that the tangent vector is fixed and the Frenet normal rotates into the parallel normal.</i>
		 * </ul>
		 *  At points where three or more consecutive curve vertices are collinear, \
		 *  some of the steps of the above algorithm aren't  well-defined.  In that case, the
		 *  correct behavior is basically to tube as if these points had been deleted before
		 *  the processing began.
		 *  <p>
		 * @param polygon	the curve to frame (as array of 3- or 4-d points)
		 * @param type		{@link TubeUtility.PARALLEL} or {TubeUtility.FRENET}
		 * @param metric	the metric metric {@link Pn}
		 * @return	an array of length (n-2) of type {@link TubeUtility.FrameInfo} containing an orthonormal frame for each internal point in the initial polygon array.
		 */
		public  FrameInfo[] makeFrameField(double[][] polygon, FrameFieldType type, int metric)		{
			if (frames!= null && !framesDirty) return frames;
		 	int numberJoints = polygon.length;
		 	double[][] polygonh;
		 	// to simplify life, convert all points to homogeneous coordinates
		 	if (polygon[0].length == 3) {
		 		polygonh = Pn.homogenize(null, polygon);
		 		Pn.normalize(polygonh, polygonh, metric);
		 	}
			else if (polygon[0].length == 4)	
				polygonh = Pn.normalize(null, polygon, metric);
			else {
				throw new IllegalArgumentException("Points must have dimension 4");
			}
		 	if ((debug & 1) != 0)	
		 		LoggingSystem.getLogger(this).log(Level.FINER,"Generating frame field for metric "+metric);
		 	if (tangentField == null || tangentField.length != (numberJoints-2))	{
				tangentField = new double[numberJoints-2][4];
				frenetNormalField = new double[numberJoints-2][4];
				parallelNormalField = new double[numberJoints-2][4];
				binormalField = new double[numberJoints-2][4];
		 	}
			frameInfo = new FrameInfo[numberJoints-2];		 		
			double[] d  = new double[numberJoints-2];			// distances between adjacent points
			if ((debug & 32) != 0)	{
				for (int i = 0; i<numberJoints; ++i)	{
					LoggingSystem.getLogger(this).log(Level.FINER,"Vertex "+i+" : "+Rn.toString(polygonh[i]));
				}
			}
			double[] frame = new double[16];
			double totalLength = 0.0;
			for (int i = 1; i<numberJoints-1; ++i)	{
				d[i-1] = (totalLength += Pn.distanceBetween(polygonh[i-1], polygonh[i], metric));
			}
			totalLength = 1.0/totalLength;
			// Normalize the distances between points to have total sum 1.
			for (int i = 1; i<numberJoints-1; ++i)	d[i-1] *= totalLength;

			for (int i = 1; i<numberJoints-1; ++i)	{
				
				/*
				 * calculate the binormal from the osculating plane
				 */
				double theta = 0.0, phi=0.0;
				boolean collinear = false;
				double[] polarPlane = Pn.polarizePoint(null, polygonh[i], metric);
				if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINER,"Polar plane is: "+Rn.toString(polarPlane));					
				
				double[] osculatingPlane = P3.planeFromPoints(null, polygonh[i-1], polygonh[i], polygonh[i+1]);
				double size = Rn.euclideanNormSquared(osculatingPlane);
				if (size < 10E-16)	{			// collinear points!
					collinear = true;
					if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINER,"degenerate binormal");
					if (i == 1)		binormalField[i-1] = getInitialBinormal(polygonh, metric);
					else Pn.projectToTangentSpace(binormalField[i-1], polygonh[i], binormalField[i-2], metric);
				} else
					Pn.polarizePlane(binormalField[i-1], osculatingPlane,metric);
				if (userBinormals != null) 
					System.arraycopy(userBinormals[i-1], 0, binormalField[i-1], 0, userBinormals[i-1].length);
				Pn.setToLength(binormalField[i-1], binormalField[i-1], 1.0, metric);
				if (i>1 && metric == Pn.ELLIPTIC)	{
					double foo = Pn.angleBetween(binormalField[i-2], binormalField[i-1], metric);
					if (Math.abs(foo) > Math.PI/2)  Rn.times(binormalField[i-1], -1, binormalField[i-1]);
				}
				if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINER,"Binormal is "+Rn.toString(binormalField[i-1]));
//				System.err.println("Binormal field = "+Rn.toString(binormalField[i-1]));

				/*
				 * Next try to calculate the tangent as a "mid-plane" if the three points are not collinear
				 */
				double[] midPlane = null, plane1 = null, plane2 = null;
				if (!collinear)	{
					plane1 = P3.planeFromPoints(null, binormalField[i-1], polygonh[i], polygonh[i-1]);
					plane2 = P3.planeFromPoints(null, binormalField[i-1], polygonh[i], polygonh[i+1]);
					midPlane = Pn.midPlane(null, plane1, plane2, metric);
					size = Rn.euclideanNormSquared(midPlane);
					if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINER,"tangent norm squared is "+size);					
					theta = Pn.angleBetween(plane1, plane2, metric);
				}
				/*
				 * if this is degenerate, then the curve must be collinear at this node
				 * get the tangent by projecting the line into the tangent space at this point
				 */ 
				if (collinear || size < 10E-16)	{
					// the three points must be collinear
					if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINER,"degenerate Tangent vector");
					// TODO figure out why much breaks 
					// if the two vertices in the following call are swapped
					double[] pseudoT = P3.lineIntersectPlane(null, polygonh[i-1], polygonh[i+1], polarPlane);	
					if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINE,"pseudo-Tangent vector is "+Rn.toString(pseudoT));
					// more euclidean/noneuclidean trouble
					// we want the plane equation of the midplane 
					if (metric != Pn.EUCLIDEAN)	{
						midPlane = Pn.polarizePoint(null, pseudoT, metric);
					} else {
						// TODO figure out why the vector (the output of lineIntersectPlane)
						// has to be flipped in this case but not in the non-euclidean case
						//midPlane = Rn.times(null, -1.0, pseudoT);
						midPlane = pseudoT;
						// the euclidean polar of a point is the plane at infinity: we want something
						// much more specific: 
						// we assume the polygonal data is dehomogenized (last coord = 1)
						midPlane[3] = -Rn.innerProduct(midPlane, polygonh[i], 3);						
					}	
					// TODO detect case where the angle is 0, also
					theta = Math.PI;
				}
				//System.err.println("calc'ed midplane is "+Rn.toString(midPlane));
				if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINE,"Midplane is "+Rn.toString(midPlane));
					Pn.polarizePlane(tangentField[i-1], midPlane, metric);						
				if (userTangents == null){
				} else {
					System.arraycopy(userTangents[i-1], 0, tangentField[i-1], 0, userTangents[i-1].length);
					midPlane = Rn.planeParallelToPassingThrough(null, userTangents[i-1], polygonh[i]);
					//System.err.println("given midplane is "+Rn.toString(midPlane));
					if (i>1) theta = Pn.angleBetween(userTangents[i-2], userTangents[i-1], metric);
					else theta = 0;
				}
				// This is a hack to try to choose the correct version of the tangent vector:
				// since we're in projective space, t and -t are equivalent but only one
				// "points" in the correct direction.  Deserves further study!
				double[] diff = Rn.subtract(null, polygonh[i], polygonh[i-1]);
				if (Rn.innerProduct(diff, tangentField[i-1]) < 0.0)  
					Rn.times(tangentField[i-1], -1.0, tangentField[i-1]);

				Pn.setToLength(tangentField[i-1], tangentField[i-1], 1.0, metric);
//				System.err.println("Tangent field = "+Rn.toString(tangentField[i-1]));
				//System.err.println("tangent is "+Rn.toString(tangentField[i-1]));
				// finally calculate the normal vector
				Pn.polarizePlane(frenetNormalField[i-1], P3.planeFromPoints(null,binormalField[i-1], tangentField[i-1],  polygonh[i]),metric);					
				Pn.setToLength(frenetNormalField[i-1], frenetNormalField[i-1], 1.0, metric);
				if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINE,"frenet normal is "+Rn.toString(frenetNormalField[i-1]));
				if (type == FrameFieldType.PARALLEL)	{
					// get started 
					if (i == 1)		System.arraycopy(frenetNormalField[0], 0, parallelNormalField[0], 0, 4);		
					else 	{
						double[] nPlane = P3.planeFromPoints(null, polygonh[i], polygonh[i-1], parallelNormalField[i-2]);
						double[] projectedN = P3.pointFromPlanes(null, nPlane, midPlane, polarPlane );
						if (Rn.euclideanNormSquared(projectedN) < 10E-16)	{
							LoggingSystem.getLogger(this).log(Level.FINE,"degenerate normal");
							projectedN = parallelNormalField[i-2];		// try something!
						}
						parallelNormalField[i-1] = Pn.normalizePlane(null, projectedN, metric);
						if ((debug & 128) != 0)	LoggingSystem.getLogger(this).log(Level.FINE,"Parallel normal is "+Rn.toString(parallelNormalField[i-1]));
					
					}
//					if (size < 10E-16)	{
//						if ((debug & 2) != 0) LoggingSystem.getLogger(this).log(Level.FINE,"degenerate parallel normal");
//						if (i > 1) parallelNormalField[i-1] = parallelNormalField[i-2];
//					}
					if (parallelNormalField[i-1] == null)	{
						parallelNormalField[i-1] = parallelNormalField[i-2];
//						throw new IllegalStateException("Null vector");
					} else 
						Pn.setToLength(parallelNormalField[i-1], parallelNormalField[i-1], 1.0, metric);
					phi = Pn.angleBetween(frenetNormalField[i-1],parallelNormalField[i-1],metric);
					if (metric == Pn.ELLIPTIC)	{
						if (phi > Math.PI/2) phi = phi - Math.PI;
						else if (phi < -Math.PI/2) phi = phi + Math.PI;
					}
					double a = Pn.angleBetween(parallelNormalField[i-1],binormalField[i-1],metric);
					if (a > Math.PI/2) phi = -phi;
				} 
//				size = Rn.euclideanNormSquared(pN[i-1]);
				else phi = 0.0;
				
				System.arraycopy(frenetNormalField[i-1], 0, frame, 0, 4);
				System.arraycopy(binormalField[i-1], 0, frame, 4, 4);
				System.arraycopy(tangentField[i-1], 0, frame, 8, 4);
				System.arraycopy(polygonh[i], 0, frame, 12, 4);
				   	
				if ((debug & 4) != 0) LoggingSystem.getLogger(this).log(Level.FINE,"determinant is:\n"+Rn.determinant(frame));
				frameInfo[i-1] = new FrameInfo(Rn.transpose(null, frame),d[i-1],theta, phi);
				if ((debug & 16) != 0) LoggingSystem.getLogger(this).log(Level.FINE,"Frame "+(i-1)+": "+frameInfo[i-1].toString());
			}
			framesDirty = false;
			return frameInfo;
		 }

		static double[] B = new double[] {Math.random(), Math.random(), Math.random(), 1.0};
		/**
		 * 
		 * @param polygon
		 * @param metric
		 * @return
		 */
		 protected static double[] getInitialBinormal(double[][] polygon, int metric)	{
			int n = polygon.length;
			for (int i = 1; i<n-1; ++i)	{
				double[] bloop = 
				Pn.polarize(null, P3.planeFromPoints(null, polygon[i-1], polygon[i], polygon[i+1]),metric);	
				if (Rn.euclideanNormSquared(bloop) > 10E-16) {
					Pn.dehomogenize(bloop,bloop);
//					boolean flip = false;
//					if (bloop[0] <0) flip = true;
//					else if (bloop[0] == 0.0 && bloop[1] < 0) flip = true;
//					else if (bloop[1] == 0.0 && bloop[2] < 0) flip = true;
//					if (flip) for (int j = 0; j<3; ++j) bloop[j] = -bloop[j];
					return bloop;
				}
			}
			// all points are collinear, choose a random plane through the first two points and polarize it to get a point
			return Pn.polarizePlane(null, P3.planeFromPoints(null, B, polygon[1], polygon[2]),metric);
		}

		 static double[][] axes = {{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
		 static int[][] axesIndices = {{0,1},{0,2},{0,3}};
		 static Color[] axesColors = {Color.red, Color.green, Color.blue};

		public static SceneGraphComponent getSceneGraphRepresentation(FrameInfo[] frames)	{
			return getSceneGraphRepresentation(frames, .02);
		}

		public static SceneGraphComponent getSceneGraphRepresentation(FrameInfo[] frames, double scale)	{
			SceneGraphComponent result = new SceneGraphComponent();
			IndexedLineSet ils;
			SceneGraphComponent geometry = getXYZAxes();
			MatrixBuilder.euclidean().scale(scale).assignTo(geometry);
			double[][] verts = new double[frames.length][];
			int i = 0;
			for (FrameInfo f : frames)	{
				SceneGraphComponent foo = new SceneGraphComponent();
				double[] scaledFrame = Rn.times(null, f.frame, 	P3.makeRotationMatrixZ(null, f.phi));
				Transformation t = new Transformation(scaledFrame);
				foo.setTransformation(t);
				foo.addChild(geometry);
				result.addChild(foo);
				verts[i++] = (new Matrix(f.frame)).getColumn(3);
			}
			SceneGraphComponent sgc = new SceneGraphComponent();
			Appearance ap = new Appearance();
			ap.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBE_RADIUS, .005);
			ap.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBES_DRAW, false);
			ap.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, new Color(100, 200, 200));
			ap.setAttribute(CommonAttributes.LINE_SHADER+"."+"polygonShader.diffuseColor", new Color(100, 200, 200));
			sgc.setAppearance(ap);
			ils = IndexedLineSetUtility.createCurveFromPoints(verts, false);
			sgc.setGeometry(ils);
			result.addChild(sgc);
			return result;
		}

		public static SceneGraphComponent getXYZAxes() {
			IndexedLineSetFactory ilsf = new IndexedLineSetFactory();
			ilsf.setVertexCount(4);
			ilsf.setVertexCoordinates(axes);
			ilsf.setEdgeCount(3);
			ilsf.setEdgeIndices(axesIndices);
			ilsf.setEdgeColors(axesColors);
			ilsf.update();
			IndexedLineSet ils = ilsf.getIndexedLineSet();
			BallAndStickFactory basf = new BallAndStickFactory(ils);
			basf.setShowArrows(true);
			basf.setArrowPosition(1.0);
			basf.setStickRadius(.05);
			basf.setArrowScale(.15);
			basf.setArrowSlope(2.0);
			basf.setShowBalls(false);
			basf.setShowSticks(true);
			basf.update();
			SceneGraphComponent geometry = basf.getSceneGraphComponent();
			return geometry;
		}
}
