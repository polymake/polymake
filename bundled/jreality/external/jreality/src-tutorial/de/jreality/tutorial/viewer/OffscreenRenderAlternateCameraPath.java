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


package de.jreality.tutorial.viewer;

import java.awt.Color;
import java.awt.Component;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.image.BufferedImage;
import java.io.File;

import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.JLabel;

import de.jreality.geometry.Primitives;
import de.jreality.jogl.JOGLViewer;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.RotateTool;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.util.ImageUtility;
import de.jreality.util.RenderTrigger;

/**
 * A simple class based on {@link de.jreality.tutorial.viewer.ViewerFromScratch} to show how to get
 * offscreen rendering to work with an alternative camera path.  
 * 
 * WARNING: this interface to the offscreen rendering will likely change in the near future to
 * be based on a "factory" pattern instead of monolithic static method.
 *
 */
public class OffscreenRenderAlternateCameraPath {
  
	public static void main(String[] args) {
    SceneGraphComponent rootNode = new SceneGraphComponent("root");
    SceneGraphComponent cameraNode = new SceneGraphComponent("camera");
    SceneGraphComponent cameraNode2 = new SceneGraphComponent("camera2");
    SceneGraphComponent geometryNode = new SceneGraphComponent("geometry");
    SceneGraphComponent lightNode = new SceneGraphComponent("light");
    
    rootNode.addChild(geometryNode);
    rootNode.addChild(cameraNode);
    cameraNode.addChild(lightNode);
    
    Light dl=new DirectionalLight();
    lightNode.setLight(dl);
    
    IndexedFaceSet ifs = Primitives.icosahedron(); 
    geometryNode.setGeometry(ifs);
    
    RotateTool rotateTool = new RotateTool();
    geometryNode.addTool(rotateTool);

    MatrixBuilder.euclidean().translate(0, 0, 3).assignTo(cameraNode);
    MatrixBuilder.euclidean().translate(0, 0, 2).assignTo(cameraNode2);

	Appearance rootApp= new Appearance();
    rootApp.setAttribute(CommonAttributes.BACKGROUND_COLOR, new Color(0f, .1f, .1f));
    rootApp.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(1f, 0f, 0f));
    rootNode.setAppearance(rootApp);
        
    Camera camera = new Camera();
    cameraNode.setCamera(camera);
    SceneGraphPath camPath = new SceneGraphPath(rootNode, cameraNode);
    camPath.push(camera);
    SceneGraphPath camPath2 = new SceneGraphPath(rootNode, cameraNode2);
    Camera camera2 = new Camera();
    camPath2.push(camera2);
   
    JOGLViewer viewer = new JOGLViewer();
    viewer.setSceneRoot(rootNode);
    viewer.setCameraPath(camPath);
    ToolSystem toolSystem = ToolSystem.toolSystemForViewer(viewer);
    toolSystem.initializeSceneTools();
    
    JFrame frame = new JFrame();
    frame.setVisible(true);
    frame.setSize(640, 480);
    frame.getContentPane().add((Component) viewer.getViewingComponent());
    frame.validate();
    frame.addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent arg0) {
        System.exit(0);
      }
    });
    
    RenderTrigger rt = new RenderTrigger();
    rt.addSceneGraphComponent(rootNode);
    rt.addViewer(viewer);
    try {
        Thread.sleep(2000);
     } catch (InterruptedException e)  {
     // TODO Auto-generated catch block
     e.printStackTrace();
  }
   for (int i=0;i<15;i++) {
        try {
           Thread.sleep(500);
        } catch (InterruptedException e) {
           // TODO Auto-generated catch block
           e.printStackTrace();
        }
        // note that the size of the image is "unantialiased"; final size will be reduced by a factor 4
        BufferedImage bi = viewer.getRenderer().getOffscreenRenderer().renderOffscreen(
              null, 1600, 1200, 4, viewer.getDrawable(), camPath2);
        File file = new File("/tmp/foo.png");
        ImageUtility.writeBufferedImage(file, bi);

        JFrame snapshot = new JFrame("snapshot");
        snapshot.getContentPane().add(new JLabel(new ImageIcon(bi)));
        snapshot.pack();
        snapshot.setVisible(true);
     }    // have to give the viewer time to initialize itself before doing offscreen rendering
 	}
}
