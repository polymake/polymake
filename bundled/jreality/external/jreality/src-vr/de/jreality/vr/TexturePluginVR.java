package de.jreality.vr;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.security.PrivilegedAction;
import java.util.HashMap;
import java.util.prefs.Preferences;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.ButtonModel;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JSlider;
import javax.swing.SwingConstants;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.filechooser.FileSystemView;

import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.util.Input;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;

public class TexturePluginVR extends AbstractPluginVR {

	// default value of tex panel
	private static final double DEFAULT_TEXTURE_SCALE = 20;
	private static final String DEFAULT_TEXTURE = "none";
	
	// maximal value of texture scale
	private static final double MAX_TEX_SCALE = 400;
	
	// ratio of maximal value and minimal value of texture scale
	private static final double TEX_SCALE_RANGE = 400;

	// tex tab
	private JPanel texturePanel;
	private ButtonGroup textureGroup;
	private JSlider texScaleSlider;
	
	private HashMap<String, String> textureNameToTexture = new HashMap<String, String>();
	private HashMap<String, ButtonModel> textureNameToButton = new HashMap<String, ButtonModel>();

	private JFileChooser texFileChooser;

	// texture of content
	private Texture2D tex;

	public TexturePluginVR() {
		super("tex");
		Secure.doPrivileged(new PrivilegedAction<Object>() {
			public Object run() {
				makeTextureFileChooser();
				return null;
			}
		});
		makeTexTab();
	}
	
	@Override
	public JPanel getPanel() {
		return texturePanel;
	}
	private void makeTexTab() {
		textureNameToTexture.put("none", null);
		textureNameToTexture.put("metal grid", "textures/boysurface.png");
		textureNameToTexture.put("metal floor", "textures/metal_basic88.png");
		textureNameToTexture.put("chain-link fence", "textures/chainlinkfence.png");
		//textureNameToTexture.put("random dots", "textures/random.png");

		ActionListener texturesListener = new ActionListener() {
			
			public void actionPerformed(ActionEvent e) {
				setTexture(e.getActionCommand());
			}
		};
		texturePanel = new JPanel(new BorderLayout());
		texturePanel.setBorder(new EmptyBorder(10, 10, 10, 10));
		Box textureButtonBox = new Box(BoxLayout.Y_AXIS);
		textureGroup = new ButtonGroup();
		for (String name : textureNameToTexture.keySet()) {
			JRadioButton button = new JRadioButton(name);
			button.setActionCommand(name);
			textureNameToButton.put(name, button.getModel());
			button.addActionListener(texturesListener);
			textureButtonBox.add(button);
			textureGroup.add(button);
		}
		texturePanel.add("Center", textureButtonBox);
		
		Box texScaleBox = new Box(BoxLayout.X_AXIS);
		texScaleBox.setBorder(new EmptyBorder(70, 5, 5, 0));
		JLabel texScaleLabel = new JLabel("scale");
		texScaleSlider = new JSlider(SwingConstants.HORIZONTAL, 0, 100,0);
		texScaleSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				setTextureScale(getTextureScale());
			}
		});
		texScaleBox.add(texScaleLabel);
		texScaleBox.add(texScaleSlider);
		textureButtonBox .add(texScaleBox);
		
		JButton textureLoadButton = new JButton("load ...");
		textureLoadButton.addActionListener(new ActionListener() {

			public void actionPerformed(ActionEvent arg0) {
				switchToTextureBrowser();
			}
		});
		texturePanel.add("South", textureLoadButton);
	}

	private void makeTextureFileChooser() {
		FileSystemView view = FileSystemView.getFileSystemView();
		String texDir = ".";
		String dataDir = Secure.getProperty(SystemProperties.JREALITY_DATA);
		if (dataDir!= null) texDir = dataDir+"/textures";
		File defaultDir = new File(texDir);
		texFileChooser = new JFileChooser(!defaultDir.exists() ? view.getHomeDirectory() : defaultDir, view);
		texFileChooser.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent ev) {
				File file = texFileChooser.getSelectedFile();
				try {
					if (ev.getActionCommand() == JFileChooser.APPROVE_SELECTION
							&& file != null) {
						ImageData img = ImageData.load(Input.getInput(file));
						tex = TextureUtility.createTexture(getViewerVR().getContentAppearance(), "polygonShader", img, false);
					}
				} catch (IOException e) {
					e.printStackTrace();
				}
				getViewerVR().switchToDefaultPanel();
			}
		});
	}

	public double getTextureScale() {
		double d = .01 * texScaleSlider.getValue();
		return Math.exp(Math.log(TEX_SCALE_RANGE) * d)/TEX_SCALE_RANGE * MAX_TEX_SCALE;
	}
	
	public void setTextureScale(double d) {
		texScaleSlider.setValue(
				(int)(Math.log(d / MAX_TEX_SCALE * TEX_SCALE_RANGE)/Math.log(TEX_SCALE_RANGE)*100)
			);
		if (tex != null) {
			tex.setTextureMatrix(MatrixBuilder.euclidean().scale(d).getMatrix());
		}
	}

	public String getTexture() {
		return textureGroup.getSelection().getActionCommand();
	}
	
	public void setTexture(String name) {
		textureGroup.setSelected(textureNameToButton.get(name),true);
		try {
			if ("none".equals(name)) {
				getViewerVR().getContentAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.TEXTURE_2D, Appearance.INHERITED, Texture2D.class);
			} else {
				ImageData img = ImageData.load(Input.getInput(textureNameToTexture.get(name)));
				tex = TextureUtility.createTexture(getViewerVR().getContentAppearance(), "polygonShader", img, false);
				setTextureScale(getTextureScale());
			}
		} catch (IOException e1) {
			e1.printStackTrace();
		}
	}
	
	public void switchToTextureBrowser() {
		getViewerVR().switchToFileChooser(texFileChooser);
	}
	
	@Override
	public void restoreDefaults() {
		// tex panel
		setTextureScale(DEFAULT_TEXTURE_SCALE);
		setTexture(DEFAULT_TEXTURE);
	}

	@Override
	public void storePreferences(Preferences prefs) {
		// tex panel
		prefs.putDouble("textureScale", getTextureScale());
		prefs.put("texture", getTexture());
	}
	
	@Override
	public void restorePreferences(Preferences prefs) {
		// tex panel
		setTextureScale(prefs.getDouble("textureScale", DEFAULT_TEXTURE_SCALE));
		setTexture(prefs.get("texture", DEFAULT_TEXTURE));
	}
	
}
