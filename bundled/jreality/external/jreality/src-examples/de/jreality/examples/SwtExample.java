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

import java.awt.Component;

import javax.swing.JFrame;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.opengl.GLCanvas;
import org.eclipse.swt.opengl.GLData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;

import de.jreality.jogl.SwtQueue;
import de.jreality.jogl.SwtViewer;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.swing.JFakeFrame;
import de.jreality.tools.DraggingTool;
import de.jreality.tools.EncompassTool;
import de.jreality.tools.RotateTool;
import de.jreality.tools.ShipNavigationTool;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.util.RenderTrigger;

public class SwtExample {

  public static void main(String[] args) throws Exception {
    
    // create scene
    
    SceneGraphComponent rootNode=new SceneGraphComponent();
    SceneGraphComponent geometryNode=new SceneGraphComponent();
    SceneGraphComponent cameraNode=new SceneGraphComponent();
    SceneGraphComponent lightNode=new SceneGraphComponent();
    
    rootNode.addChild(geometryNode);
    rootNode.addChild(cameraNode);
    cameraNode.addChild(lightNode);
    
    final CatenoidHelicoid geom=new CatenoidHelicoid(50);
    geom.setAlpha(Math.PI/2.-0.3);
    
    Camera camera=new Camera();
    Light light=new DirectionalLight();
    
    geometryNode.setGeometry(geom);
    cameraNode.setCamera(camera);
    lightNode.setLight(light);

    Appearance app=new Appearance();
    //app.setAttribute(CommonAttributes.FACE_DRAW, false);
    //app.setAttribute("diffuseColor", Color.red);
    //app.setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, true);
    //app.setAttribute(CommonAttributes.TRANSPARENCY, 0.4);
    //app.setAttribute(CommonAttributes.BACKGROUND_COLOR, Color.blue);
    rootNode.setAppearance(app);
    
    MatrixBuilder.euclidean().rotateY(Math.PI/6).assignTo(geometryNode);
    MatrixBuilder.euclidean().translate(0, 0, 12).assignTo(cameraNode);
    MatrixBuilder.euclidean().rotate(-Math.PI/4, 1, 1, 0).assignTo(lightNode);

    SceneGraphPath cameraPath=new SceneGraphPath();
    cameraPath.push(rootNode);
    cameraPath.push(cameraNode);
    cameraPath.push(camera);
    
    Viewer viewer = null;
    ToolSystem toolSystem;
    
    // true for SWT, false for AWT
    if (true) {
      // create Shell, GLCanvas and SwtViewer 
      SwtQueue f = SwtQueue.getInstance();
      final Shell shell = f.createShell();
      final GLCanvas[] can = new GLCanvas[1];
      
      f.waitFor(new Runnable() {
      public void run() {
        shell.setLayout(new FillLayout());
        Composite comp = new Composite(shell, SWT.NONE);
        comp.setLayout(new FillLayout());
        GLData data = new GLData ();
        data.doubleBuffer = true;
        System.out.println("data.depthSize="+data.depthSize);
        data.depthSize = 8;
        can[0] = new GLCanvas(comp, SWT.NONE, data);
        can[0].setCurrent();
        shell.setText("SWT");
        shell.setSize(640, 480);
        shell.open();
      }
      });
      final SwtViewer swtViewer=new SwtViewer(can[0]);
      
      // enable tools
//      viewer = new ToolSystemViewer(swtViewer);
      toolSystem = ToolSystem.toolSystemForViewer(swtViewer);
      toolSystem.initializeSceneTools();
    }
    else {
      viewer = new de.jreality.jogl.JOGLViewer();
      JFrame f = new JFrame("AWT");
      f.setSize(640, 480);
      f.getContentPane().add((Component) viewer.getViewingComponent());
      f.setVisible(true);
      f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    }
    
    viewer.setSceneRoot(rootNode);
    viewer.setCameraPath(cameraPath);

    toolSystem = ToolSystem.toolSystemForViewer(viewer);
    toolSystem.initializeSceneTools();
//    viewer.initializeTools();
    
    // add tools
    geometryNode.addTool(new RotateTool());
    geometryNode.addTool(new DraggingTool());
    geometryNode.addTool(new EncompassTool());
    
    ShipNavigationTool shipNavigationTool = new ShipNavigationTool();
    shipNavigationTool.setGravity(0);
    cameraNode.addTool(shipNavigationTool);
    
    PaintComponent pc = new PaintComponent();
    JFakeFrame jrj = new JFakeFrame();
    jrj.getContentPane().add(pc);
    
    geometryNode.setAppearance(jrj.getAppearance());
    geometryNode.addTool(jrj.getTool());
    
    // add a render trigger for auto redraw
    RenderTrigger rt = new RenderTrigger();
    rt.addViewer(viewer);
    rt.addSceneGraphComponent(rootNode);
    
  }

}
