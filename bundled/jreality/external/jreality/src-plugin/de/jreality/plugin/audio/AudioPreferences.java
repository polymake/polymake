package de.jreality.plugin.audio;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.security.PrivilegedAction;
import java.util.LinkedList;
import java.util.List;
import java.util.prefs.Preferences;

import javax.swing.BorderFactory;
import javax.swing.ButtonGroup;
import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JSpinner;
import javax.swing.JTextField;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.audio.javasound.JavaSoundUtility;
import de.jreality.plugin.audio.Audio.BackendType;
import de.jreality.plugin.audio.Audio.InterpolationType;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.util.Secure;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.PreferencesFlavor;

public class AudioPreferences extends Plugin implements PreferencesFlavor, ActionListener {
	
	private static final Integer DEFAULT_FRAME_SIZE = 2048;
	
	private JPanel
		mainPage = new JPanel(),
		interpolationPanel = new JPanel(),
		backendPanel = new JPanel(),
		javaOptions = new JPanel(),
		jackOptions = new JPanel();
	private SpinnerNumberModel
		retriesModel = new SpinnerNumberModel(5, 0, 20, 1);
	private JSpinner
		retriesSpinner = new JSpinner(retriesModel);
	private JTextField
		jackLabelField = new JTextField("jReality"),
		jackTargetField = new JTextField("");
	private JRadioButton
		noSoundChecker = new JRadioButton("No Audio", true),
		javaSoundChecker = new JRadioButton("Java Sound Stereo"),
		javaSoundVBAPChecker = new JRadioButton("Java Sound Surround (VBAP)"),
		jackAmbisonicsFOChecker = new JRadioButton("JACK Ambisonics First Order"),
		jackAmbisonicsPSOChecker = new JRadioButton("JACK Ambisonics Planar Second Order"),
		sampleHoldChecker = new JRadioButton("None"),
		linearChecker = new JRadioButton("Linear"),
		cosineChecker = new JRadioButton("Cosine"),
		cubicChecker = new JRadioButton("Cubic");
	private JCheckBox
		chooseFirstJavaSoundMixer = new JCheckBox("Always Use First Mixer", true);
	private JComboBox
		frameSizeCombo = new JComboBox(new Integer[] {128, 256, 512, 1024, 2048, 4096});
	private JComboBox
		sampleRateCombo = new JComboBox(new Integer[] {8000, 11025, 16000, 22050, 44100, 48000});

	private JButton applyButton = new JButton("Apply");
	
	private BackendType
		backendType = BackendType.javaSound;
	private InterpolationType
		interpolationType = InterpolationType.cubicInterpolation;
	private List<ChangeListener>
		changeListenerList = new LinkedList<ChangeListener>();
	

