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
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import javax.swing.JFrame;

import de.jreality.geometry.Primitives;
import de.jreality.jogl.JOGLViewer;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.RotateTool;
import de.jreality.tools.Timer;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.util.RenderTrigger;

/**
 * A simple class showing how to construct a simple viewer from scratch, without using {@link JRViewer} or another
 * ready-made method.  In contrast to ready-made viewers, this examples shows:
 * <ul>
 * <li>The camera is explicitly constructed.</li>
 * <li>Lights, tools, and geometry are explicitly added to the scene graph. </li> 
 * <li>An instance of {@link de.jreality.scene.Viewer} is created (in this case a JOGL viewer).</li>
 * <li>A tool system is attached to the viewer in order for the tools to function.</li>
 * <li>An instance of {@link RenderTrigger} is created to trigger rendering when changes occur .</li>
 * <li>A Java Frame is created and filled with the viewing component of the viewer.  </li>
 * </ul>
 * This version uses a JOGL viewer.
 *
 */
public class ViewerFromScratch {
  
	public static void main(String[] args) {
    SceneGraphComponent rootNode = new SceneGraphComponent("root");
    SceneGraphComponent cameraNode = new SceneGraphComponent("camera");
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

	final Appearance rootApp= new Appearance();
    rootApp.setAttribute(CommonAttributes.BACKGROUND_COLOR, new Color(0f, .1f, .1f));
    rootApp.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(1f, 0f, 0f));
    rootNode.setAppearance(rootApp);
        
    Camera camera = new Camera();
    cameraNode.setCamera(camera);
    SceneGraphPath camPath = new SceneGraphPath(rootNode, cameraNode);
    camPath.push(camera);
    
    JOGLViewer viewer = new JOGLViewer();
    viewer.setSceneRoot(rootNode);
    viewer.setCameraPath(camPath);
    ToolSystem toolSystem = ToolSystem.toolSystemForViewer(viewer);
    toolSystem.initializeSceneTools();
    Timer timer = new Timer(10, new ActionListener() {
		int count = 0;
		public void actionPerformed(ActionEvent arg0) {
			count++;
		    if (count > 255) count = 0;
		    rootApp.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(count, 0, (255-count)));
		}
	});
    timer.attach(ToolSystem.getToolSystemForViewer(viewer));
	timer.start();
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
  }
}
