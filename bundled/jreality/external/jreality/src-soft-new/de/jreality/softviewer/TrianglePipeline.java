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

import java.util.Arrays;

import de.jreality.math.Rn;
import de.jreality.scene.Geometry;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.softviewer.shader.DefaultPolygonShader;
import de.jreality.softviewer.shader.LineShader;
import de.jreality.softviewer.shader.PointShader;
import de.jreality.softviewer.shader.PolygonShader;
import de.jreality.softviewer.shader.VertexShader;

/**
 * A render pass will consit of the following: 1. calls to processPolygon for
 * each polygon to render. This will transform the polygon by using the state of
 * matrix. and give it the current shader.
 * <p>
 * 2. optionally sort the polygon list.<br>
 * 3. shade all Polygons using the shader given by the polygon<br>
 * 4. clip all polygons using clipPolygons()<br>
 * 5. call perspective for all polygons<br>
 * 6. render the polygons.
 * <p>
 * In fact all the calls in 3. to 6. can be done in one pass. However we have to
 * delay the shading to after the traversal, to allow the environment to gather
 * all the lights in the scene, if we want to have lights to act globally. THIS
 * IS NOT IMPLEMENTED YET.
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 */
public class TrianglePipeline {

    // private int vertexColorLength;
    // private DataList vertexColor;
    // private boolean vertexColors;
    private final boolean queueOpaque;

    private final boolean sortOpaque;

    // private final boolean triangulate =false;
    private final boolean useTexCoords = true;

    private double[] matrix;

    // private boolean itmComputed = false;
    private double[] inverseTransposeMatrix = new double[16];

    // int triangleCount = 0;

    Polygon polygon = new Polygon(4);

    // needed for clipping
    Polygon tmpPolygon = new Polygon(4);

    ArrayStack freeTriangles = new ArrayStack(1000);

    ArrayStack triangles = new ArrayStack(1000);

    // Polygon polygons[] = new Polygon[0];
    // private int vertexCount = 0;
    // double[] vertexData = new double[100 * Polygon.VERTEX_LENGTH];

    protected TriangleComparator comp = new TriangleComparator();

    private CameraProjection perspective = new PerspectiveProjection();

    private Environment environment;

    private PolygonShader faceShader; // = new DefaultPolygonShader();

    private LineShader lineShader; // = new ConstantPolygonShader(.2,.1,.1);
    private VertexShader constantVertexShader = new de.jreality.softviewer.shader.ConstantVertexShader(0,0,0);
    private PolygonShader constantLineShader = new DefaultPolygonShader(constantVertexShader);
    // private PolygonShader pointOutlineShader;
    // = new ConstantPolygonShader(.0,.0,.0);
    private PointShader pointShader;

    // = new ConstantPolygonShader(0.8,0.2,0.2);

    private PolygonShader shader;

    private double[] pointVertices = new double[48];
    private DoubleArrayArray pointVerticesDataList = new DoubleArrayArray.Inlined(pointVertices,3);

    // private static int[] pointIndices = { 0, 3, 6, 9, 12, 15, 18, 21 };
    private static int[] pointIndices = { 0, 1, 2, 3, 4, 5, 6, 7 };
    private IntArray pointIndicesDataList = new IntArray(pointIndices);

    
// private static int[][] pointOutlineIndices = { { 0, 24, 27, 3 },
// { 3, 27, 30, 6 }, { 6, 30, 33, 9 }, { 9, 33, 36, 12 },
// { 12, 36, 39, 15 }, { 15, 39, 42, 18 }, { 18, 42, 45, 21 },
// { 21, 45, 24, 0 } };
    private static int[][] pointOutlineIndices = { { 0, 8, 9, 1 },
        { 1, 9, 10, 2 }, { 2, 10, 11, 3 }, { 3, 11, 12, 4 },
        { 4, 12, 13, 5 }, { 5, 13, 14, 6 }, { 6, 14, 15, 7 },
        { 7, 15, 8, 0 } };

    private IntArrayArray pointOutlineIndicesDataList = new IntArrayArray.Array(pointOutlineIndices);

    private static double[][] pointColors = new double[16][4];
    private DoubleArrayArray pointColorsDataList = new DoubleArrayArray.Array(pointColors);
    
    private static double[][] lineFaceColors = new double[1][4];
    private DoubleArrayArray lineFaceColorsDataList = new DoubleArrayArray.Array(lineFaceColors);

    
// private DataList pointColorDataList = StorageModel.DOUBLE_ARRAY_ARRAY
// .createWritableDataList(pointColors);

    private static int[] pointNormals = { 0, 0, 0, 0, 0, 0, 0, 0 };
    private IntArray pointNormalsDataList = new IntArray(pointNormals);

    
// private static int[][] pointOutlineNormals = { { 0, 3, 6, 0 },
// { 0, 6, 9, 0 }, { 0, 9, 12, 0 }, { 0, 12, 15, 0 },
// { 0, 15, 18, 0 }, { 0, 18, 21, 0 }, { 0, 21, 24, 0 },
// { 0, 24, 3, 0 } };
    private static int[][] pointOutlineNormals = { { 0, 1, 2, 0 },
        { 0, 2, 3, 0 }, { 0, 3, 4, 0 }, { 0, 4, 5, 0 },
        { 0, 5, 6, 0 }, { 0, 6, 7, 0 }, { 0, 7, 8, 0 },
        { 0, 8, 1, 0 } };

