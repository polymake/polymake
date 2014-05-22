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

import java.io.Serializable;

import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Quaternion;
import de.jreality.math.Rn;

/**
 * I've put this class in here to be able to compare the results from the new class FactoredMatrix
 * with the reults from the former Transformation class.
 * 
 * This class will be removed soon!
 *
 * @deprecated
 * 
 * */
class OldTransformation implements Serializable {
  protected double[] theMatrix,       // the matrix transform
      defaultMatrix,
      translationVector,  
      centerVector,
      stretchVector,
      rotationAxis,
      centerMatrix, 
      invCenterMatrix;
  protected Quaternion rotationQ, 
      stretchRotationQ;
  protected int metric;
  protected boolean factorHasChanged,
      matrixHasChanged,
      isFactored,
      isIdentity,
      isSpecial,
      isReflection,
      isEditable,
      useCenter,
      doFactor;
  static final double TOLERANCE = 10E-8;
  
  /**
   * Generate a new transform with given metric and matrix
   * If <i>m</i> is null, use identity matrix.  
   * @param metric   See {@link Pn}.
   * @param m
   */
  public OldTransformation(int metric, double[] m) {
    super();
    // TODO need to consider a clone() method
    if (m == null)  {
      theMatrix = Rn.identityMatrix(4);
      isIdentity = isSpecial = true;    
    }
    else      {
      theMatrix = (double[]) m.clone();
      matrixHasChanged = true;
    } 
    defaultMatrix = null;
    translationVector = new double[4];
    stretchVector = new double[4];
    rotationAxis = new double[3];
    rotationQ = new Quaternion(1.0, 0.0, 0.0, 0.0);
    stretchRotationQ = new Quaternion(1.0, 0.0, 0.0, 0.0);
    matrixHasChanged = true;
    isEditable = true;
    useCenter = false;
    doFactor = true;
    this.metric = metric;
    update();
  }

  public OldTransformation(int metric)  {
    this(metric, null);
  }
  
  public OldTransformation(double[] m) {
    this(Pn.EUCLIDEAN, m);
  }
  
  public OldTransformation() {
    this(Pn.EUCLIDEAN, null);
  }

  public OldTransformation getInverse()  {
    OldTransformation inv;
    try {
      inv = (OldTransformation) this.clone();
    } catch (CloneNotSupportedException e) {
      // TODO Auto-generated catch block
      e.printStackTrace();
      return null;
    }
    double[] invM = new double[16];
    Rn.inverse(invM, inv.getMatrix());
    inv.setMatrix(invM);
    inv.update();
    return inv;
  }
  /**
   * Reset the matrix to the currently stored default matrix. See {@link #setDefaultMatrix()}.
   *
   */public  void resetMatrix()
  {
    if (defaultMatrix == null) Rn.setIdentityMatrix(theMatrix);
    else System.arraycopy(defaultMatrix, 0, theMatrix,0, theMatrix.length);
    matrixHasChanged = true;
    update();
  }


  /**
   * Copies the current matrix into the default matrix, so it can be restored if necessary (see {@link #resetMatrix()}.
   *
   */public void setDefaultMatrix()
  {
    defaultMatrix = ((double[]) theMatrix.clone());
  }

  /**
   * Sets the default matrix for this Transformation to the contents of <i>aMatrix</i>.
   * @param aMatrix
   */public void setDefaultMatrix(double[] aMatrix)
  {
    System.arraycopy(aMatrix, 0, defaultMatrix,0, theMatrix.length);
  }

  /**
   * 
   * @return  the current default matrix
   */public double[] getDefaultMatrix()
  {
    return defaultMatrix;
  }

  /**
   * 
   * @return  a copy of the current matrix
   */public double[] getMatrix()
  {
    return getMatrix(null);
  }

  /**
   * Copy the current matrix into <i>aMatrix</i> and return it.
   * @param aMatrix
   * @return  the filled in matrix
   */public double[] getMatrix(double[] aMatrix)
  {
    if (aMatrix!= null &&  aMatrix.length != 16) {
      System.err.println("Invalid argument");
      return null;
    }
    double[] copy = null;
    if (aMatrix == null) copy = new double[16];
    else copy = aMatrix;
    synchronized(theMatrix) {
      System.arraycopy(theMatrix, 0, copy, 0, 16);
    }
    return copy;
  }

