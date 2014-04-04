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


package de.jreality.scene.pick;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Stack;

import de.jreality.math.Matrix;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Cylinder;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.CameraUtility;
import de.jreality.util.PickUtility;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;

/**
 * 
 * Our pick system implementation. Uses Brute-force as default
 * and AABBTrees if available.
 * 
 * @author Steffen Weissmann
 *
 */
public class AABBPickSystem implements PickSystem {
  
  private Impl impl;
  private SceneGraphComponent root;
  private ArrayList<Hit> hits = new ArrayList<Hit>();
  private HashMap<IndexedFaceSet, AABBTree> aabbTreeExists = new HashMap<IndexedFaceSet, AABBTree>();
  private HashMap<Geometry, Boolean> isPickableMap = new HashMap<Geometry, Boolean>();
  private Comparator<Hit> cmp = new Hit.HitComparator();
  private double[] from;
  private double[] to;
  private double radiusFactor = 1;		// in case we have to adjust radii by radii in world coordinates bit
  private int metric;
  
  public void setSceneRoot(SceneGraphComponent root) {
    impl= new Impl();
    this.root=root;
  }
  
  public List<PickResult> computePick(double[] f, double[] t) {
    from=f.clone();
    to=t.clone();
    hits.clear();
    // get the metric fresh each invocation
    // this is actually dangerous since metric may change inside the scene graph
    // (e.g., in ViewerVR there are euclidean 2D Frames inside the (possibly noneuclidean) scene.
    if (root.getAppearance() != null) {
    	Object sig = root.getAppearance().getAttribute(CommonAttributes.METRIC, Integer.class);
    	if (sig instanceof Integer)	metric = (Integer) sig;
    } else metric = Pn.EUCLIDEAN;
    impl.visit();
    if (hits.isEmpty()) return Collections.emptyList();
    Collections.sort(hits, cmp);
    return new ArrayList<PickResult>(hits);
  }

  /**
   * TODO: optimize access to appearances to avoid use of effective appearance objects.
   *
   */
  private class Impl extends SceneGraphVisitor {

    private Stack<PickInfo> appStack = new Stack<PickInfo>();
    private PickInfo currentPI;
    
    Impl()	{
    	appStack.push(currentPI = new PickInfo(null, null));
    }
   
    private SceneGraphPath path=new SceneGraphPath();
    private ArrayList<Hit> localHits=new ArrayList<Hit>();