    private IntArrayArray pointOutlineNormalsDataList = new IntArrayArray.Array(pointOutlineNormals);

    
    // private static double[] zNormal = new double[] { 0, 0, 1 };
    private static double[] zNormal = new double[3 * 9];
    private DoubleArrayArray zNormalDataList = new DoubleArrayArray.Inlined(zNormal,3);

    // private static int[] lineIndices = { 0, 3, 6, 9 };
    private static int[] lineIndices = { 0, 1, 2, 3 };
    private IntArray lineIndicesDataList = new IntArray(lineIndices);
    
    protected final TriangleRasterizer rasterizer;

    // static {
    // }

    public TrianglePipeline(TriangleRasterizer rasterizer) {
        this(rasterizer, false);
    }

    public TrianglePipeline(TriangleRasterizer rasterizer, boolean sortAll) {
        super();
        this.sortOpaque = sortAll;
        this.queueOpaque = sortAll;
        this.rasterizer = rasterizer;
        zNormal[0] = 0;
        zNormal[1] = 0;
        zNormal[2] = 1;
        for (int i = 0; i < 8; i++) {
            int pos = 3 * i;
            pointVertices[pos] = zNormal[pos + 3] = Math.cos(2. * i * Math.PI
                    / 8.);
            pointVertices[pos + 1] = zNormal[pos + 4] = Math.sin(2. * i
                    * Math.PI / 8.);
            zNormal[pos + 5] = 0.01;
        }
    }

    public final void clearPipeline() {
        faceShader = null;
        lineShader = null;
        pointShader = null;
        // pointOutlineShader = null;
    }


    final void startGeometry(Geometry geom) {
         if (lineShader != null)
         lineShader.startGeometry(geom);
         if (faceShader != null)
         faceShader.startGeometry(geom);
         if (pointShader != null)
         pointShader.startGeometry(geom);
    }

    public final void processPolygon(final DoubleArrayArray vd,
            final IntArray vertices, final DoubleArrayArray nd,
            final IntArray normals, final DataList texCoords,
            final DataList vertexColors, final DoubleArray faceNormal,
            final DoubleArray faceColor) {
        if (faceShader == null)
            return;
        shader = faceShader;
        process(vd,vertices,nd,normals,texCoords,vertexColors,faceNormal,faceColor,true);
    }
    
    private final void process(final DoubleArrayArray vd,
                final IntArray vertices, final DoubleArrayArray nd,
                final IntArray normals, final DataList texCoords,
                final DataList vertexColors, final DoubleArray faceNormal,
                final DoubleArray faceColor,final boolean transform) {
        
        polygon.setLength(0);
        tmpPolygon.setLength(0);
        fillVertexData(vd, vertices, nd, normals, texCoords, vertexColors,
                faceNormal,faceColor,transform);
        // shade
        shader.shadePolygon(polygon, environment,vertexColors!=null | faceColor!=null);
        
        // return early if polygon is clipped out
        // TODO: debug clipPlanes
        if (clipPlanes())
            return;
        computePerspective();
        // clip to frustum:
        // TODO: debug clipPlanes

        if (clipFrustum())
           return;

        triangulateAndRaster();
    }

    Triangle[] trisFromPoly = new Triangle[6];

    private void triangulateAndRaster() {
        boolean isTransparent = transparencyEnabled && shader.needsSorting();
        // boolean isTransparent = shader.needsSorting();

        int n = polygon.getLength() - 2;
        trisFromPoly = polygon.triangulate(trisFromPoly, freeTriangles);

        for (int i = 0; i < n; i++) {
            Triangle tri = trisFromPoly[i];

            if (queueOpaque || isTransparent) {
                if (sortOpaque || isTransparent)
                    // tri.computeCenterZ(vertexData);
                    // pi.computeMaxZ(vertexData);
                    triangles.push(tri);
                
                // return;
            } else {
                rasterizer.renderTriangle(tri, false);
                freeTriangles.push(tri);
            }
        }
        return;
    }

    /**
     * clip polygon against all the clipping planes in the environment
     * 
     * @return
     */
    private boolean clipPlanes() {
        int numClip = environment.getNumClippingPlanes();
        if (numClip > 0) {
            // Intersector.dehomogenize(p,vertexData);
            for (int k = 0; k < numClip; k++) {
                ClippingPlaneSoft cp = environment.getClippingPlane(k);
                double d = VecMat.dot(cp.getNormal(), cp.getPoint());
                // System.out.println("d "+d);
                int res = PolygonUtility.clipToHalfspace(polygon,
                        AbstractPolygon.WX, cp.getNormal(), -1, d, tmpPolygon);
                if (res == PolygonUtility.CLIPPED_OUT) { // clipped out
                    return true;
                }
                if (res == PolygonUtility.CLIPPED_PARTIAL) {// clipToHalfspace returned the clipped poly in
                    // tmpPolygon
                    tmpPolygon.setShadingFrom(polygon);
                    
                    Polygon r = polygon;
                    polygon = tmpPolygon;
                    tmpPolygon = r;
                }
            }
        }
        return false;
    }

    private void computePerspective() {
        for (int i = 0; i < polygon.getLength(); i++) {
            perspective.perspective(polygon.getPoint(i));
        }

    }

