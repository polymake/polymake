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
import java.awt.Font;
import java.util.HashMap;

import javax.swing.SwingConstants;

import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Quaternion;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.shader.CommonAttributes;


/**
* Factory for creating a representation of a coordinate system in Euclidean space.
* <p>
* This  is
* created for an existing SceneGraphComponent (or for a given extent).<br> 
* For a given SceneGraphComponent the factory creates a bounding box
* and axes through the origin including ticks with their labels.<br>
* A new SceneGraphNode containing the coordinate system is added to 
* the children of the given SceneGraphComponent.
* <p>
* Use the factory as following:<br>
* <code><b><pre>
* SceneGraphCompontent component;
* double axisScale = 10.0;
* [...]
* CoordinateSystemFactory factory = new CoordinateSystemFactory(component, axisScale);</pre></b></code>
* Above line creates the coordinate system using default properties and adds it to the scene.<br>
* <code>axisScale</code> (i.e. the distance between two ticks on the coordinate axes) depends on size of component.<br>
* You can change properties afterwards:
* <code><b><pre>
* factory.showAxes(false);
* factory.showBox(true);
* factory.beautify(true);
* [setting more properties]
* </pre></b></code>
* <p>
* The coordinate system may be removed from the SceneGraph by<br>
* <code><b><pre>component.removeChild(factory.getCoordinateSystem());</pre></b></code>
* 
* 
* @author msommer
*/

public class CoordinateSystemFactory {


	private double[] boxMin, boxMax;
	
	/** Index of coordinate x (use in arrays). */
	public final static int X = 0;
	/** Index of coordinate y (use in arrays). */
	public final static int Y = 1;
	/** Index of coordinate z (use in arrays). */
	public final static int Z = 2;
	
	private double[][][] axesVertices, boxVertices;
	private SceneGraphComponent coordinateSystem, component;
	
	private final String[] axesNames = {"x", "y", "z"};
	
	private double[][] octagonalCrossSection = {
			{1,0,0}, 
			{.707, .707, 0}, 
			{0,1,0},
			{-.707, .707, 0},
			{-1,0,0},
			{-.707, -.707, 0},
			{0,-1,0},
			{.707, -.707, 0},
			{1,0,0}};
	
	private final double arrowHeight = 3;	
	private IndexedFaceSet urCone = null;
	{
		urCone = Primitives.pyramid(octagonalCrossSection(-arrowHeight), new double[]{0,0,0});
		//GeometryUtility.calculateAndSetVertexNormals(urCone);
	}
	
	//private int metric = Pn.EUCLIDEAN;
	
	private int currentClosestBoxVertex = -1;  //index of a currently closest box vertex in boxVertices[0] 
	
	private HashMap<String, SceneGraphComponent> nodes = new HashMap<String, SceneGraphComponent>();  //keep references to SceneGraphNodes
	
	private CoordinateSystemBeautifier beautifier = new CoordinateSystemBeautifier(this);
	
//-------------------------------------------------------------
//DEFAULT VALUES OF PROPERTIES
//-------------------------------------------------------------
	private double axisScale = 1.0;  //the distance between two ticks on an axis
	private double labelScale = 0.0035;  //size of labels
	private double arrowStretch = 16*labelScale; //stretch of arrows of axes (octagonalCrossSection)
	private double tickStretch = 8*labelScale; //stretch of ticks of axes (octagonalCrossSection)
	private boolean showAxes = false;  //show or hide axes
	private boolean showBox = false;  //show or hide box
	private boolean showGrid = true;  //show or hide grid on box
	private boolean showAxesArrows = true;  //show or hide arrows on axes
	private boolean showBoxArrows = false;  //show or hide arrows on box
	private boolean showLabels = true;  //show or hide labels of ticks & axes
	private Color coordinateSystemColor = Color.BLACK;
	private Color gridColor = Color.GRAY;
 private Color boxColor = Color.BLACK;
	private Color labelColor = Color.BLACK;
	private Font labelFont = new Font("Sans Serif", Font.PLAIN, 48);
	private boolean beautify = true;  //for adding tool CoordinateSystemBeautifier

	
	
//-------------------------------------------------------------
//CONSTRUCTORS
//-------------------------------------------------------------
	
	/**
	 * Creates a coordinate system where min and max values of each coordinate
	 * axis are specified by <code>extent</code>, 
	 * i.e. x,y,z are within [-<code>extent</code>, <code>extent</code>].
	 * @param extent extent of each coordinate axis
  * @param axisScale the axis scale, i.e. the distance between two ticks on the coordinate axes
	 */
	public CoordinateSystemFactory(double extent, double axisScale) {
		this(new double[]{extent, extent, extent}, axisScale);
	}
	
 /**
  * see <code>CoordinateSystemFactory(double extent, double axisScale)</code><br>
  * here axisScale is set to 1.0
  */
 public CoordinateSystemFactory(double extent) {
   this(extent, 1.0);
 }

	
	/**
	 * Creates a coordinate system where min and max values of each of the three coordinate
	 * axes is specified by <code>extent</code>, 
	 * i.e. x is within [-<code>extent[0]</code>, <code>extent[0]</code>] etc.
	 * @param extent the extent of each of the three coordinate axes
  * @param axisScale the axis scale, i.e. the distance between two ticks on the coordinate axes
	 */
	public CoordinateSystemFactory(double[] extent, double axisScale) {

		if (extent.length!=3 || extent[X]<=0 || extent[Y]<=0 || extent[Z]<=0 || axisScale<=0)
			throw new IllegalArgumentException("length of extent incorrect or negative values");
		
		boxMin = new double[]{-extent[X], -extent[Y], -extent[Z]};
		boxMax = new double[]{ extent[X],  extent[Y],  extent[Z]};
		this.axisScale = axisScale;

		//create the coordinate system
		coordinateSystem = createCoordinateSystem();
	}