  /**
   * Copy the contents of <i>aMatrix</i> into the current matrix.
   * @param aMatrix
   */
  public void setMatrix(double[] aMatrix)
  {
    if (!isEditable) return;
    // sometimes this is just used to register the change
    synchronized(theMatrix) {
      System.arraycopy(aMatrix, 0, theMatrix, 0, aMatrix.length);
      matrixHasChanged = true;
      update();
    }     
  }

  /**
   * 
   * Invoke {@link #multiplyOnRight(double[])} on the matrix attached to <i>aTform</i>.
   * @param aTform
   */
  public void multiplyOnRight( OldTransformation aTform )
  {
    synchronized(theMatrix) {
      concatenate(aTform.getMatrix(), true);      
    }
  }
    
  /**
   * Invoke {@link #multiplyOnLeft(double[])} on the matrix attached to <i>aTform</i>.
   * @param aTform
   */public void multiplyOnLeft( OldTransformation aTform )
  {
    synchronized(theMatrix) {
      concatenate(aTform.getMatrix(), false);     
    }
  }
    
  /**
   * Let M be the current matrix. Then form the matrix product M*T and store it in M.
   * 
   * @param aMatrix
   */
  public void multiplyOnRight( double[] T)
  {
    synchronized(theMatrix) {
      concatenate(T, true);
    }
  }
    
  /**
   * Let M be the current matrix. Then form the matrix product T*M and store it in M.
   * @param aMatrix
   */public void multiplyOnLeft( double[] T)
  {
    synchronized(theMatrix) {
      concatenate(T, false);
    }
  }
    
  private void concatenate(double[] aMatrix , boolean onRight)
    {
    if (!isEditable) return;
    synchronized(theMatrix) {
      if (onRight)  Rn.times(theMatrix,theMatrix, aMatrix);
      else   Rn.times(theMatrix,aMatrix, theMatrix);
      matrixHasChanged = true;
      update();     
    }
  }

  /**
   * 
   * @return  <code>true</code> if the matrix has negative determinant.
   */public boolean getIsReflection()
  {
    //return Rn.determinant(theMatrix) < 0.0 ;
    return isReflection;
  }

  /**
   * Set the matrix to be a reflection based on the value of <i>aval</i>. (This is a somewhat questionable method.-cg)
   * @param aVal
   */public void setIsReflection(boolean aVal)
  {
    if (!isEditable) return;
    synchronized(this)  {
      if (aVal == isReflection) return;
      isReflection = aVal;
      factorHasChanged = true;
      update();
      System.out.println("IsReflection ="+isReflection);      
    }
  }

  /**
   * 
   * @return  <code>true</code> if the transform has been set to respect a separate center for its rotation and 
   * stretch factors. @see #setCenter(double[]), and introductory remarks on this class.
   * 
   */public boolean getUseCenter()
  {
    return useCenter;
  }

  /**
   * Set whether the transform uses a separate center for rotation and stretch factors.  @see #setCenter(double[]).
   * @param aVal
   * @deprecated  Use {@link #setCenter(double[]) with null argument to turn off using center.
   */
  public void setUseCenter(boolean aVal)
  {
    if (!isEditable) return;
    if (centerVector == null) {
      //System.err.println("Transform: setUseCenter: First set center Vector");
      useCenter = false;
      return;
    }
    if (useCenter == aVal) return;
    useCenter = aVal;
    factorHasChanged = true;
    if (useCenter)  {
      if (centerMatrix == null) centerMatrix = new double[16];
      if (invCenterMatrix == null)  invCenterMatrix = new double[16];
    }
    update();
  }


  /**
   * 
   * @return  <code>true</code> if this instance is editable.
   */
  public boolean getIsEditable()
  {
    return isEditable;
  }

  /**
   * Set whether this transform can be edited.  Default: true.
   * @param aVal
   */public void setIsEditable(boolean aVal)
  {
    synchronized(this) {
      isEditable = aVal;
    }
  }

  /**
   * Is this transformation the identity?
   * @return
   * @deprecated
   */public boolean getIsIdentity()
  { 
    if (Rn.isIdentityMatrix(theMatrix, TOLERANCE) ) isIdentity = true;
    else isIdentity = false;
    return isIdentity;
  }
  
