package de.jreality.plugin.menu;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;

import javax.swing.ButtonGroup;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JRadioButtonMenuItem;

import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Camera;
import de.jreality.scene.Viewer;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jreality.ui.viewerapp.actions.AbstractJrToggleAction;
import de.jreality.ui.viewerapp.actions.camera.LoadCameraPreferences;
import de.jreality.ui.viewerapp.actions.camera.SaveCameraPreferences;
import de.jreality.ui.viewerapp.actions.camera.ShiftEyeSeparation;
import de.jreality.ui.viewerapp.actions.camera.ShiftFieldOfView;
import de.jreality.ui.viewerapp.actions.camera.ShiftFocus;
import de.jreality.ui.viewerapp.actions.camera.ToggleStereo;
import de.jreality.util.CameraUtility;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class CameraMenu extends Plugin {

	ViewerSwitch viewer;
	Component viewingComp;
	private Scene
		scene = null;
	private ClickWheelCameraZoomTool
		zoomTool = new ClickWheelCameraZoomTool();
	private LoadCameraPreferences loadAction;
	private SaveCameraPreferences saveAction;

	private AbstractJrToggleAction zoomToolAction = new AbstractJrToggleAction("Zoom Tool") {
		
		private static final long 
			serialVersionUID = 1L;

		@Override
		public void actionPerformed(ActionEvent e) {
			setZoomEnabled(isSelected());
		}
		
	};
	
	private JMenu createCameraMenu() {
		JMenu cameraMenu = new JMenu("Camera");
		cameraMenu.setMnemonic(KeyEvent.VK_C);

		zoomToolAction.setIcon(ImageHook.getIcon("zoom.png"));
		cameraMenu.add(zoomToolAction.createMenuItem());
		cameraMenu.add(new JMenuItem(new ShiftFieldOfView("Decrease FOV", viewer, true)));
		cameraMenu.add(new JMenuItem(new ShiftFieldOfView("Increase FOV", viewer, false)));
		cameraMenu.addSeparator();
		cameraMenu.add(new JMenuItem(new ShiftFocus("Decrease Focus", viewer, true)));
		cameraMenu.add(new JMenuItem(new ShiftFocus("Increase Focus", viewer, false)));
		cameraMenu.addSeparator();
		cameraMenu.add(new JMenuItem(new ShiftEyeSeparation("Decrease Eye Separation", viewer, true)));
		cameraMenu.add(new JMenuItem(new ShiftEyeSeparation("Increase Eye Separation", viewer, false)));
		cameraMenu.addSeparator();
		final JMenu stereoTypesMenu = new JMenu("StereoType");
		cameraMenu.add(new JMenuItem(new ToggleStereo("Toggle Stereo", viewer) {
			private static final long serialVersionUID = 1L;
			@Override
			public void actionPerformed(ActionEvent e) {
			    super.actionPerformed(e);
				Camera camera = CameraUtility.getCamera(viewer);
			    stereoTypesMenu.setEnabled(camera.isStereo());
			}
		}));
		loadAction = new LoadCameraPreferences("Load Preferences", viewer);
        loadAction.setIcon(ImageHook.getIcon("film_go.png"));
		cameraMenu.add(loadAction.createMenuItem());
        saveAction = new SaveCameraPreferences("Save Preferences", viewer);
		saveAction.setIcon(ImageHook.getIcon("film_save.png"));
		cameraMenu.add(saveAction.createMenuItem());
		
		ButtonGroup bg = new ButtonGroup();
		
		JRadioButtonMenuItem b = new JRadioButtonMenuItem("CROSS_EYED_STEREO");
		b.setSelected(true);
		b.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				Viewer v = viewer.getCurrentViewer();
				((de.jreality.jogl.JOGLViewer) v).setStereoType(0);
			}
		});
		bg.add(b);
		stereoTypesMenu.add(b);
		
		b = new JRadioButtonMenuItem("RED_BLUE_STEREO");
		b.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				Viewer v = viewer.getCurrentViewer();
				((de.jreality.jogl.JOGLViewer) v).setStereoType(1);
			}
		});
		bg.add(b);
		stereoTypesMenu.add(b);
		
		b = new JRadioButtonMenuItem("RED_GREEN_STEREO");
		b.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				Viewer v = viewer.getCurrentViewer();
				((de.jreality.jogl.JOGLViewer) v).setStereoType(2);
			}
		});
		bg.add(b);
		stereoTypesMenu.add(b);
		
		b = new JRadioButtonMenuItem("RED_CYAN_STEREO");
		b.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				Viewer v = viewer.getCurrentViewer();
				((de.jreality.jogl.JOGLViewer) v).setStereoType(3);
			}
		});
		bg.add(b);
		stereoTypesMenu.add(b);
		
//		b = new JRadioButtonMenuItem("HARDWARE_BUFFER_STEREO");
//		b.addActionListener(new ActionListener() {
//			public void actionPerformed(ActionEvent e) {
//				Viewer v = viewer.getCurrentViewer();
//				((de.jreality.jogl.JOGLViewer) v).setStereoType(4);
//			}
//		});
//		bg.add(b);
//		stereoTypesMenu.add(b);
		
		Camera camera = CameraUtility.getCamera(viewer);
	    stereoTypesMenu.setEnabled(camera.isStereo());
	    
		cameraMenu.addSeparator();
		cameraMenu.add(stereoTypesMenu);
		return cameraMenu;
	}

	
	public void setZoomEnabled(boolean enable) {
		if (scene != null) {
			scene.getSceneRoot().removeTool(zoomTool);
			if (enable) {
				scene.getSceneRoot().addTool(zoomTool);
			}
		}
		zoomToolAction.setSelected(enable);
	}
	
	public boolean isZoomEnabled() {
		return zoomToolAction.isSelected();
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Camera Menu", "jReality Group");
	}
	
	@Override
	public void install(Controller c) throws Exception {
		View view = c.getPlugin(View.class);
		viewer = view.getViewer();
		scene = c.getPlugin(Scene.class);
		viewingComp = viewer.getViewingComponent();
		c.getPlugin(ViewMenuBar.class).addMenu(getClass(), 2, createCameraMenu());
		
		ViewToolBar vtb = c.getPlugin(ViewToolBar.class);
//		vtb.addTool(getClass(), 5.0, loadAction.createToolboxItem());
//		vtb.addTool(getClass(), 5.1, saveAction.createToolboxItem());
//		vtb.addSeparator(getClass(), 5.2);
		vtb.addTool(getClass(), 1.25, zoomToolAction.createToolboxItem());
		
		setZoomEnabled(zoomToolAction.isSelected());
	}
	
	@Override
	public void uninstall(Controller c) throws Exception {
		c.getPlugin(ViewMenuBar.class).removeAll(getClass());
		c.getPlugin(ViewToolBar.class).removeAll(getClass());
	}
	
	@Override
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);
		c.storeProperty(getClass(), "zoomEnabled", zoomToolAction.isSelected());
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);
		setZoomEnabled(c.getProperty(getClass(), "zoomEnabled", isZoomEnabled()));
	}

}