 /**
  * see <code>CoordinateSystemFactory(double[] extent, double axisScale)</code><br>
  * here axisScale is set to 1.0
  */
 public CoordinateSystemFactory(double[] extent) {
   this(extent, 1.0);
 }

	
	/**
	 * Creates a coordinate system  where min and max values of each coordinate
	 * axis are specified by a given SceneGraphComponent.<br>
	 * A new SceneGraphNode containing the coordinate system is added to 
	 * the children of <code>component</code>.
	 * @param component the SceneGraphComponent specifying the extent of the coordinate system
  * @param axisScale the axis scale, i.e. the distance between two ticks on the coordinate axes
	 */
	public CoordinateSystemFactory(SceneGraphComponent component, double axisScale) {
		
		//need to calculate bounding box without transformation of component
		Transformation tmp = component.getTransformation();
		component.setTransformation(new Transformation());
				
		//get boundingbox from componment
		double[][] minMax = BoundingBoxUtility.calculateBoundingBox(component).getBounds();
		this.boxMin = minMax[0];
		this.boxMax = minMax[1];
		
		//enlarge box if graphic is 2d
		for (int axis=X; axis<=Z; axis++) {
			if (boxMin[axis] == boxMax[axis]) {
				boxMin[axis] -= 0.5;
				boxMax[axis] += 0.5;
			}
		}

   this.axisScale = axisScale;
   	this.component = component;
		//create the coordinate system
		coordinateSystem = createCoordinateSystem();
		component.addChild(coordinateSystem);
		
		//set original transformation
		component.setTransformation(tmp);
	}

 /**
  * see <code>CoordinateSystemFactory(SceneGraphComponent component, double axisScale)</code><br>
  * here axisScale is set to 1.0
  */
 public CoordinateSystemFactory(SceneGraphComponent component) {
   this(component, 1.0);
 }
	

