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

import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.ClippingPlane;
import de.jreality.scene.Cylinder;
import de.jreality.scene.Geometry;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.util.Rectangle3D;

/**
 * This class traverses a scene graph starting from the given "root" scene
 * graph component and calculates the 3D bounding box. 
 * <p>
 * At any point of the traversal, there is a current transformation representing
 * the transformation from the root.  Call this <i>M</i>. It can be initialized
 * to a value using {@link #setInitialMatrix(double[])}. For the following,
 * let the current state of the
 * bounding box during the traversal be denoted by <i>B</i>.
 * <p>
 * Only instances of {@link Geometry} currently contribute to the bounding box.
 * They can do this in three ways:
 * <ul>j
 * <li>When an instance of {@link PointSet} is found,
 * <i>M</i>  is applied to it vertices, and <i>B</i> is set to the union of 
 * the bounding box of these points
 * is union-ed with <i>B</i>.</li>
 * <li>Instances of build-in geometries such as {@link Sphere} have build-in bounding boxes
 * which are transformed and union-ed with <i>B</i>
 * <li>If an instance of {@link Geometry} has a geometry attribute with key {@link de.jreality.geometry.GeometryUtility#BOUNDING_BOX}
 * then this value is expected to be an instance of {@link Rectangle3D} and is 
 * union-ed with <i>B</i>. This overrides the first option given above.
 * </ul>
 *  <p>
 *  One can obtain the bounding box using the methods {@link #getXmin()}, etc, or all at once using
 *  {@link #getBoundingBox()}.
 *  * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>, Charles Gunn
 *
 */
public class BoundingBoxTraversal extends SceneGraphVisitor {

  private Bound bound;
  private double tmpVec[] = new double[4];

  double[]  initialTrafo,   currentTrafo;
  //private   Transformation  initialTransformation;
  protected BoundingBoxTraversal reclaimableSubcontext;

  /**
   * 
   */
  public BoundingBoxTraversal() {
    super();
     bound =new Bound();
  }

  protected BoundingBoxTraversal(BoundingBoxTraversal parentContext) {
    initializeFromParentContext(parentContext);
  }


  protected void initializeFromParentContext(BoundingBoxTraversal parentContext) {
    BoundingBoxTraversal p=parentContext;

    currentTrafo=initialTrafo=parentContext.currentTrafo;
    this.bound = parentContext.bound;
        
  }

  /**
   * Sets the initialTransformation.
   * @param initialTransformation The initialTransformation to set
   */
  public void setInitialMatrix(double[] initialMatrix) {
    this.initialTrafo= initialMatrix;
  }

  BoundingBoxTraversal subContext() {
    if (reclaimableSubcontext != null) {
      reclaimableSubcontext.initializeFromParentContext(this);
       return reclaimableSubcontext;
    } else
      return reclaimableSubcontext= new BoundingBoxTraversal(this);
  }
  /**
   * This starts the traversal of a SceneGraph starting form root.
   * @param root
   */
  public void traverse(SceneGraphComponent root) {
    if (initialTrafo == null) {
      initialTrafo= new double[16];
//    if (initialTransformation != null)
//      initialTransformation.getMatrix(initialTrafo);
//    else
      Rn.setIdentityMatrix(initialTrafo);
    }
    currentTrafo= initialTrafo;
    visit(root);
    //pipeline.setMatrix(initialTrafo);
  }

  public void visit(SceneGraphComponent c) {
	  if (c.getAppearance() != null) {
		  Object obj = c.getAppearance().getAttribute(GeometryUtility.BOUNDING_BOX);
		  if (obj != null && obj instanceof Rectangle3D)	{
		  		Rectangle3D box = (Rectangle3D) obj;
		     	unionBox(box);
		     	return;
		  }
	  }
      if(c.isVisible())
          c.childrenAccept(subContext());
  }

  public void visit(Transformation t) {
    if (initialTrafo == currentTrafo)
      currentTrafo= new double[16];
    Rn.copy(currentTrafo, initialTrafo);
    if (Rn.isNan(t.getMatrix()))	{
    	return;
    	//throw new IllegalStateException("bad matrix");
    }
    Rn.times(currentTrafo, currentTrafo, t.getMatrix());
  }


  public void visit(Geometry g) {
  	checkForBoundingBox(g);
  }
  
  public void visit(ClippingPlane p) {
  }
  
  public void visit(Cylinder c) {
      //TODO better to make this by transforming center and a
      // point on the sphere or something like that...
 	 unionBox(Rectangle3D.unitCube);
  }
  