	public AudioPreferences() {
		mainPage.setLayout(new GridBagLayout());
		GridBagConstraints c1 = new GridBagConstraints();
		GridBagConstraints c2 = new GridBagConstraints();
		
		backendPanel.setBorder(BorderFactory.createTitledBorder("Audio Backend"));
		backendPanel.setLayout(new GridLayout(6, 1));
		backendPanel.add(noSoundChecker);
		backendPanel.add(javaSoundChecker);
		backendPanel.add(javaSoundVBAPChecker);
		backendPanel.add(jackAmbisonicsFOChecker);
		backendPanel.add(jackAmbisonicsPSOChecker);
		
		c1.anchor = GridBagConstraints.WEST;
		c1.weightx = 0.0;
		c1.gridwidth = GridBagConstraints.RELATIVE;
		c1.fill = GridBagConstraints.HORIZONTAL;
		c1.insets = new Insets(2,2,2,2);
		c2.anchor = GridBagConstraints.WEST;
		c2.weightx = 1.0;
		c2.gridwidth = GridBagConstraints.REMAINDER;
		c2.fill = GridBagConstraints.HORIZONTAL;
		c2.insets = new Insets(2,2,2,2);
		mainPage.add(backendPanel, c2);

		javaOptions.setBorder(BorderFactory.createTitledBorder("Java Sound Options"));
		javaOptions.setLayout(new GridBagLayout());
		javaOptions.add(chooseFirstJavaSoundMixer, c2);
		javaOptions.add(new JLabel("Frame Size"), c1);
		javaOptions.add(frameSizeCombo, c2);
		javaOptions.add(new JLabel("Sample Rate"), c1);
		javaOptions.add(sampleRateCombo, c2);
		mainPage.add(javaOptions, c2);
		
		jackOptions.setBorder(BorderFactory.createTitledBorder("JACK Options"));
		jackOptions.setLayout(new GridBagLayout());
		jackOptions.add(new JLabel("Client name"), c1);
		jackOptions.add(jackLabelField, c2);
		jackOptions.add(new JLabel("Target"), c1);
		jackOptions.add(jackTargetField, c2);
		jackOptions.add(new JLabel("Retries after zombification"), c1);
		jackOptions.add(retriesSpinner, c2);
		mainPage.add(jackOptions, c2);
		
		interpolationPanel.setBorder(BorderFactory.createTitledBorder("Interpolation"));
		interpolationPanel.setLayout(new GridLayout(1, 4));
		interpolationPanel.add(sampleHoldChecker);
		interpolationPanel.add(linearChecker);
		interpolationPanel.add(cosineChecker);
		interpolationPanel.add(cubicChecker);
		mainPage.add(interpolationPanel, c2);
		
		mainPage.add(new JPanel(), c2);
		mainPage.add(applyButton, c2);
		
		ButtonGroup bg = new ButtonGroup();
		bg.add(noSoundChecker);
		bg.add(javaSoundChecker);
		bg.add(javaSoundVBAPChecker);
		bg.add(jackAmbisonicsFOChecker);
		bg.add(jackAmbisonicsPSOChecker);
		
		bg = new ButtonGroup();
		bg.add(sampleHoldChecker);
		bg.add(linearChecker);
		bg.add(cosineChecker);
		bg.add(cubicChecker);
		
		noSoundChecker.addActionListener(this);
		javaSoundChecker.addActionListener(this);
		javaSoundVBAPChecker.addActionListener(this);
		jackAmbisonicsFOChecker.addActionListener(this);
		jackAmbisonicsPSOChecker.addActionListener(this);
		sampleHoldChecker.addActionListener(this);
		linearChecker.addActionListener(this);
		cosineChecker.addActionListener(this);
		cubicChecker.addActionListener(this);
		applyButton.addActionListener(this);
		chooseFirstJavaSoundMixer.addActionListener(this);
		sampleRateCombo.addActionListener(this);
	}
	
	public void actionPerformed(ActionEvent e) {
		Object s = e.getSource();
		if (noSoundChecker == s) {
			backendType = BackendType.noSound;
		} 
		if (javaSoundChecker == s) {
			backendType = BackendType.javaSound;
		}
		if (javaSoundVBAPChecker == s) {
			backendType = BackendType.javaSoundVBAP;
		}
		if (jackAmbisonicsFOChecker == s) {
			backendType = BackendType.jackAmbisonicsFO;
		}
		if (jackAmbisonicsPSOChecker == s) {
			backendType = BackendType.jackAmbisonicsPSO;
		}
		if (sampleHoldChecker == s) {
			interpolationType = InterpolationType.noInterpolation;
		}
		if (linearChecker == s) {
			interpolationType = InterpolationType.linearInterpolation;
		}
		if (cosineChecker == s) {
			interpolationType = InterpolationType.cosineInterpolation;
		}
		if (cubicChecker == s) {
			interpolationType = InterpolationType.cubicInterpolation;
		}
		if (applyButton == s) {
			fireChanged();
		}
		if (chooseFirstJavaSoundMixer == s) {
			JavaSoundUtility.chooseFirstMixer = chooseFirstJavaSoundMixer.isSelected();
		}
		if (sampleRateCombo == s) {
			JavaSoundUtility.setSampleRate((Integer) sampleRateCombo.getSelectedItem());
		}
	}
	
	protected void fireChanged() {
		ChangeEvent ce = new ChangeEvent(this);
		synchronized (changeListenerList) {
			for (ChangeListener cl : changeListenerList) {
				cl.stateChanged(ce);
			}	
		}
	}
	
	public BackendType getBackendType() {
		return backendType;
	}
	
