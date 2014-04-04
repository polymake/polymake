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

import de.jreality.math.Rn;
import de.jreality.scene.data.Attribute;

/**
 * This class is similar to {@link ParametricSurfaceFactory} but it works with a triangular
 * rather than rectangular parametric domain.
 * @author gunn
 *
 */public class ParametricTriangularSurfaceFactory extends AbstractIndexedFaceSetFactory {

	final OoNode immersion = node( "immersion" );
	double[][] uvTriangle = {{0,0},{1,0},{0,1}};
	final OoNode triangle = node(uvTriangle,"triangle");
	int subdivision = 10;
	final OoNode subNode = node(new Integer(subdivision), "subdivision" );
	public ParametricTriangularSurfaceFactory() {
		setImmersion( new Immersion() {

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
		vertexCoordinates.setGenerate(true);
		faceIndices.setGenerate(true);
		edgeIndices.addIngr( faceIndices );
		vertexCoordinates.addIngr(triangle);
		vertexCoordinates.addIngr(subNode);
		vertexCoordinates.addIngr(immersion);
		vertexCoordinates.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {	
						return generateVertexCoordinates( (double[][])object);	
					}					
				}
		);
		faceIndices.addIngr(subNode);
		faceIndices.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {	
						return generateFaceIndices( (int[][])object);	
					}					
				}
		);
	}
	
	public interface Immersion {
		public boolean isImmutable();
		public int getDimensionOfAmbientSpace();
		public void evaluate(double u, double v, double[] xyz, int index);
	}

	
	public void setSubdivision(int s)	{
		subdivision = s;
		subNode.setObject(subdivision);
	}
	
	public int getSubdivision()	{
		return subdivision;
	}
	protected Object generateFaceIndices(int[][] is) {
		int[][] indices = is;
		
		log("compute", Attribute.INDICES, "face");
		if (indices == null || indices.length != (subdivision*(subdivision-1)/2)
				|| indices[0].length != 3)
			indices = new int[(subdivision-1)*(subdivision-1)][3];
		int vcount = 0;
		for(int i = 0, totalC=0; i<subdivision-1; ++i)	{
			for (int j = 0; j<subdivision-i-1; ++j) {
				indices[totalC][0] = vcount+j;
				indices[totalC][1] = vcount+j+1;
				indices[totalC][2] = vcount+j+subdivision-i;
				totalC++;
				if (j != subdivision-i-2)	{
					indices[totalC][0] = vcount+j+1;
					indices[totalC][1] = vcount+j+subdivision-i+1;
					indices[totalC][2] = vcount+j+subdivision-i;				
					totalC++;					
				}
			}
			vcount += subdivision-i;
		}
		return indices;
	}
	@Override
	int nov()	{
		return ((subdivision)*(subdivision+1))/2;
	}
	@Override
	protected int nof()	{
		return ((subdivision-1)*(subdivision-1));
	}
	double [][] generateVertexCoordinates( double [][] vertexCoordinates ) {
		
		log( "compute", Attribute.COORDINATES, "vertex ");
		
		final Immersion immersion = getImmersion();
		
		if( vertexCoordinates == null || vertexCoordinates.length != nov() )
			vertexCoordinates = new double[nov()][immersion.getDimensionOfAmbientSpace()];
			
		double[][] domain = getDomainVertices(null);
		for(int i =0; i<domain.length; ++i)
				immersion.evaluate(domain[i][0], domain[i][1], vertexCoordinates[i], 0 ); //indexOfUV], 0);
		
		return vertexCoordinates;
	}

	/**
	 * generate regularly-spaced (u,v) points in the <i>uvTriangle</i> by subdividing
	 * each side into <i>subdivision</i> points.
	 * @param uvpoints
	 * @return
	 */public double[][] getDomainVertices( double[][] uvpoints)	{
		
		if (uvpoints == null || uvpoints.length != nov() || uvpoints[0].length != 2)
			uvpoints = new double[nov()][2];
		
		double duv = 1.0/(subdivision-1);
		for(int i = 0, totalC=0; i<subdivision; ++i)	{
			for (int j = 0; j<subdivision-i; ++j,++totalC) {
				double v = i*duv;
				double u = j*duv;
				double w = 1.0 - u - v;
				Rn.add(uvpoints[totalC], 
						Rn.add(null, Rn.times(null, u, uvTriangle[1]), Rn.times(null, v, uvTriangle[2])),
						Rn.times(null, w, uvTriangle[0]));
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

	public double[][] getUVTriangle()	{
		return uvTriangle;
	}
	
	public void setUVTriangle(double[][] v)	{
		uvTriangle = v;
		triangle.setObject(v);
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
		edgeIndices.updateArray();
		faceIndices.updateArray();
	}

 }