	public void dispose()	{
		if (component!= null && component.isDirectAncestor(coordinateSystem))
			component.removeChild(coordinateSystem);
	}
//-------------------------------------------------------------
//PRIVATE METHODS FOR CALCULATING THE SYSTEM
//-------------------------------------------------------------

	
	/**
	 * creates a new SceneGraphNode containing the coordinate system
	 * and sets basic appearance attributes
	 * @return the coordinate system
	 */
	private SceneGraphComponent createCoordinateSystem() {
		
		coordinateSystem = new SceneGraphComponent();
		coordinateSystem.setName("CoordinateSystem");
		coordinateSystem.setOwner(this);
		coordinateSystem.addChild(calculateBox());  //invisible child, use displayBox(boolean)
		coordinateSystem.addChild(calculateAxes()); //invisible child, use displayBox(boolean)
		
		//set appearance of coordinate system node
		Appearance app = new Appearance();
		app.setName("Appearance");
		app.setAttribute(CommonAttributes.EDGE_DRAW, true);
		app.setAttribute(CommonAttributes.SPHERES_DRAW, true);
		app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBES_DRAW, false);
		app.setAttribute(CommonAttributes.VERTEX_DRAW, showLabels);  //label visibility
		app.setAttribute(CommonAttributes.POINT_RADIUS, 0.001);  //don't show label points
		app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, labelColor);
		app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, coordinateSystemColor);
		app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, coordinateSystemColor);
		//app.setAttribute(CommonAttributes.SPECULAR_COLOR, Color.BLACK);
		app.setAttribute(CommonAttributes.DEPTH_FUDGE_FACTOR, 1.0);
		app.setAttribute(CommonAttributes.POINT_SHADER+"."+"font", labelFont);  //label font
		app.setAttribute(CommonAttributes.POINT_SHADER+"."+"scale", labelScale);  //label scale
		app.setAttribute(CommonAttributes.POINT_SHADER+"."+"offset", new double[]{0.04, 0, 0});  //label offset of ticks
		app.setAttribute(CommonAttributes.POINT_SHADER+"."+"alignment", SwingConstants.EAST);
		coordinateSystem.setAppearance(app);

		if (beautify) {
			beautify = false;
			beautify(true); }

		return coordinateSystem;
	}
	
	
	/**
	 * creates a new SceneGraphNode containing the box
	 * @return the box
	 */
	private SceneGraphComponent calculateBox() {
		
		calculateBoxVertices();
		
		//create SceneGraphComponent and add each coordinate axis to its children
		SceneGraphComponent box = new SceneGraphComponent();
		box.setName("Box");
		
		for (int axis=X; axis<=Z; axis++) {  //for each coordinate axis
			
			SceneGraphComponent singleAxis = new SceneGraphComponent();
			singleAxis.setName(axesNames[axis] +"-axis");
			//nodes.put(axesNames[axis] +"Box", singleAxis);  //e.g. xBox
			
			for (int k=0; k<=3; k++) {
				
				//create SceneGraphComponent with children line, arrow, ticks
				SceneGraphComponent singleAxisK = new SceneGraphComponent();
				//assign binary value of k to the name of the SGC
				singleAxisK.setName(toBinaryString(k));

				nodes.put(axesNames[axis]+toBinaryString(k), singleAxisK);  //e.g. x00
				
				//create line with label
				SceneGraphComponent line = getLine(axis, boxVertices[axis][2*k], boxVertices[axis][2*k+1], true);
				nodes.put(axesNames[axis]+toBinaryString(k)+"label", line.getChildComponent(0));  //e.g. x00label
				//create arrow
				SceneGraphComponent arrow = getArrow(axis, boxVertices[axis][2*k], boxVertices[axis][2*k+1]);
				arrow.setVisible(showBoxArrows);
				nodes.put(axesNames[axis]+toBinaryString(k)+"arrow", arrow);  //e.g. x00arrow
				//create ticks with labels
				SceneGraphComponent ticks = getBoxTicks(axis, k, boxVertices[axis][2*k], boxVertices[axis][2*k+1]);
				nodes.put(axesNames[axis]+toBinaryString(k)+"ticks", ticks);  //e.g. x00ticks
				
				singleAxisK.addChild(line);
				singleAxisK.addChild(arrow);
				singleAxisK.addChild(ticks);

				singleAxis.addChild(singleAxisK);
			}
			box.addChild(singleAxis);
		}
		
	    //calculate grid and add to box
	    box.addChild(calculate2DGrid());
	    
	    Appearance app = new Appearance();
     app.setName("boxAppearance");
     app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, boxColor);
     box.setAppearance(app);

	    box.setVisible(showBox);
	    nodes.put("box", box);
	    return box;
	}
	
	
	/**
	 * creates a new SceneGraphNode containing the axes through the origin
	 * @return the axes
	 */
	private SceneGraphComponent calculateAxes() {

		calculateAxesVertices();
		
		//create SceneGraphComponent and add each coordinate axis to its children
		SceneGraphComponent axes = new SceneGraphComponent();
		axes.setName("Axes");
		
		for (int axis=X; axis<=Z; axis++) {  //for each coordinate axis

			//create SceneGraphComponent with children line, arrow, ticks
			SceneGraphComponent singleAxis = new SceneGraphComponent();
			singleAxis.setName(axesNames[axis] +"-axis");
			nodes.put(axesNames[axis] +"Axis", singleAxis);  //e.g. xAxis
			
			//create line with label
			SceneGraphComponent line = getLine(axis, axesVertices[axis][0], axesVertices[axis][1], false);
			// create arrow
			SceneGraphComponent arrow = getArrow(axis, axesVertices[axis][0], axesVertices[axis][1]);
			arrow.setVisible(showAxesArrows);
			nodes.put(axesNames[axis]+"Arrow", arrow);  //e.g. xArrow
			// create ticks with labels
			SceneGraphComponent ticks = getAxesTicks(axis, axesVertices[axis][0], axesVertices[axis][1]);
			nodes.put(axesNames[axis]+"Ticks", ticks);  //e.g. xTicks
			
			singleAxis.addChild(line);
			singleAxis.addChild(arrow);
			singleAxis.addChild(ticks);
			
			axes.addChild(singleAxis);
		}
		
	    //Appearance app = new Appearance();
	    //app.setName("axesAppearance");
	    //box.setAppearance(app);
	    axes.setVisible(showAxes);
	    
	    nodes.put("axes", axes);
	    return axes;
	}
	

	/**
	 * calculate the vertices of the axes (specified by the choice of the constructor)<br>
	 * first index specifies the coordinate axis (0,1,2)<br>
	 * second index specifies starting and endpoint of each coordinate axis {start, end}
	 */
	private void calculateAxesVertices(){
		
		this.axesVertices = new double[][][] {
			{{Math.min(boxMin[X]-0.5, 0),0,0},
			 {Math.max(boxMax[X]+0.5, 0),0,0}},
			{{0,Math.min(boxMin[Y]-0.5, 0),0},
			 {0,Math.max(boxMax[Y]+0.5, 0),0}},
			{{0,0,Math.min(boxMin[Z]-0.5, 0)},
			 {0,0,Math.max(boxMax[Z]+0.5, 0)}}
		};
	}
	
		
	/**
	 * calculate the vertices of the bounding box (specified by the choice of the constructor)<br>
	 * first index specifies the coordinate axis (0,1,2)<br>
	 * second index specifies starting and endpoint for each of the 4 copies of each coordinate axis {start1, end1, start2, end2,...}
	 */
	private void calculateBoxVertices(){
		
		this.boxVertices = new double[][][] {
			{boxMin, {boxMax[X], boxMin[Y], boxMin[Z]},  //04
			 {boxMin[X], boxMin[Y], boxMax[Z]}, {boxMax[X], boxMin[Y], boxMax[Z]},  //37
			 {boxMin[X], boxMax[Y], boxMin[Z]}, {boxMax[X], boxMax[Y], boxMin[Z]},  //15
			 {boxMin[X], boxMax[Y], boxMax[Z]}, boxMax  //26
			},
			{boxMin, {boxMin[X], boxMax[Y], boxMin[Z]},  //01
			 {boxMin[X], boxMin[Y], boxMax[Z]}, {boxMin[X], boxMax[Y], boxMax[Z]},  //32
			 {boxMax[X], boxMin[Y], boxMin[Z]}, {boxMax[X], boxMax[Y], boxMin[Z]},  //45
			 {boxMax[X], boxMin[Y], boxMax[Z]}, boxMax  //76
			},
			{boxMin, {boxMin[X], boxMin[Y], boxMax[Z]},  //03
			 {boxMin[X], boxMax[Y], boxMin[Z]}, {boxMin[X], boxMax[Y], boxMax[Z]},  //12
			 {boxMax[X], boxMin[Y], boxMin[Z]}, {boxMax[X], boxMin[Y], boxMax[Z]},  //47
			 {boxMax[X], boxMax[Y], boxMin[Z]}, boxMax  //56			 
			}
		}; 
		//note that the ordering of the copies of each coordinate axis is significant

		//  0    boxMin
		//	1	{boxMin[X], boxMax[Y], boxMin[Z]}
		//	2	{boxMin[X], boxMax[Y], boxMax[Z]}    1   ---   5     y
		//	3	{boxMin[X], boxMin[Y], boxMax[Z]}  2   ---   6       |_ x
		//	4	{boxMax[X], boxMin[Y], boxMin[Z]}    0   - -   4     /
		//	5	{boxMax[X], boxMax[Y], boxMin[Z]}  3   ---   7      z
		//	6	 boxMax
		//	7	{boxMax[X], boxMin[Y], boxMax[Z]}
	}	
	

	/**
	 * get the line for the coordinate axis specified by <code>axis</code> 
	 * between min and max as a SceneGraphComponent (IndexedLineSet)<br>
	 * (the line through min and max has to be parallel to the coordinate axis)
	 * @param axis the coordinate axis (X, Y, Z)
	 * @param min the starting point of the line
	 * @param max the endpoint of the line
	 * @param forBox is the line intended for a box (line is labelled differently then)
	 * @return the line
	 */
	private SceneGraphComponent getLine(int axis, double[] min, double[] max, boolean forBox) {
		//line through min and max has to be parallel to the coordinate axis specified by axis 
	
		IndexedLineSetFactory lineLSF = new IndexedLineSetFactory();
		lineLSF.setVertexCount(2);
		lineLSF.setEdgeCount(1);
		lineLSF.setVertexCoordinates(new double[][]{min, max});
		lineLSF.setEdgeIndices(new int[]{0,1});
		lineLSF.update();
		
		//create line label
		PointSetFactory labelPSF = new PointSetFactory();
		labelPSF.setVertexCount(1);
		if (forBox) {
			double[] p = (double[])max.clone();
			p[axis] = min[axis]+(max[axis]-min[axis])/2;
			labelPSF.setVertexCoordinates(p);
		}
		else labelPSF.setVertexCoordinates(max);

		labelPSF.setVertexLabels(new String[]{axesNames[axis]});
		labelPSF.update();
		
		SceneGraphComponent label = new SceneGraphComponent();
		label.setName("label");
		Geometry geom = labelPSF.getPointSet();
		geom.setName("anchorPoint");
		label.setGeometry(geom);
		
		SceneGraphComponent line = new SceneGraphComponent();
		line.setName("line");
		line.addChild(label);
		//set axis label offset
		Appearance app =  new Appearance();
		app.setName("lineAppearance");
		app.setAttribute(CommonAttributes.POINT_SHADER+"."+"offset", new double[]{0,-.2,0});
		//app.setAttribute(CommonAttributes.POINT_SHADER+"."+"alignment", SwingConstants.EAST);  //inherited
		line.setAppearance(app);
		geom = lineLSF.getIndexedLineSet();
		geom.setName("line");
		line.setGeometry(geom);
		
		return line;
	}
		
		
	/**
	 * get the arrow for the coordinate axis specified by <code>axis</code> 
	 * between min and max as a SceneGraphComponent (IndexedFaceSet)
	 * @param axis the coordinate axis (X, Y, Z)
	 * @param min the starting point of the line
	 * @param max the endpoint of the line
	 * @return the axis arrow
	 */
	private SceneGraphComponent getArrow(int axis, double[] min, double[] max) {
	
		SceneGraphComponent arrow = new SceneGraphComponent();
		arrow.setName("arrow");
		Geometry geom = urCone;
		geom.setName("arrow");
		arrow.setGeometry(geom);
		//get rotation for axis
		//FactoredMatrix m = new FactoredMatrix(TubeUtility.tubeOneEdge(min, max, 0.025, null, metric).getTransformation());
		//above method results in incorrect translation
		FactoredMatrix m = new FactoredMatrix();
		m.setRotation(getAxisRotation(axis));
		m.setStretch(arrowStretch); //stretch urCone
		//translate to axis tip
		m.setTranslation(max);
//		double[] matrix = new double[16];
//		Quaternion q = getAxisRotation(axis);
//		MatrixBuilder.euclidean().translate(max).rotate(q.re, q.x,q.y,q.z)
		Transformation trans = new Transformation(m.getArray());
		trans.setName("arrowTransformation");
		trans.setReadOnly(true);
		arrow.setTransformation(trans);
		
		return arrow;
	}
		
	
	double[][] axes = {{0,1,0},{1,0,0},{0,0,1}};
	double[] angles = {Math.PI/2, -Math.PI/2, 0.0};
	/**
	 * get the ticks on the coordinate axis specified by <code>axis</code> 
	 * between min and max as a SceneGraphComponent (IndexedFaceSet)
	 * @param axis the coordinate axis (X, Y, Z)
	 * @param min the starting point of the line
	 * @param max the endpoint of the line
	 * @return the axis ticks
	 */
	private SceneGraphComponent getAxesTicks(int axis, double[] min, double[] max) {
		
		//create the ticks on a line in z-direction
		//determine minimum and maximum value of the tick level
		final double minLevel = round(axisScale*Math.ceil( (min[axis])/axisScale ));  //round to 3 decimal places
		final double maxLevel = round(axisScale*Math.floor( (max[axis]-arrowHeight*arrowStretch)/axisScale ));  //give space for arrow
		
		SceneGraphComponent ticks = new SceneGraphComponent();
		ticks.setName("ticks");
		
//		if (minLevel>maxLevel)
//			return ticks;  
			
		IndexedFaceSet ticksGeom = new IndexedFaceSet();
		int numOfTicks = 0;
		for (double level=round(minLevel); level<=maxLevel; level=round(level+axisScale) ) {
			if (level==0)	continue;  //no tick at origin
			GeometryMergeFactory gMFac = new GeometryMergeFactory();
			ticksGeom = gMFac.mergeIndexedFaceSets(new PointSet[]{ ticksGeom, 
					Primitives.pyramid(octagonalCrossSection(level), new double[]{0,0,level}) });
			numOfTicks++;
		}
		//GeometryUtility.calculateAndSetVertexNormals(ticksIFS);
		
		if (numOfTicks == 0)  //no ticks
			return ticks;  //note that nodes.get(axesNames[axis]+"Ticks") has no geometry now
		
		//create labels
		PointSetFactory labelPSF = new PointSetFactory();
		labelPSF.setVertexCount(numOfTicks);
		double[][] labelPoints = new double[numOfTicks][];
		String[] labelStr = new String[numOfTicks];
		double level = minLevel;
		for (int i=0; i<numOfTicks; i++, level=round(level+axisScale) ) {
			if (level==0) level+=axisScale;  //skip tick at origin
			labelPoints[i] = new double[]{0, 0, level};
			labelStr[i] = Math.round(level*1000)/1000. + "";  //3 decimal places
		}
		labelPSF.setVertexCoordinates(labelPoints);
		labelPSF.setVertexLabels(labelStr);
		labelPSF.update();
		SceneGraphComponent labels = new SceneGraphComponent();
		labels.setName("label");
		Geometry geom = labelPSF.getPointSet();
		geom.setName("anchorPoints");
		labels.setGeometry(geom);

		//create the SceneGraphComponent and rotate the ticks onto the corresponding coordinate axis
		ticksGeom.setName("ticks");
		ticks.setGeometry(ticksGeom);
		FactoredMatrix m = new FactoredMatrix();
		m.setRotation(getAxisRotation(axis));
		double[] translation = (double[])min.clone();
		translation[axis] = 0;
		m.setTranslation(translation);
		m.setStretch(tickStretch, tickStretch, 1); //stretch ticks
		MatrixBuilder.euclidean().translate(translation).
			rotate(angles[axis], axes[axis]).scale(tickStretch, tickStretch, 1).assignTo(ticks);

		ticks.addChild(labels);
		return ticks;
	}
	
	
	/**
	 * get the ticks on the box for coordinate axis specified by <code>axis</code> 
	 * between min and max as a SceneGraphComponent (IndexedLineSet)
	 * @param axis the coordinate axis (X, Y, Z)
	 * @param min the starting point of the line
	 * @param max the endpoint of the line
	 * @return the box ticks
	 */
	private SceneGraphComponent getBoxTicks(int axis, int k, double[] min, double[] max) {
		
		//create the ticks on a line in z-direction
		//determine minimum and maximum value of the tick level
		final double minLevel = round(axisScale*Math.ceil( (min[axis]+0.05)/axisScale ));  //round to 3 decimal places
		final double maxLevel = round(axisScale*Math.floor( (max[axis]-0.05)/axisScale));  //give space for box corners
		
		SceneGraphComponent ticks = new SceneGraphComponent();
		ticks.setName("ticks");
		
		if (minLevel>maxLevel)
			return ticks;  //note that nodes.get(axesNames[axis]+toBinaryString(k)+"ticks") has no geometry now
			
		IndexedLineSet ticksGeom = new IndexedLineSet();
		IndexedLineSetFactory newTick;
		int numOfTicks = 0;
		
		for (double level=minLevel; level<=maxLevel; level=round(level+axisScale) ) {
			newTick = new IndexedLineSetFactory();
			newTick.setVertexCount(3);
			newTick.setEdgeCount(2);
			newTick.setVertexCoordinates(new double[][]{{5,0,level},{0,0,level},{0,5,level}});
			newTick.setEdgeIndices(new int[][]{{0,1},{1,2}});
			newTick.update();
			ticksGeom = mergeIndexedLineSets(ticksGeom, newTick.getIndexedLineSet());
			numOfTicks++;
		}
		
		//create labels
		PointSetFactory labelPSF = new PointSetFactory();
		labelPSF.setVertexCount(numOfTicks);
		double[][] labelPoints = new double[numOfTicks][];
		String[] labelStr = new String[numOfTicks];
		double level = minLevel;
		for (int i=0; i<numOfTicks; i++, level=round(level+axisScale) ) {
			labelPoints[i] = new double[]{0, 0, level};
			labelStr[i] = Math.round(level*1000)/1000. + "";  //3 decimal places
		}
		labelPSF.setVertexCoordinates(labelPoints);
		labelPSF.setVertexLabels(labelStr);
		labelPSF.update();
		SceneGraphComponent labels = new SceneGraphComponent();
		labels.setName("label");
		Geometry geom = labelPSF.getPointSet();
		geom.setName("anchorPoints");
		labels.setGeometry(geom);
		nodes.put(axesNames[axis]+toBinaryString(k)+"ticklabels", labels);  //e.g. x00ticklabels
		
		//create the SceneGraphComponent and rotate the ticks onto the corresponding coordinate axis
		ticksGeom.setName("ticks");
		ticks.setGeometry(ticksGeom);
		double[] translation = (double[])min.clone();
		translation[axis] = 0;
		double[] tickr = getTickRotation(axis, k);
		MatrixBuilder.euclidean().translate(translation).
				rotate(tickr[3], tickr[0], tickr[1], tickr[2]).
				rotate(angles[axis], axes[axis]).
				scale(tickStretch, tickStretch, 1).assignTo(ticks);
		ticks.addChild(labels);
		return ticks;
	}
	
	
	/**
	 * get a 2d grid on the 6 box faces determined by the box ticks 
	 * as a SceneGraphComponent (IndexedLineSet)
	 * @return the grid
	 */
	private SceneGraphComponent calculate2DGrid() {
		
		PointSet ps;
		double[] points, current, translation, result;
		FactoredMatrix trans;
		IndexedLineSet[] gridComp = new IndexedLineSet[6];
		for (int i=0; i<gridComp.length; i++)
			gridComp[i] = new IndexedLineSet();
		
		for (int axis=X; axis<=Z; axis++) {
			
			SceneGraphComponent ticks = nodes.get(axesNames[axis]+"00ticklabels");
			if (ticks == null) continue;  //no ticks on current axis

			//get number of ticks on axis
			final int n = ((PointSet)ticks.getGeometry()).getNumPoints();

			for (int k=0; k<=3; k++) {
				
				double[][] vertices = new double[2*n][3];
				int[][] indices = new int[n][2];
				final int next = new int[]{1,2,-2,-1}[k]; //toBinaryString(k+next) = name of axis copy to which grid lines are to be drawn
				
				for (int i=0; i<n; i++) {
					for (int line=0; line<=1; line++) {
						//get anchor points of tick labels
						ps = (PointSet)nodes.get(axesNames[axis]+toBinaryString(k+line*next)+"ticklabels").getGeometry();  //e.g. x00ticklabels
						points = ps.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null);
						current = new double[]{points[3*i], points[3*i+1], points[3*i+2], 0};
						translation = (double[])boxVertices[axis][2*(k+line*next)].clone();  //minimum end point
						translation[axis] = 0;
						trans = new FactoredMatrix(nodes.get(axesNames[axis]+toBinaryString(k+line*next)+"ticks").getTransformation()); //e.g. x00ticks
						result = trans.multiplyVector(current);  
						vertices[n*line+i][0] = result[0]+translation[0];
						vertices[n*line+i][1] = result[1]+translation[1];
						vertices[n*line+i][2] = result[2]+translation[2];
					}
					
					indices[i] = new int[]{i, n+i};
				}
				
				//create line set
				IndexedLineSetFactory fac = new IndexedLineSetFactory();
				fac.setVertexCount(vertices.length);
				fac.setEdgeCount(indices.length);
				fac.setVertexCoordinates(vertices);
				fac.setEdgeIndices(indices);
				fac.update();
				
				//determine face and add to gridComp
				//0=xMin, 1=xMax, 2=yMin, 3=yMax, 4=zMin, 5=zMax
				int face = 0;
				switch (axis) {
				case X: face = new int[]{2,5,4,3}[k]; break;
				case Y: face = new int[]{0,5,4,1}[k]; break;
				case Z: face = new int[]{0,3,2,1}[k];
				}
				gridComp[face] = mergeIndexedLineSets(gridComp[face], fac.getIndexedLineSet());
				
			} //end loop for k
		} //end loop for axis
		
		//create SceneGraphNodes
		SceneGraphComponent grid = new SceneGraphComponent("grid");
		nodes.put("grid", grid);
		for (int i=0; i<gridComp.length; i++) {
			SceneGraphComponent face = new SceneGraphComponent();
			face.setName("face"+i);
			gridComp[i].setName("face");
			face.setGeometry(gridComp[i]);
			nodes.put("face"+i, face);
			grid.addChild(face);
		}
		
		Appearance app = new Appearance();
		app.setName("gridAppearance");
		app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, gridColor);
		grid.setAppearance(app);
		grid.setVisible(showGrid);
		return grid;
	}
	
	
	/**
	 * get the octagonalCrossSection at a different level than 0
	 * @param level the level of the octagonalCrossSection (z-value)
	 * @return the octagonalCrossSection at specified level
	 */
	private double[][] octagonalCrossSection(double level) {
		
		double[][] octagonalCrossSection = this.octagonalCrossSection; 
		for (int i=0; i<octagonalCrossSection.length; i++)
			octagonalCrossSection[i][2] = level;
		return octagonalCrossSection;
	}
	
	
	/**
	 * merges two IndexedLineSets into a single one
	 * @param a first IndexedLineSet
	 * @param b second IndexedLineSet
	 * @return the merged IndexedLineSet
	 */
	private IndexedLineSet mergeIndexedLineSets(IndexedLineSet a, IndexedLineSet b) {
		
		GeometryMergeFactory gmf = new GeometryMergeFactory();
		return gmf.mergeIndexedLineSets(new IndexedLineSet[]{a,b});
	}
	
	
	/**
	 * calculates the rotation from z-axis onto specified axis<br>
	 * used for rotating axes and box ticks
	 * @param axis the coordinate axis (X, Y, Z)
	 * @return a quaternion specifying the rotation 
	 */
	private Quaternion getAxisRotation(int axis) {
	
		FactoredMatrix rot = new FactoredMatrix();
		switch(axis) {
		case X : rot.setRotation(Math.PI/2,0,1,0); break;
		case Y : rot.setRotation(-Math.PI/2,1,0,0); break;
		//case Z : z-axis => no rotation
		}
		return Quaternion.rotationMatrixToQuaternion(new Quaternion(), rot.getArray());
	}
	

	/**
	 * calculates the additional rotation of box ticks onto specified box edge<br>
	 * used for rotating box ticks (additionally to getAxisRotation())
	 * @param axis the coordinate axis (X, Y, Z)
	 * @param k the copy of the coordinate axis (0..3)
	 * @return a quaternion specifying the rotation 
	 */
	private double[] getTickRotation(int axis, int k) {
		
		//determine the factor of pi/2-rotation
		int c = new int[]{0,3,1,2}[k];
		switch(axis) {
		case X : c++; break;
		case Y : c*=-1; c--;  // *-1 => switch 1 and 3
		}
		
		double[] retvalue = new double[4];
		retvalue[axis] = 1;
		retvalue[3] = c*Math.PI/2;
		return retvalue;
		//return Quaternion.rotationMatrixToQuaternion(new Quaternion(), rot.getArray());
	}
	

	/**
	 * get the binary representation of <code>k</code> using (at least) 2 digits<br> 
	 * (intended for <code>k</code>=0..3)
	 * @param k integer
	 * @return binary representation as String
	 */
	private String toBinaryString(int k) {
		if (k<2) return ("0"+k);
		else return Integer.toBinaryString(k);
	}
	
	
	/**
	 * round a double to 3 decimal places
	 * @param d double
	 * @return rounded double
	 */
	private double round(double d) {
		return Math.round(d*1000)/1000.;
	}
	
	
	
