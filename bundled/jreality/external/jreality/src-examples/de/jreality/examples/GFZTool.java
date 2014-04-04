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
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.ImageIcon;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.KeyStroke;

import de.jreality.geometry.Primitives;
import de.jreality.io.JrScene;
import de.jreality.math.MatrixBuilder;
import de.jreality.reader.ReaderJRS;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.swing.ScenePanel;
import de.jreality.tools.AnimatorTask;
import de.jreality.tools.AnimatorTool;
import de.jreality.ui.viewerapp.ViewerApp;
import de.jreality.ui.viewerapp.ViewerAppMenu;
import de.jreality.util.Input;
import de.jreality.util.PickUtility;


public class GFZTool  extends AbstractTool {

	private static InputSlot actSlot = InputSlot.getDevice("SystemTime");

	public GFZTool() {
		addCurrentSlot(actSlot, "Need notification to perform once.");
	}


	private double angle = 0.001;  //angle of rotation
	private double[] axis = new double[]{0, 0, 1};  //axis of rotation
	private double layerTimer = 1500.0;  //time in millis between layer change

	private AnimatorTask task = null;
	private SceneGraphComponent gfz = null;
	private int layerCount, topLayer, direction;

	private Object toolContextKey = null;

	public void perform(ToolContext tc) {

		//this tool performs only once
		gfz = tc.getRootToToolComponent().getLastComponent();
		layerCount = 24;
		topLayer = 1;  //skip first child of gfz
		direction = -1;  //start with hiding labels
		toolContextKey = tc.getKey();  //ToolSystem

		task = new AnimatorTask() {
			double sum = 0;

			public boolean run(double time, double dt) {
				//rotate cmp
				MatrixBuilder m = MatrixBuilder.euclidean(gfz.getTransformation());
				m.rotate(0.05*dt*angle, axis);
				m.assignTo(gfz);
				//set visibility of layers
				if (sum > layerTimer) {
					if ( direction<0 && topLayer < layerCount ) {
						gfz.getChildComponent(topLayer++).setVisible(false);
						if (topLayer == layerCount) direction = 1;
					}
					else if ( direction>0 && topLayer > 1 ) {
						gfz.getChildComponent(--topLayer).setVisible(true);
						if (topLayer == 1) direction = -1;
					}
					sum = 0;
				}
				else sum += dt; 

				return true;
			}
		};

		//scheduled task
		AnimatorTool.getInstanceImpl(toolContextKey).schedule(gfz, task);

		removeCurrentSlot(actSlot);
	}



//	PROPERTIES
	static final String gfzDir = "/net/MathVis/gfz";  //GFZ: "C:\\cygwin\\home\\Administrator\\gfz"
	static final int slideInterval = 5000;  //time after which the slide changes in millis
	static final double scenePanelWidth = 1.0;
	private static SceneGraphComponent sceneCmp = null;
	private static SceneGraphComponent scenePanel = null;
	private static SceneGraphComponent legend = null;


	public static void main(String[] args) throws FileNotFoundException, IOException {
		remoteMain(args);
	}

