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
import java.awt.event.ActionEvent;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import javax.swing.JOptionPane;

import de.jreality.io.JrScene;
import de.jreality.scene.Viewer;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jreality.writer.u3d.WriterU3D;


/**
 * Saves the current scene.
 * 
 * @author msommer
 */
public class ExportU3D extends AbstractJrAction {

  private Viewer viewer;
  

  public ExportU3D(String name, Viewer viewer, Component parentComp) {
    super(name, parentComp);
    
    if (viewer == null) 
      throw new IllegalArgumentException("Viewer is null!");
    this.viewer = viewer;
    
    setShortDescription("Save scene as a U3D file");
  }

//  public SaveScene(String name, ViewerApp v) {
//    this(name, v.getViewer(), v.getFrame());
//  }
  
  
  @Override
  public void actionPerformed(ActionEvent e) {
    File file = FileLoaderDialog.selectTargetFile(parentComp, "u3d", "Adobe u3d files");
    if (file == null) return;  //dialog cancelled
   
    try {
      WriterU3D writer = new WriterU3D();
      JrScene s = new JrScene(viewer.getSceneRoot());
      s.addPath("cameraPath", viewer.getCameraPath());
      ToolSystem ts = ToolSystem.toolSystemForViewer(viewer);
      if (ts.getAvatarPath() != null)  s.addPath("avatarPath", ts.getAvatarPath());
      if (ts.getEmptyPickPath() != null) s.addPath("emptyPickPath", ts.getEmptyPickPath());
      FileOutputStream fileOutputStream = new FileOutputStream(file);
	writer.writeScene(s, fileOutputStream);
	fileOutputStream.close();
    } catch (IOException ioe) {
      JOptionPane.showMessageDialog(parentComp, "Save failed: "+ioe.getMessage());
    }
  }

}