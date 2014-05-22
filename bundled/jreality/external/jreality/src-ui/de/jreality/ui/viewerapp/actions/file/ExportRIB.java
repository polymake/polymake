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


import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.border.EmptyBorder;
import javax.swing.border.TitledBorder;

import de.jreality.renderman.RIBViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;


/**
 * Exports the scene displayed in a viewer as a RIB file.
 * 
 * @author sommer, bleicher
 */
public class ExportRIB extends AbstractJrAction {

  private Viewer viewer;
  
  //needed for options in export dialog
  private JComponent options; 
  private JComboBox type;
  private JCheckBox shadowsDialog;
  private JCheckBox reflectionDialog;
  private JCheckBox volumeDialog;
  private JButton includeFileDialog;
  private String includeFilePath = "";
  private JLabel includeFileLabel = new JLabel(" ");  

  
  public ExportRIB(String name, Viewer viewer, Component parentComp) {
    super(name, parentComp);
    
    if (viewer == null) 
      throw new IllegalArgumentException("Viewer is null!");
    this.viewer = viewer;
    
    setShortDescription("Export Renderman file");
  }

//  public ExportRIB(String name, ViewerApp v) {
//    this(name, v.getViewerSwitch(), v.getFrame());
//  }
    
  
  @Override
  public void actionPerformed(ActionEvent e) {
    
    if (options == null) options = createAccessory();
    
    File file = FileLoaderDialog.selectTargetFile(parentComp, options, "rib", "RIB files");
    if (file == null) return;  //dialog cancelled

    if(viewer.getSceneRoot().getAppearance()==null) viewer.getSceneRoot().setAppearance(new Appearance());
    viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.RMAN_SHADOWS_ENABLED,shadowsDialog.isSelected()); 
    viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.RMAN_RAY_TRACING_REFLECTIONS,reflectionDialog.isSelected()); 
    viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.RMAN_RAY_TRACING_VOLUMES,volumeDialog.isSelected()); 
    if(includeFilePath!="") viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.RMAN_GLOBAL_INCLUDE_FILE,includeFilePath); 
    
    RIBViewer rv = new RIBViewer(); 
    rv.initializeFrom(viewer);       
    rv.setRendererType(type.getSelectedIndex()+1);     
    rv.setFileName(file.getPath());
    rv.render();
  }  
  
  
  private JComponent createAccessory() {
    
    JPanel panel = new JPanel(new BorderLayout());
    TitledBorder title = BorderFactory.createTitledBorder(BorderFactory.createEtchedBorder(), "Options");
    panel.setBorder(title);
    Box box = Box.createVerticalBox();
    box.setBorder(new EmptyBorder(5,10,5,10));
    
    JLabel typeLabel = new JLabel(" renderer type:");
    box.add(typeLabel);
    
    type = new JComboBox(new String[]{
        "Pixar", "3DeLight", "Aqsis", "Pixie",  //order of static fields in RIBViewer
    });
    type.setMaximumSize(new Dimension(1000, (int)(type.getPreferredSize().getHeight())));   
    type.setAlignmentX(Component.LEFT_ALIGNMENT);
    box.add(type);
   
    box.add(Box.createRigidArea(new Dimension(0,15)));
    
    shadowsDialog = new JCheckBox("ray-traced shadows", false);
    box.add(shadowsDialog);
    
    reflectionDialog = new JCheckBox("ray-traced reflections", false);
    box.add(reflectionDialog);
    
    volumeDialog = new JCheckBox("ray-traced volumes", false);
    box.add(volumeDialog);
    
    box.add(Box.createRigidArea(new Dimension(0,15)));
    
    includeFileDialog = new JButton("global include rib-file");
    includeFileDialog.setMaximumSize(new Dimension(1000, (int)(includeFileDialog.getPreferredSize().getHeight())));    
    box.add(includeFileDialog);     
    includeFileDialog.setActionCommand("openIncludeFile");
    includeFileDialog.addActionListener(new ActionListener(){
      public void actionPerformed(ActionEvent event){  
        if (!event.getActionCommand().equals(includeFileDialog.getActionCommand())) return; 
        if (includeFilePath.equals("")){
          File file = FileLoaderDialog.loadFile(includeFileDialog, "rib", "RIB files");          
          if (file != null) {
            includeFilePath = file.getAbsolutePath(); 
            if(!File.separator.equals("/")){
            	int ind;
            	String newPath;
            	while(includeFilePath.contains(File.separator)){
            		ind=includeFilePath.indexOf(File.separator);
            		if(ind>0) newPath=includeFilePath.substring(0, ind);
            		else newPath="";
            		newPath=newPath+"/";
            		if(ind<includeFilePath.length()) newPath=newPath+includeFilePath.substring(ind+1);
            		includeFilePath=newPath;
            	}
            }
            includeFileDialog.setText("remove included file");
            includeFileLabel.setText("  > include "+file.getName());
          }  
        } 
        else {
          includeFileDialog.setText("global include rib-file");
          includeFileLabel.setText(" ");
          includeFilePath = "";
        }
      }
    });
    
    box.add(includeFileLabel);
    
    panel.add(box);
    return panel;
  }

}