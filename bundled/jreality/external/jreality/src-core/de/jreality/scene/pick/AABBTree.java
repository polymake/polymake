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


import java.awt.Color;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.Primitives;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.shader.CommonAttributes;

/**
 * An AABB tree for IndexedFaceSets.
 * <p>
 * <b>TODO:</b> the pick algorithm assumes that polygons are konvex...
 * this is easy to change, steal code from tims triangulate non convex poly...
 * </p>
 * @author Steffen Weissmann
 *
 */
public class AABBTree {

	private static final int DEFAULT_POLYS_PER_LEAF=5;
	
    /** The max number of triangles in a leaf. */
    private final int maxPerLeaf;

    /** Left tree. */
    private AABBTree left;

    /** Right tree. */
    private AABBTree right;

    /** Untransformed bounds of this tree. */
    private AABB bounds;

    /** Array of triangles this tree is indexing. */
    private TreePolygon[] tris;

    /** Start and end triangle indexes that this node contains. */
    private int myStart, myEnd;

    /**
     * more or less a hack to indicate that a geometry has no pick tree.
     * AABB pick system uses it to avoid  
     */
    public static AABBTree nullTree = new AABBTree(null, 0, 1, 0);
    
    private AABBTree(TreePolygon[] polygons, int maxPolysPerLeaf, int start, int end) {
      this.maxPerLeaf = maxPolysPerLeaf;
      this.tris = polygons;
      createTree(start, end);
    }
    
    public static AABBTree construct(double[][] coords, int[][] faces) {
    	return construct(coords, faces, DEFAULT_POLYS_PER_LEAF);
    }

    public static AABBTree construct(double[][] coords, int[][] faces, int maxPolysPerLeaf) {
        double[][][] polygons = getMeshAsPolygons(coords, faces);
        return construct(maxPolysPerLeaf, polygons);
    }
    
    public static AABBTree construct(IndexedFaceSet faceSet) {
    	return construct(faceSet, DEFAULT_POLYS_PER_LEAF);
    }

    public static AABBTree construct(IndexedFaceSet faceSet, int maxPolysPerLeaf) {
        double[][][] polygons = getMeshAsPolygons(faceSet);
        return construct(maxPolysPerLeaf, polygons);
    }

	private static AABBTree construct(int maxPolysPerLeaf, double[][][] polygons) {
		TreePolygon[] tris = new TreePolygon[polygons.length];
        for (int i = 0; i < tris.length; i++) {
        	tris[i] = new TreePolygon(polygons[i], i);
        }
        return construct(maxPolysPerLeaf, tris);
	}

	private static AABBTree construct(int maxPolysPerLeaf, TreePolygon[] tris) {
		AABBTree ret = new AABBTree(tris, maxPolysPerLeaf, 0, tris.length-1);
        for (int i = 0; i < tris.length; i++) {
            tris[i].disposeCenter();
        }
        return ret;
	}
    
    private static double[][][] getMeshAsPolygons(IndexedFaceSet faceSet) {
      int numFaces = faceSet.getNumFaces();
      double[][][] ret = new double[numFaces][][];
      IntArrayArray faces = faceSet.getFaceAttributes(Attribute.INDICES).toIntArrayArray();
      DoubleArrayArray verts = faceSet.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
      for (int i = 0; i < numFaces; i++) {
        IntArray face = faces.getValueAt(i);
        int faceLength = face.getLength();
        ret[i] = new double[faceLength][];
        for (int j = 0; j < faceLength; j++) {
          DoubleArray vertex = verts.getValueAt(face.getValueAt(j));
          ret[i][j] = vertex.toDoubleArray(null);
        }
      }
      return ret;
    }

    private static double[][][] getMeshAsPolygons(double[][] coords, int[][] faces) {
        int numFaces = faces.length;
        double[][][] ret = new double[numFaces][][];
        for (int i = 0; i < numFaces; i++) {
          int[] face = faces[i];
          int faceLength = face.length;
          ret[i] = new double[faceLength][];
          for (int j = 0; j < faceLength; j++) {
            ret[i][j] = coords[face[j]];
          }
        }
        return ret;
      }

