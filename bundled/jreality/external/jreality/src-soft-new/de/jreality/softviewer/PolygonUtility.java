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

import java.util.Vector;

/**
 * This class capsules utility methods for intersecting polygons. It is
 * <em> not</em> speed optimized. It is mainly for file exports at the moment.
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 */
public class PolygonUtility {
    private static final boolean DEBUG = false;

    public static final int CLIPPED_OUT = -1;

    public static final int CLIPPED_PARTIAL = 1;

    public static final int CLIPPED_IN = 0;

    /**
     * 
     */
    public PolygonUtility() {
        super();
    }


    public static final void dehomogenize(final AbstractPolygon p) {
        for (int i = 0; i < p.getLength(); i++) {
            final double[] vd = p.getPoint(i);
            final double w = 1 / vd[AbstractPolygon.SW];
            vd[AbstractPolygon.SX] *= w;
            vd[AbstractPolygon.SY] *= w;
            vd[AbstractPolygon.SZ] *= w;
            //vd[AbstractPolygon.SZ] = -1/( vd[AbstractPolygon.SZ]*w);
            vd[AbstractPolygon.SW] = 1;
        }

    }

    private static double[] zNormal = new double[] {0,0,1};
    
    
    /**
     * we assume that the polygons are dehomogenized, planar and convex
     * 
     * @param a
     *            the polygon to test
     * @param b
     *            the polygon to test against
     * @return 1 if a lies behind or is independent of b, 0 if a and b
     *         intersect, and -1 if b lies behind a
     */
    public static int liesBehind(AbstractPolygon a, AbstractPolygon b) {
        // first test: coarse z separation:
        // has now already been done in intersecting pipeline
//        if (maxZ(b) < minZ(a))
//            return 1;
//        if (maxZ(a) < minZ(b))
//            return -1;
        
        double[] x = b.getPoint(0);
        double[] y = b.getPoint((0 + 1) );
        double[] z = b.getPoint((0 + 2) );

        // find the plane through the three
        double[] d1 = VecMat.difference(y, Triangle.SX, x,
                Triangle.SX);
        double[] d2 = VecMat.difference(z, Triangle.SX, x,
                Triangle.SX);

        double[] normalA = new double[3];
        VecMat.cross(d1, d2, normalA);
        VecMat.normalize(normalA);
        double d = VecMat.dot(normalA,0,
                x,Polygon.SX);
        int sign = (int) Math.signum(VecMat.dot(normalA, zNormal));
        
        // test if both lie in same plane:
        int inP = 0;
        int in  = 0;
        int out = 0;
        boolean noIntersect = false;
        
        //TODO: probably obsolete:
        // the call to testPolygonInHalfSpace below should do the same
        double dist0 = sign*(VecMat.dot(normalA,0,a.getPoint(0),Polygon.SX)-d);
        for(int i = 1,n = a.getLength(); i<n+1; i++) {
            double dist1 = dist0;
            dist0 = sign*(VecMat.dot(normalA,0,a.getPoint(i%n),Polygon.SX)-d);
            inP += Math.abs(dist1)<EPS ?1:0;
            in += dist1>EPS?1:0;
            out += dist1<-EPS?1:0;
            if(Math.abs(dist1)<EPS && Math.abs(dist0)<EPS) 
                noIntersect = true;
        }
        if(inP == a.getLength())
            return 1;
        if(out == 0)
            return 1;
        
        
        int result = testPolygonInHalfSpace(a,Polygon.SX, normalA,-sign,d);
        
        if(result == CLIPPED_IN) {
            //System.out.println("fast track");
            return 1;
        } 
        if(result == CLIPPED_OUT)
            noIntersect = true;

        
        // clip along the sides of b
        // if nothing is left a was behind
        AbstractPolygon p = a;
        Polygon clipPoly = null;
        for (int j = 0, n = b.getLength(); j < n; j++) {
            double[] lx = b.getPoint(j);
            double[] ly = b.getPoint((j + 1) % n);
            double[] lz = b.getPoint((j + 2) % n);

            // find the plane through x, y, and z normal
            double[] ld1 = VecMat.difference(lx, Triangle.SX,ly, Triangle.SX);
            //d2 = VecMat.difference(z, Triangle.SX, zNormal,1);

            double[] lnormalA = new double[3];
            VecMat.cross(ld1, zNormal, lnormalA);
            VecMat.normalize(lnormalA);
            double ldist = VecMat.dot(lnormalA,0, lx,Triangle.SX);
            int lsign = (int) Math.signum( VecMat.dot(lnormalA,0, lz,Triangle.SX)-ldist);
//          TODO hack. find out why we can get double points in polygons...
            if (lsign == 0) continue; 
            clipPoly = new Polygon(3);

            int lresult = clipToHalfspace(p,Polygon.SX,lnormalA,-lsign,ldist,clipPoly);
            if(lresult == CLIPPED_OUT) return 1;
            if(lresult == CLIPPED_PARTIAL)
                p = clipPoly;
        }
        // now p is the part of a which has a projection that lies inside the projection of b
        // it is left of find out if this rest still intersects the plane of b
        Polygon poly  = new Polygon(3);
        result = clipToHalfspace(p,Polygon.SX,normalA,-sign,d,poly);
        
        if(result == CLIPPED_PARTIAL) {
            // still intersection
            if(noIntersect) {
                System.err.println("intersection detected while two points are in plane");
                for (int i = 0,n = p.getLength(); i < n; i++) {
                    double[] v = p.getPoint(i);
                    double test = -sign
                            * (VecMat.dot(v, Polygon.SX, normalA, 0) - d * v[Polygon.SX + 3]);
                    if (test < -EPS)
                    //if (test > EPS)
                        return 1;
                }
            }else
                return 0;
        }
        if(result == CLIPPED_IN)
            // everything behind
            return 1;
        else 
            // in front
            return -1;
    }

