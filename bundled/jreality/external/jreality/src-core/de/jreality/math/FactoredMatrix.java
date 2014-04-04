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


package de.jreality.math;

import de.jreality.scene.Transformation;
import de.jreality.scene.data.DoubleArray;


/**
 * 
 * <p>
 * The FactoredMatrix class is a subclass of {@link Matrix} supporting a canonical
 * factorization of the matrix into simpler factors. The class provides a
 * variety of methods for setting and getting the transformation. One instance
 * can handle a series of transformations, based on the so-called polar
 * decomposition. See
 * {@link <a href="http://www.cs.wisc.edu/graphics/Courses/838-s2002/Papers/polar-decomp.pdf">
 * Duff and Shoemaker paper</a>}.
 * To be exact, the matrix M is factored as the matrix product M=T*R*S. Note
 * that matrices act on column vectors which stand to the right of the matrix. S
 * is a "stretch" or "scale" matrix -- a diagonal matrix. R is an arbitrary
 * rotation of Euclidean 3-space, and T is a translation.
 * <p>
 * 
 * NOTE: The full polar decomposition according to Duff and Shoemaker includes a second
 * rotation U that conjugates the scaling matrix, i.e., M=TRUSU'.  We assume that U is the
 * identity, which amounts to assuming that M has no shearing component.  This appears to
 * a fairly safe assumption, but be aware that the factorization will be wrong if you have
 * a shearing component.
 * <p>
 * 
 * Users may set the matrix directly, then the factors will be computed and are
 * accessible. Or, the user can set one or more of the factors, and the
 * corresponding matrix is calculated and made available. The update mechanism
 * either decomposes or composes the matrix depending on the type of the most
 * recent "setter" called by the user.
 * <p>
 * 
 * This class is designed to work with any of the classical homogeneous
 * geometries: euclidean, elliptic, or hyperbolic. The variable {\it metric}
 * controls which geometry is active. [Note: Probably should be constructor
 * parameter and not allowed to change].
 * <p>
 * 
 * By default the origin (0,0,0,1) is the fixed point of the scale and rotation
 * part of the transformation. It is however possible to specity another
 * <i>center </i> (see {@link #setCenter(double[], boolean)}. The resulting matrix is then
 * T*C*R*S*IC where C is the translation taking the origin to C, and IC is its
 * inverse. The fixed point for the rotation and stretch is then <i>center </i>.
 * <p>
 * 
 * It is also possible to work with reflections, since any reflection can be
 * factored as T*R*S*G where G is the diagonal matrix {-1,-1,-1,1} (that is,
 * reflection around the origin). A matrix is considered a reflection if its
 * determinant is negative.
 * <p>
 * 
 * The matrix in general belongs to the matrix group GL(4,R). It is also
 * possible to query the matrix to find out if it belongs to the subgroup
 * SL(4,R) of matrices with determinant +/- 1. See {@link #getIsSpecial()}.
 * <p>
 * 
 * It is also possible to factor isometries of non-euclidean space.  In this case 
 * use a constructor that allows specifying the <i>metric</i>.  See {@link Pn} for a
 * more complete description of the non-euclidean isometries.  Then the above remarks
 * should be extended whereever they refer to euclidean metric. 
 * <p>
 * 
 * See also {@link Pn}for a collection of static methods useful for generating
 * 4x4 matrices for specific purposes.
 * <p>
 * 
 * <b>Warning! </b> The matrix is stored as type <code>double[16]</code>, not
 * <code>double[4][4]</code>, due to efficiency concerns arising from the way
 * Java implements multi-dimensional arrays.
 * <p>
 * 
 * <b>Warning </b> Angles are measured in radians.
 * <p>
 * 
 * @author Charles Gunn
 */
public class FactoredMatrix extends Matrix {

    protected double[] translationVector, centerVector, stretchVector,
            rotationAxis, centerMatrix, invCenterMatrix;

    protected Quaternion rotationQ, stretchRotationQ;

    protected boolean factorHasChanged, matrixHasChanged, isFactored,
            isIdentity, isSpecial, isReflection, useCenter;

    private int metric;

