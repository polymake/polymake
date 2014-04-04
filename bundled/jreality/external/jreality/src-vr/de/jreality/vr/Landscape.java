package de.jreality.vr;

import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.security.PrivilegedAction;
import java.util.ArrayList;
import java.util.HashMap;

import javax.swing.ButtonGroup;
import javax.swing.ButtonModel;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;
import de.jreality.util.Secure;

public class Landscape {

	private HashMap<String,Integer> boxes=new HashMap<String,Integer>();

	private static String sideNames= "rt,lf,up,dn,bk,ft";
	private static String nvidia= "posx,negx,posy,negy,posz,negz";

	private static String[][] defaultLandscapes = {
		{"snow","textures/jms_hc/jms_hc_", sideNames, ".png",},
		//{"dusk","textures/dragonvale_hc/dragonvale_hc_", sideNames, ".jpg",},
		//{"St. Peter", "textures/stPeter/stpeters_cross_", sideNames, ".png",},
		{"grace cross", "textures/grace_cross/grace_cross_", sideNames, ".jpg",},
		{"desert","textures/desert/desert_", sideNames, ".jpg",},
		{"emerald","textures/emerald/emerald_", sideNames, ".jpg",},
//		{"lobby","textures/nvlobby_new_cubemap/nvlobby_new_", nvidia, ".png",},
		//{"city","textures/city_cubemap/city_", nvidia, ".png",},
//		{"arch","textures/arch_cubemap/arch_", nvidia, ".jpg",},
		{"custom", null},
		{"procedural", null}
	};

	private final transient ArrayList<ChangeListener> listeners=new ArrayList<ChangeListener>();

	private ButtonGroup group;


	private HashMap<String,ButtonModel> envToButton = new HashMap<String,ButtonModel>();
	JPanel selectionComponent;

	private int selectionIndex=-1;

	private String[][] skyboxes;

	private ImageData[] cubeMap;

	/**
	 * 
	 * @param skyboxes an array of skybox descriptions:
	 *  { "name" "pathToFiles/filePrefix", "fileEnding",
	 *    "pathToTerrainTexture/textureFile", "terrainTextureScale" }
	 * @param selected the name of the intially selected sky box
	 */
	public Landscape(String[][] skyboxes, String selected) {
		this.skyboxes=(String[][])skyboxes.clone();
		//JPanel buttonGroupComponent = new JPanel(new GridLayout(skyboxes.length/2,2));
		int n = skyboxes.length;
		int numRows = n%2 == 1 ? (n+1)/2 : n/2;
		JPanel buttonGroupComponent = new JPanel(new GridLayout(numRows,2));
		buttonGroupComponent.setBorder(new EmptyBorder(5,5,5,5));
		selectionComponent = new JPanel(new BorderLayout());
		selectionComponent.add("Center", buttonGroupComponent);
//		selectionComponent.setBorder(new CompoundBorder(new EmptyBorder(5, 5, 5, 5),
//				LineBorder.createGrayLineBorder()));
		group = new ButtonGroup();
		for (int i = 0; i < skyboxes.length; i++) {
			final String name = skyboxes[i][0];
			JRadioButton button = new JRadioButton(skyboxes[i][0]);
			envToButton.put(skyboxes[i][0], button.getModel());
			button.getModel().addActionListener(new ActionListener() {

				public void actionPerformed(ActionEvent e) {
					setEvironment(name);
				}
			});
			buttonGroupComponent.add(button);
			group.add(button);
			boxes.put(skyboxes[i][0], new Integer(i));
		}
		if (selected != null) {
			setEvironment(selected);
		}
	}

	public Landscape(String selected) {
		this(defaultLandscapes,selected);
	}

	public Landscape() {
		this(defaultLandscapes, null);
	}

	public static void main(String[] args) {    
		Landscape l=new Landscape();
		JFrame f = new JFrame("test");
		f.add(l.selectionComponent);
		f.pack();
		f.setVisible(true);
		f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

	}

	private void load(final String env) {
		Secure.doPrivileged(new PrivilegedAction<Object>() {
			public Object run() {
				selectionIndex = ((Integer)boxes.get(env)).intValue();
				try {
					String[] selectedLandscape = skyboxes[selectionIndex];
					if (selectedLandscape[1] != null) {
						cubeMap=TextureUtility.createCubeMapData(selectedLandscape[1], selectedLandscape[2].split(","), selectedLandscape[3]);
					} else {
						cubeMap = null;
					}
				} catch(IOException e) {
					e.printStackTrace();
				}
				return null;
			}
		});
	}

	public JComponent getSelectionComponent() {
		return selectionComponent;
	}

	public ImageData[] getCubeMap() {
		return cubeMap;
	}
	
	public void addChangeListener(ChangeListener listener) {
		synchronized (listeners) {
			listeners.add(listener);
		}
	}

	public void removeChangeListener(ChangeListener listener) {
		synchronized (listeners) {
			listeners.remove(listener);
		}
	}

	private void fireChange() {
		synchronized (listeners) {
			ChangeEvent e = new ChangeEvent(this);
			for (ChangeListener l : listeners) {
				l.stateChanged(e);
			}
		}
	}

	public String getEnvironment() {
		return skyboxes[selectionIndex][0];
	}

	public void setEvironment(String environment) {
		if (selectionIndex == -1 || environment != getEnvironment()) {
			ButtonModel model = envToButton.get(environment);
			group.setSelected(model, true);
			load(environment);
			fireChange();
		}
	}
	
	public boolean isCustomEnvironment() {
		return "custom".equals(getEnvironment());
	}
	public boolean isProceduralEnvironment() {
		return "procedural".equals(getEnvironment());
	}
}