  /**
   * @return  <code>true</code> if the matrix is to be factored.  Default: true.
   */public boolean getDoFactor() 
  { 
    return doFactor; 
  }

  /**
   * Sets whether the matrix is to be factored into its factors. See {@link Transformation}.
   * @param aVal
   */public void setDoFactor( boolean aVal)
  {
    synchronized(this)  {
      doFactor = aVal;
      // this can also be used to request updating of factorization
      if (doFactor) update(); 
    }
  }

  /**
   * Is the determinant 1 or -1? (Or within {@link Transformation#TOLERANCE}.
   * @return
   */public boolean getIsSpecial()
  {
    if ( matrixHasChanged || factorHasChanged) update();
    return isSpecial;
  }

  /**
   * See {@link Pn}, {@link Pn#ELLIPTIC}, {@link Pn#EUCLIDEAN}, and {@link Pn#HYPERBOLIC}.
   * @return  the metric metric
   */public int getMetric()  
  {
    return metric;
  }
  
  /**
   * Sets the metric metric of this transform. See {@link Pn}.
   * @param aSig
   */public void setMetric( int aSig)
  {
    if (!isEditable)  return;
    synchronized(this)  {
      if (metric == aSig)  return;
      metric = aSig;
      System.out.println("Changing metrics is dangerous:");// resetting to identity");
      //setMatrix(Rn.identityMatrix(4));
      matrixHasChanged = true;
      update();
    }
  }


  /**
   * Invoke {@link #setCenter(double[], boolean)} with the second parameter <code>false</code>.
   * @param aVec
   */public void setCenter( double[] aVec)
  {
      setCenter(aVec, false);
  }

  /**
   * Set the <i>center</i> of the transformation.  See the class description above for
  * a description.  If <i>keepMatrix</i> is <code>true</code>, then the value of the transformation will
  * be left unchanged; the <i>translation</i> factor will be adjusted to achieve 
  * this effect.  If it is not, then the other factors will be left unchanged 
  * and the resulting matrix will take on a new value.
  * Side effect:  {@link #setUseCenter(boolean)} is called with parameter <code>true</code>.
   * @param aVec      the position of the center (as a 3-vector or homogeneous 4-vector)
   * @param keepMatrix  whether to preseve the value of the matrix
   */
  public void setCenter(double[] aVec, boolean keepMatrix)
  {
    if (!isEditable)  return;
    synchronized(this)  {
      if (aVec == null) {useCenter = false; return;}
      useCenter = true;
      if (centerVector == null)     centerVector = new double[4];
      if (centerMatrix == null)     centerMatrix = new double[16];
      if (invCenterMatrix == null)    invCenterMatrix = new double[16];
      centerVector[3] = 1.0;
      System.arraycopy(aVec,0,centerVector,0,aVec.length);
      P3.makeTranslationMatrix(centerMatrix, centerVector, metric);
      Rn.inverse(invCenterMatrix, centerMatrix);

      if (keepMatrix) {
        matrixHasChanged = true;
        factorHasChanged = false;
      }else {
        matrixHasChanged = false;
        factorHasChanged = true;
      }
      update();     
    }
  }

  /**
   * @return   the center vector (as homogeneous 4-vector).
   */public double[]  getCenter()
  {
    return centerVector;
  }

  /**
   * Set the translation factor with the three components <i>tx, ty, tz</i>.
   * @param tx
   * @param ty
   * @param tz
   */public void setTranslation( double tx, double ty, double tz)
  {
    if (!isEditable)  return;
    synchronized(this)  {
      if (metric != Pn.EUCLIDEAN) {
        System.err.println("Transform: setTranslation: Invalid metric");
        return;
      }
      //ASSERT( aTransV, OE_NULLPTR, OE_DEFAULT, "", return nil);
      translationVector[0] = tx;
      translationVector[1] = ty;
      translationVector[2] = tz;
      translationVector[3] = 1.0;
      factorHasChanged = true;
      update();     
    }
  }

  /**
   * Set the translation part of the transform with the vector <i>aTransV</i>. The length of <i>aTransV</i> 
   * must be less than or equal to 4. 
   * @param aTransV
   */public void setTranslation( double[]  aTransV)
  {
    if (!isEditable)  return;
    //ASSERT( aTransV, OE_NULLPTR, OE_DEFAULT, "", return nil);
    synchronized(this)  {
      int n = Math.min(aTransV.length, 4);
      System.arraycopy(aTransV,0,translationVector,0, n);
      System.arraycopy(P3.originP3, n, translationVector, n, 4-n);
      factorHasChanged = true;
      update();     
    }
  }