    /**
     * Generate a new transform with given metric and matrix If <i>m </i> is
     * null, use identity matrix.
     * 
     * @param metric
     *            See {@link Pn}.
     * @param m
     */
    public FactoredMatrix(int metric, double[] m) {
        super(m);
        this.metric = metric;
        translationVector = new double[4];
        stretchVector = new double[4];
        rotationAxis = new double[3];
        rotationQ = new Quaternion(1.0, 0.0, 0.0, 0.0);
        stretchRotationQ = new Quaternion(1.0, 0.0, 0.0, 0.0);
        matrixHasChanged = true;
        useCenter = false;
        update();
    }

    /**
     * copy constructor
     * 
     * @param metric the metric
     * @param m the matrixc to copy
     */
    public FactoredMatrix(Matrix m, int metric) {
      this(metric, (double[]) m.getArray().clone());
    }
    
    /**
     * copy constructor
     *
     * @param fm the FactoredMatrix to copy
     */
    public FactoredMatrix(FactoredMatrix fm) {
      this(fm, fm.getMetric());
    }
    
    public FactoredMatrix(int metric) {
        this(metric, null);
    }

    public FactoredMatrix(double[] m) {
        this(Pn.EUCLIDEAN, m);
    }

    public FactoredMatrix() {
      this(Pn.EUCLIDEAN, null);
  }

    public FactoredMatrix(DoubleArray da) {
      this(Pn.EUCLIDEAN, da.toDoubleArray(null));
  }

    public FactoredMatrix(Transformation trafo) {
      this(Pn.EUCLIDEAN, trafo.getMatrix());
  }

	public double[] getArray() {
		if (factorHasChanged) update();
		return super.getArray();
	}
	
	public void assignFrom(FactoredMatrix fm)	{
		super.assignFrom(fm);
		metric = fm.getMetric();
		matrixHasChanged = true;
		update();
	}
    /**
     * 
     * @return <code>true</code> if the matrix has negative determinant.
     */
    public boolean getIsReflection() {
        return isReflection;
    }

    /**
     * Set the matrix to be a reflection based on the value of <i>aval </i>.
     * (This is a somewhat questionable method.-cg)
     * 
     * @param aVal
     */
    public void setIsReflection(boolean aVal) {
        if (aVal == isReflection)
            return;
        isReflection = aVal;
        factorHasChanged = true;
        update();
    }

    /**
     * 
     * @return <code>true</code> if the transform has been set to respect a
     *         separate center for its rotation and stretch factors.
     * @see #setCenter(double[]), and introductory remarks on this class.
     *  
     */
    public boolean getUseCenter() {
        return useCenter;
    }

    /**
     * Is the determinant 1 or -1? (Or within {@link FactoredMatrix#TOLERANCE}.
     * 
     * @return
     */
    public boolean getIsSpecial() {
        if (isMatrixHasChanged() || factorHasChanged)
            update();
        return isSpecial;
    }

    /**
     * Invoke {@link #setCenter(double[], boolean)}with the second parameter
     * <code>false</code>.
     * 
     * @param aVec
     */
    public void setCenter(double[] aVec) {
        setCenter(aVec, false);
    }

    /**
     * Set the <i>center </i> of the transformation. See the class description
     * above for a description. If <i>keepMatrix </i> is <code>true</code>,
     * then the value of the transformation will be left unchanged; the
     * <i>translation </i> factor will be adjusted to achieve this effect. If it
     * is not, then the other factors will be left unchanged and the resulting
     * matrix will take on a new value. Side effect:
     * 
     * @param aVec
     *            the position of the center (as a 3-vector or homogeneous
     *            4-vector)
     * @param keepMatrix
     *            whether to preseve the value of the matrix
     */
    public void setCenter(double[] aVec, boolean keepMatrix) {
        if (aVec == null) {
            useCenter = false;
            return;
        }
        useCenter = true;
        if (centerVector == null)
            centerVector = new double[4];
        if (centerMatrix == null)
            centerMatrix = new double[16];
        if (invCenterMatrix == null)
            invCenterMatrix = new double[16];
        centerVector[3] = 1.0;
        System.arraycopy(aVec, 0, centerVector, 0, aVec.length);
        P3.makeTranslationMatrix(centerMatrix, centerVector, metric);
        Rn.inverse(invCenterMatrix, centerMatrix);

        if (keepMatrix) {
            matrixHasChanged = true;
            factorHasChanged = false;
        } else {
            matrixHasChanged = false;
            factorHasChanged = true;
        }
        update();
    }

