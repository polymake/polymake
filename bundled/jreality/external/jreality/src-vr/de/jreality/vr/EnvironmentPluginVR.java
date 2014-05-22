package de.jreality.vr;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.security.PrivilegedAction;
import java.util.prefs.Preferences;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.SwingConstants;
import javax.swing.border.CompoundBorder;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.filechooser.FileFilter;
import javax.swing.filechooser.FileSystemView;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;
import de.jreality.sunflow.PerezSky;
import de.jreality.util.Input;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;
import de.jtem.beans.SimpleColorChooser;

public class EnvironmentPluginVR extends AbstractPluginVR {

	// defaults for env panel
	private static final String DEFAULT_ENVIRONMENT = "snow";
	private static final boolean DEFAULT_SKYBOX_HIDDEN = false;
	private static final Color DEFAULT_BACKGROUND_COLOR = Color.white;

	private File customCubeMapFile;
	private ImageData[] customCubeMap;
	
	// env tab
	private JPanel envPanel;
	private JCheckBox skyBoxHidden;
	private SimpleColorChooser backgroundColorChooser;
	private JButton backgroundColorButton;
	private JButton envLoadButton;

	private JFileChooser cubeMapFileChooser;
	
	private Landscape landscape;
	
	private PerezSky perezBox;
	
	private ActionListener closeListener = new ActionListener() {
		public void actionPerformed(ActionEvent e) {
			getViewerVR().switchToDefaultPanel();
		}
	};
	private JSlider sunHeightSlider;
		
	public EnvironmentPluginVR() {
		super("env");
		landscape = new Landscape();
		perezBox = new PerezSky();
		makeEnvTab();
		Secure.doPrivileged(new PrivilegedAction<Object>() {
			public Object run() {
				makeCubeMapFileChooser();
				return null;
			}
		});
	}
	
