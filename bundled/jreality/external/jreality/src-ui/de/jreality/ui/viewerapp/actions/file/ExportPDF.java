/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

package de.jreality.ui.viewerapp.actions.file;

import java.awt.Component;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JSeparator;
import javax.swing.JSpinner;
import javax.swing.JTextField;
import javax.swing.SpinnerNumberModel;

import de.jreality.io.JrScene;
import de.jreality.scene.Viewer;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jreality.writer.pdf.WriterPDF;
import de.jreality.writer.pdf.WriterPDF.PDF3DGridMode;
import de.jreality.writer.pdf.WriterPDF.PDF3DLightingScene;
import de.jreality.writer.pdf.WriterPDF.PDF3DRenderMode;
import de.jreality.writer.pdf.WriterPDF.PDF3DTool;

/**
 * Saves the current scene.
 * 
 * @author msommer
 */
public class ExportPDF extends AbstractJrAction {

	private static final long 
		serialVersionUID = 1L;
	private Viewer 
		viewer = null;
	private File
		userFile = null;
	private PDFExportAccessory
		accessory = new PDFExportAccessory();

	public ExportPDF(String name, Viewer viewer, Component parentComp) {
		super(name, parentComp);

		if (viewer == null)
			throw new IllegalArgumentException("Viewer is null!");
		this.viewer = viewer;

		setShortDescription("Save scene as a PDF file");
	}
	
	public PDFExportAccessory getAccessory() {
		return accessory;
	}


	@Override
	public void actionPerformed(ActionEvent e) {
		File file = FileLoaderDialog.selectTargetFile(parentComp, accessory, "pdf", "PDF Files");
		if (file == null)
			return; // dialog canceled

		try {
			WriterPDF writer = new WriterPDF();
			writer.setTool(accessory.getTool());
			writer.setSize(accessory.getPDFSize());
			writer.setLighting(accessory.getLighting());
			writer.setUserScriptFile(userFile);
			writer.setRenderMode(accessory.getRenderMode());
			writer.setGridMode(accessory.getGridMode());
			writer.setShowGrid(accessory.isUseGrid());
			writer.setShowInventory(accessory.isShowInventory());
			writer.setShowAxes(accessory.isUseAxes());
			JrScene s = new JrScene(viewer.getSceneRoot());
			s.addPath("cameraPath", viewer.getCameraPath());
			ToolSystem ts = ToolSystem.toolSystemForViewer(viewer);
			if (ts.getAvatarPath() != null)
				s.addPath("avatarPath", ts.getAvatarPath());
			if (ts.getEmptyPickPath() != null)
				s.addPath("emptyPickPath", ts.getEmptyPickPath());
			FileOutputStream fileOutputStream = new FileOutputStream(file);
			writer.writeScene(s, fileOutputStream);
			fileOutputStream.close();
		} catch (IOException ioe) {
			JOptionPane.showMessageDialog(parentComp, "Save failed: "
					+ ioe.getMessage());
		}
	}

	private class PDFExportAccessory extends JPanel implements ActionListener {

		private static final long 
			serialVersionUID = 1L;
		private JComboBox 
			lightingCombo = new JComboBox(PDF3DLightingScene.values()),
			toolCombo = new JComboBox(PDF3DTool.values()),
			renderModeCombo = new JComboBox(PDF3DRenderMode.values()),
			gridModeCombo = new JComboBox(PDF3DGridMode.values());
		private SpinnerNumberModel
			widthModel = new SpinnerNumberModel(800, 1, 10000, 10),
			heightModel = new SpinnerNumberModel(600, 1, 10000, 10);
		private JSpinner
			widthSpinner = new JSpinner(widthModel),
			heightSpinner = new JSpinner(heightModel);
		private JTextField
			userFileField = new JTextField();
		private JButton
			browseButton= new JButton("User Script...");
		private JCheckBox
			showInventoryChecker = new JCheckBox("PDF Inventory"),
			useGridChecker = new JCheckBox("Grid"),
			useAxesChecker = new JCheckBox("Show Axes");
		
		public PDFExportAccessory() {
			setBorder(BorderFactory.createTitledBorder("PDF Options"));
			setLayout(new GridBagLayout());
			GridBagConstraints c = new GridBagConstraints();
			c.insets = new Insets(2,2,2,2);
			c.weightx = 1.0;
			c.fill = GridBagConstraints.HORIZONTAL;
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(new JLabel("Tool:"), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(toolCombo, c);
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(new JLabel("Lighting:"), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(lightingCombo, c);
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(new JLabel("Mode:"), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(renderModeCombo, c);
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(useGridChecker, c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(gridModeCombo, c);
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(showInventoryChecker, c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(useAxesChecker, c);
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(new JLabel("Size:"), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			JPanel sizePanel = new JPanel();
			sizePanel.setLayout(new FlowLayout());
			sizePanel.add(widthSpinner);
			sizePanel.add(new JLabel("X"));
			sizePanel.add(heightSpinner);
			add(sizePanel, c);
			add(new JSeparator(), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(browseButton, c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(userFileField, c);

			// default values
			toolCombo.setSelectedItem(PDF3DTool.SPIN);
			lightingCombo.setSelectedItem(PDF3DLightingScene.DAY);
			renderModeCombo.setSelectedItem(PDF3DRenderMode.SOLID);
			gridModeCombo.setSelectedItem(PDF3DGridMode.GRID_MODE_OFF);
			
			userFileField.setEditable(false);
			browseButton.addActionListener(this);
		}

		public PDF3DTool getTool() {
			return (PDF3DTool)toolCombo.getSelectedItem();
		}

		public Dimension getPDFSize() {
			return new Dimension(widthModel.getNumber().intValue(), heightModel.getNumber().intValue());
		}
		
		public PDF3DLightingScene getLighting() {
			return (PDF3DLightingScene)lightingCombo.getSelectedItem();
		}
		
		public PDF3DRenderMode getRenderMode() {
			return (PDF3DRenderMode)renderModeCombo.getSelectedItem();
		}
		
		public PDF3DGridMode getGridMode() {
			return (PDF3DGridMode)gridModeCombo.getSelectedItem();
		}
		
		public boolean isUseGrid() {
			return useGridChecker.isSelected();
		}
		
		public boolean isUseAxes() {
			return useAxesChecker.isSelected();
		}
		
		public boolean isShowInventory() {
			return showInventoryChecker.isSelected();
		}
		

		public void actionPerformed(ActionEvent e) {
			userFile = FileLoaderDialog.loadFile(parentComp, "js", "Java Script Files");
			userFileField.setText(userFile.getName());
		}
		
	}

}