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


package de.jreality.examples;

import java.util.Collections;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.math.Rn;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;

/**
 * x = cos alpha sinh v sin u + sin alpha cosh v cos u
 * y = -cos alpha sinh v cos u + sin alpha cosh v sin u 
 * z = u cos alpha + v sin alpha
 * 
 * u \in [0,2 Pi[
 * 
 * alpha = 0 helicoid
 * alpha = Pi/2 catenoid
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class CatenoidHelicoid extends IndexedFaceSet {
  double[] vertices;
  double[] normals;
  double[] texCoords;
  
  private int d;

  double alpha= 0 * Math.PI / 2.4;
private int[][] faceIndices;
  /**
   * @param pointsProvider
   * @param vertexNormalsProvider
   * @param edgesProvider
   * @param facesProvider
   */
  public CatenoidHelicoid(int detail) {
	super(detail*detail, (detail-1)*(detail-1));
    int numPoints=getNumPoints(), numFaces=getNumFaces();
    this.d= detail;
    double r= 1;
    vertices= new double[numPoints * 3];
    normals= new double[numPoints * 3];
    texCoords= new double[numPoints * 2];
	faceIndices = new int[(detail-1)*(detail-1)][4];
	
    computeVertices();
    computeNormals();
    computeTexCoords();
    generateFaceIndices();
    
    vertexAttributes.addWritable(Attribute.COORDINATES,
      StorageModel.DOUBLE3_INLINED, vertices);
    vertexAttributes.addWritable(Attribute.NORMALS,
            StorageModel.DOUBLE3_INLINED, normals);
    vertexAttributes.addWritable(Attribute.TEXTURE_COORDINATES,
            StorageModel.DOUBLE2_INLINED, texCoords);
	faceAttributes.addWritable(Attribute.INDICES, StorageModel.INT_ARRAY_ARRAY, faceIndices);
	
	IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(this);

  }

  private void generateFaceIndices() {
	  startWriter();
    try {
      final int uLineCount = d;
      final int vLineCount = d;
  	
    	final int numUFaces = uLineCount-1;
    	final int numVFaces = vLineCount-1;
    	
    	final int numPoints = d*d;
    	
    	for (int i = 0, k=0; i<numVFaces; ++i) {
    		for (int j = 0; j< numUFaces; ++j, k++)	{
    			final int [] face = faceIndices[k];
    			face[0] = (i * uLineCount + j);
    			face[1] = (((i+1) * uLineCount) + j) % numPoints;
    			face[2] = (((i+1) * uLineCount) + (j+1)%uLineCount) % numPoints;
    			face[3] = ((i* uLineCount) + (j+1)%uLineCount) % numPoints;				
    		}
    	}
    } finally {
      finishWriter();
    }
  }
  
  private void computeTexCoords() {
    for (int i= 0; i < d; i++) {
        for (int j= 0; j < d; j++) {
            int pos= 2 * (i + d * j);
            texCoords[pos] = i/(double)(d-1);
            texCoords[pos+1] = j/(double)(d-1);
        }
    }
  }

  private static final double sinh(final double x) {
    return .5 * (Math.exp(x) - Math.exp(-x));
  }
  private static final double cosh(final double x) {
    return .5 * (Math.exp(x) + Math.exp(-x));
  }

  private void computeVertices() {
    final double[] vert=vertices;
    startWriter();
    try {
      final double cosalpha=Math.cos(alpha);
      final double sinalpha=Math.sin(alpha);
      for (int i= 0; i < d; i++) {
        for (int j= 0; j < d; j++) {
          double u= i * Math.PI * 2. / (d - 1.);
          double v= 4. * j / (d - 1.) - 2;
          int pos= 3 * (i + d * j);
          /*
           * x = cos alpha sinh v sin u + sin alpha cosh v cos u
           * y = -cos alpha sinh v cos u + sin alpha cosh v sin u 
           * z = u cos alpha + v sin alpha
           */
          final double sinhV   =      sinh(v);
          final double coshV   =      cosh(v);
          final double cosU    = Math.cos( u);
          final double sinU    = Math.sin( u);
          vert[pos]=      cosalpha * sinhV * sinU + sinalpha * coshV * cosU;
          vert[pos + 1]= -cosalpha * sinhV * cosU + sinalpha * coshV * sinU;
          vert[pos + 2]= u * cosalpha + v * sinalpha;
        }
      }
      fireGeometryDataChanged(CATEGORY_VERTEX, Collections.singleton(Attribute.COORDINATES));
    } finally {
      finishWriter();
    }	
  }

  private void computeNormals() {
    final double[] nn=new double[3];
    startWriter();
    try {
      for (int i= 0; i < d; i++) {
        for (int j= 0; j < d; j++) {
          double u= i * Math.PI * 2. / (d - 1.);
          double v= 4. * j / (d - 1.) - 2;
          int pos= 3 * (i + d * j);
      
          final double coshV= cosh(v);
          nn[0]= -Math.cos(u) * coshV;
          nn[1]= -Math.sin(u) * coshV;
          nn[2]= coshV * sinh(v);
          Rn.normalize(nn, nn);
          normals[pos  ] = nn[0];
          normals[pos+1] = nn[1];
          normals[pos+2] = nn[2];
        }
      }
      fireGeometryDataChanged(CATEGORY_VERTEX, Collections.singleton(Attribute.NORMALS));
    } finally {
      finishWriter();
    }
  }

  public double getAlpha() {
    return alpha;
  }

  public void setAlpha(double alpha) {
    this.alpha= alpha;
    computeVertices();
  }

}
