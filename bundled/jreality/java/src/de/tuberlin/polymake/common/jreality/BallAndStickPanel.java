package de.tuberlin.polymake.common.jreality;

import java.awt.EventQueue;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.LinkedList;
import java.util.List;

import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.BallAndStickFactory;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.ui.LayoutFactory;

public class BallAndStickPanel extends JPanel implements ActionListener, ChangeListener {

	private static final long serialVersionUID = 1L;

	private BallAndStickFactory
		basf = null;
	
	private JCheckBox
		showArrowBox = new JCheckBox("Show arrows");
	
	private SpinnerNumberModel
		stickRadiusModel = new SpinnerNumberModel(0.1, 0.0, 2.0, 0.1),
		positionModel = new SpinnerNumberModel(.7, 0.0, 1.0, 0.1),
		scaleModel = new SpinnerNumberModel(.1, 0.0, 1.0, 0.1),
		slopeModel = new SpinnerNumberModel(1.5,0.5,10.0,0.5);
	
	private JSpinner
		stickRadiusSpinner = new JSpinner(stickRadiusModel),
		positionSpinner = new JSpinner(positionModel),
		scaleSpinner = new JSpinner(scaleModel),
		slopeSpinner = new JSpinner(slopeModel);
	
	private SceneGraphComponent
		basComponent = new SceneGraphComponent("Arrows");
	
	private List<ArrowChangeListener>
		listeners = new LinkedList<ArrowChangeListener>();
	
	public BallAndStickPanel() {
		super(new GridBagLayout());
		GridBagConstraints rc = LayoutFactory.createRightConstraint();
		GridBagConstraints lc = LayoutFactory.createLeftConstraint();
		
		showArrowBox.addActionListener(this);
		stickRadiusSpinner.addChangeListener(this);
		positionSpinner.addChangeListener(this);
		scaleSpinner.addChangeListener(this);
		slopeSpinner.addChangeListener(this);
		
		
		add(showArrowBox,rc);
		add(new JLabel("Tube radius"),lc);
		add(stickRadiusSpinner, rc);
		add(new JLabel("Arrow position"),lc);
		add(positionSpinner,rc);
		add(new JLabel("Head size"),lc);
		add(scaleSpinner,rc);
		add(new JLabel("Head slope"),lc);
		add(slopeSpinner,rc);
		
		enableSpinners();
	}


	private void enableSpinners() {
		showArrowBox.setEnabled(basf != null);
		positionSpinner.setEnabled(basf != null);
		scaleSpinner.setEnabled(basf != null);
		slopeSpinner.setEnabled(basf != null);
	}
	
	private void updateComponent() {
		if(basf != null) {
			basf.setStickRadius(stickRadiusModel.getNumber().doubleValue());
			basf.setShowBalls(false);
			basf.setShowArrows(true);
			basf.setArrowScale(scaleModel.getNumber().doubleValue());
			basf.setArrowSlope(slopeModel.getNumber().doubleValue());
			basf.setArrowPosition(positionModel.getNumber().doubleValue());
			basf.update();
		}
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		if(basf != null) {
			basComponent.setVisible(showArrowBox.isSelected());
		}
	}

	@Override
	public void stateChanged(ChangeEvent e) {
		updateComponent();
		fireArrowsChanged();
	}
	
	public void setLineSet(IndexedLineSet lineset) {
		basf = new BallAndStickFactory(lineset);
		enableSpinners();
	}
	
	public SceneGraphComponent getSceneGraphComponent() {
		updateComponent();
		if(basf == null) {
			return null;
		} else {
			basf.update();
			basComponent = basf.getSceneGraphComponent();
			return basComponent;
		}
	}


	public void setShowArrows(boolean showArrows) {
		showArrowBox.setSelected(showArrows);
	}
	
	public void addArrowChangeListener(ArrowChangeListener acl) {
		listeners.add(acl);
	}
	
	public void removeArrowChangeListener(ArrowChangeListener acl) {
		listeners.remove(acl);
	}
	
	private void fireArrowsChanged() {
		Runnable r = new Runnable() {
			@Override
			public void run() {
				synchronized (listeners) {
					for(ArrowChangeListener acl : listeners) {
						acl.arrowChanged();
					}
				}
			}
		};
		EventQueue.invokeLater(r);
	}
}