//-------------------------------------------------------------
//the following methods are intended to be used in a TOOL
//to hide specific box vertices, axes or labels
//-------------------------------------------------------------
	
	/**
	 * get index of a box vertex which is "closest to the screen" 
	 * when looking along specified direction vector<br>
	 * note that there could be more than one closest box vertices, 
	 * this method returns the first
	 * @param dir the direction vector
	 * @return the index of a closest box vertex in boxVertices[X]
	 */
	private int getClosestBoxVertex(double[] dir) {
		double[] direction;
		if (dir.length==3) direction=dir;
		else {  //dir.length==4
			direction=new double[3];
			direction[X]=dir[X]/dir[3];
			direction[Y]=dir[Y]/dir[3];
			direction[Z]=dir[Z]/dir[3];
		}
		//closest box vertex has minimal inner product with direction
		int closest = 0;
		
		double tmp = Rn.innerProduct(boxVertices[X][closest], direction);
		
		for (int k=1; k<8; k++) { //boxVertices[X] contains all box vertices
			if ( Rn.innerProduct(boxVertices[X][k], direction) < tmp) {
				closest = k;
				tmp = Rn.innerProduct(boxVertices[X][k], direction);
			}
		}
		return closest; //index of closest box vertex in boxVertices[X]
	}
	

	/**
	 * Beautifies the box regarding the current transformation from camera coordinates to local coordinates of the coordiante system,  
	 * i.e. hides certain box vertices including box edges and grid faces.
	 * @param cameraToObject the transformation matrix from camera coordinates to local coordinates of the coordinate system within the SceneGraph
	 */
	public void updateBox(double[] cameraToObject) {
		
		//direction of view in camera coordinates is (0,0,-1)
		//transform camera coordinates to local coordinates
		double[] direction = new Matrix(cameraToObject).multiplyVector(new double[]{0,0,-1, 0});
		direction[3]=1;
		
		//calculate closest box vertex
		final int index = getClosestBoxVertex(direction);
		//only do something if closest box vertex changed
		if (currentClosestBoxVertex == index) return;
		
		
		//set all box edges and grid faces to visible
		for (int axis=X; axis<=Z; axis++) {
			for (int k=0; k<=3; k++) {
				nodes.get(axesNames[axis]+toBinaryString(k)).setVisible(true);
			}
			nodes.get("face"+2*axis).setVisible(true);
			nodes.get("face"+(2*axis+1)).setVisible(true);
		}
		
		currentClosestBoxVertex = index;
		double[] closest = boxVertices[X][currentClosestBoxVertex];
		int[] edgeCriteria = new int[3];
		
		//get the 3 edges belonging to a closest box vertex
		for (int axis=X; axis<=Z; axis++) {
			if (closest[axis] == boxMin[axis]) edgeCriteria[axis] = 0;  //0 corresponds to vertex with minimum value on axis i
			else edgeCriteria[axis] = 1;  //1 corresponds to vertex with maximum value on axis i 
		}

		//hide edges which don't have copies of same "distance to the screen"
		int[] invisibleEdges = new int[3];
		if (direction[Y]!=0 && direction[Z]!=0) {
			nodes.get("x" + edgeCriteria[Y] + edgeCriteria[Z]).setVisible(false);
			invisibleEdges[X] = edgeCriteria[Y]*2 + edgeCriteria[Z]; }
		if (direction[X]!=0 && direction[Z]!=0) {
			nodes.get("y" + edgeCriteria[X] + edgeCriteria[Z]).setVisible(false);
			invisibleEdges[Y] = edgeCriteria[X]*2 + edgeCriteria[Z]; }
		if (direction[X]!=0 && direction[Y]!=0) {
			nodes.get("z" + edgeCriteria[X] + edgeCriteria[Y]).setVisible(false);
			invisibleEdges[Z] = edgeCriteria[X]*2 + edgeCriteria[Y]; }
		
		//hide corresponding grid faces
		for (int axis=X; axis<=Z; axis++) 
			nodes.get("face"+(2*axis+edgeCriteria[axis])).setVisible(false);
		
		if (beautifyBoxLabels) updateLabelsOfBoxEdges(cameraToObject, invisibleEdges);
	}
	
	
	