  private boolean checkForBoundingBox(Geometry g)	{
	Object bbox = g.getGeometryAttributes(GeometryUtility.BOUNDING_BOX);
	if (bbox != null && bbox instanceof Rectangle3D)	{
			Rectangle3D box = (Rectangle3D) bbox;
			if (box == Rectangle3D.EMPTY_BOX) return true;
    	 	unionBox(box);		
    	 	return true;
  	}
	return false;
  }
  public void visit(PointSet p) {
  // Following code should only be activated if we have listeners installed to update 
  // the bounding box when it goes out of date.
	  if (checkForBoundingBox(p)) return;
  	Object domain = p.getGeometryAttributes(GeometryUtility.HEIGHT_FIELD_SHAPE);
	if (domain != null && domain instanceof Rectangle2D)	{
 		Rectangle2D box = (Rectangle2D) domain;
  	  	double[][] data = p.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
 		double[][] zbnds = new double[2][1];
 		Rn.calculateBounds(zbnds, data);
		double[][] xyzbnds = new double[2][3];
		xyzbnds[0][0] = box.getMinX();
		xyzbnds[1][0] = box.getMaxX();
		xyzbnds[0][1] = box.getMinY();
		xyzbnds[1][1] = box.getMaxY();
		xyzbnds[0][2] = zbnds[0][0];
		xyzbnds[1][2] = zbnds[1][0];
		if (Double.isNaN(xyzbnds[0][0]))
			throw new IllegalStateException("Nan");
		Rectangle3D box3 = new Rectangle3D(xyzbnds);		
	 	unionBox(box3);		
	 	return;
		
	}
    DataList vv = p.getVertexAttributes(Attribute.COORDINATES);
    if (vv == null)	{
    	//signal error
    	return;
    }
     unionVectors(vv);
        
  }
  
  public void visit(Sphere s) {
    //TODO better to make this by transforming center and a
    // point on the sphere or something like that...
	 unionBox(Rectangle3D.unitCube);
  }

  private final  void unionVectors(DataList dl) {
  	double[][] data = dl.toDoubleArrayArray(null);
	double[][] tmpVec = new double[2][3];
	int length = data.length;
  if (length == 0) return;
	int vectorLength = data[0].length;
	if (vectorLength<3 || vectorLength > 4) return;
	Rn.matrixTimesVector(data, currentTrafo, data);
	try {
		if (vectorLength == 4)	{
			Pn.calculateBounds(tmpVec, data);
		} else if (vectorLength == 3){
			Rn.calculateBounds(tmpVec, data);
		}		
	} catch (IllegalStateException e){
		e.printStackTrace();
	}
	if (Rn.isNan(tmpVec[0]) || Rn.isNan(tmpVec[1])) return;
	bound.xmin = Math.min(bound.xmin,tmpVec[0][0]);
	bound.xmax = Math.max(bound.xmax,tmpVec[1][0]);
	bound.ymin = Math.min(bound.ymin,tmpVec[0][1]);
	bound.ymax = Math.max(bound.ymax,tmpVec[1][1]);
	bound.zmin = Math.min(bound.zmin,tmpVec[0][2]);
	bound.zmax = Math.max(bound.zmax,tmpVec[1][2]);
 }
  
  private final void unionBox(Rectangle3D bbox) {
	if (bbox.isEmpty()) return;
    Rectangle3D tbox = bbox.transformByMatrix(null, currentTrafo);
  	double[][] bnds = tbox.getBounds();
    bound.xmin = Math.min(bound.xmin,bnds[0][0]);
    bound.xmax = Math.max(bound.xmax,bnds[1][0]);
    bound.ymin = Math.min(bound.ymin,bnds[0][1]);
    bound.ymax = Math.max(bound.ymax,bnds[1][1]);
    bound.zmin = Math.min(bound.zmin,bnds[0][2]);
    bound.zmax = Math.max(bound.zmax,bnds[1][2]);
}
/**
 * @return Returns the xmax.
 */
public double getXmax() {
    return bound.xmax;
}

/**
 * @return Returns the xmin.
 */
public double getXmin() {
    return bound.xmin;
}

/**
 * @return Returns the ymax.
 */
public double getYmax() {
    return bound.ymax;
}

/**
 * @return Returns the ymin.
 */
public double getYmin() {
    return bound.ymin;
}

/**
 * @return Returns the zmax.
 */
public double getZmax() {
    return bound.zmax;
}

/**
 * @return Returns the zmin.
 */
public double getZmin() {
    return bound.zmin;
}

public double[] getBoundingBoxCenter(double[] c) {
    if(c == null) c =new double[3];
    c[0] =(bound.xmin+bound.xmax)/2.;
    c[1] =(bound.ymin+bound.ymax)/2.;
    c[2] =(bound.zmin+bound.zmax)/2.;
    return c;
}
private class Bound {
    double xmin,xmax,ymin,ymax,zmin,zmax;
    public Bound() {
        super();
        xmin =ymin = zmin = Double.MAX_VALUE;
        xmax =ymax = zmax =-Double.MAX_VALUE;
    }
}

	/**
	 * Convert result into Rectangle3D instance (see {@link de.jreality.util.Rectangle3D}
	 */
	 public Rectangle3D getBoundingBox()	{
		
		Rectangle3D rect3d = new Rectangle3D();
		double[][] bnds = rect3d.getBounds();
		bnds[0][0] = getXmin();
		bnds[1][0] = getXmax();
		bnds[0][1] = getYmin();
		bnds[1][1] = getYmax();
		bnds[0][2] = getZmin();
		bnds[1][2] = getZmax();
		rect3d.setBounds(bnds);
		if (Rn.isNan(bnds[0]) || Rn.isNan(bnds[1]))
			return Rectangle3D.EMPTY_BOX;

		return rect3d;
	}

	 public static Rectangle3D getBoundingBox(double[] initialMatrix, SceneGraphComponent sgc)	{
	 	BoundingBoxTraversal bt = new BoundingBoxTraversal();
	 	if (initialMatrix != null) bt.setInitialMatrix(initialMatrix);
	 	bt.traverse(sgc);
	 	return bt.getBoundingBox();
	 }
}