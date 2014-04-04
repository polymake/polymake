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

import de.jreality.scene.data.Attribute;

/**
 * This factory specializes the {@link QuadMeshFactory} further, replacing the explicit definition of the 
 * vertex coordinates with a functional definition.  The constructor for this factory 
 * requires a class that implements {@link Immersion}. To implement this interface for a map
 * of two variables into three space extend the abstract class {@link DefaultImmersion}. 
 * <p>
 * The domain of the immersion is a rectangle in (u,v) space specified by the four methods {@link #setUMin(double)}, {@link #setUMax(double)}, etc.
 * The number of samples in each direction is specified using the methods inherited from {@link QuadMeshFactory}: {@link QuadMeshFactory#setULineCount(int)}, etc.
 * <p>
 *  For an example, see
 * <a href=http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/Parametrized_Surfaces> this tutorial</a>.
 * <p>
 * @author gunn
 *
 */
public class ParametricSurfaceFactory extends AbstractQuadMeshFactory {

	final OoNode uMin = node( new Double(0), "uMin" );
	final OoNode uMax = node( new Double(1), "uMax" );
	final OoNode vMin = node( new Double(0), "vMin" );
	final OoNode vMax = node( new Double(1), "vMax" );
	
	final OoNode immersion = node( "immersion" );
	
	ParametricSurfaceFactory( Immersion immersion, double uMin, double uMax, double vMin, double vMax ) {
		super();
		
		setUMin( uMin );
		setUMax( uMax );
		setVMin( vMin );
		setVMax( vMax );
		
		setImmersion( immersion );
		
		vertexCoordinates.setGenerate(true);
	}
	
	public ParametricSurfaceFactory( Immersion immersion ) {
		this( immersion, 0, 1, 0, 1 );
	}
	
	public ParametricSurfaceFactory() {
		this( new Immersion() {

			public int getDimensionOfAmbientSpace() {
				return 3;
			}
			public void evaluate(double u, double v, double[] xyz, int index) {
				xyz[index+0]=u; 
				xyz[index+1]=v;				
			}
			public boolean isImmutable() {
				return true;
			}			
		});
	}
	
	/**
	 * Represents a map of two variables into n--dimensional space. 
	 * 
	 */
	public interface Immersion {
		/** Mutable immersions are always recalculated when the <code>update</code> method is called; 
		 * immutable immersions are only recalculated when the parameter domain changes.*/
		public boolean isImmutable();
		/** The dimension of the target space. */
		public int getDimensionOfAmbientSpace();
		/** The implementation of the  formula. If in mathematical notation 
		 * (f1(u,v), ..., fn(u,v)) is your R<sup>n</sup> valued map, where n is the dimension of 
		 * the ambient space. Then your implementation of <code>evaluate</code> should read
		 * 
		 *  <code><pre>
		 * 		xyz[3*index]=f1(u,v);
		 * 		xyz[3*index+1]=f2(u,v);
		 * 		   ...
		 *		xyz[3*index+n-1]=fn(u,v);
		 *  </pre></code>
		 *	 
		 * @param u coordinate of the requested value
		 * @param v coordinate of the requested value
		 * @param xyz an array to put the result of the evaluation into
		 * @param index at which to put the result into <code>xyz</code>. 
		 */
		public void evaluate(double u, double v, double[] xyz, int index);
	}


