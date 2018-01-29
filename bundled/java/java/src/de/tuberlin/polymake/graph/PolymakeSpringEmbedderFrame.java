/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

package de.tuberlin.polymake.graph;

import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.util.Iterator;
import java.util.Properties;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JPanel;

import de.tuberlin.polymake.common.PolymakeControl;
import de.tuberlin.polymake.common.PolymakeFrame;
import de.tuberlin.polymake.common.SelectorThread;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.geometry.PointSet;
import de.tuberlin.polymake.common.io.SimpleGeometryParser;
import de.tuberlin.polymake.common.ui.ParameterPanel;
import de.tuberlin.polymake.common.ui.PolymakeSlider;
import de.tuberlin.polymake.common.ui.SliderEvent;
import de.tuberlin.polymake.common.ui.SliderListener;

/**
 * @author thilosch
 *
 */
public abstract class PolymakeSpringEmbedderFrame extends PolymakeFrame implements SliderListener{

	/**
	 * 
	 */
	private static final long serialVersionUID = 2178122685636533022L;

	protected boolean startup = true;
	
	
	/**
	 * The panel containing the but
	 */
	protected SpringEmbedderAnimationPanel seaPanel = new SpringEmbedderAnimationPanel();

	/**
	 * @param geom
	 * @param title
	 * @param params
	 * @param iparams
	 * @param parent
	 */
	public PolymakeSpringEmbedderFrame(EmbeddedGeometries geom, String title, Properties params, Properties iparams, PolymakeControl parent) {
		super(geom, title, params, iparams, parent);
		parameters.setProperty("continue", params.getProperty("continue"));
	}

	/** update the frame with new values for the vertices and the zoom Factor */
	public void update(PointSet ps, Properties params) {
//		if (!getTitle().equals(ps.getName())) {
//			geomTitle = ps.getName();
//			setTitle(geomTitle);
//		}
		

		for (Iterator it = iparameters.keySet().iterator(); it.hasNext();) {
			String param = (String) (it.next());
			if (!params.getProperty(param).equals("null")) {
				parameters.setProperty(param, params.getProperty(param));
			}
		}
		seaPanel.setStopButtonEnabled(!parameters.getProperty("continue").equals("null"));
		paramPanel.updateSliders(parameters);
	
		if (!params.getProperty("continue").equals("null")) {
			parameters.setProperty("continue", params.getProperty("continue"));
			seaPanel.setStopButtonEnabled(!(parameters.getProperty("continue")
					.equals("0")));
		}
		if (!params.getProperty("delay").equals("null")) {
			seaPanel.setDelay(parameters.getProperty("delay"));
		}
		geometry.update(ps, false);
	
//		if (startup) {
//			startup = false;
//			encompass();
//		}
	}

	private JPanel getComputationPanel() {
		JPanel computationPanel = new JPanel(new BorderLayout());
		JPanel buttonPanel = new JPanel(new GridLayout(1, 2));
		JButton resetButton = new JButton("Reset");
		resetButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				try {
					for (Iterator it = paramPanel.getParameters().iterator(); it
							.hasNext();) {
						String param = (String) it.next();
						parameters.setProperty(param, paramPanel
								.getParameter(param));
					}
					seaPanel.resetStepValue();
					//	String stepValue = seaPanel.getStepValue();
					//	parameters.setProperty("step", stepValue);
					//	parameters.setProperty("continue",
					//	(!stepValue.equals("0")) ? "1" : "0");
					//	autoRecompute = !stepValue.equals("0");
					parameters.setProperty("continue", "0");
					parameters.setProperty("restart", "1");
					parentControl.putMessage(SimpleGeometryParser.write(geometry
							.getName(), parameters), 'C', true);
					parameters.remove("restart");
					statusBar.setText("Press <Recompute> to start embedding.");
				} catch (IOException ex) {
					SelectorThread.newErr
							.println("SpringEmbedderFrame: error writing to client");
					ex.printStackTrace(SelectorThread.newErr);
				}
			}
		});
	
		JButton recomputeButton = new JButton("Recompute");
		recomputeButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				try {
					for (Iterator it = paramPanel.getParameters().iterator(); it
							.hasNext();) {
						String param = (String) it.next();
						parameters.setProperty(param, paramPanel
								.getParameter(param));
					}
					parameters.setProperty("delay", seaPanel.getDelay());
					String stepValue = seaPanel.getStepValue();
					parameters.setProperty("step", stepValue);
					parameters.setProperty("continue",
							(!stepValue.equals("0")) ? "1" : "0");
					Vector markedVertices = geometry.getMarkedVertices();
					parentControl.putMessage(SimpleGeometryParser.write(geometry
							.getEmbedding(), markedVertices, parameters),'C', true);
					statusBar.setText(" ");
				} catch (IOException ex) {
					SelectorThread.newErr
							.println("SpringEmbedderFrame: error writing to client");
					ex.printStackTrace(SelectorThread.newErr);
				}
			}
		});
		buttonPanel.add(resetButton);
		buttonPanel.add(recomputeButton);
		computationPanel.add(buttonPanel, BorderLayout.CENTER);
		computationPanel.add(statusBar, BorderLayout.SOUTH);
		return computationPanel;
	}

	/**
	 * This method needs to be called at the end of the contructor of a
	 * subclass of the PolymakeSpringEmbedderFrame to setup a uniform
	 * GUI.
	 */
	public void setupSpringEmbedderGUI() {
		seaPanel.addStopButtonListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				seaPanel.setStopButtonEnabled(false);
				try {
					parentControl.putMessage("s continue 0\nx\n",'C', true);
				} catch (IOException e) {
					setStatus("Could not stop animation because of IOException.");
					e.printStackTrace(SelectorThread.newErr);
				}
			}
		});
		southBox.add(seaPanel);
		paramPanel = new ParameterPanel(parameters, iparameters, this);
		southBox.add(paramPanel);
		southBox.add(getComputationPanel());
		setupFrame();
	}

	public void sliderValueChanged(SliderEvent event) {
		PolymakeSlider paramSlider = (PolymakeSlider) event.getSource();
		String paramName = paramSlider.getLabel();
		if (Double.parseDouble((String) (parameters.get(paramName))) == paramSlider
				.getDoubleValue()) {
			return;
		}
		parameters.setProperty(paramName, Double.toString(paramSlider
				.getDoubleValue()));
		try {
			for (Iterator it = paramPanel.getParameters().iterator(); it.hasNext();) {
				String param = (String) it.next();
				parameters.setProperty(param, paramPanel.getParameter(param));
			}
			String stepValue = seaPanel.getStepValue();
			parameters.setProperty("step", stepValue);
			parameters.setProperty("continue", (!stepValue.equals("0")) ? "1"
					: "0");
			// autoRecompute = !stepValue.equals("0");
			parentControl.putMessage(SimpleGeometryParser.write(geometry.getName(),
					parameters), 'C',true);
			statusBar.setText(" ");
		} catch (IOException ex) {
			SelectorThread.newErr.println("SpringEmbedderFrame: communication error");
			ex.printStackTrace(SelectorThread.newErr);
		}
	}
}
