package de.jreality.plugin.audio;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.util.ArrayList;
import java.util.List;

import javax.swing.BorderFactory;
import javax.swing.ButtonGroup;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.audio.AudioAttributes;
import de.jreality.audio.DistanceCue;
import de.jreality.audio.DistanceCueChain;
import de.jreality.audio.DistanceCueFactory;
import de.jreality.audio.EarlyReflections;
import de.jreality.audio.FDNReverb;
import de.jreality.audio.SampleProcessor;
import de.jreality.audio.SampleProcessorFactory;
import de.jreality.audio.SchroederReverb;
import de.jreality.audio.ShiftProcessor;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Appearance;
import de.jreality.scene.data.SampleReader;
import de.jreality.ui.JSliderVR;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;

/**
 * 
 * Plugin for setting basic audio parameters such as speed of sound and such
 * 
 * TODO: Implement AppearanceListener to update widgets in line with appearance changes.
 * 
 * @author brinkman
 *
 */
public class AudioOptions extends ShrinkPanelPlugin {

	private float speedOfSound = AudioAttributes.DEFAULT_SPEED_OF_SOUND;
	private float gain = AudioAttributes.DEFAULT_GAIN;
	private float reverbGain = AudioAttributes.DEFAULT_DIRECTIONLESS_GAIN;
	private float reverbTime = AudioAttributes.DEFAULT_REVERB_TIME;
	private float pitchShift = 1;

	private JSliderVR gainWidget, speedWidget, reverbGainWidget, reverbTimeWidget, pitchShiftWidget;
	private JRadioButton noReverbButton, schroederReverbButton, fdnReverbButton;
	private JCheckBox reflectionBox, shiftBox;
	private JCheckBox conicalBox, cardioidBox, lowpassBox, linearBox, exponentialBox;

	private int[] selectedProcs = new int[0];
	private class PreProcessorFactory implements SampleProcessorFactory {
		public SampleProcessor getInstance(SampleReader reader) {
			for(int n: selectedProcs) {
				int cnt = 1;
				if      (n==cnt++) reader = new EarlyReflections(reader);
				else if (n==cnt++) reader = new ShiftProcessor(reader);
			}
			return (reader instanceof SampleProcessor) ? (SampleProcessor) reader : new SampleProcessor(reader);
		}
	};

	private int[] selectedCues = new int[0];
	private class DirectedCueFactory implements DistanceCueFactory {
		public DistanceCue getInstance(float sampleRate) {
			List<DistanceCue> list = new ArrayList<DistanceCue>(4);
			for(int i: selectedCues) {
				int cnt = 1;
				if      (i==cnt++) list.add(new DistanceCue.CONICAL());
				else if (i==cnt++) list.add(new DistanceCue.CARDIOID());
				else if (i==cnt++) list.add(new DistanceCue.LOWPASS());
				else if (i==cnt++) list.add(new DistanceCue.LINEAR());
				else if (i==cnt++) list.add(new DistanceCue.EXPONENTIAL());
			}
			DistanceCue dc = DistanceCueChain.create(list);
			dc.setSampleRate(sampleRate);
			return dc;
		}
	};

	private int reverbType = 0;
	private class ReverbFactory implements SampleProcessorFactory {
		public SampleProcessor getInstance(SampleReader reader) {
			int cnt = 1;
			if      (reverbType==cnt++) return new SchroederReverb(reader);
			else if (reverbType==cnt++) return new FDNReverb(reader);
			else                    	return new SampleProcessor(reader);
		}
	};

	private Appearance rootAppearance;