	{
		vertexCoordinates.addIngr(vLineCount);
		vertexCoordinates.addIngr(uLineCount);
		vertexCoordinates.addIngr(uMin);
		vertexCoordinates.addIngr(uMax);
		vertexCoordinates.addIngr(vMin);
		vertexCoordinates.addIngr(vMax);
		vertexCoordinates.addIngr(immersion);
		vertexCoordinates.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {	
						return generateVertexCoordinates( (double[][])object);	
					}					
				}
		);
	}
	
	double [][] generateVertexCoordinates( double [][] vertexCoordinates ) {
		
		log( "compute", Attribute.COORDINATES, "vertex ");
		
		final Immersion immersion = getImmersion();
		
		if( vertexCoordinates == null || vertexCoordinates.length != nov() )
			vertexCoordinates = new double[nov()][immersion.getDimensionOfAmbientSpace()];
			
		final int vLineCount = getVLineCount();
		final int uLineCount = getULineCount();
		
		final double dv=(getVMax() - getVMin()) / (vLineCount - 1);
		final double du=(getUMax() - getUMin()) / (uLineCount - 1);
		
		final double uMin = getUMin();
		final double vMin = getVMin();
		
		double v=vMin;
		for(int iv=0, firstIndexInULine=0;
		iv < vLineCount;
		iv++, v+=dv, firstIndexInULine+=uLineCount) {
			double u=uMin;
			for(int iu=0; iu < uLineCount; iu++, u+=du) {
				final int indexOfUV=firstIndexInULine + iu;
				immersion.evaluate(u, v, vertexCoordinates[ uLineCount*iv + iu ], 0 ); //indexOfUV], 0);
			}
		}
			
		
		return vertexCoordinates;
	}

	public double[][] getDomainVertices( double[][] uvpoints)	{
		return getDomainVertices(uvpoints, false);
	}
	public double [][] getDomainVertices( double [][] uvpoints, boolean offset ) {
		
		if (uvpoints == null || uvpoints.length != nov() || uvpoints[0].length != 2)
			uvpoints = new double[nov()][2];
			
		final int vLineCount = getVLineCount();
		final int uLineCount = getULineCount();
		
		final double dv=(getVMax() - getVMin()) / (vLineCount - 1);
		final double du=(getUMax() - getUMin()) / (uLineCount - 1);
		
		final double uMin = getUMin();
		final double vMin = getVMin();
		
		double v=vMin;
		for(int iv=0, firstIndexInULine=0;
		iv < vLineCount;
		iv++, v+=dv, firstIndexInULine+=uLineCount) {
			double u=uMin + ( ( ( (iv%2) == 1) && offset) ? du/2 : 0);
			for(int iu=0; iu < uLineCount; iu++, u+=du) {
				int i = uLineCount*iv + iu;
				uvpoints[ i ][0] = u;
				uvpoints[ i ][1] = v;
			}
		}
			
		
		return uvpoints;
	}

	public Immersion getImmersion() {
		return (Immersion)immersion.getObject();
	}

	public void setImmersion(Immersion f) {
		if( f==null)
			throw new IllegalArgumentException( "Immersion cannot set to null." );
		immersion.setObject(f);
	}

	public double getUMax() {
		return ((Double)uMax.getObject()).doubleValue();
	}

	public void setUMax(double max) {
		uMax.setObject(new Double(max));
	}

	public double getUMin() {
		return ((Double)uMin.getObject()).doubleValue();
	}

	public void setUMin(double min) {
		uMin.setObject(new Double(min));
	}

	public double getVMax() {
		return ((Double)vMax.getObject()).doubleValue();
	}

	public void setVMax(double max) {
		vMax.setObject(new Double(max));
	}

	public double getVMin() {
		return ((Double)vMin.getObject()).doubleValue();
	}

	public void setVMin(double min) {
		vMin.setObject(new Double(min));
	}


	void recompute() {
		if( !getImmersion().isImmutable() )
			immersion.outdate();

		super.recompute();

		vertexCoordinates.update();
	}
	
	protected void updateImpl() {
	
		super.updateImpl();
	
		vertexCoordinates.updateArray();
	}

  /**
   * An abstract implementation of the interface <code>Immersion</code> for
   * a map of two variables into 3-space. Override the abstract
   * method <code>evaluate</code> and assign the protected variables
   * <code>x</code>, <code>y</code>, and <code>z</code> there depending on 
   * the given <code>u</code>, <code>v</code> values.
   * 
   */
	public abstract static class DefaultImmersion implements Immersion {

		public boolean isImmutable() {
			return false;
		}

		public int getDimensionOfAmbientSpace() {
			return 3;
		}

		public void evaluate(double u, double v, double[] xyz, int index) {
			evaluate( u, v );
			xyz[3*index+0] = x;
			xyz[3*index+1] = y;
			xyz[3*index+2] = z;
		}
		
		protected double x, y, z;
		
    /**
     * Assign the protected variables x, y, z here.
     * 
     * @param u 
     * @param v
     */
		abstract public void evaluate( double u, double v );
	}
}
