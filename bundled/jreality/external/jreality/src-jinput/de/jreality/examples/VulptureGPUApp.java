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
import java.awt.Color;
import java.io.IOException;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Scene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Sphere;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.util.SystemProperties;

/*
 * Created on 11.01.2006
 *
 * This file is part of the  package.
 * 
 * This program is free software; you can redistribute and/or modify 
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation; either version 2 of the license, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITTNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the 
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307
 * USA 
 */

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class VulptureGPUApp implements Runnable {
    private static final boolean USE_VIDEO = false;
    
    de.jreality.shader.Texture2D tex;
    Appearance appearance = new Appearance();
    private static final int COLS = 8;
    private static final int ROWS = 40;
    private static final int SIZEC = COLS*COLS;
    private static final int SIZEL = 1;
    
    private static final int FISHC = 2;//80;
    private static final int FISHL = 40;

    
//    private static final boolean SURFACE = true;

    private SceneGraphComponent sphere1Component;
    private SceneGraphComponent sphere2Component;
    private SceneGraphComponent root;
    
    private IndexedFaceSet ifs = new IndexedFaceSet();
    private SceneGraphComponent fan = new SceneGraphComponent();
    double pts[] = new double[3*SIZEC*SIZEL];
    double ptsm[] = new double[3*SIZEC*SIZEL];
    double ptsmm[] = new double[3*SIZEC];
    
    private IndexedFaceSet ifsf = new IndexedFaceSet();
    private SceneGraphComponent fish = new SceneGraphComponent();
    double fts[] = new double[3*FISHC*FISHL];
    double ftsm[] = new double[3*FISHC*FISHL];
    double ftsmm[] = new double[3*FISHC];
    
    
    double xOld, yOld, zOld;
    double px, py, pz;
    double pxOld, pyOld, pzOld;
    
    double deltax, deltay, deltaz;
    double ddeltax, ddeltay, ddeltaz;
    double pdeltax, pdeltay, pdeltaz;
    double dpdeltax, dpdeltay, dpdeltaz;
    
    GameTrak gt = new GameTrak();
    
    //MouseButtonTrak mb = new MouseButtonTrak();

    
   
    
    /**
     * 
     */
    public VulptureGPUApp() {
        super();
 
        
        sphere1Component = new SceneGraphComponent();
        sphere1Component.setGeometry(new Sphere());
        sphere1Component.setTransformation(new Transformation());
        sphere2Component = new SceneGraphComponent();
        sphere2Component.setGeometry(new Sphere());
        sphere2Component.setTransformation(new Transformation());
        root = new SceneGraphComponent();
            
 
 
        
        
        
        
        
        
       Appearance a = new Appearance();
       a.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(.1f, 0.3f, 0.6f));
       //a.setAttribute(CommonAttributes.TRANSPARENCY, 0.5);
       //a.setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, true);
       
       a.setAttribute(CommonAttributes.LIGHTING_ENABLED, true);
       a.setAttribute(CommonAttributes.SMOOTH_SHADING, true);
       a.setAttribute(CommonAttributes.VERTEX_DRAW, false);
       a.setAttribute(CommonAttributes.FACE_DRAW, true);
       a.setAttribute(CommonAttributes.SPHERES_DRAW, false);
       
 
//       //// skybox ?
//        try {
//            ImageData id;
////            id = ImageData.load(new Input(getClass().getResource("caustic2.gif")));
//            id = ImageData.load(new Input(getClass().getResource("/net/MathVis/data/testData3D/textures/grid.jpeg")));
//            TextureUtility.createSkyBox(a,new ImageData[] {
//                    id,id,id,id,id,id});
//            a.setAttribute(CommonAttributes.AT_INFINITY,true);
//        } catch (MalformedURLException e) {
//            // TODO Auto-generated catch block
//            e.printStackTrace();
//        } catch (IOException e) {
//            // TODO Auto-generated catch block
//            e.printStackTrace();
//        }
        
       
       
       // reflectionmap...
       a = new Appearance();
//       try {
//           ImageData id = ImageData.load(new Input(getClass().getResource("caustic2.gif")));
////                   new URL("file:///tmp/caustic2.gif")));
//        TextureUtility.createReflectionMap(a,"polygonShader",new ImageData[] {id,id,id,id,id,id});
//        
//    } catch (MalformedURLException e) {
//        // TODO Auto-generated catch block
//        e.printStackTrace();
//    } catch (IOException e) {
//        // TODO Auto-generated catch block
//        e.printStackTrace();
//    }
    
       root.setAppearance(a);
       
       
       buildFan(root);
       buildFish(root);
       
       
       a = new Appearance();
      
       a.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(.6f, 0.3f, 0.1f));
       sphere1Component.setAppearance(a);
       
       
       root.addChild(sphere1Component);
       root.addChild(sphere2Component);
       
    
    }
    long timestamp,ts;
    private double[] coords = new double[6];
    private double fishWidth;
    
    public void run() {
        timestamp = ts=System.currentTimeMillis();
        while(true) {
////            MatrixBuilder.euclidean().scale(20).rotate(Math.sin(System.currentTimeMillis()/1200.),1,0,Math.cos(System.currentTimeMillis()/3200.)).assignTo(w);
        //for(int i =0;i<200;i++) {
            //System.out.print(".");
            getPoints(coords);
            computeMesh(coords);            
            computeFish(coords);
//            mb.poll();
//            System.out.println("mb 0"+mb.getButton1());
            //repaint();
            long d = System.currentTimeMillis()-ts;
            try {
                if(d<400) {
                    ts =  System.currentTimeMillis();
                    //System.out.println("sleep");
                    Thread.sleep(400-d);
                } else //time for other things...
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
        }
        
    }
    
    private final SceneGraphComponent buildFan(SceneGraphComponent root) {
//        int[][] idx = new int[(SIZEC-1)*(SIZEL-1)][];
//        for (int j = 0; j < SIZEL-1; j++) {
//            for (int i = 0; i < SIZEC-1; i++) {
//                idx[(i)+(j)*(SIZEC-1)] = new int[] {(i)+(j)*(SIZEC),
//                        (i+1)+(j)*(SIZEC),
//                        (i+1)+(j+1)*(SIZEC),
//                        (i)+(j+1)*(SIZEC)};
//            }
//        }
//        double tc[] = new double[2*(SIZEC*SIZEL)];
//        for (int j = 0; j < SIZEL; j++) {
//            for (int i = 0; i < SIZEC; i++) {
//                tc[2*(i+SIZEC*j)] = i/(SIZEC-1.);
//                tc[2*(i+SIZEC*j)+1]= j/(SIZEL-1.);
//            }
//        }
//        ifs.setVertexCountAndAttributes(Attribute.COORDINATES,StorageModel.DOUBLE3_INLINED.createReadOnly(pts));
//        ifs.setFaceCountAndAttributes(Attribute.INDICES,StorageModel.INT_ARRAY_ARRAY.createReadOnly(idx));
//        ifs.setVertexAttributes(Attribute.TEXTURE_COORDINATES,Attribute.TEXTURE_COORDINATES.getDefaultStorageModel().createReadOnly(tc));

        fan.setGeometry(new IndexedLineSet());
        fan.setName("fan");
        root.addChild(fan);
        
        if(USE_VIDEO)
            appearance.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(1f, 1f, 1f));
        fan.setAppearance(appearance);

        appearance.setAttribute(CommonAttributes.LINE_SHADER,"cloth");
        appearance.setAttribute("rows",ROWS);
        appearance.setAttribute("columns",COLS);
        appearance.setAttribute("gravity",G);
        appearance.setAttribute("damping",.9);
////        appearance.setAttribute("factor",.9);
        appearance.setAttribute("diffuseColor", new java.awt.Color(180, 200, 220));
        try {
          TextureUtility.createTexture(appearance, "lineShader", "/home/timh/jRdata/testData3D/textures/grid.jpeg");
        } catch (IOException e) {
          // TODO Auto-generated catch block
          e.printStackTrace();
        }
        return root;
    }
    private final SceneGraphComponent buildFish(SceneGraphComponent root) {
        int[][] idx = new int[(FISHC/2)*(FISHL-1)][];
        for (int j = 0; j < FISHL-1; j++) {
            for (int i = 0; i < FISHC; i+=2) {
                idx[(i/2)+(j)*(FISHC/2)] = new int[] {(i)+(j)*(FISHC),
                        (i+1)+(j)*(FISHC),
                        (i+1)+(j+1)*(FISHC),
                        (i)+(j+1)*(FISHC)};
            }
        }
        double tc[] = new double[2*(FISHC*FISHL)];
        for (int j = 0; j < FISHL; j++) {
            for (int i = 0; i < FISHC; i++) {
                tc[2*(i+FISHC*j)] = i%2;
                tc[2*(i+FISHC*j)+1]= j/(FISHL-1.);
            }
        }
        for (int i = 0; i < fts.length; i++) {
            fts[i] = Math.random() -.5;
        }
        ifsf.setVertexCountAndAttributes(Attribute.COORDINATES,StorageModel.DOUBLE3_INLINED.createReadOnly(fts));
        ifsf.setFaceCountAndAttributes(Attribute.INDICES,StorageModel.INT_ARRAY_ARRAY.createReadOnly(idx));
        ifsf.setVertexAttributes(Attribute.TEXTURE_COORDINATES,StorageModel.DOUBLE2_INLINED.createReadOnly(tc));

        fish.setGeometry(ifsf);
        fish.setName("fish");
        
        Appearance a = new Appearance();
//        a.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(180,220,250));
        a.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(250,250,250));
        a.setAttribute(CommonAttributes.TRANSPARENCY_ENABLED,true);
        a.setAttribute(CommonAttributes.TRANSPARENCY,0.0);
        //fishWidth = .45;
        fishWidth = .45;
        try {
            
            //Texture2D textur = TextureUtility.createTexture(a,"polygonShader","fish3.png");
            Texture2D textur = TextureUtility.createTexture(a,"polygonShader","ifsh.png");
            //System.err.println("textur is "+textur);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        fish.setAppearance(a);
        root.addChild(fish);
        return root;
    }

  //  private final static double RHOSQ = 0.0;
    private final static double D = .9;
    private final static double[] G = {-.0,-.02,0};    
    
    
  //  private double strech = 40.1;

 //   private double mx,my,mz;
 //   private double dmx,dmy,dmz;
    double factor= 1;
    
    /**
         * @param coords2
         */
        private void computeMesh(double[] coords2) {
    
            
            //if(mb.pollButton1()) {
                if(true) {
            double x = coords2[0];
            double y = coords2[1];
            double z = coords2[2];
            
            px = coords2[3];
            py = coords2[4];
            pz = coords2[5];
            /*
            double d = Math.abs((x - xOld)*(x - xOld)+(y - yOld)*(y - yOld)+ (z - zOld)*(z - zOld));
            d = d>0.00001?d:1;
            d = 1;
            xOld= x-strech*(x - xOld)/d;
            yOld= y-strech*(y - yOld)/d;
            zOld= z-strech*(z - zOld)/d;
            
            d = Math.abs((px - pxOld)*(px - pxOld)+(py - pyOld)*(py - pyOld)+ (pz - pzOld)*(pz - pzOld));
            d = d>0.00001?d:1;
            d= 1;
            pxOld= px-strech*(px - pxOld)/d;
            pyOld= py-strech*(py - pyOld)/d;
            pzOld= pz-strech*(pz - pzOld)/d;
            */
            pts[0] = ptsmm[0] = x;
            pts[1] = ptsmm[1] = y;
            pts[2] = ptsmm[2] = z;
            pts[3*SIZEC-3] = ptsmm[3*SIZEC-3] = px;
            pts[3*SIZEC-2] = ptsmm[3*SIZEC-2] = py;
            pts[3*SIZEC-1] = ptsmm[3*SIZEC-1] = pz;
            
           
            for(int i = 1; i<SIZEC-1;i++) {
                int pos = 3*i;
                double u = i/(SIZEC-1.);
                double s = 1-u;
                
    //          pts[pos  ] = ptsm[pos  ] = 1*(x*s*s*s + 3*xOld*u*s*s + 3*pxOld*u*u*s + px*u*u*u + 0*u*s*ptsm[pos]);
    //          pts[pos+1] = ptsm[pos+1] = 1*(y*s*s*s + 3*yOld*u*s*s + 3*pyOld*u*u*s + py*u*u*u + 0*u*s*ptsm[pos+1]);
    //          pts[pos+2] = ptsm[pos+2] = 1*(z*s*s*s + 3*zOld*u*s*s + 3*pzOld*u*u*s + pz*u*u*u + 0*u*s*ptsm[pos+2]);
    /*
                mx += 0.001*dmx;
                my += 0.001*dmy;
                mz += 0.001*dmz;
                dmx = .4*dmx + (.5*(x+px)-mx);
                dmy = .4*dmy + (.5*(y+py)-my);
                dmz = .4*dmz + (.5*(z+pz)-mz);
                
              pts[pos  ] = ptsm[pos  ] = 1*(x*s*s*s + 3*mx*u*s*s + 3*mx*u*u*s + px*u*u*u + 0*u*s*ptsm[pos]);
              pts[pos+1] = ptsm[pos+1] = 1*(y*s*s*s + 3*my*u*s*s + 3*my*u*u*s + py*u*u*u + 0*u*s*ptsm[pos+1]);
              pts[pos+2] = ptsm[pos+2] = 1*(z*s*s*s + 3*mz*u*s*s + 3*mz*u*u*s + pz*u*u*u + 0*u*s*ptsm[pos+2]);
    */
        
              double xn = .5*((x*s + px*u) + pts[pos  ])+0*ptsm[pos  ];
              double yn = .5*((y*s + py*u) + pts[pos+1])+0*ptsm[pos+1];
              double zn = .5*((z*s + pz*u) + pts[pos+2])+0*ptsm[pos+2];            
              
              final double c = 0.5;
              final double cm = 0;
              final double dm = .4;
              final double dmm= .0;
              ptsm[pos  ] = c*ptsm[pos  ] + cm*(.5*(xn-pts[pos  ])) + dm*(ptsmm[pos-3  ]-2*pts[pos  ]+ptsmm[pos+3  ]) + dmm*(ptsm[pos-3  ]+ptsm[pos+3  ]);
              ptsm[pos+1] = c*ptsm[pos+1] + cm*(.5*(yn-pts[pos+1])) + dm*(ptsmm[pos-3+1]-2*pts[pos+1]+ptsmm[pos+3+1]) + dmm*(ptsm[pos-3+1]+ptsm[pos+3+1]);
              ptsm[pos+2] = c*ptsm[pos+2] + cm*(.5*(zn-pts[pos+2])) + dm*(ptsmm[pos-3+2]-2*pts[pos+2]+ptsmm[pos+3+2]) + dmm*(ptsm[pos-3+2]+ptsm[pos+3+2]);
              
              pts[pos  ] += ptsm[pos  ];
              pts[pos+1] += ptsm[pos+1];
              pts[pos+2] += ptsm[pos+2];
     
              ptsmm[pos  ] = (pts[pos  ]);
              ptsmm[pos+1] = (pts[pos+1]);
              ptsmm[pos+2] = (pts[pos+2]);
              
             
    //System.out.println("dir "+pts[pos]+", "+ptsm[pos+1]+", "+ptsm[pos+2]);
    //            double xn = .5*((x*s + px*u) + pts[pos  ])+0.*ptsm[pos  ];
    //            double yn = .5*((y*s + py*u) + pts[pos+1])+0.*ptsm[pos+1];
    //            double zn = .5*((z*s + pz*u) + pts[pos+2])+0.*ptsm[pos+2];            
    //            pts[pos  ] = xn;
    //            pts[pos+1] = yn;
    //            pts[pos+2] = zn;
    
            }
            /*
            for(int j = 1;j<SIZEL;j++)
                for(int i = 0; i<SIZEC;i++) {
                    int pos = 3*(i+SIZEC*j);
                    
    
                    double ox = pts[pos-3*SIZEC];
                    double oy = pts[pos+1-3*SIZEC];
                    double oz = pts[pos+2-3*SIZEC];
    
                    ptsm[pos] += 0.0004*Math.sin(System.currentTimeMillis()/2000.);
                    ptsm[pos+1]+= G;
                    ptsm[pos+2] += 0.0001*Math.cos((System.currentTimeMillis()+pts[pos+1]*pts[pos])/1005.);
                    
                    double dirx = (pts[pos  ]-ox) + factor*ptsm[pos  ];
                    double diry = (pts[pos+1]-oy) + factor*ptsm[pos+1];
                    double dirz = (pts[pos+2]-oz) + factor*ptsm[pos+2];
                    double l = Math.sqrt(dirx*dirx+diry*diry+dirz*dirz);
                    if(l> 0.01) {
    //                    System.out.println("l "+l+" dirx "+dirx +" G "+G);
    //                    System.out.println("l "+l+" diry "+diry);
    //                    System.out.println("l "+l+" dirz "+dirz);
    
                        dirx =dirx/l;
                        diry =diry/l;
                        dirz =dirz/l;
    //                    System.out.println("l "+l+" dirx "+dirx);
    //                    System.out.println("l "+l+" diry "+diry);
    //                    System.out.println("l "+l+" dirz "+dirz);
    //                     l = Math.sqrt(dirx*dirx+diry*diry*+dirz*dirz);
    //                    System.out.println("norm "+l);
                    double sc = ptsm[pos  ]*dirx + ptsm[pos+1]*diry+ptsm[pos+2]*dirz;
                    ptsm[pos  ] -= sc*dirx;
                    ptsm[pos+1] -= sc*diry;
                    ptsm[pos+2] -= sc*dirz;
                    ptsm[pos  ] *= D;
                    ptsm[pos+1] *= D;
                    ptsm[pos+2] *= D;
                    pts[pos  ] = ox + 0.1*dirx;
                    pts[pos+1] = oy + 0.1*diry;
                    pts[pos+2] = oz + 0.1*dirz;
                
    //                pts[pos  ] = ptsm[pos  -3*SIZEC];
    //                pts[pos+1] = ptsm[pos+1-3*SIZEC];
    //                pts[pos+2] = ptsm[pos+2-3*SIZEC];
                    }
            }
            
            Scene.executeWriter(ifs,new Runnable() {
                public void run() {
                    //ifs.setVertexCountAndAttributes(Attribute.COORDINATES,new DoubleArrayArray.Inlined(pts, 3));
                    ifs.setVertexAttributes(Attribute.COORDINATES,new DoubleArrayArray.Inlined(pts, 3));
                    GeometryUtility.calculateAndSetVertexNormals(ifs);
                }
            }
            
            );
            */

            appearance.setAttribute("initialPositions",pts);
            //appearance.setAttribute("dummy",new Object());
            ddeltax = x-xOld -deltax;
            ddeltay = y-yOld -deltay;
            ddeltaz = z-zOld -deltaz;

            deltax = x-xOld ;
            deltay = y-yOld ;
            deltaz = z-zOld ;

            
            xOld = coords2[0];
            yOld = coords2[1];
            zOld = coords2[2];
 
            dpdeltax = px-pxOld -pdeltax;
            dpdeltay = py-pyOld -pdeltay;
            dpdeltaz = pz-pzOld -pdeltaz;

            pdeltax = px-pxOld ;
            pdeltay = py-pyOld ;
            pdeltaz = pz-pzOld ;

         
            pxOld = px;
            pyOld = py;
            pzOld = pz;
            
            
            }
            
            //System.out.println("-> "+pts[idx]);
            Transformation tr = sphere1Component.getTransformation();
            MatrixBuilder m = MatrixBuilder.euclidean();
            m.translate(coords2[0],coords2[1],coords2[2]);
    //        m.translate(px,py,pz);
            m.scale(.1);
            tr.setMatrix(m.getMatrix().getArray());
    
            tr = sphere2Component.getTransformation();
            m = MatrixBuilder.euclidean();
            m.translate(coords2[3],coords2[4],coords2[5]);
    //        m.translate(px,py,pz);
            m.scale(.1);
            tr.setMatrix(m.getMatrix().getArray());
    
            
            factor = 1+0*(System.currentTimeMillis()-timestamp)/24.;
            timestamp = System.currentTimeMillis();
            //System.out.println("factor "+factor);
            }

    /**
     * @param coords2
     */
    private void computeFish(double[] coords2) {
        double x = coords2[0];
        double y = coords2[1];
        double z = coords2[2];
        
        px = coords2[3];
        py = coords2[4];
        pz = coords2[5];

   
        double vv = Math.sqrt(ddeltax*ddeltax+ddeltay*ddeltay+ddeltaz*ddeltaz);
        double pvv = Math.sqrt(dpdeltax*dpdeltax+dpdeltay*dpdeltay+dpdeltaz*dpdeltaz);
        double prox = 4;
        for(int i = 0; i<FISHC;i+= 2) {
            int pos = 3*i;
            double u = i/(FISHC-1.);
            double s = 1-u;
    
            double sx = x- fts[pos];
            double sy = y- fts[pos+1];
            double sz = z- fts[pos+2];
            
            
            double dd = Math.sqrt(sx*sx+sy*sy+sz*sz);
            if(dd> 0.) {
                dd = (0.02*signum(dd - prox) - vv/dd)/dd;
                sx *=dd;
                sy *=dd;
                sz *=dd;
            }

            double spx = px- fts[pos];
            double spy = py- fts[pos+1];
            double spz = pz- fts[pos+2];
            
            
            double dpd = Math.sqrt(spx*spx+spy*spy+spz*spz);
            if(dpd> 0.) {
                dpd = (0.02*signum(dpd - prox) - pvv/dpd)/dpd;
                spx *=dpd;
                spy *=dpd;
                spz *=dpd;
            }

            
          final double c = 0.8;
          final double d = 0.1;
          ftsm[pos  ] = c*ftsm[pos  ] + d*(Math.random()-.5) + sx + spx; 
          ftsm[pos+1] = c*ftsm[pos+1] + d*(Math.random()-.5) + sy + spy; 
          ftsm[pos+2] = c*ftsm[pos+2] + d*(Math.random()-.5) + sz + spz; 
          ftsm[pos+3] = c*ftsm[pos  ] + d*(Math.random()-.5); 
          ftsm[pos+4] = c*ftsm[pos+1] + d*(Math.random()-.5); 
          ftsm[pos+2] = c*ftsm[pos+2] + d*(Math.random()-.5); 
          
          
          fts[pos  ] += ftsm[pos  ];
          fts[pos+1] += ftsm[pos+1];
          fts[pos+2] += ftsm[pos+2];

          double dx = fts[pos+3] +ftsm[pos+3] - fts[pos];
          double dy = fts[pos+4] +ftsm[pos+4] - fts[pos+1];
          double dz = fts[pos+5] +ftsm[pos+5] - fts[pos+2];
          dd = Math.sqrt(dx*dx+dy*dy+dz*dz);
          if(dd> 0.) {
              dx*= fishWidth/dd;
              dy*= fishWidth/dd;
              dz*= fishWidth/dd;
          }
          fts[pos+3] = dx+fts[pos+0];
          fts[pos+4] = dy+fts[pos+1];
          fts[pos+5] = dz+fts[pos+2];
//          System.out.println(" ----->>>> "+Math.sqrt(dx*dx+dy*dy+dz*dz)+"  "+dd);
//          
//          System.out.println(" ----->>>> "+(
//        (fts[pos+3]-fts[pos  ])*(fts[pos+3]-fts[pos  ])+ 
//        (fts[pos+4]-fts[pos+1])*(fts[pos+4]-fts[pos+1])+ 
//        (fts[pos+5]-fts[pos+2])*(fts[pos+5]-fts[pos+2])));

//          ftsmm[pos  ] = (fts[pos  ]);
//          ftsmm[pos+1] = (fts[pos+1]);
//          ftsmm[pos+2] = (fts[pos+2]);
//          
         

        }
        for(int j = 1;j<FISHL;j++)
            for(int i = 0; i<FISHC;i++) {
                int pos = 3*(i+FISHC*j);
                

                double ox = fts[pos-3*FISHC];
                double oy = fts[pos+1-3*FISHC];
                double oz = fts[pos+2-3*FISHC];

                ftsm[pos] += 0.0004*Math.sin(System.currentTimeMillis()/2000.)+G[0];
                ftsm[pos+1]+= G[1];
                ftsm[pos+2] += 0.0001*Math.cos((System.currentTimeMillis()+fts[pos+1]*fts[pos])/1005.)+ G[2];
                
                double dirx = (fts[pos  ]-ox) + factor*ftsm[pos  ];
                double diry = (fts[pos+1]-oy) + factor*ftsm[pos+1];
                double dirz = (fts[pos+2]-oz) + factor*ftsm[pos+2];
                double l = Math.sqrt(dirx*dirx+diry*diry+dirz*dirz);
                if(l> 0.01) {
//                    System.out.println("l "+l+" dirx "+dirx +" G "+G);
//                    System.out.println("l "+l+" diry "+diry);
//                    System.out.println("l "+l+" dirz "+dirz);

                    dirx =dirx/l;
                    diry =diry/l;
                    dirz =dirz/l;
//                    System.out.println("l "+l+" dirx "+dirx);
//                    System.out.println("l "+l+" diry "+diry);
//                    System.out.println("l "+l+" dirz "+dirz);
//                     l = Math.sqrt(dirx*dirx+diry*diry*+dirz*dirz);
//                    System.out.println("norm "+l);
                double sc = ftsm[pos  ]*dirx + ftsm[pos+1]*diry+ftsm[pos+2]*dirz;
                ftsm[pos  ] -= sc*dirx;
                ftsm[pos+1] -= sc*diry;
                ftsm[pos+2] -= sc*dirz;
                ftsm[pos  ] *= D;
                ftsm[pos+1] *= D;
                ftsm[pos+2] *= D;
                fts[pos  ] = ox + 0.1*dirx;
                fts[pos+1] = oy + 0.1*diry;
                fts[pos+2] = oz + 0.1*dirz;
            
//                pts[pos  ] = ptsm[pos  -3*SIZEC];
//                pts[pos+1] = ptsm[pos+1-3*SIZEC];
//                pts[pos+2] = ptsm[pos+2-3*SIZEC];
                }
        }
        
        Scene.executeWriter(ifsf,new Runnable() {
            public void run() {
                //ifs.setVertexCountAndAttributes(Attribute.COORDINATES,new DoubleArrayArray.Inlined(pts, 3));
                ifsf.setVertexAttributes(Attribute.COORDINATES,new DoubleArrayArray.Inlined(fts, 3));
                IndexedFaceSetUtility.calculateAndSetVertexNormals(ifsf);
            }
        }
        
        );
    }

    private double signum(double e) {
		return e > 0 ? 1 : -1;
	}
	/**
     * @param coords2
     */
    private static double t = 0;
    private float[] f = new float[3];
    private void getPoints(double[] coords2) {
        /*
        coords2[0] = 5*Math.sin(.1* t+ Math.cos(3*t));
        coords2[1] = 5*Math.cos(.051* t + Math.cos(2*t)*Math.sin(3*t));
        coords2[2] = 5*Math.sin( 3.15*t - Math.sin(3*t+ .5*Math.sin(.5*t)));
        coords2[3] = 1;

        t+=.05;
        */
        gt.poll();
        gt.getPoint1(f);
        coords2[0] = 5*f[0];
        coords2[1] = 5*f[1];
        coords2[2] = 5*f[2];
        gt.getPoint2(f);
        coords2[3] = 5*f[0];
        coords2[4] = 5*f[1];
        coords2[5] = 5*f[2];
//        coords[0] = -4; 
//        coords[1] = 1; 
//        coords[2] = 0;
//        coords[3] = 4; 
//        coords[4] = 1.5; 
//        coords[5] = 0;
        
    }

    
   
    
    public static void main(String[] args) {        
        VulptureGPUApp st = new VulptureGPUApp();
    
       for(int i = 0; i< -100;i++) {
           st.getPoints(st.coords);
            st.computeMesh(st.coords);
        }

       System.setProperty(SystemProperties.VIEWER, "de.jreality.jogl.GpgpuViewer"); // de.jreality.portal.DesktopPortalViewer");
       
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		v.setContent(st.root);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
        
        System.out.println("-> go!");
        Thread t = new Thread(st);
        t.start();
        t.setPriority(Thread.MIN_PRIORITY);
    }




    public void receiveImageData(ImageData id) {
        if(id != null) {
            if(tex == null)
                TextureUtility.createTexture(appearance, "polygonShader",id);
            else tex.setImage(id);
        }
    }

}