//-------------------------------------------------------------
//SETTING & GETTING PROPERTIES
//-------------------------------------------------------------
	
	
	/**
	 * Get the coordinate system specified in this factory.<br> 
	 * (The coordinate system may be removed from the SceneGraph using this method.)
	 * @return the coordinate system
	 */
	public SceneGraphComponent getCoordinateSystem() {
		
		return coordinateSystem;
	}
	
	
	/**
	 * Set the axis scale, i.e. the distance between two ticks on the coordinate axes.
	 * @param axisScale the axis scale
	 */
	public void setAxisScale(double axisScale) {
		if (this.axisScale == axisScale) return;
		//else
		this.axisScale = axisScale;

		//update ticks and labels
		for (int axis = X; axis <= Z; axis++) { // for each coordinate axis

			//for box:
			for (int k = 0; k <= 3; k++) {
				//remove old ticks and labels
				SceneGraphComponent singleAxisK = nodes.get(axesNames[axis]+ toBinaryString(k));  //e.g. x00
				singleAxisK.removeChild(nodes.get(axesNames[axis]+ toBinaryString(k) + "ticks"));  //e.g. x00ticks
				//create new ticks with labels
				SceneGraphComponent ticks = getBoxTicks(axis, k, 
						boxVertices[axis][2 * k], boxVertices[axis][2 * k + 1]);
				//update hash table
				nodes.put(axesNames[axis] + toBinaryString(k) + "ticks", ticks);  //e.g. x00ticks
				//add new ticks and labels to SceneGraph
				singleAxisK.addChild(ticks);
			}

			//for axes:

			//remove old ticks and labels
			SceneGraphComponent singleAxis = nodes.get(axesNames[axis] +"Axis");  //e.g. xAxis
			singleAxis.removeChild(nodes.get(axesNames[axis]+"Ticks"));  //e.g. xTicks
			//create new ticks with labels
			SceneGraphComponent ticks = getAxesTicks(axis,
					axesVertices[axis][0], axesVertices[axis][1]);
			//update hash table
			nodes.put(axesNames[axis] + "Ticks", ticks);  //e.g. xTicks
			//add new ticks and labels to SceneGraph
			singleAxis.addChild(ticks);
		}
		
		//update grid
		nodes.get("box").removeChild(nodes.get("grid"));
		nodes.get("box").addChild(calculate2DGrid());
	}
	
	
	/**
	 * Get the current axis scale, i.e. the distance between two ticks on the coordinate axes.
	 * @return the current axis scale
	 */
	public double getAxisScale() {
		return axisScale;
	}
	
	
	/**
	 * Set the label scale, i.e. the size of labels of axes and ticks<br>
	 * (including the size of arrows and ticks).
	 * @param labelScale the label scale
	 */
	public void setLabelScale(double labelScale) {
		if (this.labelScale == labelScale) return;
		//else
		this.labelScale = labelScale;
		
		coordinateSystem.getAppearance().setAttribute("pointShader.scale", labelScale);
		
		//update size of arrows and ticks
		arrowStretch = 16*labelScale;
		tickStretch = 8*labelScale;
		
		SceneGraphComponent arrow, ticks;
		FactoredMatrix m;

		for (int axis=X; axis<=Z; axis++) {
			//for box:
			for (int k=0; k<=3; k++) {
				arrow = nodes.get(axesNames[axis]+toBinaryString(k)+"arrow");  //e.g. x00arrow
				m = new FactoredMatrix(arrow.getTransformation());
				m.setStretch(arrowStretch); //stretch urCone
				Transformation trans = new Transformation(m.getArray());
				trans.setName("arrowTransformation");
				trans.setReadOnly(true);
				arrow.setTransformation(trans);
				
				ticks = nodes.get(axesNames[axis]+toBinaryString(k)+"ticks");  //e.g. x00ticks
				if (ticks.getTransformation()!=null)
					m = new FactoredMatrix(ticks.getTransformation());
				else m = new FactoredMatrix();
				m.setStretch(tickStretch, tickStretch, 1); //stretch ticks
				trans = new Transformation(m.getArray());
				trans.setName("tickTransformation");
				trans.setReadOnly(true);
				ticks.setTransformation(trans);
			}
			//for axes:
			arrow = nodes.get(axesNames[axis]+"Arrow");  //e.g. xArrow
			m = new FactoredMatrix(arrow.getTransformation());
			m.setStretch(arrowStretch); //stretch urCone
			Transformation trans = new Transformation(m.getArray());
			trans.setName("arrowTransformation");
			trans.setReadOnly(true);
			arrow.setTransformation(trans);
			
			ticks = nodes.get(axesNames[axis]+"Ticks");  //e.g. xTicks
			if (ticks.getTransformation()!=null)
				m = new FactoredMatrix(ticks.getTransformation());
			else m = new FactoredMatrix();
			m.setStretch(tickStretch, tickStretch, 1); //stretch ticks
			trans = new Transformation(m.getArray());
			trans.setName("tickTransformation");
			trans.setReadOnly(true);
			ticks.setTransformation(trans);
		}
		// recalculate tick levels since arrow size changed ???
	}
	

	/**
	 * Get the current label scale, i.e. the size of labels of axes and ticks.
	 * @return the current label scale
	 */
	public double getLabelScale() {
		return labelScale;
	}
	
	
	/**
	 * Show or hide the axes of the coordinate system going through the origin.
	 * @param b true iff axes are to be shown
	 */
	public void showAxes(boolean b) {
		showAxes = b;
		nodes.get("axes").setVisible(b);
	}

	
	/**
	 * Show or hide the bounding box of the coordinate system.
	 * @param b true iff the box is to be shown
	 */
	public void showBox(boolean b) {
		showBox = b;
		nodes.get("box").setVisible(b);
	}


	/**
	 * Show or hide the grid on the bounding box faces.
	 * @param b true iff the grid is to be shown
	 */
	public void showGrid(boolean b) {
		showGrid = b;
		nodes.get("grid").setVisible(b);
	}
	
	
	/**
	 * Show or hide the arrows of the axes of the coordinate system going through the origin.
	 * @param b true iff the arrows are to be shown
	 */
	public void showAxesArrows(boolean b) {
		//if (showAxesArrows==b) return;
		showAxesArrows = b;
		//set visiblity of all arrows
		for (int axis=X; axis<=Z; axis++)
			nodes.get(axesNames[axis]+"Arrow").setVisible(b);
	}


	/**
	 * Show or hide the arrows of the bounding box of the coordinate system.
	 * @param b true iff the arrows are to be shown
	 */
	public void showBoxArrows(boolean b) {
		//if (showBoxArrows==b) return;
		showBoxArrows = b;
		//set visiblity of all arrows
		for (int axis=X; axis<=Z; axis++) {
			for (int k=0; k<=3; k++) {
				nodes.get(axesNames[axis]+toBinaryString(k)+"arrow").setVisible(b);
			}
		}
	}
	
	
	/**
	 * Show or hide the tick & axis labels of axes and bounding box of the coordinate system.
	 * @param b true iff the labels are to be shown
	 */
	public void showLabels(boolean b) {
		//if (showLabels==b) return;
		showLabels = b;
		coordinateSystem.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, b);
	}


	/**
	 * Set the color of the coordinate system (axes and bounding box).
	 * @param c the color
	 */
	public void setColor(Color c) {
		coordinateSystemColor = c;
		coordinateSystem.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, c);
		coordinateSystem.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, c);
	}


	/**
	 * Get the current color of the coordinate system (axes and bounding box).
	 * @return the current color
	 */
	public Color getColor() {
		return coordinateSystemColor;
	}


	/**
	 * Set the color of the grid on the bounding box faces.
	 * @param c the color
	 */
	public void setGridColor(Color c) {
		gridColor = c;
		nodes.get("grid").getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, c);
	}


	/**
	 * Get the current color of the grid on the bounding box faces.
	 * @return the current color
	 */
	public Color getGridColor() {
		return gridColor;
	}
	
	
 /**
  * Set the color of the bounding box.
  * @param c the color
  */
 public void setBoxColor(Color c) {
   boxColor = c;
   nodes.get("box").getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, c);
 }


 /**
  * Get the current color of the bounding box.
  * @return the current color
  */
 public Color getBoxColor() {
   return boxColor;
 }


	/**
	 * Set the color of all labels of the coordinate system.
	 * @param c the color
	 */
	public void setLabelColor(Color c) {
		labelColor = c;
		coordinateSystem.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, c);
	}


	/**
	 * Get the current color of all labels of the coordinate system.
	 * @return the current color
	 */
	public Color getLabelColor() {
		return labelColor;
	}

	
	/**
	 * Set the font of all labels of the coordinate system.
	 * @param f the font
	 */
	public void setLabelFont(Font f) {
		labelFont = f;
		coordinateSystem.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+"font", f);
	}


	/**
	 * Get the current font of all labels of the coordinate system.
	 * @return the current font
	 */
	public Font getLabelFont() {
		return labelFont;
	}


	/**
	 * Beautify the coordinate system automatically iff set to true,  
	 * i.e. hide certain box vertices including box edges and grid faces.<br>
	 * (Adds or removes the tool CoordinateSystemBeautifier to the coordinate system.)
	 * @param b true iff coordinate system is to be beautified automatically
	 */
	public void beautify(boolean b) {
		if (beautify==b) return;
		beautify = b;
		if (b) coordinateSystem.addTool(beautifier);
		else coordinateSystem.removeTool(beautifier);
	}

	
	/**
	 * Set the box edges on which labels are to be shown. <br>
	 * Edges={{dir_y, dir_z}, {dir_x, dir_z}, {dir_x, dir_y}} specifies
	 * on which three edges of the bounding box labels are shown. 
	 * The dir_i must be either -1 or +1 and specifies whether labels are shown on
	 * the edge of the box with a larger or smaller value of coordinate i, respectively.<br>
	 * (This method is analogous to the Mathematica Graphics3D directive 'AxesEdges'.) 
	 * @param edges specifies on which edges of the bounding box labels are to be shown
	 */
	public void setLabelBoxEdges(int[][] edges) {
				
		String a, b;
		SceneGraphComponent labels;
		for (int axis=X; axis<=Z; axis++) {
			a = (edges[axis][0] == -1)? "0":"1";
			b = (edges[axis][1] == -1)? "0":"1";
			for (int k=0; k<=3; k++) {
				labels = nodes.get(axesNames[axis]+toBinaryString(k)+"ticklabels");  //e.g. x00ticklabels
				if (labels==null) break;
				labels.setVisible(toBinaryString(k).equals(a+b));
			}
		}
	}
	
	
	