  /**
   * Get the translation vector for this transform
   * @return    double[4]
   */public double[]  getTranslation()
  {
    if ( matrixHasChanged) update();
    return translationVector;
  }

  /**
   * Set the rotation axis of this transformation using the three components <i>(ax, ay, ax)</i>.
   * @param ax
   * @param ay
   * @param az
   */public void setRotationAxis( double ax, double ay, double az)
  {
      double[] axis = new double[3];
      axis[0] = ax; axis[1] = ay; axis[2] = az;
      setRotation(getRotationAngle(), axis);
  }

  /**
   * Set the rotation axis of this transformation using the 3-vector <i>axis</i>.
   * @param axis
   */
   public void setRotationAxis( double[]  axis)
  {
    setRotation(getRotationAngle(), axis);
  }

  /**
   * Set the rotation angle for this transformation. 
   * @param angle   The angle measured in radians.
   */public void setRotationAngle( double angle)
  {
    setRotation( angle, getRotationAxis());
  }
     
  /**
   * Set the angle and the axis simulataneously.
   * @param angle
   * @param axis
   */public void setRotation( double angle,double[]  axis ) 
  {
    if (!isEditable)  return;
    synchronized(this)  {
      Quaternion.makeRotationQuaternionAngle(rotationQ, angle, axis);
      factorHasChanged = true;;
      update();
    }
  }

  /**
   * Set the angle and the axis (= (ax, ay, az)) simulataneously.
   * @param angle
   * @param axis
   */public void setRotation( double angle, double ax, double ay, double az)  
  {
    if (!isEditable)  return;
    double[] axis = new double[3];
    axis[0] = ax; axis[1] = ay; axis[2] = az;
    setRotation(angle, axis);
  }

  /**
   * Set the rotation for this transformation using the unit quaternion <i>aQ</i>.
   * @param aQ
   */public void setRotation(Quaternion aQ) 
  {
    if (!isEditable)  return;
    //ASSERT( aQ, OE_NULLPTR, OE_DEFAULT, "", return nil);
    //if ( Quaternion.equalsRotation(rotationQ, aQ, TOLERANCE)) return;
    synchronized(this)  {
      Quaternion.copy(rotationQ, aQ);
      Quaternion.normalize(rotationQ, rotationQ);
      getRotationAxis();
      factorHasChanged = true;
      update();     
    }
  }

  /**
   * 
   * @return  double[3] the rotation axis.
   */public double[]  getRotationAxis()
  {
    return Rn.normalize(rotationAxis,Quaternion.IJK(rotationAxis, rotationQ));
  }
     
     /**
      * 
      * @return the rotation angle (in radians)
      */
  public double getRotationAngle()
  {
    double angle = 2.0 * Math.acos(rotationQ.re);
    return angle;
  }
     
     /**
      * Get the rotation specified as a unit quaternion
      * @return
      */
  public Quaternion getRotationQuaternion()
  {
    return rotationQ;
  }

/* this is all probably unnecessary, at least for now   31.12.03
  public void setStretchRotationAxis( double[]  axis
  {
    return [self setStretchRotation( axis angle( [self getStretchRotationAngle]];
  }

  public setStretchRotationAngle( (float) aFloat
  {
    return [self setStretchRotation( [self getStretchRotationAxis] angle( aFloat];
  }
    
  public setStretchRotation( double[]  axis  angle( (float) aFloat
  {
    float c,s;
    if (!isEditable)  return self;
    ASSERT( axis, OE_NULLPTR, OE_DEFAULT, "", return nil);
    c = cos(.5 * aFloat);
    s = sin(.5 * aFloat);
    laNormVec3f(axis, axis);
    Rn.times(axis, axis, (double ) s);
    quMakef(&theStretchRotation, c, axis->v[0], axis->v[1], axis->v[2]);
    quNormf(&theStretchRotation, &theStretchRotation);
    factorHasChanged = true
    return update();
  }

  public double[]  getStretchRotationAxis
  {
    OuVec3f axis;
    System.arraycopy(&axis, &theStretchRotation.q.R);
    if ( laAbsSqrVec3f(&axis) ) {
      laNormVec3f(&theStretchRotAxis, &axis); 
    }
    return &theStretchRotAxis;
  }
     
  public (float) getStretchRotationAngle
  {
    float theAngle;
    OuQuaternionf *rot = [self getStretchRotation];
    theAngle = 2 * acos(  (double) RR(rot));
  #ifdef __linux__
    {double foo;
    theAngle = 2*M_PI * modf(theAngle/(2*M_PI), &foo) ;
    }
  #else
    theAngle = fmodf(theAngle, (float) 2 * M_PI);
  #endif
    return theAngle;
  }
     
  public setStretchRotation( Quaternion aQ;
  {
    if (isFrozen) return self;
    ASSERT( aQ, OE_NULLPTR, OE_DEFAULT, "", return nil);
    theStretchRotation = *aQ;
    factorHasChanged = true
    return update();
  }

  public Quaternion getStretchRotation()
  {
    return stretchRotationQ;
  }
*/

