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

import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Vector;

public class IntersectingPipeline extends TrianglePipeline {

    public IntersectingPipeline(TriangleRasterizer rasterizer, boolean sortAll) {
        super(rasterizer, sortAll);
    }

    private int count;

    public void finish() {
        int preCount = triangles.getSize();
        // super.finish();
        count = 0;
        rasterRemaining_new();
        System.out.println(" rastered " + count + " triangles (" + preCount
                + ")");
        System.out.println(" polys " + polys.size());
        System.out.println(" ignore " + ignore.size());
        System.out.println(" obstruct " + obstruct.size());
    }

    LinkedList<AbstractPolygon> polys = new LinkedList<AbstractPolygon>();

    LinkedList<AbstractPolygon> postponed = new LinkedList<AbstractPolygon>();

    HashMap<AbstractPolygon, List<AbstractPolygon>> ignore = new HashMap<AbstractPolygon, List<AbstractPolygon>>();

    HashMap<AbstractPolygon, List<AbstractPolygon>> obstruct = new HashMap<AbstractPolygon, List<AbstractPolygon>>();

    Vector<Pair> looP = new Vector<Pair>();

    private List<AbstractPolygon> getIgnoreList(AbstractPolygon p) {
        List<AbstractPolygon> l = ignore.get(p);
        if (l == null) {
            l = new LinkedList<AbstractPolygon>();
            ignore.put(p, l);
        }
        return l;
    }

    private List<AbstractPolygon> getObstructList(AbstractPolygon p) {
        List<AbstractPolygon> l = obstruct.get(p);
        if (l == null) {
            l = new LinkedList<AbstractPolygon>();
            obstruct.put(p, l);
        }
        return l;
    }

    private Comparator<AbstractPolygon> comparator = new Comparator<AbstractPolygon>() {

        public int compare(AbstractPolygon o1, AbstractPolygon o2) {
            double z = PolygonUtility.maxZ(o1);
            z -= PolygonUtility.maxZ(o2);
            return z < 0 ? 1 : (z > 0 ? -1 : 0);
        }

    };

    /**
     * new
     */
    private void rasterRemaining_new() {
        rasterizer.setTransparencyEnabled(true);
        ignore.clear();
        obstruct.clear();
        // we need to dehomogenize for the intersection to work
        int n = triangles.getSize();
        // Triangle[] array = triangles.getArray();
        for (int i = 0; i < n; i++) {
            Triangle t = triangles.pop();
            PolygonUtility.dehomogenize(t);
            polys.add(t);
        }
        // hope this helps
        // sortTriangles();
        Collections.sort(polys, comparator);
        // System.out.println("first "+tris.get(0).getCenterZ());
        // System.out.println("last "+tris.peek().getCenterZ());
        while (!polys.isEmpty()) {
            AbstractPolygon t = polys.removeFirst();
            // todo.add(t);
            rasterQueque_new(polys, t, 0, 0);
            // while(!postponed.isEmpty()) {
            // t = postponed.removeFirst();
            // rasterQueque_new(tris, t, 0,0);
            // }
        }

    }

    private Triangle[] tris = new Triangle[1];

    private void rasterQueque_new(LinkedList<AbstractPolygon> polys,
            AbstractPolygon a, int n, int depth) {
        // System.out.println("rqn "+n+" "+tris.size());
        boolean passed = test_new(polys, a, n, depth);
        if (passed) {
            //PolygonUtility.dehomogenize(a);
            // passed everything: raster...
            tris = a.triangulate(tris, freeTriangles);
            for (int i = 0, l = a.getLength() - 2; i < l; i++) {
                rasterizer.renderTriangle(tris[i], false);
                freeTriangles.push(tris[i]);
                count++;
            }
            ignore.remove(a);
            obstruct.remove(a);
            //System.out.println("rastered " + count);
            looP.removeAllElements();
        }
    }