    /**
     * @return the center vector (as homogeneous 4-vector).
     */
    public double[] getCenter() {
        return centerVector;
    }

    /**
     * Set the translation factor with the three components <i>tx, ty, tz </i>.
     * 
     * @param tx
     * @param ty
     * @param tz
     */
    public void setTranslation(double tx, double ty, double tz) {
//        if (metric != Pn.EUCLIDEAN)
//            throw new IllegalStateException("Transform: setTranslation: Invalid metric");
        translationVector[0] = tx;
        translationVector[1] = ty;
        translationVector[2] = tz;
        translationVector[3] = 1.0;
        factorHasChanged = true;
        update();
    }

    /**
     * Set the translation part of the transform with the vector <i>aTransV
     * </i>. The length of <i>aTransV </i> must be less than or equal to 4.
     * 
     * @param aTransV
     */
    public void setTranslation(double[] aTransV) {
        if (aTransV.length == 4 && metric == Pn.EUCLIDEAN
                && aTransV[3] == 0.0) {
            // TODO: how does that fit to the input validation of the previous method???
            throw new IllegalArgumentException("Invalid euclidean translation");
        }
        int n = Math.min(aTransV.length, 4);
        System.arraycopy(aTransV, 0, translationVector, 0, n);
        System.arraycopy(P3.originP3, n, translationVector, n, 4 - n);
        factorHasChanged = true;
        update();
    }

    /**
     * Get the translation vector for this transform
     * 
     * @return double[4]
     */
    public double[] getTranslation() {
        if (isMatrixHasChanged())
            update();
        return translationVector;
    }

    /**
     * Set the rotation axis of this transformation using the three components
     * <i>(ax, ay, ax) </i>.
     * 
     * @param ax
     * @param ay
     * @param az
     */
    public void setRotationAxis(double ax, double ay, double az) {
        double[] axis = new double[3];
        axis[0] = ax;
        axis[1] = ay;
        axis[2] = az;
        setRotation(getRotationAngle(), axis);
    }

    /**
     * Set the rotation axis of this transformation using the 3-vector <i>axis
     * </i>.
     * 
     * @param axis
     */
    public void setRotationAxis(double[] axis) {
        setRotation(getRotationAngle(), axis);
    }

    /**
     * Set the rotation angle for this transformation.
     * 
     * @param angle
     *            The angle measured in radians.
     */
    public void setRotationAngle(double angle) {
        setRotation(angle, getRotationAxis());
    }

    /**
     * Set the angle and the axis simulataneously.
     * 
     * @param angle
     * @param axis
     */
    public void setRotation(double angle, double[] axis) {
        Quaternion.makeRotationQuaternionAngle(rotationQ, angle, axis);
        factorHasChanged = true;
        update();
    }

    /**
     * Set the angle and the axis (= (ax, ay, az)) simulataneously.
     * 
     * @param angle
     * @param axis
     */
    public void setRotation(double angle, double ax, double ay, double az) {
        double[] axis = new double[3];
        axis[0] = ax;
        axis[1] = ay;
        axis[2] = az;
        setRotation(angle, axis);
    }

    /**
     * Set the rotation for this transformation using the unit quaternion <i>aQ
     * </i>.
     * 
     * @param aQ
     */
    public void setRotation(Quaternion aQ) {
        Quaternion.copy(rotationQ, aQ);
		Quaternion.normalize(rotationQ, rotationQ);
        getRotationAxis();
        factorHasChanged = true;
        update();
    }

    /**
     * 
     * @return double[3] the rotation axis.
     */
    public double[] getRotationAxis() {
        return Rn.normalize(rotationAxis, Quaternion.IJK(rotationAxis,
                rotationQ));
    }