    private final void fillVertexData(final DoubleArrayArray vd,
            final IntArray vertices, final DoubleArrayArray nd,
            final IntArray normals, final DataList texCoords, final DataList vertexColors,
            final DoubleArray faceNormal, final DoubleArray faceColor,final boolean transform) {

        // only two vertices
        if (vertices.getLength() == 2) {
            processLine(vd.item(vertices.getValueAt(0)).toDoubleArray(), vd
                    .item(vertices.getValueAt(1)).toDoubleArray());
            return;
        }

        // do we have vertexColors?
        int vertexColorLength = vertexColors != null ? vertexColors.item(0)
                .size() : 0;


        final boolean r3 = (vd.getLengthAt(0) == 3);

        double[] center = polygon.getCenter();
        if(faceNormal != null) {
            if(transform) {
                VecMat.transformNormal(inverseTransposeMatrix,
                        faceNormal.getValueAt(0), faceNormal.getValueAt(1), faceNormal
                                .getValueAt(2), center, AbstractPolygon.NX);
            } else {
                center[Polygon.NX] = faceNormal.getValueAt(0);
                center[Polygon.NY] = faceNormal.getValueAt(1);
                center[Polygon.NZ] = faceNormal.getValueAt(2);
            }
        }
        boolean buildFaceNormal = faceNormal== null && !shader.interpolateColor();

        if(faceColor != null) { 
            center[Polygon.R] = faceColor.getValueAt(0);
            center[Polygon.G] = faceColor.getValueAt(1);
            center[Polygon.B] = faceColor.getValueAt(2);
            if(faceColor.size() == 4)
                center[Polygon.A] = faceColor.getValueAt(3);
        }
        // iterate over vertives
        // to get world coordinates into vertexData
        final int n = vertices.getLength();
        for (int i = 0; i < n; i++) {
            // int vc = i* Triangle.VERTEX_LENGTH;
            int vi = vertices.getValueAt(i);
            DoubleArray vertex = vd.item(vi).toDoubleArray();
            double[] vertexData = polygon.getPoint(i);
            
            DoubleArray normal = null;
            if(nd != null) {
            int ni = normals.getValueAt(i);
            normal = nd.item(ni).toDoubleArray();
            } else {
                normal = faceNormal;
            }
            if(transform) {
            if (r3) {
                VecMat.transform(matrix, vertex.getValueAt(0), vertex
                        .getValueAt(1), vertex.getValueAt(2), vertexData,
                        Triangle.WX);
                vertexData[Triangle.WW] = 1;
            } else {
                VecMat.transform(matrix, vertex.getValueAt(0), vertex
                        .getValueAt(1), vertex.getValueAt(2), vertex
                        .getValueAt(3), vertexData, Triangle.WX);
                if (vertexData[Triangle.WW] != 0) {
                    final double d = vertexData[AbstractPolygon.WW];
                    vertexData[Triangle.WX] /= d;
                    vertexData[Triangle.WY] /= d;
                    vertexData[Triangle.WZ] /= d;
                    vertexData[Triangle.WW] = 1;
                }
            }
            
           if (normal != null)
//        	   System.err.println("bad normal");
        	   VecMat.transformNormal(inverseTransposeMatrix,
                    normal.getValueAt(0), normal.getValueAt(1), normal
                            .getValueAt(2), vertexData, AbstractPolygon.NX);
            
            } else {
                vertexData[Polygon.WX] = vertex.getValueAt(0);
                 vertexData[Polygon.WY] = vertex.getValueAt(1);
                 vertexData[Polygon.WZ] = vertex.getValueAt(2);
                 vertexData[Polygon.WW] = 1;
                 vertexData[Polygon.NX] = normal.getValueAt(0);
                 vertexData[Polygon.NY] = normal.getValueAt(1);
                 vertexData[Polygon.NZ] = normal.getValueAt(2);
                 // VecMat.normalize(vertexData, vc + Polygon.NX);
            }
            
            if(buildFaceNormal) {
                center[AbstractPolygon.NX] = 0;
                center[AbstractPolygon.NY] = 0;
                center[AbstractPolygon.NZ] = 0;
                center[AbstractPolygon.NX] +=vertexData[Polygon.NX];                center[AbstractPolygon.NX] +=vertexData[Polygon.NX];
                center[AbstractPolygon.NY] +=vertexData[Polygon.NY];
                center[AbstractPolygon.NZ] +=vertexData[Polygon.NZ];
                
            }
            if (vertexColors != null) {
                DoubleArray color = vertexColors.item(vi).toDoubleArray();
                vertexData[AbstractPolygon.R] = color.getValueAt(0);
                vertexData[AbstractPolygon.G] = color.getValueAt(1);
                vertexData[AbstractPolygon.B] = color.getValueAt(2);
                vertexData[AbstractPolygon.A] = (vertexColorLength == 4 || (vertexColorLength == -1 && color
                        .getLength() > 3)) ? color.getValueAt(3) : 1.;
            } else if(faceColor != null) { 
                vertexData[Polygon.R] = faceColor.getValueAt(0);
                vertexData[Polygon.G] = faceColor.getValueAt(1);
                vertexData[Polygon.B] = faceColor.getValueAt(2);
                if(faceColor.size() == 4)
                    vertexData[Polygon.A] = faceColor.getValueAt(3);
            }
            if (useTexCoords && texCoords != null) {
                DoubleArray tc = texCoords.item(vi).toDoubleArray();
                vertexData[AbstractPolygon.U] = tc.getValueAt(0);
                vertexData[AbstractPolygon.V] = tc.getValueAt(1);

            }
        }
        if(buildFaceNormal) {
// center[AbstractPolygon.NX] /= n;
// center[AbstractPolygon.NY] /= n;
// center[AbstractPolygon.NZ] /= n;
            VecMat.normalize(center,AbstractPolygon.NX);
        }
        // compute(vertices.getLength());

    }