    /**
     * 
     * @param a the triangle to be cut
     * @param b the triangle to cut along
     * @return
     */
    public static AbstractPolygon[] cutOut(AbstractPolygon a, AbstractPolygon b) {

        Vector<AbstractPolygon>  v = new Vector<AbstractPolygon>();
        
        // clip along the sides of b
        // if nothing is left a was behind
        AbstractPolygon p = a;
        Polygon clipPoly = null;
        final int length = b.getLength();
        for (int j = 0; j < length; j++) {
            double[] lx = b.getPoint(j);
            double[] ly = b.getPoint((j + 1) % length);
            double[] lz = b.getPoint((j + 2) % length);

            // find the plane through x, y, and z normal
            double[] ld1 = VecMat.difference(lx, Triangle.SX,ly, Triangle.SX);
            //d2 = VecMat.difference(z, Triangle.SX, zNormal,1);

            double[] lnormalA = new double[3];
            VecMat.cross(ld1, zNormal, lnormalA);
            VecMat.normalize(lnormalA);
            double ldist = VecMat.dot(lnormalA,0, lx,Triangle.SX);
            int lsign = (int) Math.signum( VecMat.dot(lnormalA,0, lz,Triangle.SX)-ldist);
                   
            clipPoly = new Polygon(3);

            int lresult = clipToHalfspace(p,Polygon.SX,lnormalA,-lsign,ldist,clipPoly);
            if(lresult == CLIPPED_PARTIAL) {
                Polygon poly  = new Polygon(3);
                clipToHalfspace(p,Polygon.SX,lnormalA,lsign,ldist,poly);
                v.add(poly);
                p = clipPoly;
            } else
                if(lresult == CLIPPED_OUT) {
                    //if(p.getLength()>2)
                    v.add(p);
                    p = clipPoly;
                    clipPoly.setLength(0);
                    break;
                }
                
        }
        if(p.getLength() != 0) {
            //v.addAll(Arrays.asList(p.triangulate(null,new ArrayStack(0))));
//            double[] x = b.getPoint(0);
//            double[] y = b.getPoint((0 + 1) % 3);
//            double[] z = b.getPoint((0 + 2) % 3);
//
//            // find the plane through the three
//            double[] d1 = VecMat.difference(y, Triangle.SX, x,
//                    Triangle.SX);
//            double[] d2 = VecMat.difference(z, Triangle.SX, x,
//                    Triangle.SX);
//
//            double[] normalA = new double[3];
//            VecMat.cross(d1, d2, normalA);
//            VecMat.normalize(normalA);
//            double d = VecMat.dot(normalA,0,
//                    x,Polygon.SX);
//            int sign = (int) Math.signum(VecMat.dot(normalA, zNormal));
//            int result = testPolygonInHalfSpace(a,Polygon.SX, normalA,sign,d);  
//            if(result == CLIPPED_IN) {
//                v.addAll(Arrays.asList(p.triangulate(null,new ArrayStack(0))));
//            } else if(result == CLIPPED_PARTIAL) {
//                
//            Polygon poly  = new Polygon (3);
//                result = clipToHalfspace(p,Polygon.SX,normalA,-sign,d,poly);
//            }
        }
        
        return v.toArray(new AbstractPolygon[0]);
    }
    
    
    
    private static String s(double[] u) {
        return "("+u[0]+", "+u[1]+", "+u[2]+")";
    }
    
    public static double minZ(AbstractPolygon p) {
        double min = Double.MAX_VALUE;
        for (int i = 0, n = p.getLength(); i < n; i++)
            min = Math.min(min, p.getPoint(i)[AbstractPolygon.SZ]/p.getPoint(i)[AbstractPolygon.SW]);
        return min;
    }