    private boolean test_new(LinkedList<AbstractPolygon> polys,
            AbstractPolygon a, int n, int depth) {

        List<AbstractPolygon> ignoreForA = getIgnoreList(a);

        List<AbstractPolygon> obstructsA = getObstructList(a);
        

        double minA = PolygonUtility.minZ(a);
        for (int i = n; i < polys.size(); i++) {
            AbstractPolygon b = polys.get(i);
        List<AbstractPolygon> obstructsB = getObstructList(b);
            if (PolygonUtility.maxZ(b) < minA)
                return true;
            // System.out.println(" a "+getColor(a)+" on b
            // ("+i+")"+getColor(b)+"\n (depth "+depth+", remaining
            // "+polys.size()+": ");
            // for (int j = 0; j < polys.size(); j++) {
            // System.out.println("___"+getColor(polys.get(j)));
            // }
            // System.out.println( ")");
            int test;
            if (ignoreForA.contains(b))
                //test = 1;
                continue;
            else 
                if (obstructsA.contains(b))
                test = -1;
            else
                test = PolygonUtility.liesBehind(a, b);

            if (test == 1) { // a behind b
            // System.out.println("trough "+i);
                ignoreForA.add(b);
                continue;
            } else if (test == -1) { // a before b
                Pair p = new Pair(a, b);
                if (looP.contains(p)) {
                   
                    
                    AbstractPolygon[] array = PolygonUtility.cutOut(b, a);
                    System.out.println("obstruction loop resolved by adding "
                            + array.length + " polygons (to " + polys.size()
                            + " depth " + depth + ") loopsize " + looP.size());

                    polys.remove(i);
                    List<AbstractPolygon> ib = getIgnoreList(b);
                    obstruct.remove(b);
                    ignore.remove(b);
                    if (b instanceof Triangle)
                        freeTriangles.push((Triangle) b);
                    for (int j = 0; j < array.length; j++) {

                        insert(polys, array[j]);
                        ignoreForA.add(array[j]);
                        List<AbstractPolygon> l = new LinkedList<AbstractPolygon>();
                        l.add(a);
                        l.addAll(ib);
                        for (int k = 0; k < array.length; k++) {
                            if (k != j)
                                l.add(array[k]);
                        }
                        ignore.put(array[j], l);
                    }
                    
                    continue;
                    
                } else { // not yet in loop
                // System.out.println("obstructed by "+i);
                   
                    polys.remove(i);
                    looP.add(p);
                    // System.out.println("loopsize "+looP.size());
                    insert(polys, a);
                    obstructsA.add(b);
                    List<AbstractPolygon> ignoreForB = getIgnoreList(b);
                    ignoreForB.add(a);
                    polys.add(0, b);
                    // rasterQueque_new(tris, b, 0,depth+1);
                    return false;
                }
            } else { // test == 0  -> intersect
                if(obstructsB.contains(a))
                    System.err.println("try to do unncecessary cut");
            // System.out.println("intersect_n "+i+" (depth "+depth+" remaining
            // "+tris.size()+")");

                // intersect a along the plane of b and get both parts
                //
                //resultBehindB can be rastered prior to b
                Polygon resultBehindB = new Polygon(3);
//              resultBeforeB ns the part of a in front of b
                Polygon resultBeforeB = new Polygon(3);
                
                PolygonUtility.intersect(a, b, 1, resultBeforeB, resultBehindB);
                for (List<AbstractPolygon> l : ignore.values()) {
                    if(l.contains(a)) {
                        l.remove(a);
                        l.add(resultBeforeB);
                        l.add(resultBehindB);
                    }
                }
                
                List<AbstractPolygon> ignoreForB = getIgnoreList(b);
                ignoreForB.add(resultBeforeB);
                //List<AbstractPolygon> obstructsB = getObstructList(b);
                obstructsB.add(resultBehindB);

                // ignore for the resultBehindB polygon
                List<AbstractPolygon> l = new LinkedList<AbstractPolygon>();
                l.add(b);
                l.add(resultBeforeB);
                l.addAll(ignoreForA);
                ignore.put(resultBehindB, l);
                
//              obstruct for the resultBeforeB polygon
                l = new LinkedList<AbstractPolygon>();
                l.add(b);
                obstruct.put(resultBeforeB, l);

//              ignore for the resultBeforeB polygon
                l = new LinkedList<AbstractPolygon>();
                l.addAll(ignoreForA);
                l.add(resultBehindB);
                ignore.put(resultBeforeB, l);
                
                insert(polys, resultBeforeB);
                insert(polys, resultBehindB);
                if (a instanceof Triangle)
                    freeTriangles.push((Triangle) a);
                obstruct.remove(a);
                ignore.remove(a);
                // restart
                looP.removeAllElements();
                return false;
            }
        }
        // trough every thing: we can raster
        return true;
    }

    private String getColor(AbstractPolygon b) {
        double[] v = b.getPoint(0);
        return "[" + v[Triangle.R] + ", " + v[Triangle.G] + ", "
                + v[Triangle.B] + "]";
    }

    private void insert(LinkedList<AbstractPolygon> tris2,
            AbstractPolygon triangle) {
        int i = Collections.binarySearch(tris2, triangle, comparator);
        if (i < 0) {
            i = -i - 1;
        }
        tris2.add(i, triangle);
    }

    private class Pair {
        private AbstractPolygon one;

        private AbstractPolygon two;

        Pair(AbstractPolygon t1, AbstractPolygon t2) {
            one = t1;
            two = t2;
        }

        public boolean equals(Object obj) {
            if (obj instanceof Pair) {
                Pair p = (Pair) obj;
                return one == p.one && two == p.two;
            } else
                return false;
        }

    }

}
