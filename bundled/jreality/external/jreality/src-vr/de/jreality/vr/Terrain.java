package de.jreality.vr;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.security.PrivilegedAction;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;

import javax.swing.ButtonGroup;
import javax.swing.ButtonModel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.shader.ImageData;
import de.jreality.util.Input;
import de.jreality.util.Secure;

public class Terrain {

	public enum GeometryType {
		FLAT("flat"),
		NON_FLAT("non-flat"),
		CUSTOM("custom");
		private String name;
		GeometryType(String name) {
			this.name=name;
		}
		public String getName() {
			return name;
		}
	};
	
	private static String[][] defaultLandscapes = {
		{"grid","textures/grid.jpeg"},
		{"black grid","textures/gridBlack.jpg"},
		{"tiles","textures/recycfloor1_clean2.png"},
		//{"tech","textures/kf_techfloor10c.png"},
		{"rust","textures/outfactory3.png"},
		//{"tiles","textures/recycfloor1_clean2.png"},
	};

	private GeometryType geometryType = GeometryType.FLAT;
	
	private HashMap<GeometryType, ButtonModel> geometryButtons = new HashMap<GeometryType, ButtonModel>();
	private HashMap<String, ImageData> imageMap = new HashMap<String, ImageData>();
	
	private HashMap<String, JRadioButton> texNameToButton = new HashMap<String, JRadioButton>();
	
	private List<ChangeListener> listeners = new LinkedList<ChangeListener>();
	private JPanel texureSelection;
	private JPanel geometrySelection;

	private boolean customTexture;

	private ImageData texture;

	private HashMap<String, JRadioButton> geomNameToButton=new HashMap<String, JRadioButton>();
	
	public Terrain() {
		geometrySelection = new JPanel(new GridLayout(3, 1));
		ButtonGroup terrainGeometrySelection = new ButtonGroup();
		JRadioButton button = new JRadioButton(GeometryType.FLAT.getName());
		geomNameToButton.put(GeometryType.FLAT.getName(), button);
		button.getModel().addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				setGeometryType(GeometryType.FLAT);
			}
		});
		terrainGeometrySelection.add(button);
		geometrySelection.add(button);
		geometryButtons.put(GeometryType.FLAT, button.getModel());
		
		button = new JRadioButton(GeometryType.NON_FLAT.getName());
		geomNameToButton.put(GeometryType.NON_FLAT.getName(), button);
		button.getModel().addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				setGeometryType(GeometryType.NON_FLAT);
			}
		});
		terrainGeometrySelection.add(button);
		geometrySelection.add(button);
		geometryButtons.put(GeometryType.NON_FLAT, button.getModel());
				
		button = new JRadioButton(GeometryType.CUSTOM.getName());
		geomNameToButton.put(GeometryType.CUSTOM.getName(), button);
		button.getModel().addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				setGeometryType(GeometryType.CUSTOM);
			}
		});
		terrainGeometrySelection.add(button);
		geometrySelection.add(button);
		geometryButtons.put(GeometryType.CUSTOM, button.getModel());
		
		
		ButtonGroup terrainTextureSelection = new ButtonGroup();

		List<JRadioButton> buttons = new LinkedList<JRadioButton>();
		for (String[] tex : defaultLandscapes) {
			final ImageData id = loadTexture(tex[1]);
			if (id == null) continue;
			imageMap.put(tex[0], id);
			button = new JRadioButton(tex[0]);
			texNameToButton.put(tex[0], button);
			button.getModel().addActionListener(new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					setTexture(id);
				}
			});
			terrainTextureSelection.add(button);
			buttons.add(button);
		}
		
		int numRows = Math.round(.1f+(imageMap.size()+2)*.5f);
		texureSelection = new JPanel(new GridLayout(numRows, 2));
		
		for (JRadioButton b : buttons) {
			texureSelection.add(b);
		}
		
		button = new JRadioButton("none");
		button.getModel().addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				setTexture(null);
			}
		});
		terrainTextureSelection.add(button);
		texureSelection.add(button);
		texNameToButton.put("none", button);

		button = new JRadioButton("custom");
		button.getModel().addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				setCustomTexture();
			}
		});
		terrainTextureSelection.add(button);
		texureSelection.add(button);
		texNameToButton.put("custom", button);
	}
	
	protected void setCustomTexture() {
		customTexture = true;
		texture = null;
		fireChange();
	}

	protected void setTexture(ImageData id) {
		customTexture = false;
		texture = id;
		fireChange();
	}

	private ImageData loadTexture(final String resourceName) {
		return Secure.doPrivileged(new PrivilegedAction<ImageData>() {
			public ImageData run() {
				try {
					return ImageData.load(Input.getInput(resourceName));
				} catch (Exception e) {
					// error
				}
				return null;
			}
		});
	}
	
	public GeometryType getGeometryType() {
		return geometryType;
	}
	public void setGeometryType(GeometryType geometryType) {
		if (this.geometryType != geometryType) {
			this.geometryType = geometryType;
			geometryButtons.get(geometryType).setSelected(true);
			fireChange();
		}
	}

	public void addChangeListener(ChangeListener cl) {
		listeners.add(cl);
	}

	public void removeChangeListener(ChangeListener cl) {
		listeners.add(cl);
	}
	
	void fireChange() {
		ChangeEvent ev = new ChangeEvent(this);
		synchronized (listeners) {
			for (ChangeListener cl : listeners) {
				cl.stateChanged(ev);
			}
		}
	}
	
	public JPanel getGeometrySelection() {
		return geometrySelection;
	}

	public JPanel getTexureSelection() {
		return texureSelection;
	}

	public ImageData getTexture() {
		return texture;
	}

	public ImageData getImageData() {
		return texture;
	}

	public boolean isCustomTexture() {
		return customTexture;
	}

	public void setTextureByName(String name) {
		JRadioButton b = texNameToButton.get(name);
		if (b!=null) b.doClick();
	}
	
	public String getTextureName() {
		if (isCustomTexture()) return "custom";
		for (Entry<String, ImageData> id : imageMap.entrySet())
			if (id.getValue() == texture) return id.getKey();
		return "none";
	}

	public void setGeometryByName(String name) {
		JRadioButton b = geomNameToButton.get(name);
		if (b!=null) b.doClick();
	}
	
	public String getGeometryName() {
		return geometryType.getName();
	}
}
