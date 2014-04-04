/*
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

package de.jreality.softviewer;

import de.jreality.backends.texture.SimpleTexture;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.Primitives;
import de.jreality.geometry.SphereUtility;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CubeMap;
import de.jreality.softviewer.shader.SkyboxPolygonShader;

public class PrimitiveCache {

    private static IndexedFaceSet[] sphere = new IndexedFaceSet[5];

    private static IntArrayArray[] sphereIndices = new IntArrayArray[5];

    private static DoubleArrayArray[] sphereVertices = new DoubleArrayArray[5];

    private static DoubleArrayArray[] sphereNormals = new DoubleArrayArray[5];

    private static IndexedFaceSet[] cylinder = new IndexedFaceSet[13];

    private static IntArrayArray[] cylinderIndices = new IntArrayArray[13];

    private static DoubleArrayArray[] cylinderVertices = new DoubleArrayArray[13];

    private static DoubleArrayArray[] cylinderNormals = new DoubleArrayArray[13];
    
    private static IntArrayArray[] tubeIndices = new IntArrayArray[13];

    private static DoubleArrayArray[] tubeVertices = new DoubleArrayArray[13];
    private static double[][] tubeVerticesDoubles = new double[13][];

    private static DoubleArrayArray[] tubeNormals = new DoubleArrayArray[13];

    private static SkyboxPolygonShader skybox = new SkyboxPolygonShader();

    private static final DoubleArrayArray[] cubeVertices = new DoubleArrayArray[] {
            new DoubleArrayArray.Array(new double[][] { { 1, 1, 1 },
                    { 1, 1, -1 }, { 1, -1, -1 }, { 1, -1, 1 } }),// right
            new DoubleArrayArray.Array(new double[][] { { -1, 1, -1 },
                    { -1, 1, 1 }, { -1, -1, 1 }, { -1, -1, -1 } }),// left
                    
            new DoubleArrayArray.Array(new double[][] { { -1, -1, 1 },
                    { 1, -1, 1 }, { 1, -1, -1 }, { -1, -1, -1 } }),// down
            new DoubleArrayArray.Array(new double[][] { { -1, 1, -1 },
                    { 1, 1, -1 }, { 1, 1, 1 }, { -1, 1, 1 } }),// up
                    
            new DoubleArrayArray.Array(new double[][] { { -1, 1, 1 },
                    { 1, 1, 1 }, { 1, -1, 1 }, { -1, -1, 1 } }),// back
            new DoubleArrayArray.Array(new double[][] { { 1, 1, -1 },
                    { -1, 1, -1 }, { -1, -1, -1 }, { 1, -1, -1 } }), // front
    };

    
    private static final DoubleArrayArray cubeTex = new DoubleArrayArray.Array( new double[][] 
                                                                                                  {{0,0},{1,0},{1,1},{0,1}}
    ); 
    private static final IntArray cubeIndices = new IntArray(new int[] { 0, 1,
            2, 3 });

    private static final DoubleArray[] cubeNormals = new DoubleArray[] {
            new DoubleArray(new double[] { 1, 0, 0 }),
            new DoubleArray(new double[] { -1, 0, 0 }),
            new DoubleArray(new double[] { 0, 1, 0 }),
            new DoubleArray(new double[] { 0, -1, 0 }),
            new DoubleArray(new double[] { 0, 0, 1 }),
            new DoubleArray(new double[] { 0, 0, -1 }) };
    /*
     * static private double[][][] cubeVerts3 = { {{1,1,1}, {1,1,-1}, {1, -1,
     * -1}, {1, -1, 1}}, // right { {-1, 1, -1}, {-1, 1, 1},{-1,-1,1},
     * {-1,-1,-1}}, // left { {-1, 1,-1}, {1, 1,-1},{1, 1,1}, {-1, 1,1}}, // up {
     * {-1,-1,1},{1,-1,1},{1,-1,-1}, {-1,-1,-1}}, // down {{-1,1,1}, {1,1,1},
     * {1,-1,1},{-1,-1,1}}, // back { {1,1,-1},{-1,1,-1}, {-1,-1,-1},{1,-1,-1}} //
     * front };
     */
    static {
        double d = 1 / Math.sqrt(3.);
        double[][] tetrahedronVerts3 = { { d, d, d }, { d, -d, -d },
                { -d, d, -d }, { -d, -d, d } };

        int[][] tetrahedronIndices = { { 0, 1, 2 }, { 2, 1, 3 }, { 1, 0, 3 },
                { 0, 2, 3 } };
        IndexedFaceSet tetra = new IndexedFaceSet(4, 4);

        tetra.setFaceAttributes(Attribute.INDICES, new IntArrayArray.Array(
                tetrahedronIndices));
        tetra.setVertexAttributes(Attribute.COORDINATES,
                StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(
                        tetrahedronVerts3));

        IndexedFaceSetUtility.calculateAndSetVertexNormals(tetra);

        sphere[0] = tetra;
        IndexedFaceSetUtility.calculateAndSetVertexNormals(sphere[0]);
        for (int i = 0; i < sphere.length; i++) {
            if (i > 0)
                sphere[i] = SphereUtility.tessellatedIcosahedronSphere(i - 1,
                        true);
            sphereIndices[i] = sphere[i].getFaceAttributes(Attribute.INDICES)
                    .toIntArrayArray();
            sphereVertices[i] = sphere[i].getVertexAttributes(
                    Attribute.COORDINATES).toDoubleArrayArray();
            sphereNormals[i] = sphere[i].getVertexAttributes(Attribute.NORMALS)
                    .toDoubleArrayArray();
        }
        for (int i = 0; i < cylinder.length; i++) {
            cylinder[i] = Primitives.cylinder(i + 3);
            cylinderIndices[i] = cylinder[i].getFaceAttributes(
                    Attribute.INDICES).toIntArrayArray();
            cylinderVertices[i] = cylinder[i].getVertexAttributes(
                    Attribute.COORDINATES).toDoubleArrayArray();
            cylinderNormals[i] = cylinder[i].getVertexAttributes(
                    Attribute.NORMALS).toDoubleArrayArray();
        }
        
        
        makeCylinders();
    }

    private PrimitiveCache() {
        super();
        // TODO Auto-generated constructor stub
    }

    public static void renderSphere(TrianglePipeline pipeline, double lod) {
        int i = (int) Math.min(4 * Math.pow(lod, 1 / 4.), 4);
        // i= 0;
        for (int j = 0, n = sphereIndices[i].size(); j < n; j++) {
            pipeline.processPolygon(sphereVertices[i], sphereIndices[i]
                    .getValueAt(j), sphereNormals[i], sphereIndices[i]
                    .getValueAt(j), null, null, null, null);
        }
    }

    public static void renderCylinder(TrianglePipeline pipeline, double lod) {
        int i = ((int) Math.min(12 * Math.pow(lod, 1 / 2.5), 12));
        // i= 0;
        for (int j = 0, n = cylinderIndices[i].size(); j < n; j++) {
            pipeline.processPolygon(cylinderVertices[i], cylinderIndices[i]
                    .getValueAt(j), cylinderNormals[i], cylinderIndices[i]
                    .getValueAt(j), null, null, null, null);
        }
    }

    public static IndexedFaceSet getSphere(double lod) {
        int i = (int) Math.min(4 * Math.pow(lod, 1 / 3.5), 4);
        // i= 0;
        return sphere[i] == null ? sphere[i] = SphereUtility
                .tessellatedIcosahedronSphere(i - 1, true) : sphere[i];
        // if( sphere == null )
        // sphere = SphereUtility.tessellatedIcosahedronSphere(2,true);
        // return sphere;
    }

    public static IndexedFaceSet getCylinder(double lod) {
        int i = (int) Math.min(16 * Math.pow(lod, 1 / 3.5), 16);
        return cylinder[i];
    }

    public static void renderSky(TrianglePipeline pipeline, CubeMap sky) {
        pipeline.setFaceShader(skybox);
        
        SimpleTexture tex = SimpleTexture.create(sky.getBack());
        skybox.setTexture(tex);
        pipeline.processPolygon(cubeVertices[0], cubeIndices, null, null, cubeTex,
                null, cubeNormals[0], null);

        tex = SimpleTexture.create(sky.getFront());
        skybox.setTexture(tex);
        pipeline.processPolygon(cubeVertices[1], cubeIndices, null, null, cubeTex,
                null, cubeNormals[1], null);

        tex = SimpleTexture.create(sky.getTop());
        skybox.setTexture(tex);
        pipeline.processPolygon(cubeVertices[2], cubeIndices, null, null, cubeTex,
                null, cubeNormals[2], null);

        tex = SimpleTexture.create(sky.getBottom());
        skybox.setTexture(tex);
        pipeline.processPolygon(cubeVertices[3], cubeIndices, null, null, cubeTex,
                null, cubeNormals[3], null);

        tex = SimpleTexture.create(sky.getLeft());
        skybox.setTexture(tex);
        pipeline.processPolygon(cubeVertices[4], cubeIndices, null, null, cubeTex,
                null, cubeNormals[4], null);

        tex = SimpleTexture.create(sky.getRight());
        skybox.setTexture(tex);
        pipeline.processPolygon(cubeVertices[5], cubeIndices, null, null, cubeTex,
                null, cubeNormals[5], null);

    }

    public static void renderCylinder2(TrianglePipeline pipeline, double lod) {
        int i = ((int) Math.min(12 * Math.pow(lod, 1 / 2.5), 12));

 /*
 

        double[] v =  tubeVerticesDoubles[i];
        double angle = 0, delta = Math.PI*2/(i+3);
        for(int k = i+3; k<2*(i+3);k++) {
            angle = k*delta;
            v[4*k+0] = e*Math.cos(angle);
            v[4*k+1] = e*Math.sin(angle);            
            v[4*k+2] = d;
            v[4*k+3] = e;
        }
        */
        for (int j = 0, n = tubeIndices[i].size(); j < n; j++) {
            pipeline.processPolygon(tubeVertices[i], tubeIndices[i]
                    .getValueAt(j), tubeNormals[i], tubeIndices[i]
                    .getValueAt(j), null, null, null, null);
        }
        
    }

    public static void makeCylinders() {
        for(int n= 3; n<16; n++) {
            int rn = n;
            double[] verts = new double[2*4*rn];
            double[] norms = new double[2*3*rn];
            int[][] idx = new int[rn][4];
            double angle = 0, delta = Math.PI*2/(n);
            for (int i = 0 ;i<rn; ++i)  {
                angle = i*delta;
                verts[4*(i+rn)] = 0;
                verts[4*i] = norms[3*(i+rn)] = norms[3*i] = Math.cos(angle);
                verts[4*(i+rn)+1] = 0;
                verts[4*i+1] =  norms[3*(i+rn)+1] = norms[3*i+1] = Math.sin(angle);
                verts[4*i+2] = 0;
                verts[4*(i+rn)+2] = 1;
                verts[4*i+3] = 1;
                verts[4*(i+rn)+3] = 0;
                norms[3*i+2] = norms[3*(i+rn)+2] = 0;
                idx[i][0] = i;
                idx[i][1] = (i+1)%rn;
                idx[i][2] = (i+1)%rn+rn;
                idx[i][3] = (i+rn);
            }   
            
            tubeVerticesDoubles[n-3] = verts;
            tubeVertices[n-3] = new DoubleArrayArray.Inlined(verts,4);
            tubeNormals[n-3] = new DoubleArrayArray.Inlined(norms,3);
            tubeIndices[n-3] = new IntArrayArray.Array(idx);
        }
    }
}