    /**
     * 
     */
    private final boolean clipFrustum() {
        // ClippingBox box = perspective.getFrustum();
        int x0out = 0;
        int x1out = 0;
        int y0out = 0;
        int y1out = 0;
        int z0out = 0;
        int z1out = 0;

        int n = polygon.getLength();
        tmpPolygon.setLength(0);
        // after clipping at the clipping plane vertices need not be in order
        // anymore!
        // for (int v = p1.vertices[0], i = n;
        // i > 0;
        // i--, v += Polygon.VERTEX_LENGTH) {
        
        for (int i = 0; i < n; i++) {
            double[] v = polygon.getPoint(i);
            double vsw = v[AbstractPolygon.SW];
            if (v[AbstractPolygon.SX] < perspective.getFrustumXmin() * vsw)
                x0out++;
            if (v[AbstractPolygon.SX] > perspective.getFrustumXmax() * vsw)
                x1out++;

            if (v[AbstractPolygon.SY] < perspective.getFrustumYmin() * vsw)
                y0out++;
            if (v[AbstractPolygon.SY] > perspective.getFrustumYmax() * vsw)
                y1out++;

            if (v[AbstractPolygon.SZ] < perspective.getFrustumZmin() * vsw)
                z0out++;
            if (v[AbstractPolygon.SZ] > perspective.getFrustumZmax() * vsw)
                z1out++;

        }

        if (x0out + x1out + y0out + y1out + z0out + z1out == 0) {
            // shader.shadePolygon(p1, vertexData, environment);
            return false; /* POLY_CLIP_IN */
        }
        if (x0out == n || x1out == n || y0out == n || y1out == n || z0out == n
                || z1out == n) {
            // p1->n = 0;
            // System.out.println("OUT "+(n*Polygon.VERTEX_LENGTH));
            return true; /* POLY_CLIP_OUT */
        }
        // shader.shadePolygon(p1, vertexData, environment);
        // now clip:
        // if (x0out) CLIP_AND_SWAP(sx, -1., box->x0, p, q, r);
        tmpPolygon.setShadingFrom(polygon);
        if (x0out != 0)
            clipToHalfspace(AbstractPolygon.SX, -1, -perspective
                    .getFrustumXmin());
        if (x1out != 0)
            clipToHalfspace(AbstractPolygon.SX, 1, perspective.getFrustumXmax());

        if (y0out != 0)
            clipToHalfspace(AbstractPolygon.SY, -1, -perspective
                    .getFrustumYmin());
        if (y1out != 0)
            clipToHalfspace(AbstractPolygon.SY, 1, perspective.getFrustumYmax());

        if (z0out != 0)
            clipToHalfspace(AbstractPolygon.SZ, -1, -perspective
                    .getFrustumZmin());
        if (z1out != 0)
            clipToHalfspace(AbstractPolygon.SZ, 1, perspective.getFrustumZmax());

        if (polygon.getLength() == 0)
            return true; // should not happen...
        else
            return false;
    }

    /**
     * This helper method is similar to PolygonUtility.clipToHalfSpace()... but
     * uses that the planes are axix aligned.
     * 
     * @param index
     *            The index to compare --- e.g. Polyggn.SX
     * @param sign
     *            +/-1 which side of the halfspace.
     * @param k
     *            the location of the halfspace --- e.g. ClippingBox.x0
     */
    private void clipToHalfspace(int index, int sign, double k) {
        tmpPolygon.setLength(0);
        int length = polygon.getLength();
        if (length == 0)
            return;
        // int vc = vertexCount;
        double[] u = polygon.getPoint(length - 1);
        double[] v = polygon.getPoint(0);
        double tu = sign * u[index] - k * u[AbstractPolygon.SW];
        double tv = 0;
        
        int pos = 0;
        for (int i = 0; i < length; i++, u = v, tu = tv ){
            v = polygon.getPoint(i);
            tv = sign * v[index] - k * v[AbstractPolygon.SW];
            if (tu <= 0. ^ tv <= 0.) { // edge crosses plane...
                double t = tu / (tu - tv);

                double[] vertexData = tmpPolygon.getPoint(pos++);

                for (int j = 0; j < AbstractPolygon.VERTEX_LENGTH; j++) {
                    vertexData[j] = u[j] + t * (v[j] - u[j]);
                }
            }
            if (tv <= 0.) { // vertex v is in ...
                tmpPolygon.setPointFrom(pos++, v);
            }
        }
        Polygon r = polygon;
        polygon = tmpPolygon;
        tmpPolygon = r;
    }

    /**
     * Sets the current matrix for the
     * 
     * @link processPolygon method.
     * @param matrix
     *            The matrix to set
     */
    public final void setMatrix(final double[] matrix) {
        this.matrix = matrix;
        Rn.transpose(inverseTransposeMatrix, matrix);
        Rn.inverse(inverseTransposeMatrix, inverseTransposeMatrix);
        environment.setMatrix(matrix);
    }

    public final void sortTriangles() {
        // int n = triangles.getSize();
        // System.err.println(" "+n+" sorted trinagles to render");
        // eSystem.out.println("scheduled polys "+polygonCount);
        // it might be better to sort the non transparent polygons too
        // since the setPixel call is one of the most speed relevant. If the
        // polygons are drawn front to back (the reverse oder compared to the
        // painter's algorithm) the setPixel calls return immediately:
        Arrays.sort(triangles.getArray(), 0, triangles.getSize(), comp);
        // Arrays.sort(transparentPolygons,0,transparentPolygonCount,comp);
    }