    /**
     * 
     * @return the rotation angle (in radians)
     */
    public double getRotationAngle() {
        double angle = 2.0 * Math.acos(rotationQ.re);
        return angle;
    }

    /**
     * Get the rotation specified as a unit quaternion
     * 
     * @return
     */
    public Quaternion getRotationQuaternion() {
        if (isMatrixHasChanged()) update();
        return rotationQ;
    }

    /**
     * Set the stretch vector associated to this transform using the factor
     * <i>stretch </i> for all three dimensions.
     * 
     * @param stretch
     */
    public void setStretch(double stretch) {
        stretchVector[0] = stretch;
        stretchVector[1] = stretch;
        stretchVector[2] = stretch;
        stretchVector[3] = 1.0;
        factorHasChanged = true;
        update();
    }

    /**
     * Set the stretch factor using the the vector <i>(sx, sy, sz) </i>
     * 
     * @param sx
     * @param sy
     * @param sz
     */
    public void setStretch(double sx, double sy, double sz) {
        stretchVector[0] = sx;
        stretchVector[1] = sy;
        stretchVector[2] = sz;
        stretchVector[3] = 1.0;
        factorHasChanged = true;
        update();
    }

    /**
     * Set the stretch using the 3-vector </i>stretchV </i>.
     * 
     * @param sV
     */
    public void setStretch(double[] stretchV) {
        System.arraycopy(stretchV, 0, stretchVector, 0, Math.min(
                stretchV.length, stretchV.length));
        if (stretchV.length == 3)
            stretchVector[3] = 1.0;
        factorHasChanged = true;
        update();
    }

    /**
     * Return the stretch vector for this transformation. Default: (1,1,1).
     * 
     * @return double[3]
     */
    public double[] getStretch() {
        if (isMatrixHasChanged()) update();
        return stretchVector;
    }

    /**
     * Updates the current state of the transformation.  If a factor was most recently changed, then the
     * matrix is updated.  Otherwise, if the matrix has changed since the last invocation, the factorization
     * is updated.
     *
     */
    public void update() {
        boolean[] isFlipped = new boolean[1];
        double[] MC = new double[16], TTmp;
        if (factorHasChanged) {
            isFlipped[0] = isReflection;
            P3.composeMatrixFromFactors(matrix, translationVector,
                    rotationQ, stretchRotationQ, stretchVector, isReflection,
                    metric);
            if (useCenter) {
                Rn.times(matrix, matrix, invCenterMatrix);
                Rn.times(matrix, centerMatrix, matrix);
            }
        } else if (isMatrixHasChanged()) {
            if (useCenter) { // coule use Rn.conjugate but don't want to recalculate inverse each time ...
                Rn.times(MC, matrix, centerMatrix);
                Rn.times(MC, invCenterMatrix, MC);
                TTmp = MC;
            } else
                TTmp = matrix;
            P3.factorMatrix(TTmp, translationVector, rotationQ,
                    stretchRotationQ, stretchVector, isFlipped, metric);
            isReflection = isFlipped[0];
        }
        isSpecial = Rn.isSpecialMatrix(matrix, TOLERANCE);
        matrixHasChanged = factorHasChanged = super.matrixChanged = false;
    }

    public FactoredMatrix getInverseFactored() {
        return new FactoredMatrix(getMetric(), Rn.inverse(null, matrix));
    }

    // need this to be public in order to, for example, interpolate properly between two
    // existing instances [gunn]
    public int getMetric() {
        return metric;
    }
	
   public boolean isMatrixHasChanged() {
      return matrixHasChanged || super.matrixChanged;
    }
    
    public Matrix getRotation() {
      Matrix m = new Matrix();
      Quaternion.quaternionToRotationMatrix(m.getArray(), getRotationQuaternion());
      return m;
    }

	@Override
	public String toString() {
		String rot = "rotation + "+Rn.toString(getRotationAxis())+" "+getRotationAngle()/(Math.PI);
		String trans = "translation + "+Rn.toString(getTranslation());
		String stretch = "scale + "+Rn.toString(getStretch());
		return "metric="+metric+"\n"+rot+"\n"+trans+"\n"+stretch+"\n";
	}

}
