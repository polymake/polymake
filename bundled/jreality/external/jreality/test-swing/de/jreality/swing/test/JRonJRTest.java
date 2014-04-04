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


package de.jreality.swing.test;

import java.awt.Color;
import java.awt.Component;

import de.jreality.examples.CatenoidHelicoid;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.swing.JFakeFrame;
import de.jreality.tools.DraggingTool;
import de.jreality.tools.RotateTool;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.util.RenderTrigger;
import de.jreality.util.SceneGraphUtility;

public class JRonJRTest {

    public JRonJRTest() {
        super();
        // TODO Auto-generated constructor stub
    }

    /**
     * @param args
     */
    public static void main(String[] args) {

        CatenoidHelicoid catenoid = new CatenoidHelicoid(50);
        catenoid.setAlpha(Math.PI / 2. - 0.3);

        SceneGraphComponent catComp = new SceneGraphComponent();
        Transformation gt = new Transformation();

        catComp.setTransformation(gt);
        catComp.setGeometry(catenoid);

        SceneGraphComponent catComp2 = new SceneGraphComponent();
        Transformation gt2 = new Transformation();

        catComp2.setTransformation(gt2);
        catComp2.setGeometry(catenoid);
        catComp2.setGeometry(new CatenoidHelicoid(20));
        Appearance a = new Appearance();
        a.setAttribute(CommonAttributes.EDGE_DRAW,true);
        a.setAttribute(CommonAttributes.DIFFUSE_COLOR,Color.BLUE.brighter());
        a.setAttribute("lineShader."+CommonAttributes.DIFFUSE_COLOR,Color.BLACK);
        catComp2.setAppearance(a);
        Viewer v2 = createViewer(catComp2);
        JFakeFrame f = new JFakeFrame();
        f.getContentPane().add((Component) v2.getViewingComponent());

        catComp.addTool(f.getTool());
        f.setSize(512, 512);
        f.validate();
        f.setVisible(true);
        System.out.print("setting appearance ");
        catComp.setAppearance(f.getAppearance());
        System.out.println("done");
        JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(catComp);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
        System.out.println("frame size " + f.getSize());
    }

    private static Viewer createViewer(SceneGraphComponent root) {
        Viewer viewer = new de.jreality.soft.DefaultViewer();
        //Viewer viewer = new de.jreality.jogl.Viewer();

        // renderTrigger.addViewer(viewerSwitch);

        SceneGraphComponent scene = new SceneGraphComponent();

        scene.addChild(root);
        SceneGraphComponent camNode = new SceneGraphComponent();
        Camera c = new Camera();
        camNode.setCamera(c);
        camNode.setTransformation(new Transformation());
        MatrixBuilder.euclidean().translate(0, 0, 20).assignTo(camNode);
        RotateTool rt = new RotateTool();
        //rt.setMoveChildren(true);
        DraggingTool dt = new DraggingTool();
        //dt.setMoveChildren(true);
        root.addTool(rt);
        root.addTool(dt);
        scene.addChild(camNode);
        SceneGraphPath cp = (SceneGraphPath) SceneGraphUtility.getPathsBetween(
                scene, c).get(0);

        Appearance a = new Appearance();
        a.setAttribute(CommonAttributes.BACKGROUND_COLOR, Color.WHITE);
        scene.setAppearance(a);

        SceneGraphComponent lightNode = new SceneGraphComponent();
        Transformation lt = new Transformation();
        MatrixBuilder.euclidean().rotate(-Math.PI / 4, 1, 1, 0).assignTo(lt);
        // lt.setRotation(-Math.PI / 4, 1, 1, 0);
        lightNode.setTransformation(lt);
        DirectionalLight light = new DirectionalLight();
        lightNode.setLight(light);
        // root.addChild(lightNode);

        SceneGraphComponent lightNode2 = new SceneGraphComponent();
        Transformation lt2 = new Transformation();
        // lt2.assignScale(-1);
        MatrixBuilder.euclidean().rotate(-Math.PI / 4, 1, 1, 0).assignTo(lt2);
        // lt.setRotation(-Math.PI / 4, 1, 1, 0);
        lightNode2.setTransformation(lt2);
        DirectionalLight light2 = new DirectionalLight();

        lightNode2.setLight(light2);

        scene.addChild(lightNode);
        scene.addChild(lightNode2);

        RenderTrigger trigger = new RenderTrigger();
        trigger.addSceneGraphComponent(scene);
        trigger.addViewer(viewer);
 //       ToolSystemViewer v = new ToolSystemViewer(viewer);
        viewer.setSceneRoot(scene);
        viewer.setCameraPath(cp);
        ToolSystem toolSystem = ToolSystem.toolSystemForViewer(viewer);
        toolSystem.setAvatarPath(cp);
        toolSystem.initializeSceneTools();
        return viewer;
    }

}