	public AudioOptions() {
		shrinkPanel.setShrinked(true);
		speedWidget = new JSliderVR(0, 1000);
		gainWidget = new JSliderVR(-60, 30, (int) toDecibels(gain));
		pitchShiftWidget = new JSliderVR(-120, 120, (int) (toCents(pitchShift)/10));
		reflectionBox = new JCheckBox("Early reflections");
		shiftBox = new JCheckBox("Pitch shift");
		conicalBox = new JCheckBox("Conical");
		cardioidBox = new JCheckBox("Cardioid");
		lowpassBox = new JCheckBox("Lowpass");
		linearBox = new JCheckBox("Linear");
		exponentialBox = new JCheckBox("Exponential");
		noReverbButton = new JRadioButton("None");
		schroederReverbButton = new JRadioButton("Schroeder");
		fdnReverbButton = new JRadioButton("FDN");
		reverbTimeWidget = new JSliderVR(0, 50, (int) reverbTime*10);
		reverbGainWidget = new JSliderVR(-60, 30, (int) toDecibels(reverbGain));

		ButtonGroup bg = new ButtonGroup();
		bg.add(noReverbButton);
		bg.add(schroederReverbButton);
		bg.add(fdnReverbButton);
		noReverbButton.setSelected(true);

		ChangeListener cl = new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				int rt = 0;
				if (schroederReverbButton.isSelected()) rt = 1;
				else if (fdnReverbButton.isSelected())  rt = 2;
				if (rt!=reverbType) {
					reverbType = rt;
					setReverbAttribute();
				}
			}
		};
		noReverbButton.addChangeListener(cl);
		schroederReverbButton.addChangeListener(cl);
		fdnReverbButton.addChangeListener(cl);
		
		cl = new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				int[] sel = new int[] {0, 0, 0, 0, 0};
				if (conicalBox.isSelected()) sel[0] = 1;
				if (cardioidBox.isSelected()) sel[1] = 2;
				if (lowpassBox.isSelected()) sel[2] = 3;
				if (linearBox.isSelected()) sel[3] = 4;
				if (exponentialBox.isSelected()) sel[4] = 5;
				selectedCues = sel;
				setDistanceCueAttribute();
			}
		};
		conicalBox.addChangeListener(cl);
		cardioidBox.addChangeListener(cl);
		lowpassBox.addChangeListener(cl);
		linearBox.addChangeListener(cl);
		exponentialBox.addChangeListener(cl);
		
		cl = new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				int[] sel = new int[] {0, 0};
				if (reflectionBox.isSelected()) sel[0] = 1;
				if (shiftBox.isSelected())      sel[1] = 2;
				selectedProcs = sel;
				setProcessorAttribute();
			}
		};
		reflectionBox.addChangeListener(cl);
		shiftBox.addChangeListener(cl);
		
		pitchShiftWidget.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				pitchShift = fromCents(pitchShiftWidget.getValue()*10);
				setPitchAttribute();
			}
		});
		speedWidget.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				speedOfSound = speedWidget.getValue();
				setSpeedAttribute();
			}
		});
		gainWidget.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				gain = fromDecibels(gainWidget.getValue());
				setGainAttribute();
			}
		});
		reverbGainWidget.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				reverbGain = fromDecibels(reverbGainWidget.getValue());
				setReverbGainAttribute();
			}
		});
		reverbTimeWidget.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				reverbTime = (float) reverbTimeWidget.getValue()/10;
				setReverbTimeAttribute();
			}
		});

		JPanel generalPanel = new JPanel();
		JPanel preprocPanel = new JPanel();
		JPanel distCuePanel = new JPanel();
		JPanel reverbPanel = new JPanel();

		generalPanel.setLayout(new GridBagLayout());
		generalPanel.setBorder(BorderFactory.createTitledBorder("General"));
		preprocPanel.setLayout(new GridBagLayout());
		preprocPanel.setBorder(BorderFactory.createTitledBorder("Preprocessor"));
		distCuePanel.setLayout(new GridBagLayout());
		distCuePanel.setBorder(BorderFactory.createTitledBorder("Distance and direction cues"));
		reverbPanel.setLayout(new GridBagLayout());
		reverbPanel.setBorder(BorderFactory.createTitledBorder("Reverb"));

		GridBagConstraints gbc = new GridBagConstraints();
		gbc.fill = GridBagConstraints.BOTH;
		gbc.insets = new Insets(2, 2, 2, 2);
		gbc.weighty = 0;

		gbc.weightx = 0;
		gbc.gridwidth = 1;
		generalPanel.add(new JLabel("<html>Speed&nbsp;of sound&nbsp;(m/s)</html>"), gbc);
		gbc.weightx = 1;
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		generalPanel.add(speedWidget, gbc);

		gbc.weightx = 0;
		gbc.gridwidth = 1;
		generalPanel.add(new JLabel("Gain (dB)"), gbc);
		gbc.weightx = 1;
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		generalPanel.add(gainWidget, gbc);

		gbc.weightx = 0;
		gbc.gridwidth = 1;
		preprocPanel.add(shiftBox, gbc);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		preprocPanel.add(reflectionBox, gbc);

		gbc.weightx = 0;
		gbc.gridwidth = 1;
		preprocPanel.add(new JLabel("<html>Pitch&nbsp;shift (10&nbsp;cents)</html>"), gbc);
		gbc.weightx = 1;
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		preprocPanel.add(pitchShiftWidget, gbc);

		gbc.weightx = 0;
		gbc.gridwidth = 1;
		distCuePanel.add(conicalBox, gbc);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		distCuePanel.add(cardioidBox, gbc);
		gbc.gridwidth = 1;
		distCuePanel.add(linearBox, gbc);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		distCuePanel.add(exponentialBox, gbc);
		gbc.gridwidth = 1;
		distCuePanel.add(lowpassBox, gbc);

		gbc.weightx = 0;
		gbc.gridwidth = 1;
		reverbPanel.add(noReverbButton, gbc);
		reverbPanel.add(schroederReverbButton, gbc);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		reverbPanel.add(fdnReverbButton, gbc);

		gbc.weightx = 0;
		gbc.gridwidth = 1;
		reverbPanel.add(new JLabel("Time (0.1s)"), gbc);
		gbc.weightx = 1;
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		reverbPanel.add(reverbTimeWidget, gbc);

		gbc.weightx = 0;
		gbc.gridwidth = 1;
		reverbPanel.add(new JLabel("Gain (dB)"), gbc);
		gbc.weightx = 1;
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		reverbPanel.add(reverbGainWidget, gbc);

		shrinkPanel.setLayout(new ShrinkPanel.MinSizeGridBagLayout());
		gbc.insets = new Insets(0, 0, 0, 0);
		shrinkPanel.add(generalPanel, gbc);
		shrinkPanel.add(preprocPanel, gbc);
		shrinkPanel.add(distCuePanel, gbc);
		shrinkPanel.add(reverbPanel, gbc);
	}

	private void setProcessorAttribute() {
		rootAppearance.setAttribute(AudioAttributes.PREPROCESSOR_KEY, new PreProcessorFactory());
	}

	private void setDistanceCueAttribute() {
		rootAppearance.setAttribute(AudioAttributes.DISTANCE_CUE_KEY, new DirectedCueFactory());
	}

	private void setReverbAttribute() {
		rootAppearance.setAttribute(AudioAttributes.DIRECTIONLESS_PROCESSOR_KEY, (reverbType==0) ? Appearance.DEFAULT : new ReverbFactory());
	}

	private void setSpeedAttribute() {
		rootAppearance.setAttribute(AudioAttributes.SPEED_OF_SOUND_KEY, speedOfSound);
	}

	private void setPitchAttribute() {
		rootAppearance.setAttribute(AudioAttributes.PITCH_SHIFT_KEY, pitchShift);
	}

	private void setGainAttribute() {
		rootAppearance.setAttribute(AudioAttributes.VOLUME_GAIN_KEY, gain);
	}

	private void setReverbGainAttribute() {
		rootAppearance.setAttribute(AudioAttributes.DIRECTIONLESS_GAIN_KEY, reverbGain);
	}

	private void setReverbTimeAttribute() {
		rootAppearance.setAttribute(AudioAttributes.REVERB_TIME_KEY, reverbTime);
	}

	private void updateProcWidgets() {
		boolean refl = false, shift = false;
		for(int i: selectedProcs) {
			refl = refl || i==1;
			shift = shift || i==2;
		}
		reflectionBox.setSelected(refl);
		shiftBox.setSelected(shift);
	}

	private void updateCueWidgets() {
		boolean conical = false, cardioid = false, lowpass = false, lin = false, exp = false;
		for(int i: selectedCues) {
			conical = conical || (i==1);
			cardioid = cardioid || (i==2);
			lowpass = lowpass || (i==3);
			lin = lin || (i==4);
			exp = exp || (i==5);
		}
		conicalBox.setSelected(conical);
		cardioidBox.setSelected(cardioid);
		lowpassBox.setSelected(lowpass);
		linearBox.setSelected(lin);
		exponentialBox.setSelected(exp);
	}

	private void updatePitchWidget() {
		pitchShiftWidget.setValue((int) (toCents(pitchShift)/10));
	}
	private void updateSpeedWidget() {
		speedWidget.setValue((int) speedOfSound);
	}

	private void updateGainWidget() {
		gainWidget.setValue((int) toDecibels(gain));
	}

	private void updateReverbWidgets() {
		int cnt = 0;
		if (reverbType==cnt++) {
			noReverbButton.setSelected(true);
		} else if (reverbType==cnt++) {
			schroederReverbButton.setSelected(true);
		} else if (reverbType==cnt++) {
			fdnReverbButton.setSelected(true);
		}
	}

	private void updateReverbGainWidget() {
		reverbGainWidget.setValue((int) toDecibels(reverbGain));
	}

	private void updateReverbTimeWidget() {
		reverbTimeWidget.setValue((int) reverbTime*10);
	}

	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Audio Options";
		info.vendorName = "Peter Brinkmann"; 
		info.icon = ImageHook.getIcon("audio/sound_add.png");
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);

		rootAppearance = c.getPlugin(Scene.class).getRootAppearance();

		updateProcWidgets();
		updateCueWidgets();
		updatePitchWidget();
		updateSpeedWidget();
		updateGainWidget();
		updateReverbWidgets();
		updateReverbGainWidget();
		updateReverbTimeWidget();
	}

	@Override
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);

		try {
			selectedProcs = c.getProperty(getClass(), AudioAttributes.PREPROCESSOR_KEY, new int[0]);
			selectedCues = c.getProperty(getClass(), AudioAttributes.DISTANCE_CUE_KEY, new int[0]);
			speedOfSound = c.getProperty(getClass(), AudioAttributes.SPEED_OF_SOUND_KEY, AudioAttributes.DEFAULT_SPEED_OF_SOUND);
			pitchShift = c.getProperty(getClass(), AudioAttributes.PITCH_SHIFT_KEY, AudioAttributes.DEFAULT_PITCH_SHIFT);
			gain = c.getProperty(getClass(), AudioAttributes.VOLUME_GAIN_KEY, AudioAttributes.DEFAULT_GAIN);
			reverbType = c.getProperty(getClass(), AudioAttributes.DIRECTIONLESS_PROCESSOR_KEY, 0);
			reverbGain = c.getProperty(getClass(), AudioAttributes.DIRECTIONLESS_GAIN_KEY, AudioAttributes.DEFAULT_DIRECTIONLESS_GAIN);
			reverbTime = c.getProperty(getClass(), AudioAttributes.REVERB_TIME_KEY, AudioAttributes.DEFAULT_REVERB_TIME);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);

		c.storeProperty(getClass(), AudioAttributes.PREPROCESSOR_KEY, selectedProcs);
		c.storeProperty(getClass(), AudioAttributes.DISTANCE_CUE_KEY, selectedCues);
		c.storeProperty(getClass(), AudioAttributes.SPEED_OF_SOUND_KEY, speedOfSound);
		c.storeProperty(getClass(), AudioAttributes.PITCH_SHIFT_KEY, pitchShift);
		c.storeProperty(getClass(), AudioAttributes.VOLUME_GAIN_KEY, gain);
		c.storeProperty(getClass(), AudioAttributes.DIRECTIONLESS_PROCESSOR_KEY, reverbType);
		c.storeProperty(getClass(), AudioAttributes.DIRECTIONLESS_GAIN_KEY, reverbGain);
		c.storeProperty(getClass(), AudioAttributes.REVERB_TIME_KEY, reverbTime);
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
	}

	private static final double dbq = 20/Math.log(10);
	private float toDecibels(float q) {
		return (float) (Math.log(q)*dbq);
	}
	private float fromDecibels(float db) {
		return (float) Math.exp(db/dbq);
	}

	private static final double cpq = 1200/Math.log(2);
	private float toCents(float q) {
		return (float) (Math.log(q)*cpq);
	}
	private float fromCents(float c) {
		return (float) Math.exp(c/cpq);
	}
}
