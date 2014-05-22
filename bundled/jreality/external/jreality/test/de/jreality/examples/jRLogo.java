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

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Frame;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.Primitives;
import de.jreality.math.FactoredMatrix;
import de.jreality.renderman.RIBViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.soft.DefaultViewer;
import de.jreality.util.SceneGraphUtility;

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class jRLogo {
  
static double[][] points = {{ 6.15625, 25.0625, 0.0 }, { 6.15625, 28.90625, 0.0 }, { 10.0, 28.90625, 0.0 }, { 10.0, 25.0625, 0.0 }, { 6.15625, 25.0625, 0.0 }, { 10.0, 21.21875, 0.0 }, { 10.0, 0.234375, 0.0 }, { 9.556640625, -3.4501953125, 0.0 }, { 8.2265625, -6.08203125, 0.0 }, { 6.009765625, -7.6611328125, 0.0 }, { 2.90625, -8.1875, 0.0 }, { -1.453125, -7.421875, 0.0 }, { -1.453125, -2.65625, 0.0 }, { 0.71875, -2.65625, 0.0 }, { 0.75, -3.015625, 0.0 }, { 1.28125, -6.140625, 0.0 }, { 3.21875, -7.0, 0.0 }, { 5.421875, -6.15234375, 0.0 }, { 6.15625, -3.609375, 0.0 }, { 6.15625, -1.609375, 0.0 }, { 6.15625, 16.15625, 0.0 }, { 5.8046875, 19.1328125, 0.0 }, { 4.21875, 19.890625, 0.0 }, { 2.890625, 19.984375, 0.0 }, { 2.546875, 20.0, 0.0 }, { 2.546875, 21.21875, 0.0 }, { 10.0, 21.21875, 0.0 }, { 22.36328125, 12.71875, 0.0 }, { 22.36328125, 5.0625, 0.0 }, { 22.48828125, 3.125, 0.0 }, { 22.87890625, 1.6328125, 0.0 }, { 25.14453125, 1.234375, 0.0 }, { 25.50390625, 1.21875, 0.0 }, { 25.50390625, 0.0, 0.0 }, { 14.69140625, 0.0, 0.0 }, { 14.69140625, 1.21875, 0.0 }, { 15.06640625, 1.234375, 0.0 }, { 16.36328125, 1.328125, 0.0 }, { 17.80859375, 1.7109375, 0.0 }, { 18.19140625, 3.125, 0.0 }, { 18.30078125, 5.0625, 0.0 }, { 18.30078125, 23.84375, 0.0 }, { 18.19140625, 25.78125, 0.0 }, { 17.80859375, 27.203125, 0.0 }, { 16.36328125, 27.59375, 0.0 }, { 15.06640625, 27.671875, 0.0 }, { 14.69140625, 27.71875, 0.0 }, { 14.69140625, 28.90625, 0.0 }, { 21.64453125, 28.90625, 0.0 }, { 23.55078125, 28.953125, 0.0 }, { 25.16015625, 29.0, 0.0 }, { 26.72265625, 29.046875, 0.0 }, { 30.646484375, 28.58984375, 0.0 }, { 33.44921875, 27.21875, 0.0 }, { 35.130859375, 24.93359375, 0.0 }, { 35.69140625, 21.734375, 0.0 }, { 34.1953125, 16.671875, 0.0 }, { 29.70703125, 13.609375, 0.0 }, { 35.37890625, 5.09375, 0.0 }, { 37.19140625, 2.578125, 0.0 }, { 39.30078125, 1.296875, 0.0 }, { 40.61328125, 1.234375, 0.0 }, { 41.00390625, 1.21875, 0.0 }, { 41.00390625, 0.0, 0.0 }, { 33.62890625, 0.0, 0.0 }, { 25.73828125, 12.71875, 0.0 }, { 22.36328125, 12.71875, 0.0 }, { 22.41015625, 14.046875, 0.0 }, { 23.84765625, 14.046875, 0.0 }, { 27.197265625, 14.5, 0.0 }, { 29.58984375, 15.859375, 0.0 }, { 31.025390625, 18.125, 0.0 }, { 31.50390625, 21.296875, 0.0 }, { 30.06640625, 26.1640625, 0.0 }, { 25.17578125, 27.59375, 0.0 }, { 22.41015625, 27.59375, 0.0 }, { 22.41015625, 14.046875, 0.0 }};
//j dot
static int[] face0 = {0, 1, 2, 3};
// j body
static int[] face1 = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
// R
//static int[] face2 = {27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};
//static int[] face3 = {67, 68, 69, 70, 71, 72, 73, 74, 75};

static int[] face2 = { 67, 68, 69, 70, 71, 72, 73, 74, 75, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,27};



public static SceneGraphComponent logo;
static {
    IndexedFaceSet fs = new IndexedFaceSet();
    fs.setVertexCountAndAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(points));
    fs.setFaceCountAndAttributes(Attribute.INDICES,StorageModel.INT_ARRAY_ARRAY.createReadOnly(new int[][] {face0,face1,face2}));
    IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(fs);
    IndexedFaceSetUtility.calculateAndSetFaceNormals(fs);
    IndexedFaceSet ts = IndexedFaceSetUtility.triangulate(fs);
    
    logo = new SceneGraphComponent();
    SceneGraphComponent cpc = new SceneGraphComponent();
    Transformation trans= new Transformation();
//    trans.setTranslation(-20,-12,0);
    cpc.setTransformation(trans);
    cpc.setGeometry(ts);
    
    Appearance a = new Appearance();
    a.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(0.3f,.3f,.4f));
    //a.setAttribute(CommonAttributes.POLYGON_SHADER, "flat");
    //a.setAttribute(CommonAttributes.POLYGON_SHADER+".vertexShader", "constant");
    //a.setAttribute("outline", true);
    a.setAttribute(CommonAttributes.FACE_DRAW, true);
    a.setAttribute(CommonAttributes.EDGE_DRAW, true);
    a.setAttribute(CommonAttributes.VERTEX_DRAW, true);
    a.setAttribute(CommonAttributes.POINT_RADIUS, 0.5);
    a.setAttribute(CommonAttributes.LINE_WIDTH, 0.5);
    a.setAttribute("pointShader.outlineFraction", .95);
    //a.setAttribute("pointShader.outlineShader.vertexShader", "constant");
    //a.setAttribute("pointShader.outlineShader.color", Color.BLACK);
    Color outlineColor = new Color(.1f,.1f,.1f);
    a.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, outlineColor);
    a.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, outlineColor);
    cpc.setAppearance(a);
    logo.addChild(cpc);
    
    Geometry t = Primitives.torus(20,1.5,30,3);
    //Sphere t = new Sphere();
    SceneGraphComponent tpc = new SceneGraphComponent();
    trans= new Transformation();
    //trans.setRotation(-.2,0,1,0);
    //tpc.setTransformation(trans);
    tpc.setGeometry(t);
    
    a = new Appearance();
    a.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(1.0f,.9f,.0f));
    a.setAttribute(CommonAttributes.TRANSPARENCY, 0.5);
    a.setAttribute(CommonAttributes.FACE_DRAW, true);
    a.setAttribute(CommonAttributes.EDGE_DRAW, false);
    a.setAttribute(CommonAttributes.VERTEX_DRAW, false);
    tpc.setAppearance(a);
    logo.addChild(tpc);
}

     public static void main(String[] args) {
         DefaultViewer softViewer;
         Frame         frameSoft;
         SceneGraphComponent root = new SceneGraphComponent();
         
         // scene View
         SceneGraphComponent cameraNode= new SceneGraphComponent();
         Transformation ct= new Transformation();
//         ct.setTranslation(0, 0, 16);
         cameraNode.setTransformation(ct);
         Camera firstCamera= new Camera();
         firstCamera.setFieldOfView(30);
         firstCamera.setFar(50);
         firstCamera.setNear(3);
         cameraNode.setCamera(firstCamera);

         SceneGraphComponent lightNode= new SceneGraphComponent();
         Transformation lt= new Transformation();
         FactoredMatrix fm = new FactoredMatrix(lt);
		fm.setRotation(-Math.PI / 4, 1, 1, 0);
		lt.setMatrix(fm.getArray());
         lightNode.setTransformation(lt);
         DirectionalLight light= new DirectionalLight();
         lightNode.setLight(light);
         root.addChild(lightNode);

         SceneGraphComponent lightNode2= new SceneGraphComponent();
         Transformation lt2= new Transformation();
         //   lt2.assignScale(-1);
         fm = new FactoredMatrix(lt);
		     fm.setRotation(-Math.PI / 4, 1, 1, 0);
		     lt.setMatrix(fm.getArray());
         lightNode2.setTransformation(lt2);
         DirectionalLight light2= new DirectionalLight();
         lightNode2.setLight(light2);
         root.addChild(lightNode2);


         Appearance ap= new Appearance();
         ap.setAttribute("diffuseColor", new Color(1f, 0f, 0f));
         ap.setAttribute("lightingEnabled", true);
         
         root.setAppearance(ap);
         root.addChild(cameraNode);
         
         
         // scene
         SceneGraphComponent logo = new SceneGraphComponent();
         logo.addChild(jRLogo.logo);
         Transformation t = new Transformation();
         logo.setTransformation(t);
         root.addChild(logo);
         
         
         // Frame etc...
         
         softViewer= new DefaultViewer();
         softViewer.setBackground(new Color(1.f,1.f,1.f));
         frameSoft= new Frame();
         frameSoft.addWindowListener(new WindowAdapter() {
             public void windowClosing(WindowEvent e) {
                 System.exit(0);
             }
         });
         frameSoft.setLayout(new BorderLayout());
         frameSoft.add(softViewer, BorderLayout.CENTER);

         /* TODO: mouse tool is removed...
         MouseTool mouseTool= new MouseTool(softViewer);
         mouseTool.setViewer(softViewer.getViewingComponent());
         mouseTool.setRoot(root);
         mouseTool.setCamera(firstCamera);
         */
         //softViewer.getViewingComponent().addKeyListener(this);
         softViewer.setSceneRoot(root);
         softViewer.setCameraPath(((SceneGraphPath) SceneGraphUtility.getPathsBetween(root, firstCamera).get(0)));
         
         frameSoft.setSize(4*134, 4*114);
         frameSoft.validate();
         frameSoft.setVisible(true);

         softViewer.render();
         
         /* TODO: mouse tool removed
         mouseTool.encompass();
         */
         
         RIBViewer rv = new RIBViewer();
         rv.initializeFrom(softViewer);
         for(int i = 0; i<20;i++) {
             FactoredMatrix fm1 = new FactoredMatrix(t);
			fm1.setRotation(2*Math.PI/20*i, (new double[]{1,0,0}));
			t.setMatrix(fm1.getArray());
             softViewer.render();
             rv.setFileName("jRLogo_"+numString(i,4));
             rv.render();
         }
     }
         private static String numString(int mx,int numType) {
            String s = Integer.toString(mx);
            StringBuffer b  = new StringBuffer(s);
            int leadingZ = 0;
            switch (numType) {
                case 4 :
                leadingZ = 4 - s.length();
                break;
                case 3:
                leadingZ = 3 - s.length();
                break;

                default :
                    leadingZ = 0;
                    break;
            }
            if(leadingZ<0) leadingZ = 0;
            
            while (leadingZ >0) {
                b.insert(0,"0");
                leadingZ--;
            }
            return b.toString();
        }


}