	public InterpolationType getInterpolationType() {
		return interpolationType;
	}
	
	public String getJackLabel() {
		return jackLabelField.getText();
	}
	
	public String getJackTarget() {
		return jackTargetField.getText();
	}
	
	public int getJackRetries() {
		return retriesModel.getNumber().intValue();
	}
	
	public int getJavaSoundFrameSize() {
		return (Integer) frameSizeCombo.getSelectedItem();
	}
	
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Audio Preferences", "jReality Group");
	}

	public Icon getMainIcon() {
		return ImageHook.getIcon("audio/sound.png");
	}

	public String getMainName() {
		return "Audio";
	}

	public JPanel getMainPage() {
		return mainPage;
	}

	public int getNumSubPages() {
		return 0;
	}

	public JPanel getSubPage(int i) {
		return null;
	}

	public Icon getSubPageIcon(int i) {
		return null;
	}

	public String getSubPageName(int i) {
		return null;
	}
	
	public boolean addChangeListener(ChangeListener c){
		synchronized (changeListenerList) {
			return changeListenerList.add(c);			
		}
	}
	
	public boolean removeChangeListener(ChangeListener c){
		synchronized (changeListenerList) {
			return changeListenerList.remove(c);			
		}
	}
	
	@Override
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);
		Preferences prefs = getPreferences(getClass().getSimpleName());
		prefs.put("backendType", backendType.name());
		prefs.put("interpolationType", interpolationType.name());
		prefs.putInt("javaFrameSize", (Integer) frameSizeCombo.getSelectedItem());
		prefs.put("jackLabel", jackLabelField.getText());
		prefs.put("jackTarget", jackTargetField.getText());
		prefs.putInt("retries", retriesModel.getNumber().intValue());
		prefs.putBoolean("chooseFirstJavaSoundMixer", chooseFirstJavaSoundMixer.isSelected());
		prefs.putInt("sampleRate", (Integer) sampleRateCombo.getSelectedItem());
		prefs.flush();
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);
		Preferences prefs = getPreferences(getClass().getSimpleName());
		backendType = BackendType.valueOf(BackendType.class, prefs.get("backendType", backendType.name()));
		interpolationType = InterpolationType.valueOf(InterpolationType.class, prefs.get("interpolationType", interpolationType.name()));
		
		switch (backendType) {
		case noSound:
			noSoundChecker.setSelected(true);
			break;
		case javaSound:
			javaSoundChecker.setSelected(true);
			break;
		case javaSoundVBAP:
			javaSoundVBAPChecker.setSelected(true);
			break;
		case jackAmbisonicsFO:	
			jackAmbisonicsFOChecker.setSelected(true);
			break;
		case jackAmbisonicsPSO:
			jackAmbisonicsPSOChecker.setSelected(true);
			break;
		}
		switch (interpolationType) {
		case noInterpolation:
			sampleHoldChecker.setSelected(true);
			break;
		case linearInterpolation:
			linearChecker.setSelected(true);
			break;
		case cosineInterpolation:
			cosineChecker.setSelected(true);
			break;
		case cubicInterpolation:
			cubicChecker.setSelected(true);
			break;
		}

		frameSizeCombo.setSelectedItem(prefs.getInt("javaFrameSize", DEFAULT_FRAME_SIZE));
		jackLabelField.setText(prefs.get("jackLabel", jackLabelField.getText()));
		jackTargetField.setText(prefs.get("jackTarget", jackTargetField.getText()));
		retriesModel.setValue(prefs.getInt("retries", retriesModel.getNumber().intValue()));
		chooseFirstJavaSoundMixer.setSelected(prefs.getBoolean("chooseFirstJavaSoundMixer", chooseFirstJavaSoundMixer.isSelected()));
		sampleRateCombo.setSelectedItem(prefs.getInt("sampleRate", JavaSoundUtility.getSampleRate()));
	}

	
	protected Preferences getPreferences(final String nodeName) {
		return Secure.doPrivileged(new PrivilegedAction<Preferences>() {
			public Preferences run() {
				return Preferences.userNodeForPackage(getClass()).node(nodeName);
			}
		});
	}

}
