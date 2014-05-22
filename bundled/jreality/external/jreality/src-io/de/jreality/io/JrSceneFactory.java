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


package de.jreality.io;

import java.awt.Color;

import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.tool.Tool;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.tools.DraggingTool;
import de.jreality.tools.EncompassTool;
import de.jreality.tools.PickShowTool;
import de.jreality.tools.PointerDisplayTool;
import de.jreality.tools.RotateTool;


public class JrSceneFactory {

	public static JrScene getDefaultPortalScene() {
		return getDefaultPortalScene(false);
	}

	public static JrScene getDefaultPortalRemoteScene() {
		return getDefaultPortalScene(true);
	}

	private static JrScene getDefaultPortalScene(boolean remote) {

		// root
		SceneGraphComponent sceneRoot = new SceneGraphComponent("root");
		Appearance rootAppearance = new Appearance();
		sceneRoot.addTool(new PickShowTool());
		sceneRoot.setAppearance(rootAppearance);
		
		// lights
		DirectionalLight light = new DirectionalLight();
		light.setIntensity(0.4);
		
		SceneGraphComponent lightNode1  = new SceneGraphComponent("light 1");
		lightNode1.setLight(light);
		MatrixBuilder.euclidean().rotateFromTo(new double[]{0,0,1}, new double[]{-1,1,-1}).assignTo(lightNode1);
		sceneRoot.addChild(lightNode1);

		SceneGraphComponent lightNode2 = new SceneGraphComponent("light 2");
		lightNode2.setLight(light);
		MatrixBuilder.euclidean().rotateFromTo(new double[]{0,0,1}, new double[]{1,1,-1}).assignTo(lightNode2);
		sceneRoot.addChild(lightNode2);

		SceneGraphComponent lightNode3 = new SceneGraphComponent("light 3");
		lightNode3.setLight(light);
		MatrixBuilder.euclidean().rotateFromTo(new double[]{0,0,1}, new double[]{1,1,1}).assignTo(lightNode3);
		sceneRoot.addChild(lightNode3);

		SceneGraphComponent lightNode4 = new SceneGraphComponent("light 4");
		lightNode4.setLight(light);
		MatrixBuilder.euclidean().rotateFromTo(new double[]{0,0,1}, new double[]{-1,1,1}).assignTo(lightNode3);
		sceneRoot.addChild(lightNode4);

		// scene
		SceneGraphComponent sceneNode=new SceneGraphComponent("scene");
		sceneRoot.addChild(sceneNode);
		
		// tools
		RotateTool rotateTool = new RotateTool();
		rotateTool.setFixOrigin(false);
		rotateTool.setMoveChildren(false);
		rotateTool.setUpdateCenter(false);
		rotateTool.setAnimTimeMin(250.0);
		rotateTool.setAnimTimeMax(750.0);
		sceneNode.addTool(rotateTool);
		
		DraggingTool draggingTool = new DraggingTool();
		draggingTool.setMoveChildren(false);
		sceneNode.addTool(draggingTool);
		
		// avatar
		SceneGraphComponent avatarNode = new SceneGraphComponent("avatar");
		avatarNode.addTool(new PointerDisplayTool());
		sceneRoot.addChild(avatarNode);

		// camera
		SceneGraphComponent camNode = new SceneGraphComponent("camNode");
		MatrixBuilder.euclidean().translate(0,1.7,0).assignTo(camNode);
		Camera cam = new Camera();
		cam.setNear(0.01);
		cam.setFar(1500);
		cam.setOnAxis(false);
		cam.setStereo(true);
		camNode.setCamera(cam);
		String headMoveTool = remote ? "de.jreality.tools.RemotePortalHeadMoveTool" : "de.jreality.tools.PortalHeadMoveTool";
		try {
			Tool t = (Tool) Class.forName(headMoveTool).newInstance();
			camNode.addTool(t);
		} catch (Throwable t) {
			System.err.println("crating headMoveTool failed");
		}
		avatarNode.addChild(camNode);

		// paths
		SceneGraphPath cameraPath = new SceneGraphPath();
		cameraPath.push(sceneRoot);
		SceneGraphPath emptyPickPath = cameraPath.pushNew(sceneNode);
		cameraPath.push(avatarNode);
		cameraPath.push(camNode);
		SceneGraphPath avatarPath = cameraPath.popNew();
		cameraPath.push(cam);

		// create JrScene
		JrScene scene = new JrScene(sceneRoot);
		scene.addPath("avatarPath", avatarPath);
		scene.addPath("cameraPath", cameraPath);
		scene.addPath("emptyPickPath", emptyPickPath);
		return scene;
	}

