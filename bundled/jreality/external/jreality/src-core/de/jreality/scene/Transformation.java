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


package de.jreality.scene;

import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationEventMulticaster;
import de.jreality.scene.event.TransformationListener;
/**
 * A simple transformation class which wraps a 4x4 real matrix. Access is read-only.
 * When contained as a field in an instance of {@link SceneGraphComponent},
 * this transformation is applied to any geometry contained in the component as well as 
 * to all children.
 * <p>
 * For generating and manipulating matrices meeting specific constraints
 *  (isometries, etc.)
 * see {@link P3}, {@link de.jreality.math.MatrixBuilder MatrixBuilder} and
 * {@link de.jreality.math.FactoredMatrix}.
 * <p>
 * For other support, see {@link de.jreality.util.DefaultMatrixSupport}.
 * @author Charles Gunn, Steffen Weissman
 */
public class Transformation extends SceneGraphNode {

  private transient TransformationEventMulticaster transformationListener = new TransformationEventMulticaster();
  
  protected double[] theMatrix;
  
  private transient boolean matrixChanged;

  private static int UNNAMED_ID;
  
	/**
	 * Generate a new transform with given matrix
	 * If <i>m</i> is null, use identity matrix.  
	 * @param metric		See {@link Pn}.
	 * @param m
	 */
	public Transformation(String name, double[] m) {
		super(name);
		if (m == null)	theMatrix = Rn.identityMatrix(4);
		else theMatrix = (double[]) m.clone();
	}
	
	public Transformation(double[] matrix) {
		this("trafo "+(UNNAMED_ID++), matrix);
	}
	
	public Transformation(String name) {
		this(name, null);
	}

	public Transformation()	{
		this((double[]) null);
	}

	/**
	 * A copy constructor.
	 * @param t
	 * @deprecated use <code>new Transformation(oldTrafo.getMatrix())</code> instead
	 */
	public Transformation(Transformation t)	{
		this(t.theMatrix);
	}
	
	/**
	 * 
	 * @return	a copy of the current matrix
	 */
	public double[] getMatrix() {
		return getMatrix(null);
	}

	/**
	 * Copy the current matrix into <i>aMatrix</i> and return it.
	 * @param aMatrix
	 * @return	the filled in matrix
	 */
   public double[] getMatrix(double[] aMatrix) {
     startReader();
     try {
  		if (aMatrix!= null &&  aMatrix.length != 16) {
  			throw new IllegalArgumentException("lenght != 16");
  		}
  	 	if (aMatrix == null) aMatrix = new double[16];
  	 	System.arraycopy(theMatrix, 0, aMatrix, 0, 16);
  	 	return aMatrix;
     } finally {
       finishReader();
     }
	}

	/**
	 * Assign <i>aMatrix</i> to this Transformation.
	 * @param aMatrix
	 */
	public void setMatrix(double[] aMatrix) {
		checkReadOnly();
    startWriter();
    try {
  		System.arraycopy(aMatrix, 0, theMatrix, 0, aMatrix.length);
      fireTransformationChanged();
    } finally {
      finishWriter();
    }
	}

	public void multiplyOnRight(double[] T) {
		startWriter();
		try {
			Rn.times(theMatrix, theMatrix, T);
			fireTransformationChanged();
		} finally {
			finishWriter();
		}
	}

	public void multiplyOnLeft(double[] T) {
		startWriter();
		try {
			Rn.times(theMatrix, T, theMatrix);
			fireTransformationChanged();
		} finally {
			finishWriter();
		}
	}


	public void addTransformationListener(TransformationListener listener) {
    startReader();
		transformationListener.add(listener);
    finishReader();
	}
	public void removeTransformationListener(TransformationListener listener) {
    startReader();
		transformationListener.remove( listener);
    finishReader();
	}

	/**
	 * Tell the outside world that this transformation has changed.
	 * This methods takes no parameters and is equivalent
	 * to "everything has/might have changed".
	 */
	protected void writingFinished() {
	  if (matrixChanged && transformationListener != null) 
	      transformationListener.transformationMatrixChanged(new TransformationEvent(this));
	  matrixChanged=false;
	};
	  
	protected void fireTransformationChanged() {
	  matrixChanged=true;
	}
  
	public static void superAccept(Transformation t, SceneGraphVisitor visitor) {
		t.superAccept(visitor);
	}

	private void superAccept(SceneGraphVisitor v) {
	  super.accept(v);
	}

	public void accept(SceneGraphVisitor v)	{
	    startReader();
	    try {
	      v.visit(this);
	    } finally {
	      finishReader();
	    }
	}

}