    /**
     * @return Perspective
     */
    public final CameraProjection getPerspective() {
        return perspective;
    }

    /**
     * Sets the perspective.
     * 
     * @param perspective
     *            The perspective to set
     */
    public final void setPerspective(final CameraProjection perspective) {
        this.perspective = perspective;
    }

    /**
     * @return Environment
     */
    public final Environment getEnvironment() {
        return environment;
    }

    /**
     * Sets the environment.
     * 
     * @param environment
     *            The environment to set
     */
    public final void setEnvironment(Environment environment) {
        this.environment = environment;
    }

    /**
     * 
     */
private final void rasterRemaining() {
        // we render the polygons front to back in order to have less to do in
        // setPixel. transparent polys are
        // sorted in reverse order and at the beginning, so we know, that they
        // are rendered last and in
        // back to front order.
        // for (int i = polygonCount; i > 0;) {
        // renderer.renderPolygon(polygons[--i],vertexData,polygons[i].getShader().isOutline());
        // polygons[i].setShader(null);
        // }
        sortTriangles();
        rasterizer.setTransparencyEnabled(true);
        while(! triangles.isEmpty()) {
            Triangle tri = triangles.pop();
            rasterizer.renderTriangle(tri,false);
            freeTriangles.push(tri);
            // renderer.renderPolygon(p, vertexData, p.getShader().isOutline());
        }

    }
    private double[] point0 = new double[4];

    private double[] normal0 = new double[3];

    private double[] point1 = new double[4];

    private double[] normal1 = new double[3];
    private DoubleArrayArray normal1DataList = new DoubleArrayArray.Inlined(normal1,3);

    private double[] substMatrix = new double[16];

    
    public final void processPseudoSphere(final double[] data, int index, int length) {
        if (pointShader == null)
            return;
        double r = pointShader.getPointRadius();
        if (length == 4)
            processPseudoSphere(data[index], data[index + 1], data[index + 2],
                    data[index + 3],r,null);
        else
            processPseudoSphere(data[index], data[index + 1], data[index + 2], 1,r,null);
    }

    public final void processPseudoSphere(final DoubleArrayArray data, int index,DataList vertexColors,DataList vertexRadii) {
        DoubleArray da = data.item(index).toDoubleArray();
        double w = 1;
        if (da.size() == 4)
            w = da.getValueAt(3);
       // DataList vertexColorsOld;
        if (vertexColors!=null) {
            DoubleArray color = vertexColors.item(index).toDoubleArray();
            for (int i = 0; i < 16; i++) {
                pointColors[i][0] = color.getValueAt(0);
                pointColors[i][1] = color.getValueAt(1);
                pointColors[i][2] = color.getValueAt(2);
                pointColors[i][3] = (color.size() > 3) ? color.getValueAt(3) : 1.;
            }
        }
        double r;
        if(vertexRadii != null)
            r = vertexRadii.toDoubleArray().getValueAt(index);
            else r = pointShader.getPointRadius();
        // vertexColorsOld = vertexColors;
        // vertexColors = pointColorsDataList;
        processPseudoSphere(da.getValueAt(0), da.getValueAt(1), da.getValueAt(2), w,r,vertexColors!=null?pointColorsDataList:null);
        // vertexColors = vertexColorsOld;
    }

    public final void processPseudoSphere(final double x, final double y,
            final double z, final double w,double pointRadius,DataList pointColors) {
        if (pointShader == null)
            return;
        
        VecMat.transform(matrix, x, y, z, w, point0, 0);
        VecMat.transformUnNormalized(matrix, 0, 0, pointRadius, normal0, 0);
        double[] mat = matrix;
        double[] tmat = inverseTransposeMatrix;
        matrix = substMatrix;
        double outlineFraction = pointShader.getOutlineFraction();
        double d = 1 - outlineFraction;
        // inner point :
        double l = VecMat.norm(normal0);
        VecMat.assignScale(matrix, l * d);
        matrix[4 * 0 + 3] = point0[0];
        matrix[4 * 1 + 3] = point0[1];
        matrix[4 * 2 + 3] = point0[2] + pointRadius;
        Rn.transpose(inverseTransposeMatrix, matrix);
        Rn.inverse(inverseTransposeMatrix, inverseTransposeMatrix);
        shader = pointShader.getCoreShader();
        // computeArray(pointVertices, pointIndices, zNormal, pointNormals);
        
        process(pointVerticesDataList,pointIndicesDataList,zNormalDataList,pointNormalsDataList,null,pointColors,null,null,true);
        // outline :
        if (outlineFraction > 0 /* && pointShader!= null */) {
            shader = pointShader.getOutlineShader();
            d = 1 / d;

            for (int i = 0; i < 24; i += 3) {
                pointVertices[24 + i] = pointVertices[i] * d;
                pointVertices[25 + i] = pointVertices[i + 1] * d;
                pointVertices[26 + i] = -d;
            }
            for (int i = 0; i < 8; i++) {
// computeArray(pointVertices, pointOutlineIndices[i], zNormal,
// pointOutlineNormals[i]);
                process(pointVerticesDataList,pointOutlineIndicesDataList.item(i).toIntArray(),zNormalDataList,pointOutlineNormalsDataList.item(i).toIntArray(),null,pointColorsDataList,null,null,true);
            }
        }

        // reset the matrix:
        matrix = mat;
        inverseTransposeMatrix = tmat;

    }
    