	public static ViewerApp remoteMain(String[] args) throws FileNotFoundException, IOException {

		GFZTool gfzTool = new GFZTool(); 

//		LOAD GFZ DATA
		File file = new File(gfzDir + "/gfz.jrs");
		ReaderJRS r = new ReaderJRS();
		r.setInput(new Input(file));
		JrScene scene = r.getScene();

		//adjust camara properties if displayed in 3D environment
//		Camera camera = (Camera) scene.getPath("cameraPath").getLastElement();
//		camera.setStereo(true);
//		camera.setEyeSeparation(100.0);
//		camera.setFocus(16000.0);
		
		final SceneGraphComponent root = scene.getSceneRoot();
		sceneCmp = scene.getPath("emptyPickPath").getLastComponent();
		SceneGraphComponent gfz = sceneCmp.getChildComponent(0);

		PickUtility.assignFaceAABBTrees(gfz);  //allows fast picking
		gfz.addTool(gfzTool);
		//gfz transformation
		MatrixBuilder.euclidean().rotateX(-Math.PI/2.3).assignTo(gfz);


//		BOTTOM RIGHT PANEL
		scenePanel = new SceneGraphComponent("scenePanel");
		gfzTool.addSlide(gfzDir + "/sheet1.jpg", scenePanel);
		gfzTool.addSlide(gfzDir + "/sheet2.jpg", scenePanel);
		gfzTool.addSlide(gfzDir + "/sheet3.jpg", scenePanel);
		gfzTool.addSlide(gfzDir + "/sheet4.jpg", scenePanel);
		//panel transformation
		MatrixBuilder.euclidean().translate(4500, -2800, 1000).scale(2500).rotateY(-Math.PI/5).assignTo(scenePanel);
		root.addChild(scenePanel);
		new Thread(new Runnable(){
			public void run() {
				final int children = scenePanel.getChildComponentCount();
				int index = 0;
				scenePanel.getChildComponent(index).setVisible(true);
				while (true) {
					try { Thread.sleep(slideInterval); } 
					catch (InterruptedException e) { e.printStackTrace(); }
					int nextIndex = (index+1) % children;
					scenePanel.getChildComponent(nextIndex).setVisible(true);
					scenePanel.getChildComponent(index).setVisible(false);
					index = nextIndex;
				}
			}
		}).start();
//		panCmp.addTool(new DraggingTool());


//		LEGEND
		legend = new SceneGraphComponent();
		legend.setName("legend");
		root.addChild(legend);
		legend.setGeometry(Primitives.texturedQuadrilateral(new double[]{0,1,0,1,1,0,1,0,0,0,0,0}));
		Appearance app = new Appearance();
		app.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		app.setAttribute(CommonAttributes.EDGE_DRAW, false);
		app.setAttribute(CommonAttributes.LINE_WIDTH, 2.0);
		app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.GRAY);
		app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBES_DRAW, false);
		app.setAttribute(CommonAttributes.FACE_DRAW, true);
		app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.WHITE);
		app.setAttribute(CommonAttributes.DEPTH_FUDGE_FACTOR, 1.0);
		legend.setAppearance(app);
		ImageData img = ImageData.load(Input.getInput(gfzDir + "/img/Legende.png"));
		//TODO: add texture to component
		Texture2D tex = TextureUtility.createTexture(app, CommonAttributes.POLYGON_SHADER, img);
		tex.setTextureMatrix(MatrixBuilder.euclidean().scale(1).getMatrix());
		//legend transformation
		final double ratio = 1.0;  //size of billboard
		MatrixBuilder.euclidean().translate(-6500, -2800, 0).scale(5).scale(ratio*img.getWidth(), ratio*img.getHeight(), 0).assignTo(legend);
//		legend.addTool(new EncompassTool());


//		START VIEWERAPP
		ViewerApp viewerApp = new ViewerApp(scene);