    public static double maxZ(AbstractPolygon p) {
        double max = -Double.MAX_VALUE;
        for (int i = 0, n = p.getLength(); i < n; i++)
            max = Math.max(max, p.getPoint(i)[AbstractPolygon.SZ]/p.getPoint(i)[AbstractPolygon.SW]);
        return max;
    }

    
//    public static double minZ(Triangle t) {
//        return Math.min(t.getP0()[AbstractPolygon.SZ], Math.min(
//                t.getP1()[AbstractPolygon.SZ], t.getP2()[AbstractPolygon.SZ]));
//    }
//
//    public static double maxZ(Triangle t) {
//        return Math.max(t.getP0()[AbstractPolygon.SZ], Math.max(
//                t.getP1()[AbstractPolygon.SZ], t.getP2()[AbstractPolygon.SZ]));
//    }

    //private static final double EPS = 0.0000000001;
    private static final double EPS = Math.ulp(2.);

    
    //ivate static final double EPS = 0.000000001

    private static int testTriangleInHalfSpace(Triangle t,
            double[] planeNormal, int sign, double k
    // TrianglePipeline pipeline
    ) {
        if (DEBUG) {
            System.out.println(" Normal: (" + planeNormal[0] + ", "
                    + planeNormal[1] + ", " + planeNormal[2] + ")");
        }

        // ****
        int hin = 0;
        int hout = 0;
        double[] vd = t.getP0();
        double test = sign
                * (VecMat.dot(vd, Triangle.SX, planeNormal, 0) - k
                        * vd[Triangle.SW]);
        if (test < -EPS)
            hin++;
        else if (test > EPS)
            hout++;

        vd = t.getP1();
        test = sign
                * (VecMat.dot(vd, Triangle.SX, planeNormal, 0) - k
                        * vd[Triangle.SW]);
        if (test < -EPS)
            hin++;
        else if (test > EPS)
            hout++;

        vd = t.getP2();
        test = sign
                * (VecMat.dot(vd, Triangle.SX, planeNormal, 0) - k
                        * vd[Triangle.SW]);
        if (test < -EPS)
            hin++;
        else if (test > EPS)
            hout++;

        if (hin == 0) {
            return CLIPPED_OUT;
        }
        if (hout == 0) {
            return CLIPPED_IN;
        }
        return CLIPPED_PARTIAL;
    }

    public static int clipTriangleToHalfspace(
    // int polPos,
            Triangle tri, double[] planeNormal, int sign, double k, Triangle a, // for
            // the
            // result
            Triangle b // for the result
    ) {

        int testResult = testTriangleInHalfSpace(tri, planeNormal, sign, k);

        if (testResult != CLIPPED_PARTIAL)
            return testResult;

        int result = 1;

        double[] u = tri.getPoint(2);
        double[] v = tri.getPoint(0);
        double tu = sign
                * (VecMat.dot(u, Triangle.SX, planeNormal, 0) - k
                        * u[Triangle.SW]);
        double tv = 0;
        // HERE!!!!!

        int newTriVertex = 0;
        Triangle newTri = a;
        a.setShadingFrom(tri);
        for (int i = 0; i < 3; i++, u = v, tu = tv) {
            v = tri.getPoint(i);
            tv = sign
                    * (VecMat.dot(v, Triangle.SX, planeNormal, 0) - k
                            * v[Triangle.SW]);
            if (DEBUG)
                System.out.println(" new tv " + tv);
            if (tu <= 0. ^ tv <= 0.) { // edge crosses plane...
                double t = tu / (tu - tv);

                double[] vd = newTri.getPoint(newTriVertex);
                for (int j = 0; j < Triangle.VERTEX_LENGTH; j++) {
                    vd[j] = u[j] + t * (v[j] - u[j]);
                }
                newTriVertex++;
            }
            if (tv <= 0.) { // vertex v is in ...
                newTri.setPointFrom(newTriVertex++, v);

                // newP.vertices[newP.length++] = v;
            }
            if (newTriVertex == 3) {
                b.setPointFrom(0, a.getP0());
                b.setPointFrom(1, a.getP2());
                b.setShadingFrom(tri);
                newTri = b;

                newTriVertex = 2;
                result = 2;
            }
        }
        return result;
    }