    private final static DoubleArrayArray procPointNormalData = new DoubleArrayArray.Array(new double[][] {{0,0,1}});
    private final static IntArray procPointNormals = new IntArray(new int[]{0,0,0,0});
    private final static DoubleArray zNormalDoubleArray = new DoubleArray(new double[] {0,0,1});
    
    public final void processPoint(final DoubleArrayArray data, int index,DataList vertexColors,DataList vertexRadii) {
        if (vertexColors!=null) {
            DoubleArray color = vertexColors.item(index).toDoubleArray();
            constantVertexShader.setColor(color.getValueAt(0),color.getValueAt(1),color.getValueAt(2));
        }        
        shader = pointShader.getCoreShader();
        constantVertexShader.setColor(shader.getRed(), shader.getGreen(), shader.getBlue());
        shader = constantLineShader;
        // the following is similar to a
        // process(...); call
        // 
        polygon.setLength(0);
        tmpPolygon.setLength(0);
       
        IntArray vertices = new IntArray(new int[]{index,index,index,index});
        fillVertexData(data, vertices, procPointNormalData, procPointNormals, null, null,
                zNormalDoubleArray, null,true);
        
        // shade
        shader.shadePolygon(polygon, environment,false);
        
        // return early if polygon is clipped out
        // TODO: debug clipPlanes
        if (clipPlanes())
            return;
        computePerspective();
        
        //
        // now we add width to the line:
        //
        double minDim = rasterizer.getMinDim();
        double pointSize = pointShader.getPointSize();
        
        double[] f1 = polygon.getPoint(0);
        double[] f2 = polygon.getPoint(1);
        double[] t1 = polygon.getPoint(2);
        double[] t2 = polygon.getPoint(3);
        
        pointSize/= minDim;
        f1[Polygon.SX] += f1[Polygon.SW]*pointSize;
        
        f2[Polygon.SY] += f2[Polygon.SW]*pointSize;

        t1[Polygon.SX] -= t1[Polygon.SW]*pointSize;
        
        t2[Polygon.SY] -= t2[Polygon.SW]*pointSize;

        // clip to frustum:
        if (clipFrustum())
           return;
        
        triangulateAndRaster();
    }
    
    