//-------------------------------------------------------------
//IN PROCESS...
//-------------------------------------------------------------

	private boolean beautifyBoxLabels = false;  //do or do not beautify labels of box edges
	
	private void updateLabelsOfBoxEdges(double[] cameraToObject, int[] invisibleEdges) {
		
		//direction of x-axis in camera coordinates is (1,0,0)
		//transform camera coordinates to local coordinates
		double[] tmp = new Matrix(cameraToObject).multiplyVector(new double[]{1,0,0,0});
		double[] direction = new double[]{tmp[0], tmp[1], tmp[2]};

		//determine neighbour edges of invisible edge
		int[][] neighbourEdges = new int[3][2];
		for (int axis=X; axis<=Z; axis++) {
			neighbourEdges[axis] = ( invisibleEdges[axis]==0 || invisibleEdges[axis]==3 )?
				new int[]{1,2} : new int[]{0,3};
		}
		
		//middle point of rightest box edge of neighbour edges has maximum inner product with direction
		int[] result = new int[3];
		for (int axis=X; axis<=Z; axis++) {
			//get coordinates of the middle points of the two neighbour edges via anchor point of label 
			PointSet ps = (PointSet)nodes.get(axesNames[axis]+toBinaryString(neighbourEdges[axis][0])+"label").getGeometry();
			double[] first = ps.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null);
			ps = (PointSet)nodes.get(axesNames[axis]+toBinaryString(neighbourEdges[axis][1])+"label").getGeometry();
			double[] second = ps.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null);
			
			//determine the rightest between the two
			result[axis] = ( Rn.innerProduct(second, direction) > Rn.innerProduct(first, direction) )?
				neighbourEdges[axis][1] : neighbourEdges[axis][0];
		}

		//translate result into directive for setAxesEdges()
		int[][] directive = new int[3][2];
		for (int axis=X; axis<=Z; axis++) {
			directive[axis][0] = ( toBinaryString(result[axis]).substring(0,1).equals("0") )? -1 : 1;
			directive[axis][1] = ( toBinaryString(result[axis]).substring(1,2).equals("0") )? -1 : 1;
		}

		//set visibility of box edges
		setLabelBoxEdges(directive);
	}
	
}