    private static int testPolygonInHalfSpace(final AbstractPolygon p,
            final int off, final double[] planeNormal, final int sign,
            final double k) {
        if (DEBUG) {
            System.out.println(" Normal: (" + planeNormal[0] + ", "
                    + planeNormal[1] + ", " + planeNormal[2] + ")");
            System.out.println(" p.length: " + p.getLength());
        }
        int length = p.getLength();
        if (length == 0)
            return CLIPPED_OUT;

        // ****
        int hin = 0;
        int hout = 0;

        for (int i = 0; i < length; i++) {
            double[] v = p.getPoint(i);

            double test = sign
                    * (VecMat.dot(v, off, planeNormal, 0) - k * v[off + 3]);
            if (DEBUG)
                System.out.println(" vertex: (" + v[off] + "," + " "
                        + v[off + 1] + ", " + v[off + 2] + ") ~ " + test);

            //original:
// if (test < -EPS)
// hin++;
// else if (test > EPS)
// hout++;
            
//            if (test < EPS)
//                hin++;
//                else if (test > -EPS)
//                hout++;
            if (test < -EPS)
                hin++;
            else if (test > EPS)
                hout++;
// 
        }
        if (hin == 0) {
            return CLIPPED_OUT;
        }
        if (hout == 0) {
            return CLIPPED_IN;
        }
        return CLIPPED_PARTIAL;
    }

    /**
     * @param p
     *            the polygon to clip
     * @param off
     *            the offset into vertex data (AbstractPolygon.SX or
     *            AbstractPolygon.WX
     * @param planeNormal
     *            the normal of the plane to clip against
     * @param sign
     *            the side of the plane
     * @param k
     *            the distance of the plane from the origin
     * @param dst
     *            the polygon to place the clipped result in.
     * @return
     */
    public static int clipToHalfspace(final AbstractPolygon p, final int off,
            final double[] planeNormal, final int sign, final double k,
            final Polygon dst) {

        int testResult = testPolygonInHalfSpace(p, off, planeNormal, sign, k);

        if (testResult != CLIPPED_PARTIAL)
            return testResult;
        // testResult == 1;
        final int length = p.getLength();

        double[] u = p.getPoint(length - 1);
        double[] v = p.getPoint(0);
        double tu = sign
                * (VecMat.dot(u, off, planeNormal, 0) - k * u[off + 3]);
        double tv = 0;
        // HERE!!!!!
        dst.setLength(0);
        int pos = 0;
        for (int i = 0; i < length; i++, u = v, tu = tv) {
            v = p.getPoint(i);
            tv = sign * (VecMat.dot(v, off, planeNormal, 0) - k * v[off + 3]);
            if (DEBUG)
                System.out.println(" new tv " + tv);
            if (tu <= 0. ^ tv <= 0.) { // edge crosses plane...
                double t = tu / (tu - tv);
                double[] vd = dst.getPoint(pos++);
                if (DEBUG)
                    System.out.println(" new vertex " + pos);
                for (int j = 0; j < Triangle.VERTEX_LENGTH; j++) {
                    vd[j] = u[j] + t * (v[j] - u[j]);
                }
            }
            // TODO original was as follows. check if it is safe this way...
            //if (tv < 0.) { // vertex v is in ...
            if (tv < 0.) { // vertex v is in ...
                dst.setPointFrom(pos++, v);
            }
        }
        dst.setShadingFrom(p);
        return CLIPPED_PARTIAL;
    }

    /**
     * @param a
     *            polygon to split
     * @param b
     *            polygon to intersect against
     * @param s
     *            signum giving the side of the plane of b to get.
     */
    public static void intersect(AbstractPolygon a, AbstractPolygon b, int s,
            Polygon resBefore, Polygon resBehind) {

        double[] x = b.getPoint(0);
        double[] y = b.getPoint(1);
        double[] z = b.getPoint(2);

        // find the plane through the three
        double[] d1 = VecMat.difference(y, Triangle.SX, x, Triangle.SX);
        double[] d2 = VecMat.difference(z, Triangle.SX, x, Triangle.SX);
        double[] normal = new double[3];
        VecMat.cross(d1, d2, normal);
        // might be lin dependent
//        if(VecMat.norm(normal)<EPS && b.getLength()>3) {
//            System.err.println("WARNING lindep points in poly");
//            z = b.getPoint(3);
//            d2 = VecMat.difference(z, Triangle.SX, x, Triangle.SX);
//            VecMat.cross(d1, d2, normal);
//        }
        VecMat.normalize(normal);
        double dist = VecMat.dot(normal, 0, x, Triangle.SX);
        int sign = (int) (s * Math.signum(VecMat.dot(normal, new double[] { 0,
                0, 1 })));
//        Triangle u = new Triangle();
//        Triangle v = new Triangle();
        int test = clipToHalfspace(a, Triangle.SX, normal, sign, dist, resBefore);
        int test2 = clipToHalfspace(a, Triangle.SX, normal, -sign, dist, resBehind);
        if (test != CLIPPED_PARTIAL) 
            System.err.println(" warning: intersect gives no intersection!!!");
        if (resBefore.getLength() < 3)
            System.err.println("WARNING intersect generated polygon of length "
                    + resBefore.getLength());
        
        if (resBehind.getLength() < 3)
            System.err.println("WARNING intersect generated polygon of length "
                    + resBehind.getLength());
    
    }

}
