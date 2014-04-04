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


package de.jreality.util;

import java.awt.geom.Rectangle2D;
import java.util.logging.Level;

import de.jreality.math.Pn;
import de.jreality.math.Rn;
/**
	* A Rectangle3D represents a rectangular parallelopiped in three dimensional space: a "box".
	* <p>
	* It exists  primarily to provide bounding boxes for 3D geometry. 
	* <p>
	* @author Charles Gunn
	* 
	* @see java.awt.geom.Rectangle2D
	* 
	* TODO: add dirty flag to avoid unnecessary recalculation of extent and center
*/
public class Rectangle3D {
	public static final Rectangle3D EMPTY_BOX, unitCube;
	double[][] bounds;
	double[] center, extent;
	//private boolean isEmpty;
	static	{
		EMPTY_BOX = new Rectangle3D();
		unitCube = new Rectangle3D();
		double[][] bds = new double[2][3];
		bds[0][0] = bds[0][1] = bds[0][2] = -1.0;
		bds[1][0] = bds[1][1] = bds[1][2] = 1.0;
		unitCube.setBounds(bds);
		unitCube.update();
	}
	// for now only allow 4-dim bounds (actually 3dim w/ homog coords)
	public Rectangle3D()
	{
		bounds = new double[2][3];
		center = new double[3];
		extent = new double[3];
		this.initialize();
	}
	
	public Rectangle3D(double w, double h, double d)	{
		this();
		bounds[0][0] = -w/2;
		bounds[0][1] = -h/2;
		bounds[0][2] = -d/2;
		bounds[1][0] = w/2;
		bounds[1][1] = h/2;
		bounds[1][2] = d/2;
	}
	public Rectangle3D(double[][] vlist)	{
		//assert dim checks
		this();
		this.computeFromVectorList(vlist);
	}

	public void initialize()	{
		// TODO should avoid reallocating if right size, and just copy over the default values
		Rn.setToValue(bounds[0], Double.MAX_VALUE);
		Rn.setToValue(bounds[1], -Double.MAX_VALUE);
	}
	
	// TODO test this method!
	public void copyInto(Rectangle3D bb)	{
		System.arraycopy(bounds[0], 0, bb.bounds[0], 0, bounds[0].length);
		System.arraycopy(bounds[1], 0, bb.bounds[1], 0, bounds[1].length);
		bb.update();
	}
	
	public String toString()	{
		StringBuffer sb = new StringBuffer();
		if (isEmpty())	{
			sb.append("IsEmpty");
			return sb.toString();
		}

		sb.append("Min:\t"+Rn.toString(bounds[0])+"\n");
		sb.append("Max:\t"+Rn.toString(bounds[1])+"\n");
		sb.append("Center:\t"+Rn.toString(center)+"\n");
		sb.append("Extent:\t"+Rn.toString(extent)+"\n");
		return sb.toString();
	}
	
	public boolean isEmpty()	{
		return getMinX() > getMaxX() || getMinY() > getMaxY() || getMinZ() > getMaxZ();
	}
	
	public void update()	{
		// assert checks
		if (isEmpty()) return;
		Rn.linearCombination(center, .5, bounds[0], .5, bounds[1]);
		Rn.subtract(extent, bounds[1], bounds[0]);		 //max - min
	}
	
	/**
	  *  Transform a bounding box by a matrix.
	  * The corners of the box are transformed and the bounding box of the
	  * resulting 8 vertices is computed.
	  * If <i>flag</i> is <code>true</code>, then return a new instance containing the result,
	  * else overwrite <code>this</code>.
	 * 
	 * @param aTransform
	 * @param copyflag
	 * @return
	 */
	public  Rectangle3D transformByMatrix(Rectangle3D target, double[] aTransform)	{
		double[][] cube = new double[8][3], tcube = new double[8][3];
		if (target == null)  target = new Rectangle3D();
		// create a cube with our bounds 
		for (int i=0; i<2; ++i)	
			for (int j=0; j<2; ++j)
				for (int k=0; k<2; ++k)		{
					cube[i*4+j*2+k][0] = bounds[i][0];
					cube[i*4+j*2+k][1] = bounds[j][1];
					cube[i*4+j*2+k][2] = bounds[k][2];
			}

		Rn.matrixTimesVector(tcube, aTransform,  cube);
		for (int i= 0; i<8; ++i)	LoggingSystem.getLogger(this).log(Level.FINER,Rn.toString(tcube[i]));
		//Pn.dehomogenize(tcube, tcube);
		Rn.calculateBounds( target.bounds, tcube); 
		
		// was: target.isEmpty = false;
		// i think this makes more sense... [steffen]
	
		target.update();
		return target;
	}