    private Matrix m=new Matrix();
    private Matrix mInv=new Matrix();
    
//    private int metric=Pn.EUCLIDEAN;
    private Matrix[] matrixStack = new Matrix[256];
    int stackCounter = 0;
    /**
     * This class avoids using an effective appearance by directly reading the Appearances.
     * @author Charles Gunn
     *
     */
    private class PickInfo {
        private boolean pickPoints=true, drawVertices = true;
        private boolean pickEdges=true, drawEdges = true;
        private boolean pickFaces=true, drawFaces = true;
        private boolean radiiWorldCoords = false;
        private boolean hasNewPickInfo = false;
        private double tubeRadius=CommonAttributes.TUBE_RADIUS_DEFAULT;
        private double pointRadius=CommonAttributes.POINT_RADIUS_DEFAULT;
        int metric = AABBPickSystem.this.metric;
        PickInfo(PickInfo old, Appearance ap)	{
        	if (old != null)	{
               	drawVertices = old.drawVertices;
            	drawEdges = old.drawEdges;
            	drawFaces = old.drawFaces;
            	pickPoints = old.pickPoints;
            	pickEdges = old.pickEdges;
            	pickFaces = old.pickFaces;
            	tubeRadius = old.tubeRadius;
            	pointRadius = old.pointRadius;    
            	metric = old.metric;
        	}
        	if (ap == null) return;
            Object foo = ap.getAttribute(CommonAttributes.VERTEX_DRAW, Boolean.class);
            if (foo != Appearance.INHERITED) { hasNewPickInfo = true; pickPoints=drawVertices = (Boolean) foo;}
//           if (drawVertices)	{
            	foo = ap.getAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.PICKABLE,Boolean.class);
                if (foo != Appearance.INHERITED) { hasNewPickInfo = true; pickPoints = (Boolean) foo; }
//             }
            foo = ap.getAttribute(CommonAttributes.EDGE_DRAW, Boolean.class);
            if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; pickEdges = drawEdges = (Boolean) foo;}
            if (drawEdges)	{
            	foo = ap.getAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.PICKABLE,Boolean.class);
                if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; pickEdges = (Boolean) foo;}
           }
            foo = ap.getAttribute(CommonAttributes.FACE_DRAW, Boolean.class);
            if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; pickFaces = drawFaces = (Boolean) foo;}
            if (drawFaces)	{
            	foo = ap.getAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.PICKABLE,Boolean.class);
                if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; pickFaces = (Boolean) foo; }
           }
           foo = ap.getAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_RADIUS, Double.class);
          if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; pointRadius = (Double) foo;}
          else {
              foo = ap.getAttribute(CommonAttributes.POINT_RADIUS, Double.class);
              if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; pointRadius = (Double) foo;}
          }
          foo = ap.getAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBE_RADIUS, Double.class);
          if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; tubeRadius = (Double) foo; }
          else {
              foo = ap.getAttribute(CommonAttributes.TUBE_RADIUS, Double.class);
              if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; tubeRadius = (Double) foo;}
          }
          foo = ap.getAttribute(CommonAttributes.METRIC, Integer.class);
          if (foo != Appearance.INHERITED)  { hasNewPickInfo = true; metric = (Integer) foo; }
          
          foo = ap.getAttribute(CommonAttributes.RADII_WORLD_COORDINATES, Boolean.class);
          if (foo != Appearance.INHERITED)  { hasNewPickInfo = true;  radiiWorldCoords = (Boolean) foo; }
          
         }
        public String toString()	{
        	return "Pick vef = "+pickPoints+" "+pickEdges+" "+pickFaces+" "+pointRadius+" "+tubeRadius;
        }
        public boolean hasNewPickInfo()	{
        	return hasNewPickInfo;
        }
   }
    
	public void visit(SceneGraphComponent c) {
		if (!c.isVisible() || !c.isPickable())
			return;
		PickInfo pickInfo = null;
		// System.err.println("visiting "+c.getName());
		path.push(c);
		if (c.getTransformation() != null) {
			if (matrixStack[stackCounter + 1] == null)
				matrixStack[stackCounter + 1] = new Matrix();
			Rn.times(matrixStack[stackCounter + 1].getArray(),
					matrixStack[stackCounter].getArray(), c
							.getTransformation().getMatrix());
			stackCounter++;
			m = matrixStack[stackCounter];
			mInv = m.getInverse();
		}
		if (c.getAppearance() != null) {
			// following is actually deprecated and can be removed any time
			// now
			Object foo = c.getAppearance().getAttribute(CommonAttributes.PICKABLE);
			if (foo instanceof Boolean && ((Boolean) foo).booleanValue() == false) {
				path.pop();
				return;
			}
			pickInfo = new PickInfo(currentPI, c.getAppearance());
			if (pickInfo.hasNewPickInfo) {
				appStack.push(currentPI = pickInfo);
			}
			if (pickInfo.radiiWorldCoords) {
				double[] o2w = path.getMatrix(null);
				radiusFactor = CameraUtility.getScalingFactor(o2w, pickInfo.metric);
				radiusFactor = 1.0 / radiusFactor;
			}
		}
		
		if (currentPI.radiiWorldCoords) {
			double[] o2w = path.getMatrix(null);
			radiusFactor = CameraUtility.getScalingFactor(o2w, currentPI.metric);
			radiusFactor = 1.0 / radiusFactor;
		} else {
			radiusFactor = 1.0;
		}
		
		c.childrenAccept(this);
		if (c.getAppearance() != null && pickInfo.hasNewPickInfo) {
			appStack.pop();
			currentPI = appStack.elementAt(appStack.size() - 1);
		}
		if (c.getTransformation() != null) {
			stackCounter--;
			m = matrixStack[stackCounter];
			mInv = m.getInverse();
		}
		path.pop();
	}
    
    
    private boolean isPickable(Geometry g) {
    	Boolean boo = isPickableMap.get(g);
    	if (boo == null) {
    	    Object o = g.getGeometryAttributes(CommonAttributes.PICKABLE);
    	    boo =  !(o != null && o.equals(Boolean.FALSE));
    		isPickableMap.put(g, boo);
    	}
      return boo.booleanValue();
    }

    public void visit() {
    	stackCounter = 0;
    	matrixStack[0] = new Matrix();
    	aabbTreeExists.clear();
    	isPickableMap.clear();
    	path.clear();
    	visit(root);
    }

    public void visit(Sphere s) {
      if (!currentPI.pickFaces || !isPickable(s)) return;
      
      localHits.clear();
      
      BruteForcePicking.intersectSphere(s, metric, path, m, mInv, from, to, localHits);
      
      extractHits(localHits);
    }
    
    public void visit(Cylinder c) {
       if (!currentPI.pickFaces || !isPickable(c)) return;
      
      localHits.clear();
      
      BruteForcePicking.intersectCylinder(c, metric, path, m, mInv, from, to, localHits);

      extractHits(localHits);
    }
    
    public void visit(IndexedFaceSet ifs) {
      if (!isPickable(ifs)) return;
      visit((IndexedLineSet)ifs);

      if (!currentPI.pickFaces) return;      
      
      AABBTree tree = aabbTreeExists.get(ifs);
      if (tree == null) {
    	  // not yet processed
    	  tree = (AABBTree) ifs.getGeometryAttributes(PickUtility.AABB_TREE);
    	  if (tree == null) tree = AABBTree.nullTree;
    	  aabbTreeExists.put(ifs, tree);
      }
      
      localHits.clear();
      
        if (tree == AABBTree.nullTree) {
          BruteForcePicking.intersectPolygons(ifs, metric, path, m, mInv, from, to, localHits);
        } else {
          tree.intersect(ifs, metric, path, m, mInv, from, to, localHits);
        }
        extractHits(localHits);
    }
    
    public void visit(IndexedLineSet ils) {
      if (!isPickable(ils)) return;
      visit((PointSet)ils);
      if (!currentPI.pickEdges) return;

      localHits.clear();

 //     System.err.println("Picking indexed line set "+ils.getName());
       BruteForcePicking.intersectEdges(ils, metric, path, m, mInv, from, to, currentPI.tubeRadius*radiusFactor, localHits);
       extractHits(localHits);
    }

    public void visit(PointSet ps) {
     
      if (!currentPI.pickPoints || !isPickable(ps)) return;

      localHits.clear();

      BruteForcePicking.intersectPoints(ps, metric, path, m, mInv, from, to, currentPI.pointRadius*radiusFactor, localHits);
      extractHits(localHits);        
    }

    private void extractHits(List<Hit> l) {
      for (Hit h : l ) {
    	  if (h.affineCoordinate < 0) {
//    		  System.err.println(SystemProperties.hostname+" rejecting "+h.getPickPath().getLastComponent().getName());
    		  continue;
    	  }
    	  AABBPickSystem.this.hits.add(h);
      	}
    }
  }

	/**
	 * Calculate the segment of the line spanned by <code>from</code> and <code>to</code> which lies within the
	 * viewing frustum.  We assume the from lies on the near plane of the frustum.  <code>from</code> and <code>to</code>
	 * are assumed to be in world coordinates.
	 * @param from
	 * @param to
	 * @param viewer
	 */
  static boolean isPortal = false;
  static {
	  String foo = Secure.getProperty(SystemProperties.ENVIRONMENT);
	  if (foo != null && foo.indexOf("portal") != -1) isPortal = true;
  }
  public static void getFrustumInterval(double[] from, double[] to, Viewer viewer) {
	  if (!CameraUtility.getCamera(viewer).isPerspective() || isPortal)	{
		  //System.err.println(SystemProperties.hostname+"from = "+Rn.toString(from)+" to = "+Rn.toString(to));
		  return;
	  }
			double[] c2w = viewer.getCameraPath().getMatrix(null); //deviceManager.getTransformationMatrix(camera2worldSlot).toDoubleArray(null);
			Camera cam = CameraUtility.getCamera(viewer);
			double[] eyeW = Rn.matrixTimesVector(null, c2w, cam.isPerspective() ? P3.originP3 : Pn.zDirectionP3);
			double[] fromndc, tondc;
			Graphics3D gc = new Graphics3D(viewer);
			double[] w2ndc = gc.getWorldToNDC(); //deviceManager.getTransformationMatrix(world2ndcSlot).toDoubleArray(null);
			fromndc = Rn.matrixTimesVector(null, w2ndc, from);
			Pn.dehomogenize(fromndc, fromndc);
			tondc = Rn.matrixTimesVector(null, w2ndc, to);
			Pn.dehomogenize(tondc, tondc);
			double[] v = Rn.subtract(null, tondc, fromndc);
			double zto = v[2];
			// we're assuming here that from[2] == -1 (near clipping plane
			tondc = Rn.linearCombination(null, zto, fromndc, 2, v);
			double[] ndc2w = gc.getNDCToWorld(); //deviceManager.getTransformationMatrix(ndc2worldSlot).toDoubleArray(null);
			Pn.dehomogenize(to, Rn.matrixTimesVector(null, ndc2w, tondc));
	//		System.err.println("fromndc = "+Rn.toString(fromndc));
	//		System.err.println("tondc = "+Rn.toString(Pn.dehomogenize(tondc, tondc))); //tondc));
			// the origin should have negative affine coordinate with respect to the to and from points
			// so that valid pick points have positive affine coordinate
			double[] weights = P3.barycentricCoordinates(null, from, to, eyeW);
			if (weights[0] * weights[1] > 0) {
				Rn.times(to, -1, to);
			}
//			double d1 = Rn.innerProduct(eyeW, from);
//			double d2 = Rn.innerProduct(eyeW, to);
//			if (d1*d2 > 0) {
//				Rn.times(to, -1, to);
//			}
		}

	public static void filterList(HitFilter hf, double[] from, double[] to, List<PickResult> list)	{
		ArrayList<PickResult> rejected = new ArrayList<PickResult>();
		for (PickResult h : list)	{
			if (!hf.accept(from, to, h)) rejected.add(h);
		}
		list.removeAll(rejected);
	}

}