//		viewerApp.setShowMenu(false);
		viewerApp.setAttachNavigator(false);
		viewerApp.setExternalNavigator(true);
		viewerApp.setAttachBeanShell(false);
		viewerApp.setExternalBeanShell(true);

		viewerApp.update();

		viewerApp.getMenu().removeMenu(ViewerAppMenu.CAMERA_MENU);
		viewerApp.getMenu().addMenu(gfzTool.setupMenu());

		viewerApp.display();

		return viewerApp;
	}



	private ScenePanel createImagePanel(String fileName) {

		ScenePanel pan = new ScenePanel();
		pan.setShowFeet(false);
		pan.setAngle(Math.PI/2);
		final Image img = new ImageIcon(fileName).getImage();
		final int w = img.getWidth(null);
		final int h = img.getHeight(null);
		final int border=20;
		JPanel imgPanel = new JPanel() {
			@Override
			public void paint(Graphics g) {
				g.clearRect(0, 0, w+2*border, h+2*border);
				g.drawImage(img, border, border, w, h, null);
			}
		};
		Dimension d = new Dimension(w+2*border, h+2*border);
		imgPanel.setSize(w+2*border, h+2*border);
		imgPanel.setPreferredSize(d);
		imgPanel.setMinimumSize(d);
		imgPanel.setMaximumSize(d);

		pan.getFrame().getContentPane().add(imgPanel);
		pan.getFrame().pack();

		pan.getFrame().setVisible(true);
		pan.setPanelWidth(scenePanelWidth);

		return pan;
	}


	private void addSlide(String filename, SceneGraphComponent parent) {
		final ScenePanel pan = createImagePanel(filename);
		final SceneGraphComponent child = pan.getComponent();
		child.setVisible(false);
		parent.addChild(child);
	}


	private JCheckBoxMenuItem animate, scenepanelCB, legendCB, blueCB, greenCB, redCB;

	private JMenu setupMenu() {

		JMenu gfzMenu = new JMenu("GFZ");
		gfzMenu.setMnemonic(KeyEvent.VK_G);
		Action action;

		//SPACE
		animate = new JCheckBoxMenuItem( new AbstractAction("animate"){
			{
				putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_SPACE, 0));
				putValue(Action.SHORT_DESCRIPTION, "Stop or resume animation");
			}
			public void actionPerformed(ActionEvent arg0) {
				if (animate.isSelected())
					AnimatorTool.getInstanceImpl(toolContextKey).schedule(gfz, task);
				else AnimatorTool.getInstanceImpl(toolContextKey).deschedule(gfz);
			}
		});
		animate.setSelected(true);
		gfzMenu.add(animate);

		gfzMenu.addSeparator();

		//PAGE UP
		action = new AbstractAction("Show next layer"){
			public void actionPerformed(ActionEvent arg0) {
				if (topLayer > 1) {
					gfz.getChildComponent(--topLayer).setVisible(true);
					if (topLayer == 1) direction = -1;
				}
			}
		};
		action.putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_PAGE_UP, 0));
		action.putValue(Action.SHORT_DESCRIPTION, "Shows the next layer");
		gfzMenu.add(new JMenuItem(action));

		//PAGE DOWN
		action = new AbstractAction("Hide next layer"){
			public void actionPerformed(ActionEvent arg0) {
				if (topLayer < layerCount) {
					gfz.getChildComponent(topLayer++).setVisible(false);
					if (topLayer == layerCount) direction = 1;
				}
			}
		};
		action.putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_PAGE_DOWN, 0));
		action.putValue(Action.SHORT_DESCRIPTION, "Hides the next layer");
		gfzMenu.add(new JMenuItem(action));

		gfzMenu.addSeparator();

		//SHOW/HIDE SCENE PANEL
		scenepanelCB = new JCheckBoxMenuItem( new AbstractAction("show scene panel"){
			{
				putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_S, 0));
				putValue(Action.SHORT_DESCRIPTION, "Show or hide the panel displayed in the scene");
			}
			public void actionPerformed(ActionEvent arg0) {
				scenePanel.setVisible(scenepanelCB.isSelected());
			}
		});
		scenepanelCB.setSelected(scenePanel.isVisible());
		gfzMenu.add(scenepanelCB);

		//SHOW/HIDE LEGEND
		legendCB = new JCheckBoxMenuItem( new AbstractAction("show legend"){
			{
				putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_L, 0));
				putValue(Action.SHORT_DESCRIPTION, "Show or hide the legend displayed in the scene");
			}
			public void actionPerformed(ActionEvent arg0) {
				legend.setVisible(legendCB.isSelected());
			}
		});
		legendCB.setSelected(legend.isVisible());
		gfzMenu.add(legendCB);

		gfzMenu.addSeparator();

		//WELLS
		SceneGraphComponent wells = gfz.getChildComponent(0);
		final SceneGraphComponent red = wells.getChildComponent(0);
		final SceneGraphComponent green = wells.getChildComponent(1);
		final SceneGraphComponent blue = wells.getChildComponent(2);

		blueCB = new JCheckBoxMenuItem( new AbstractAction("show blue well"){
			{
				//putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_L, 0));
				putValue(Action.SHORT_DESCRIPTION, "Show or hide the blue well");
			}
			public void actionPerformed(ActionEvent arg0) {
				blue.setVisible(blueCB.isSelected());
			}
		});
		blueCB.setSelected(blue.isVisible());
		gfzMenu.add(blueCB);

		greenCB = new JCheckBoxMenuItem( new AbstractAction("show green well"){
			{
				//putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_L, 0));
				putValue(Action.SHORT_DESCRIPTION, "Show or hide the green well");
			}
			public void actionPerformed(ActionEvent arg0) {
				green.setVisible(greenCB.isSelected());
			}
		});
		greenCB.setSelected(green.isVisible());
		gfzMenu.add(greenCB);

		redCB = new JCheckBoxMenuItem( new AbstractAction("show red well"){
			{
				//putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_L, 0));
				putValue(Action.SHORT_DESCRIPTION, "Show or hide the red well");
			}
			public void actionPerformed(ActionEvent arg0) {
				red.setVisible(redCB.isSelected());
			}
		});
		redCB.setSelected(red.isVisible());
		gfzMenu.add(redCB);

		gfzMenu.addSeparator();

		//RESET
		action = new AbstractAction("Reset scene"){
			public void actionPerformed(ActionEvent arg0) {
				sceneCmp.setTransformation(null);
			}
		};
		action.putValue(Action.ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_R, 0));
		action.putValue(Action.SHORT_DESCRIPTION, "Undo all transformations caused by rotating or dragging");
		gfzMenu.add(new JMenuItem(action));

		return gfzMenu;
	}

}