    private static final double[] identity = new double[] { 1, 0, 0, 0, 0, 1,
            0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

    private double[] line = new double[12];
    DoubleArrayArray lineDataList = new DoubleArrayArray.Inlined(line,3);
    
    private double[] direction = new double[3];

    
    private static final IntArray procLineVertices = new IntArray(new int[]{0,0,1,1});
    private static final IntArray procLineNormals = new IntArray(new int[]{0,0,0,0});
    private static final DoubleArray faceNormalDataList = new DoubleArray(new double[] {0,0,1});
    
    public final void processLine(DoubleArray from, DoubleArray to) {
        if (lineShader == null)
            return;
        shader = lineShader.getPolygonShader();
        constantVertexShader.setColor(shader.getRed(), shader.getGreen(), shader.getBlue());
        shader = constantLineShader;
        // the following is similar to a
        // process(...); call
        // 
        polygon.setLength(0);
        tmpPolygon.setLength(0);
        
        DoubleArrayArray vd = new DoubleArrayArray.Array(new double[][] { from.toDoubleArray(null), to.toDoubleArray(null)});
        
        fillVertexData(vd, procLineVertices, zNormalDataList, procLineNormals, null, null,
                faceNormalDataList,null,true);
        
        // shade
        shader.shadePolygon(polygon, environment,false);
        
        // return early if polygon is clipped out
        // TODO: debug clipPlanes
        if (clipPlanes())
            return;
        computePerspective();
        
        //
        // now we add width to the line:
        //
        double lineWidth = lineShader.getLineWidth();
        double minDim = rasterizer.getMinDim();
        
        double[] f1 = polygon.getPoint(0);
        double[] f2 = polygon.getPoint(1);
        double[] t1 = polygon.getPoint(2);
        double[] t2 = polygon.getPoint(3);
        
        double fx = f1[Polygon.SX]*minDim/f1[Polygon.SW];
        double fy = f1[Polygon.SY]*minDim/f1[Polygon.SW];
        double tx = t1[Polygon.SX]*minDim/t1[Polygon.SW];
        double ty = t1[Polygon.SY]*minDim/t1[Polygon.SW];
        
        double dirx = tx - fx;
        double diry = ty - fy;
        double fac = lineWidth/Math.sqrt(dirx*dirx + diry*diry);
        dirx *= fac/minDim;
        diry *= fac/minDim;
        
        f1[Polygon.SX] += -f1[Polygon.SW]*diry;
        f1[Polygon.SY] += f1[Polygon.SW]*dirx;
        f2[Polygon.SX] -= -f2[Polygon.SW]*diry;
        f2[Polygon.SY] -= f2[Polygon.SW]*dirx;

        t1[Polygon.SX] -= -t1[Polygon.SW]*diry;
        t1[Polygon.SY] -= t1[Polygon.SW]*dirx;
        t2[Polygon.SX] += -t2[Polygon.SW]*diry;
        t2[Polygon.SY] += t2[Polygon.SW]*dirx;


        // clip to frustum:
        // TODO: debug clipPlanes

        if (clipFrustum())
           return;

        
        
        triangulateAndRaster();
    }
    
    public final void processLineOld(DoubleArray from, DoubleArray to) {
        if (lineShader == null)
            return;
        shader = lineShader.getPolygonShader();
        double lineWidth = lineShader.getLineWidth();
        double w = 1;
        if (from.size() == 4)
            w = from.getValueAt(3);
        VecMat.transform(matrix, from.getValueAt(0), from.getValueAt(1), from
                .getValueAt(2), w, point0, 0);

        VecMat.transformUnNormalized(matrix, 0, 0, lineWidth, normal0, 0);
        // Rn.normalize(normal0, normal0);
        // Rn.times(normal0, lineWidth/rasterizer.getMinDim(), normal0);
        w = 1;
        if (to.size() == 4)
            w = from.getValueAt(3);
        VecMat.transform(matrix, to.getValueAt(0), to.getValueAt(1), to
                .getValueAt(2), w, point1, 0);
        // VecMat.transformUnNormalized(matrix,0,0,.2,normal0,0);

        
        
        
        double[] mat = matrix;
        double[] tmat = inverseTransposeMatrix;
        matrix = identity;
        inverseTransposeMatrix = identity;
        // VecMat.assignIdentity(matrix);

        direction[0] = point1[0] - point0[0];
        direction[1] = point1[1] - point0[1];
        direction[2] = point1[2] - point0[2];
        // TODO make only one call to norm...
        if (VecMat.norm(direction) == 0) {
            matrix = mat;
            inverseTransposeMatrix = tmat;
            return;
        }
        VecMat.normalize(direction);
        double length = VecMat.norm(normal0);
        VecMat.cross(direction, zNormal, normal0);
        VecMat.normalize(normal0);
        normal0[0] *= length;
        normal0[1] *= length;
        normal0[2] *= length;

        VecMat.cross(normal0, direction, normal1);
        VecMat.normalize(normal1);

        line[0] = point0[0] - normal0[0];
        line[1] = point0[1] - normal0[1];
        line[2] = point0[2] + length; // - normal0[2];

        line[3] = point0[0] + normal0[0];
        line[4] = point0[1] + normal0[1];
        line[5] = point0[2] + length; // + normal0[2];

        line[6] = point1[0] + normal0[0];
        line[7] = point1[1] + normal0[1];
        line[8] = point1[2] + length; // + normal0[2];

        line[9] = point1[0] - normal0[0];
        line[10] = point1[1] - normal0[1];
        line[11] = point1[2] + length; // + normal0[2];

// computeArray(line, lineIndices, normal1, pointNormals);
        process(lineDataList,lineIndicesDataList,normal1DataList,pointNormalsDataList,null,null,null,null,true);
        matrix = mat;
        inverseTransposeMatrix = tmat;

    }

    private double[] normal2 = new double[6];
    private DoubleArrayArray normal2DataList = new DoubleArrayArray.Inlined(normal2,3);
    
    // private static int[] nIndices = { 0, 3, 3, 0 };
    private static int[] nIndices = { 0, 1, 1, 0 };
    private final IntArray nIndicesDataList = new IntArray(nIndices);
    
    private static final double cs = Math.cos(0.2);

    private static final double ss = Math.sin(0.2);
/*
 * private double test1[] = new double[3];
 * 
 * private double test2[] = new double[3];
 * 
 * private DoubleArray tda1 = new DoubleArray(test1);
 * 
 * private DoubleArray tda2 = new DoubleArray(test2);
 * 
 * private double test3[] = new double[3];
 * 
 * private double test4[] = new double[3];
 * 
 * private DoubleArray tda3 = new DoubleArray(test3);
 * 
 * private DoubleArray tda4 = new DoubleArray(test4);
 */
    private double[] zzNormal = new double[3];

private boolean transparencyEnabled;

    
    public final void processPseudoTube(DoubleArray from, DoubleArray to, double lineWidth,DoubleArray colors) {

        if (lineShader == null)
            return;
        shader = lineShader.getPolygonShader();
        // double lineWidth = lineShader.getTubeRadius();
        double w = 1;
        if (from.size() == 4)
            w = from.getValueAt(3);
        VecMat.transform(matrix, from.getValueAt(0), from.getValueAt(1), from
                .getValueAt(2), w, point0, 0);

        VecMat.transformUnNormalized(matrix, 0, 0, lineWidth, normal0, 0);
        w = 1;
        if (to.size() == 4)
            w = to.getValueAt(3);
        VecMat.transform(matrix, to.getValueAt(0), to.getValueAt(1), to
                .getValueAt(2), w, point1, 0);

        double[] mat = matrix;
        double[] tmat = inverseTransposeMatrix;
        matrix = identity;
        inverseTransposeMatrix = identity;

        direction[0] = point1[0] - point0[0];
        direction[1] = point1[1] - point0[1];
        direction[2] = point1[2] - point0[2];

        // TODO: decide: better but slower:
        zzNormal[0] = point1[0] + point0[0];
        zzNormal[1] = point1[1] + point0[1];
        zzNormal[2] = point1[2] + point0[2];
        VecMat.normalize(zzNormal);

        // TODO make only one call to norm...
        if (VecMat.norm(direction) == 0) {
            matrix = mat;
            inverseTransposeMatrix = tmat;

            return;
        }
        VecMat.normalize(direction);
        double length = VecMat.norm(normal0);
        VecMat.cross(direction, zzNormal, normal0);
        VecMat.normalize(normal0);
        VecMat.cross(normal0, direction, normal1);

        normal0[0] *= length;
        normal0[1] *= length;
        // allways zero:
        normal0[2] *= length;

        VecMat.normalize(normal1);

        if (VecMat.dot(normal1, zzNormal) < 0) {
            normal1[0] *= -1;
            normal1[1] *= -1;
            normal1[2] *= -1;
            System.out.println("flip");
        }

        line[0] = point0[0] - normal0[0];
        line[1] = point0[1] - normal0[1];
        line[2] = point0[2];

        line[3] = point0[0];
        line[4] = point0[1];
        line[5] = point0[2] + length;

        line[6] = point1[0];
        line[7] = point1[1];
        line[8] = point1[2] + length;

        line[9] = point1[0] - normal0[0];
        line[10] = point1[1] - normal0[1];
        line[11] = point1[2];

        normal2[0] = +cs / length * normal0[0] + ss * normal1[0];
        normal2[1] = +cs / length * normal0[1] + ss * normal1[1];
        normal2[2] = ss * normal1[2];

        normal2[3] = normal1[0];
        normal2[4] = normal1[1];
        normal2[5] = normal1[2];

        // computeArrayNoTransform(line, lineIndices, normal2, nIndices);
        process(lineDataList,lineIndicesDataList,normal2DataList,nIndicesDataList,null,null,null,colors,false);

        // ///

        line[0] = point0[0];
        line[1] = point0[1];
        line[2] = point0[2] + length;

        line[3] = point0[0] + normal0[0];
        line[4] = point0[1] + normal0[1];
        line[5] = point0[2];

        line[6] = point1[0] + normal0[0];
        line[7] = point1[1] + normal0[1];
        line[8] = point1[2];

        line[9] = point1[0];
        line[10] = point1[1];
        line[11] = point1[2] + length;

        normal2[0] = normal1[0];
        normal2[1] = normal1[1];
        normal2[2] = normal1[2];

        normal2[3] = -cs / length * normal0[0] + ss * normal1[0];
        normal2[4] = -cs / length * normal0[1] + ss * normal1[1];
        normal2[5] = ss * normal1[2];

        // computeArrayNoTransform(line, lineIndices, normal2, nIndices);
        process(lineDataList,lineIndicesDataList,normal2DataList,nIndicesDataList,null,null,null,colors,false);
        /*
         * test1[0] = point1[0]; test1[1] = point1[1]; test1[2] = point1[2];
         * test2[0] = point1[0]+normal1[0]; test2[1] = point1[1]+normal1[1];
         * test2[2] = point1[2]+normal1[2]; test3[0] = point0[0]; test3[1] =
         * point0[1]; test3[2] = point0[2]; test4[0] = point0[0]+normal1[0];
         * test4[1] = point0[1]+normal1[1]; test4[2] = point0[2]+normal1[2];
         * processLine(tda1,tda2); processLine(tda3,tda4);
         */

        matrix = mat;
        inverseTransposeMatrix = tmat;

    }

    /**
     * @return PolygonShader
     */
    public PolygonShader getFaceShader() {
        return faceShader;
    }

    /**
     * Sets the faceShader.
     * 
     * @param faceShader
     *            The faceShader to set
     */
    public void setFaceShader(PolygonShader faceShader) {
        this.faceShader = faceShader;
    }

    /**
     * @return PolygonShader
     */
    public LineShader getLineShader() {
        return lineShader;
    }

    /**
     * @return PolygonShader
     */
    public PointShader getPointShader() {
        return pointShader;
    }

    /**
     * Sets the lineShader.
     * 
     * @param lineShader
     *            The lineShader to set
     */
    public void setLineShader(LineShader lineShader) {
        this.lineShader = lineShader;
    }

    /**
     * Sets the pointShader.
     * 
     * @param pointShader
     *            The pointShader to set
     */
    public void setPointShader(PointShader pointShader) {
        this.pointShader = pointShader;
        if (pointShader != null) {
        }
    }

    public void finish() {
        rasterRemaining();
        
    }

    public void transformNDC(double[] p1,double[] p2) {
        VecMat.transformUnNormalized(matrix, p2[0], p2[1], p2[2], p2);
        VecMat.transform(matrix, p1[0], p1[1], p1[2], p1);
        double r = VecMat.norm(p2);
        p2[0] = r + p1[0];
        p2[1] =  p1[1];
        p2[2] =  p1[2];
        perspective.perspective(p1);
        perspective.perspective(p2);
    }

    public void setTransparencyEnabled(boolean transparencyEnabled) {
        this.transparencyEnabled = transparencyEnabled;
        rasterizer.setTransparencyEnabled(transparencyEnabled);
    }

    
    /**
     * @return PolygonShader
     */
    // public PolygonShader getPointOutlineShader() {
    // return pointOutlineShader;
    // }
    /**
     * Sets the pointOutlineShader.
     * 
     * @param pointOutlineShader
     *            The pointOutlineShader to set
     */
    // public void setPointOutlineShader(PolygonShader pointOutlineShader) {
    // this.pointOutlineShader = pointOutlineShader;
    // }
    /**
     * @return double
     */
// public double getOutlineFraction() {
// return outlineFraction;
// }

    /**
     * Sets the outlineFraction.
     * 
     * @param outlineFraction
     *            The outlineFraction to set
     */
// public void setOutlineFraction(double outlineFraction) {
// this.outlineFraction = outlineFraction;
// }
}