    /**
     * Creates an OBB tree recursivly from the tris's array of triangles.
     *
     * @param start
     *            The start index of the tris array, inclusive.
     * @param end
     *            The end index of the tris array, exclusive.
     */
    private void createTree(int start, int end) {
    	if (start > end) return;
        myStart = start;
        myEnd = end;
  			bounds = new AABB();
        bounds.compute(tris, start, end);
        if (end - start < maxPerLeaf) return;
        else {
          splitTris(start, end);
					int half = (start + end) / 2;
          this.left = new AABBTree(tris, maxPerLeaf, start, half);
					if (half < end) this.right = new AABBTree(tris, maxPerLeaf, half + 1, end);
        }
    }

     /**
     * Stores in the given array list all indexes of triangle intersection
     * between this tree and a given ray.
     *
     * @param ray
     *            The ray to test this tree against.
     * @param hits
     *            The arraylist to hold indexes of this OBBTree's triangle
     *            intersections.
     * @deprecated
     */
    void intersect(IndexedFaceSet ifs, int metric, SceneGraphPath path, double[] from, double[] to, List<Hit> hits) {
    	Matrix m=new Matrix();
    	Matrix mInv=new Matrix();

    	path.getMatrix(m.getArray());
    	path.getInverseMatrix(mInv.getArray());
    	intersect(ifs, metric, path, m, mInv, from, to, hits);
    }
    
    void intersect(IndexedFaceSet ifs, int metric, SceneGraphPath path, Matrix m, Matrix mInv, double[] from, double[] to, List<Hit> hits) {
     
      double[] fromLocal=mInv.multiplyVector(from);
      double[] toLocal=mInv.multiplyVector(to);
      double[] dir = toLocal.length==3 || toLocal[3]==0 ? toLocal : Rn.subtract(null, toLocal, fromLocal);
      
      if (!bounds.intersects(fromLocal, dir)) {
        return;
      }
      if (left != null) {
        left.intersect(ifs, metric, path,m, mInv, from, to, hits);
      }
  
      if (right != null) {
        right.intersect(ifs, metric, path, m, mInv, from, to, hits);
      } else if (left == null) { // left == right == null
        double[] p1=new double[4], p2=new double[4], p3=new double[4], pobj=new double[4];
        p1[3]=p2[3]=p3[3]=1;
        TreePolygon tempt;
        for (int i = myStart; i <= myEnd; i++) {
          tempt = tris[i];
          for (int j = 0; j < tempt.getNumTriangles(); j++) {
            tempt.getTriangle(j, p1, p2, p3);
            if (BruteForcePicking.intersects(pobj, fromLocal, toLocal, p1, p2, p3, null)) {
              double[] pw = m.multiplyVector(pobj);
              hits.add(new Hit(path.pushNew(ifs), pw, Pn.distanceBetween(from, pw,metric), 
            		  P3.affineCoordinate(from, to, pw), PickResult.PICK_TYPE_FACE, tempt.getIndex(),j));
//              System.err.println("AABB polygon hit");
            }
          }
        }
      }
    }

    /**
     * Splits the root obb acording to the largest bounds extent.
     *
     * @param start
     *            Start index in the tris array, inclusive, that is the OBB to
     *            split.
     * @param end
     *            End index in the tris array, exclusive, that is the OBB to
     *            split.
     */
    private void splitTris(int start, int end) {
        if (bounds.extent[0] > bounds.extent[1]) {
            if (bounds.extent[0] > bounds.extent[2])
                sort(start, end, 0);
            else
                sort(start, end, 2);
        } else {
            if (bounds.extent[1] > bounds.extent[2])
                sort(start, end, 1);
            else
                sort(start, end, 2);
        }
    }