	/**
	 * Get the default scene for desktop environment.
	 * @return the default desktop scene
	 */
	public static JrScene getDefaultDesktopScene() {
		JrScene scene = getDefaultDesktopSceneWithoutTools();

		SceneGraphComponent sceneNode = scene.getPath("emptyPickPath").getLastComponent();
		
		// encompass
		EncompassTool encompassTool = new EncompassTool();
		sceneNode.addTool(encompassTool);
		
		// rotate
		RotateTool rotateTool = new RotateTool();
		rotateTool.setFixOrigin(false);
		rotateTool.setMoveChildren(false);
		rotateTool.setUpdateCenter(false);
		rotateTool.setAnimTimeMin(250.0);
		rotateTool.setAnimTimeMax(750.0);
		sceneNode.addTool(rotateTool);
		
		// drag
		DraggingTool draggingTool = new DraggingTool();
		draggingTool.setMoveChildren(false);
		sceneNode.addTool(draggingTool);
		
		// zoom
		scene.getSceneRoot().addTool(new ClickWheelCameraZoomTool());

		return scene;
	}
	
	public static JrScene getDefaultDesktopSceneWithoutTools() {

		//sceneRoot of the JrScene
		SceneGraphComponent sceneRoot = new SceneGraphComponent("root");
		sceneRoot.setVisible(true);
		Appearance app = new Appearance("root appearance");
		ShaderUtility.createRootAppearance(app);
		sceneRoot.setAppearance(app);

		//scene
		SceneGraphComponent scene = new SceneGraphComponent("scene");
		scene.setTransformation(new Transformation("scene trafo"));
		sceneRoot.addChild(scene);
		
		// sun light
		SceneGraphComponent lightNode = new SceneGraphComponent("lightNode");
		lightNode.setVisible(true);
		double[] trafoMatrix = new double[]{
				0.8535533905932737,  0.14644660940672619, -0.4999999999999999, 0.0,
				0.14644660940672619, 0.8535533905932737,   0.4999999999999999, 0.0, 
				0.4999999999999999, -0.4999999999999999,   0.7071067811865476, 0.0,
				0.0,                 0.0,                  0.0,                1.0
		};
		Transformation trafo = new Transformation(trafoMatrix);
		trafo.setName("lightNode trafo");
		lightNode.setTransformation(trafo);
		Light light = new DirectionalLight("light");
		light.setColor(new Color(255,255,255,255));
		light.setIntensity(0.75);
		lightNode.setLight(light);
		sceneRoot.addChild(lightNode);
		
		//avatar
		SceneGraphComponent avatar = new SceneGraphComponent("avatar");
		avatar.setVisible(true);
		trafoMatrix = Rn.identityMatrix(4);
		trafoMatrix[11] = 16;
		trafo = new Transformation(trafoMatrix);
		trafo.setName("avatar trafo");
		avatar.setTransformation(trafo);
		sceneRoot.addChild(avatar);
		
		//camera
		SceneGraphComponent cameraNode = new SceneGraphComponent("cameraNode");
		cameraNode.setVisible(true);
		trafoMatrix = Rn.identityMatrix(4);
		trafo = new Transformation(trafoMatrix);
		trafo.setName("camera trafo");
		cameraNode.setTransformation(trafo);
		Camera camera = new Camera("camera"); 
		camera.setFar(50.0);
		camera.setFieldOfView(30.0);
		camera.setFocus(3.0);
		camera.setNear(3.0);
		camera.setOnAxis(true);
		camera.setStereo(false);
		cameraNode.setCamera(camera);
		
		// head light
		light = new DirectionalLight("camera light");
		light.setColor(new Color(255,255,255,255));
		light.setIntensity(0.75);
		cameraNode.setLight(light);
		avatar.addChild(cameraNode);

		//create JrScene
		JrScene defaultScene = new JrScene(sceneRoot);

		//cameraPath
		SceneGraphPath cameraPath = new SceneGraphPath();
		cameraPath.push(sceneRoot);
		cameraPath.push(avatar);
		cameraPath.push(cameraNode);
		cameraPath.push(camera);
		defaultScene.addPath("cameraPath", cameraPath);
		
		//avatarPath
		SceneGraphPath avatarPath = new SceneGraphPath();
		avatarPath.push(sceneRoot);
		avatarPath.push(avatar);
		defaultScene.addPath("avatarPath", avatarPath);
		
		//emptyPickPath
		SceneGraphPath emptyPickPath = new SceneGraphPath();
		emptyPickPath.push(sceneRoot);
		emptyPickPath.push(scene);
		defaultScene.addPath("emptyPickPath", emptyPickPath);

		return defaultScene;
	}
}