	/**
	 * 	Receiver configures himself as bounding box of the
	  * vertices in the list <i>vlist</i>.  The vectors can be either 3-vectors or homogeneous 4-vectors.
	  * In the latter case the input vectors are dehomogenized before the bounds are calculated.
	 * @param vlist
	 * @return
	 */
	public Rectangle3D computeFromVectorList(double[][] vlist)	{
		// we need to pay attention to homogeneous coordinates here
		if (vlist[0].length == 3) 		Rn.calculateBounds(bounds, vlist);  
		else if (vlist[0].length == 4) 	Pn.calculateBounds(bounds, vlist);
		else throw new IllegalArgumentException("computeFromVectorList: invalid vlist dimension");

		update();
		return this;
	}

	/**
	 * Finds the union of the receiver and <i>aBound</i>, places
	  * result in <i>target</i> and returns it.  If <i>target</i> is null, allocates new instance.
	  * Behaves correctly if either or both of union-ands are empty.
	 * @param aBound
	 * @param target
	 * @return
	 */
	public Rectangle3D unionWith(Rectangle3D aBound, Rectangle3D target)	{
		if (target == null)	{
			target = this;
		}

		if ( isEmpty() && !aBound.isEmpty())	{
			System.arraycopy(aBound.bounds[0], 0, bounds[0], 0, 3);
			System.arraycopy(aBound.bounds[1], 0, bounds[1], 0, 3);
			target.update();
			return target;
		} else if ( !isEmpty() && aBound.isEmpty())	{
			System.arraycopy(bounds[0], 0, target.bounds[0], 0, 3);
			System.arraycopy(bounds[1], 0, target.bounds[1], 0, 3);
			target.update();
			return target;
		} else if ( isEmpty() && aBound.isEmpty() ) 	{
			return target;
		}
		Rn.min(target.bounds[0],aBound.bounds[0], bounds[0]);
		Rn.max(target.bounds[1],aBound.bounds[1], bounds[1]);
		target.update();
		return target;
	}

	/**
	 * Get the center of this box.
	 * @return	double[3]
	 */public double[] getCenter()	
	{
		update();
		return (double[]) center.clone();
	}

	/**
	 * Get the dimensions of the box (length, width, depth).
	 * @return	double[3]
	 */public double[] getExtent()
	{
		update();
		return (double[]) extent.clone();
	}

	 public double getMaxExtent()	{
		 double max = Math.max( Math.max(extent[0], extent[1]), extent[2]);
		 return max;
	 }
	/**
	 * Project this box onto its first two dimensions
	 * @param rec
	 * @return
	 */
	 public Rectangle2D convertToRectangle2D(Rectangle2D rec)	{
		Rectangle2D screenExtent;
		if (rec == null) screenExtent = new Rectangle2D.Double();
		else screenExtent = rec;
		screenExtent.setFrameFromDiagonal(bounds[0][0],bounds[1][0],bounds[1][0],bounds[1][1]);		
		return screenExtent;
	}

		/**
		 * Get the two opposite corners of this box (min(x,y,z) and max(x,y,z)).
		 * @return	double[2][3]
		 */
		public double[][] getBounds() {
			return (double[][]) bounds.clone();
		}

		/**
		 * Get the two opposite corners of this box (min(x,y,z) and max(x,y,z)).
		 * @return	double[2][3]
		 */
		public void setBounds(double[][] b) {
			if (b.length != 2 || b[0].length != 3) return;
			bounds = (double[][])b.clone();
			update();
		}

    public double getMinX() {
        return bounds[0][0];
    }

    public double getMaxX() {
        return bounds[1][0];
    }

    public double getMinY() {
        return bounds[0][1];
    }

    public double getMaxY() {
        return bounds[1][1];
    }

    public double getMinZ() {
        return bounds[0][2];
    }

    public double getMaxZ() {
        return bounds[1][2];
    }
    
    public double[] getCenter(double[] store) {
        if (store == null) return getCenter();
        update();
        System.arraycopy(center, 0, store, 0, 3);
        return store;
    }

    public double[] getExtent(double[] store) {
        if (store == null) return getExtent();
        update();
        System.arraycopy(extent, 0, store, 0, 3);
        return store;
    }

    /**
     * 
     * @param store double[2][3]
     * @return 
     */
    public double[][] getBounds(double[][] store) {
        if (store == null) return getBounds();
        System.arraycopy(bounds[0], 0, store[0], 0, 3);
        System.arraycopy(bounds[1], 0, store[1], 0, 3);
        return store;
    }

	public void scale(double d) {
		
		for (int i =0; i<3; ++i)
			for (int j = 0; j<2; ++j)	bounds[j][i] *= d;
		
	}

	public void add(double d) {
		
		for (int i =0; i<3; ++i)
			for (int j = 0; j<2; ++j)	bounds[j][i] += (j == 0 ? -d : d);
		
	}
	public boolean contains(Rectangle3D b2) {
		for (int i = 0; i<3; ++i)	{
			if (bounds[0][i] > b2.bounds[0][i]) return false;
			if (bounds[1][i] < b2.bounds[1][i]) return false;
		}
		return true;
	}

}