    /**
    *
    * <code>sort</code> sorts the bounds of the tree.
    *
    * @param start
    *            the start index of the triangle list.
    * @param end
    *            the end index of the triangle list.
    */
   private void sort(int start, int end, int index) {
     double[] tmp=null;
     for (int i = start; i < end; i++) {
         tmp = Rn.subtract(tmp, tris[i].centroid, bounds.center);
         tris[i].projection = tmp[index];
     }
     Arrays.sort(tris, start, end, treeCompare);
       Arrays.sort(tris, start, end, treeCompare);
   }

    /**
     * This class is simply a container for a triangle.
     */
    static class TreePolygon {

        private double[][] verts;

        private double projection;

        private int index;

        double[] centroid;

        TreePolygon(double[][] verts, int index) {
          this.verts = verts;
          // handle 4-vectors
          if (verts[0].length == 4)	
        	  this.verts = Pn.dehomogenize(new double[verts.length][3], verts);
           this.index=index;
           int count = verts.length;
					centroid = Rn.copy(null, verts[0]);
					for (int i = 1; i < count; i++) Rn.add(centroid, centroid, verts[i]);
					Rn.times(centroid, 1./count, centroid);
        }

        void disposeCenter() {
        	centroid=null;
        }
        
        int getNumTriangles() {
          return verts.length-2;
        }
        
        void getTriangle(int i, double[] p1, double[] p2, double[] p3) {
          System.arraycopy(verts[0], 0, p1, 0, 3);
          System.arraycopy(verts[i+1], 0, p2, 0, 3);
          System.arraycopy(verts[i+2], 0, p3, 0, 3);
        }
        
        double[][] getVertices() {
          return verts;
        }

        public int getIndex() {
          return index;
        }
        
    }

    /**
     * Class to sort TreeTriangle acording to projection.
     */
    private Comparator<TreePolygon> treeCompare = new Comparator<TreePolygon>() {

        public int compare(TreePolygon a, TreePolygon b) {
            if (a.projection < b.projection) { return -1; }
            if (a.projection > b.projection) { return 1; }
            return 0;
        }
    };
    
	/**
	 * this is only for debugging and might be removed in future.
	 * @return A component that contains the AABBs of the tree as
	 * IndexedLineSets.
	 */
	public SceneGraphComponent display() {
		SceneGraphComponent cmp = new SceneGraphComponent();
		Appearance app = new Appearance();
		app.setAttribute("showPoints", false);
		app.setAttribute("showLines", true);
		app.setAttribute("showFaces", false);
		cmp.setAppearance(app);
		display(cmp, Color.BLUE, Color.RED, true, (bounds.extent[0]+bounds.extent[1]+bounds.extent[2])*0.003, 0.99);
		return cmp;
	}

	void display(SceneGraphComponent parent, Color leftColor, Color rightColor, boolean isLeft, double radius, double factor) {
		if (left != null) left.display(parent, leftColor.brighter(), rightColor.brighter(), true, radius*factor, factor);
		if (right != null) right.display(parent, leftColor.darker(), rightColor.darker(), false, radius*factor, factor);
		else if (left == null && right == null) { // leaf
			SceneGraphComponent myComp = new SceneGraphComponent();
			double[] t = bounds.center;
			double[] s = bounds.extent;
			MatrixBuilder.euclidean().translate(t).getMatrix().assignTo(myComp);
			IndexedFaceSet box = Primitives.box(2*s[0], 2*s[1], 2*s[2], false);
			myComp.setGeometry(box);
			IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(box);
			IndexedFaceSetUtility.calculateAndSetNormals(box);
			Appearance a = new Appearance();
			a.setAttribute("lineShader.diffuseColor", isLeft ? leftColor : rightColor);
			a.setAttribute("lineShader."+CommonAttributes.TUBE_RADIUS, radius);
			myComp.setAppearance(a);
			parent.addChild(myComp);
		}
	}
}
