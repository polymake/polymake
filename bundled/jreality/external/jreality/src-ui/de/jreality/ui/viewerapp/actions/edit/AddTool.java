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


package de.jreality.ui.viewerapp.actions.edit;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

import javax.swing.JDialog;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JScrollPane;
import javax.swing.ListSelectionModel;

import de.jreality.scene.tool.Tool;
import de.jreality.tools.AirplaneTool;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.tools.DampedDraggingTool;
import de.jreality.tools.DraggingTool;
import de.jreality.tools.EncompassTool;
import de.jreality.tools.FlyToPickTool;
import de.jreality.tools.FlyTool;
import de.jreality.tools.HeadTransformationTool;
import de.jreality.tools.LookAtTool;
import de.jreality.tools.PickShowTool;
import de.jreality.tools.PointerDisplayTool;
import de.jreality.tools.RotateTool;
import de.jreality.tools.ScaleTool;
import de.jreality.tools.ShipNavigationTool;
import de.jreality.tools.ShipRotateTool;
import de.jreality.tools.ShipScaleTool;
import de.jreality.tools.ShowPropertiesTool;
import de.jreality.tools.TranslateTool;
import de.jreality.ui.viewerapp.SelectionEvent;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.actions.AbstractSelectionListenerAction;


/**
 * Adds tools to a selected SceneGraphComponent (if no SceneGraphComponent is selected, this action is disabled).
 * 
 * @author msommer
 */
public class AddTool extends AbstractSelectionListenerAction {

  private boolean initialized = false;
  private JList toolList = null;
  private JOptionPane pane = null;
  private JDialog dialog = null;
  
  
  public AddTool(String name, SelectionManager sm, Component frame) {
    
    super(name, sm, frame);
    setShortDescription("Add Tools");
  }


  public void actionPerformed(ActionEvent e) {
    
    if (!initialized) initializeToolList();
    
    //show dialog
    dialog.setLocationRelativeTo(parentComp);
    dialog.setVisible(true);
    
    //add selected tools
    if (pane.getValue() instanceof Integer && 
        (Integer) pane.getValue() == JOptionPane.OK_OPTION) {
      Object[] selectedTools = toolList.getSelectedValues();
      for (int i=0; i<selectedTools.length; i++) {
        try {
          final Tool t = (Tool) Class.forName((String)selectedTools[i]).newInstance();
          getSelection().getLastComponent().addTool(t);
        } catch (Exception exc) {
          exc.printStackTrace();
          //System.out.println("Could not add tool!");
        }
      }
    }
    toolList.clearSelection();
  }
  
  
  @Override
  public boolean isEnabled(SelectionEvent e) {
    return e.componentSelected();
  }
 
  
  private void initializeToolList() {
    
    List<String> tools = new LinkedList<String>();
    
    tools.add(AirplaneTool.class.getName());
    tools.add(ClickWheelCameraZoomTool.class.getName());
    tools.add(DampedDraggingTool.class.getName());
    tools.add(DraggingTool.class.getName());
    tools.add(EncompassTool.class.getName());
    tools.add(FlyTool.class.getName());
    tools.add(FlyToPickTool.class.getName());
    tools.add(HeadTransformationTool.class.getName());
    tools.add(LookAtTool.class.getName());
    tools.add(PickShowTool.class.getName());
    tools.add(PointerDisplayTool.class.getName());
    tools.add(RotateTool.class.getName());
    tools.add(ScaleTool.class.getName());
    tools.add(ShipNavigationTool.class.getName());
    tools.add(ShipRotateTool.class.getName());
    tools.add(ShipScaleTool.class.getName());
    tools.add(ShowPropertiesTool.class.getName());
    tools.add(TranslateTool.class.getName());

    try {  //different source folder
      tools.add(Class.forName("de.jreality.tools.PortalHeadMoveTool").getName());
    } catch (ClassNotFoundException exc) {}
    
    //sort tool list entries
    Object[] obj = tools.toArray();
    Arrays.sort(obj);

    toolList = new JList(obj);
    toolList.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
    
    pane = new JOptionPane(new JScrollPane(toolList), JOptionPane.PLAIN_MESSAGE, JOptionPane.OK_CANCEL_OPTION);
    dialog = pane.createDialog(parentComp, "Add Tools");
    
    //enable choice by double-click without pressing OK
    toolList.addMouseListener(new MouseAdapter() {
      public void mouseClicked(MouseEvent evt) {
        if (evt.getClickCount() == 2) {
          dialog.setVisible(false);
          pane.setValue(JOptionPane.OK_OPTION);
        }
      }
    });

    initialized = true;
  }

}