	private void makeCubeMapFileChooser() {
		FileSystemView view = FileSystemView.getFileSystemView();
		FileFilter ff = new FileFilter() {
			@Override
			public boolean accept(File f) {
				return f.isDirectory() || f.getName().toLowerCase().endsWith(".zip");
			}
			@Override
			public String getDescription() {
				return "ZIP archives";
			}
		};
		String texDir = ".";
		String dataDir = Secure.getProperty(SystemProperties.JREALITY_DATA);
		if (dataDir!= null) texDir = dataDir;
		File defaultDir = new File(texDir);
		cubeMapFileChooser = new JFileChooser(!defaultDir.exists() ? view.getHomeDirectory() : defaultDir, view);
		cubeMapFileChooser.setFileFilter(ff);
		cubeMapFileChooser.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent ev) {
				File file = cubeMapFileChooser.getSelectedFile();
				try {
					if (ev.getActionCommand() == JFileChooser.APPROVE_SELECTION
							&& file != null) {
						customCubeMapFile = file;
						customCubeMap = TextureUtility.createCubeMapData(Input.getInput(customCubeMapFile));
						getViewerVR().setEnvironment(customCubeMap);
						updateEnv();
					}
				} catch (IOException e) {
					e.printStackTrace();
				}
				getViewerVR().switchToDefaultPanel();
			}
		});
	}

	@Override
	public void setViewerVR(ViewerVR vvr) {
		super.setViewerVR(vvr);
	}
	
	private void makeEnvTab() {
		backgroundColorChooser = new SimpleColorChooser();
		backgroundColorChooser.setBorder(new EmptyBorder(8,8,8,8));
		backgroundColorChooser.addChangeListener( new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setBackgroundColor(backgroundColorChooser.getColor());
			}
		});
		backgroundColorChooser.addActionListener(closeListener);

		landscape.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				setEnvironment(landscape.getEnvironment());
			}
		});	
		
		Insets insets = new Insets(0,5,0,5);
		
		envPanel = new JPanel(new BorderLayout());
		envPanel.setBorder(new EmptyBorder(0,0,0,0));
		JPanel selectionPanel = new JPanel(new BorderLayout());
		selectionPanel.setBorder(
				new CompoundBorder(
						new EmptyBorder(0, 5, 2, 3),
						BorderFactory.createTitledBorder(
								BorderFactory.createEtchedBorder(),
								"Cube map"
						)
				)
		);
		selectionPanel.add(landscape.getSelectionComponent(), BorderLayout.CENTER);
		
		envLoadButton = new JButton("load");
		selectionPanel.add(BorderLayout.SOUTH, envLoadButton);
		envLoadButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				switchToEnvLoadPanel();
			}
		});
		
		envPanel.add(selectionPanel, BorderLayout.CENTER);
		
		Box proceduralBox = new Box(BoxLayout.Y_AXIS);
		proceduralBox.setBorder(
				new CompoundBorder(
						new EmptyBorder(0, 5, 2, 3),
						BorderFactory.createTitledBorder(
								BorderFactory.createEtchedBorder(),
								"Procedural"
						)
				)
		);
		Box envControlBox = new Box(BoxLayout.X_AXIS);
		skyBoxHidden = new JCheckBox("flat background");
		skyBoxHidden.setBorder(new EmptyBorder(0,5,5,10));
		skyBoxHidden.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setSkyBoxHidden(isSkyBoxHidden());
			}
		});
		envControlBox.add(skyBoxHidden);
		
		backgroundColorButton = new JButton("color");
		backgroundColorButton.setMargin(insets);
		Box colorBox = new Box(BoxLayout.X_AXIS);
		colorBox.setBorder(new EmptyBorder(0,0,5,0));
		backgroundColorButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				switchToTopColorChooser();
			}
		});
		colorBox.add(backgroundColorButton);
		envControlBox.add(colorBox);
		proceduralBox.add(envControlBox);
		
		Box sunHeightBox = new Box(BoxLayout.X_AXIS);
		JLabel heightLabel = new JLabel("sun height");
		sunHeightBox.add(heightLabel);
		sunHeightSlider = new JSlider(SwingConstants.HORIZONTAL, 0, 90, 45);
		sunHeightSlider.setPreferredSize(new Dimension(70,20));
		sunHeightSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setSunHeight(getSunHeight());
			}
		});
		sunHeightBox.add(sunHeightSlider);
		proceduralBox.add(sunHeightBox);
		envPanel.add(proceduralBox, BorderLayout.SOUTH);
	}

	protected void switchToEnvLoadPanel() {
		getViewerVR().switchToFileChooser(cubeMapFileChooser);
	}
	
	public double getSunHeight() {
		return sunHeightSlider.getValue();
	}

	@Override
	public JPanel getPanel() {
		return envPanel;
	}
	
	public void setSunHeight(double angle) {
		double t = angle * Math.PI/180;
		perezBox.setSunDirection(new double[]{Math.cos(Math.PI/3)*Math.cos(t), Math.sin(t),Math.sin(Math.PI/3)*Math.cos(t)});
		getViewerVR().setEnvironment(perezBox.getCubeMap());
		updateEnv();
	}
	
	private void updateEnv() {
		getViewerVR().setSkyLightNode(getSkyLightNode());
		ImageData[] imgs = skyBoxHidden.isSelected() ? null : getViewerVR().getEnvironment();
		TextureUtility.createSkyBox(getViewerVR().getRootAppearance(), imgs);
	}

	private void switchToTopColorChooser() {
		getViewerVR().switchTo(backgroundColorChooser);
	}

	public Color getBackgroundColor() {
		return backgroundColorChooser.getColor();
	}

	public void setBackgroundColor(Color color) {
		backgroundColorChooser.setColor(color);
		getViewerVR().getRootAppearance().setAttribute(
				CommonAttributes.BACKGROUND_COLOR,
				getBackgroundColor()
		);
	}

	public boolean isSkyBoxHidden() {
		return skyBoxHidden.isSelected();
	}
	
	public void setSkyBoxHidden(boolean b) {
		skyBoxHidden.setSelected(b);
		backgroundColorButton.setEnabled(b);
		updateEnv();
	}
	
	public String getEnvironment() {
		return landscape.getEnvironment();
	}

	public void setEnvironment(String environment) {
		landscape.setEvironment(environment);
		ImageData[] cm;
		if (landscape.isCustomEnvironment()) {
			cm = customCubeMap;
		} else if (landscape.isProceduralEnvironment()) {
			cm = perezBox.getCubeMap();
		} else {
			cm = landscape.getCubeMap();
		}
		getViewerVR().setEnvironment(cm);
		envLoadButton.setEnabled(landscape.isCustomEnvironment());

		boolean proc = landscape.isProceduralEnvironment();
		sunHeightSlider.setEnabled(proc);
		getViewerVR().getSceneRoot().setGeometry(
				proc ? perezBox : null
		);
		updateEnv();
	}
	
	@Override
	public void storePreferences(Preferences prefs) {
		prefs.put("environment", getEnvironment());
		if ("custom".equals(getEnvironment())) {
			if (customCubeMapFile != null) {
				prefs.put("environmentFile", customCubeMapFile.getAbsolutePath());
			}
		}		
		prefs.putBoolean("skyBoxHidden", isSkyBoxHidden());
		Color c = getBackgroundColor();
		prefs.putInt("backgroundColorRed", c.getRed());
		prefs.putInt("backgroundColorGreen", c.getGreen());
		prefs.putInt("backgroundColorBlue", c.getBlue());	
	}
	
	@Override
	public void restoreDefaults() {
		customCubeMapFile = null;
		customCubeMap = null;
		setEnvironment(DEFAULT_ENVIRONMENT);
		setSkyBoxHidden(DEFAULT_SKYBOX_HIDDEN);
		setBackgroundColor(DEFAULT_BACKGROUND_COLOR);
	}
	
	@Override
	public void restorePreferences(Preferences prefs) {
		String env = prefs.get("environment", DEFAULT_ENVIRONMENT);
		if ("custom".equals(env)) {
			String customFile = prefs.get("environmentFile", null);
			if (customFile != null) {
				File f = new File(customFile);
				if (f.exists()) customCubeMapFile = f;
				try {
					customCubeMap = TextureUtility.createCubeMapData(Input.getInput(customFile));
				} catch (IOException e) {
					e.printStackTrace();
					customCubeMapFile = null;
				}
			}
		}
		setEnvironment(env);
		setSkyBoxHidden(prefs.getBoolean("skyBoxHidden", DEFAULT_SKYBOX_HIDDEN));
		int r = prefs.getInt("backgroundColorRed", DEFAULT_BACKGROUND_COLOR.getRed());
		int g = prefs.getInt("backgroundColorGreen", DEFAULT_BACKGROUND_COLOR.getGreen());
		int b = prefs.getInt("backgroundColorBlue", DEFAULT_BACKGROUND_COLOR.getBlue());
		setBackgroundColor(new Color(r,g,b));
	}
	
	public SceneGraphComponent getSkyLightNode() {
		return landscape.isProceduralEnvironment() ? perezBox.getLightComponent() : null;
	}
}
