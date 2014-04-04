package de.jreality.plugin.scene;

import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.Insets;

import javax.swing.Box;
import javax.swing.JLabel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.basic.RunningEnvironment;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tools.HeadTransformationTool;
import de.jreality.tools.ShipNavigationTool;
import de.jreality.tools.ShipNavigationTool.PickDelegate;
import de.jreality.ui.JSliderVR;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel.MinSizeGridBagLayout;

public class Avatar extends Plugin implements ChangeListener {

	public static final double DEFAULT_SPEED = 4;
	public static final double DEFAULT_JUMP_SPEED = 4;

	private SceneGraphComponent avatar;
	public SceneGraphComponent getAvatar() {
		return avatar;
	}

	private SceneGraphComponent cameraComponent;
	private ShipNavigationTool shipNavigationTool;
	private HeadTransformationTool headTransformationTool;
	public HeadTransformationTool getHeadTransformationTool() {
		return headTransformationTool;
	}

	private ShrinkPanel panel;
	private JSliderVR speedSlider;
	private JSliderVR jumpSpeedSlider;

	public Avatar() {
		panel = new ShrinkPanel("Avatar");
		panel.setShrinked(true);
		panel.setIcon(getPluginInfo().icon);
		panel.add(Box.createHorizontalStrut(5));
		panel.setLayout(new MinSizeGridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.HORIZONTAL;
		c.insets = new Insets(2,2,2,2);
		
		JLabel gainLabel = new JLabel("Navigation Speed");
		c.weightx = 0.0;
		c.gridwidth = GridBagConstraints.RELATIVE;
		panel.add(gainLabel, c);
		speedSlider = new JSliderVR(0, 3000, (int) (100 * DEFAULT_SPEED));
		speedSlider.setPreferredSize(new Dimension(200,26));
		speedSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				setNavigationSpeed(getNavigationSpeed());
			}
		});
		c.weightx = 1.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		panel.add(speedSlider, c);

	
		gainLabel = new JLabel("Jump Speed");
		c.weightx = 0.0;
		c.gridwidth = GridBagConstraints.RELATIVE;
		panel.add(gainLabel, c);
		jumpSpeedSlider = new JSliderVR(0, 3000, (int) (100 * DEFAULT_JUMP_SPEED));
		jumpSpeedSlider.setPreferredSize(new Dimension(200,26));
		jumpSpeedSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				setJumpSpeed(getJumpSpeed());
			}
		});
		c.weightx = 1.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		panel.add(jumpSpeedSlider, c);
	}

	private void createTools(RunningEnvironment environment) {
		boolean portal = environment == RunningEnvironment.PORTAL;
		boolean portalRemote = environment == RunningEnvironment.PORTAL_REMOTE;
		// navigation tool
		shipNavigationTool = new ShipNavigationTool();

		// head transformation tool
		if (!portal && !portalRemote) {
			headTransformationTool = new HeadTransformationTool();
		}
		
		setNavigationSpeed(getNavigationSpeed());
		setJumpSpeed(getJumpSpeed());
	}
	
	private void updateComponents(Scene scene) {
		avatar = scene.getAvatarComponent();
		cameraComponent = scene.getCameraComponent();
		if (cameraComponent != null) {
			Camera cam = cameraComponent.getCamera();
			if (cam != null) {
				cam.setFieldOfView(60);
				// the depth buffer has 23 bits.  The commented settings lead to very unsatisfactory images
				// TODO: allow control over these settings, since each scene is different
				// and when the terrain is active, the encompass() method does not automatically
				// set the near and far clipping planes.
				cam.setNear(0.1);		// cam.setNear(0.1);
				cam.setFar(1000);		// cam.setFar(10000);
			}
			MatrixBuilder.euclidean().translate(0,1.7,0).assignTo(cameraComponent);
		} else {
			System.out.println("Avatar.updateComponents(): CAMERA CMP == NULL");
		}
		setAvatarPosition(0, 0, 20);
	}
	
	private void installTools() {
		if (avatar != null) avatar.addTool(shipNavigationTool);
		if (cameraComponent != null && headTransformationTool != null) cameraComponent.addTool(headTransformationTool);
	}
		
	private void uninstallTools() {
		if (avatar != null) avatar.removeTool(shipNavigationTool);
		if (cameraComponent != null && headTransformationTool != null) cameraComponent.removeTool(headTransformationTool);
	}
	
	public Component getPanel() {
		return panel;
	}

	public double getNavigationSpeed() {
		double speed = 0.01*speedSlider.getValue();
		return speed;
	}

	public void setNavigationSpeed(double navigationSpeed) {
		int speed = (int)(100*navigationSpeed);
		speedSlider.setValue(speed);
		if (shipNavigationTool != null) {
			shipNavigationTool.setGain(navigationSpeed);
		}
	}
	

	public double getJumpSpeed() {
		double speed = 0.01*jumpSpeedSlider.getValue();
		return speed;
	}

	public void setJumpSpeed(double jumpSpeed) {
		int speed = (int)(100*jumpSpeed);
		jumpSpeedSlider.setValue(speed);
		if (shipNavigationTool != null) {
			shipNavigationTool.setJumpSpeed(jumpSpeed);
		}
	}
	

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		createTools(c.getPlugin(View.class).getRunningEnvironment());
		Scene scene = c.getPlugin(Scene.class);
		updateComponents(scene);
		installTools();
		
		scene.addChangeListener(this);
		VRPanel vp = c.getPlugin(VRPanel.class);
		vp.addComponent(getClass(), panel, 4.0, "VR");
		
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		Scene scene = c.getPlugin(Scene.class);
		scene.removeChangeListener(this);
		uninstallTools();
		VRPanel vp = c.getPlugin(VRPanel.class);
		vp.removeAll(getClass());
	}


	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Avatar";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("vr/avatar.png");
		return info; 
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		setNavigationSpeed(c.getProperty(getClass(), "navigationSpeed", getNavigationSpeed()));
		setJumpSpeed(c.getProperty(getClass(), "jumpSpeed", getJumpSpeed()));
		super.restoreStates(c);
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "navigationSpeed", getNavigationSpeed());
		c.storeProperty(getClass(), "jumpSpeed", getJumpSpeed());
		super.storeStates(c);
	}

	public void setPickDelegate(PickDelegate pickDelegate) {
		shipNavigationTool.setPickDelegate(pickDelegate);
	}

	public void stateChanged(ChangeEvent e) {
		if (e.getSource() instanceof Scene) {
			Scene scene = (Scene) e.getSource();
			uninstallTools();
			updateComponents(scene);
			installTools();
		}
	}

	public void setAvatarPosition(double x, double y, double z) {
		if (avatar != null) {
			MatrixBuilder.euclidean().translate(x, y, z).assignTo(avatar);
		}
	}

}