  /**
   * Set the stretch vector associated to this transform using the factor <i>stretch</i> for all three dimensions.
   * @param stretch 
   */
  public void setStretch( double  stretch)
  {
    if (!isEditable)  return;
    //ASSERT( aS, OE_NULLPTR, OE_DEFAULT, "", return nil);
    synchronized(this)  {
      stretchVector[0] = stretch;
      stretchVector[1] = stretch;
      stretchVector[2] = stretch;
      stretchVector[3] = 1.0;
      factorHasChanged = true;
      update();     
    }
  }

  /**
   * Set the stretch factor using the the vector <i>(sx, sy, sz)</i>
   * @param sx
   * @param sy
   * @param sz
   */public void setStretch( double  sx, double sy, double sz)
  {
    if (!isEditable)  return;
    synchronized(this)  {
      stretchVector[0] = sx;
      stretchVector[1] = sy;
      stretchVector[2] = sz;
      stretchVector[3] = 1.0;
      factorHasChanged = true;
      update();     
    }
  }

  /**
   * Set the stretch using the 3-vector </i>stretchV</i>.
   * @param sV
   */
  public void setStretch( double[]  stretchV)
  {
    if (!isEditable)  return;
    //ASSERT( aS, OE_NULLPTR, OE_DEFAULT, "", return nil);
    synchronized(this)  {
      System.arraycopy( stretchV,0,stretchVector,0, Math.min(stretchV.length,stretchV.length));
      if (stretchV.length == 3)   stretchVector[3] = 1.0;
      factorHasChanged = true;
      update();     
    }
  }

  /**
   * Return the stretch vector for this transformation. Default: (1,1,1).
   * @return  double[3]
   */public double[]  getStretch()
  {
    if (matrixHasChanged && doFactor) update();
    return stretchVector;
  }

  /**
   * Updates the current state of the transformation.  If a factor was most recently changed, then the
   * matrix is updated.  Otherwise, if the matrix has changed since the last invocation, the factorization
   * is updated.
   *
   */public void update()
  {
    boolean[] isFlipped = new boolean[1];
    double  det;
    double[] MC = new double[16], TTmp;
    double[] centerTmp;

    synchronized(this)  {
      if (factorHasChanged )  {
        isFlipped[0]  = isReflection;
        P3.composeMatrixFromFactors(theMatrix, translationVector, rotationQ, stretchRotationQ, stretchVector, isReflection, metric);
        if (useCenter)  {
          Rn.times(theMatrix, theMatrix, invCenterMatrix);
          Rn.times(theMatrix, centerMatrix, theMatrix);
        } 
      } else if (matrixHasChanged && doFactor)  {
        if (useCenter)  {  // coule use Rn.conjugate but don't want to recalculate inverse each time ...
          Rn.times(MC, theMatrix, centerMatrix);
          Rn.times(MC, invCenterMatrix, MC);
          TTmp = MC;
        }else 
          TTmp = theMatrix;
        P3.factorMatrix(TTmp, translationVector, rotationQ, stretchRotationQ, stretchVector, isFlipped, metric); 
        isReflection = isFlipped[0];
      }
      isSpecial = Rn.isSpecialMatrix(theMatrix, TOLERANCE);
      matrixHasChanged = factorHasChanged = false;
    }
  }
}
