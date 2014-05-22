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
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import javax.swing.JOptionPane;

import de.jreality.ui.viewerapp.FileFilter;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.SelectionEvent;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.actions.AbstractSelectionListenerAction;
import de.jreality.writer.SceneWriter;
import de.jreality.writer.WriterJRS;
import de.jreality.writer.u3d.WriterU3D;


/**
 * Saves the selected SceneGraphComponent into a file 
 * (if no SceneGraphComponent is selected, this action is disabled).
 * 
 * @author msommer
 */
@SuppressWarnings("serial")
public class SaveSelected extends AbstractSelectionListenerAction {

	FileFilter ff = new FileFilter("Supported file formats", "jrs", "u3d");
	
  public SaveSelected(String name, SelectionManager sm, Component parentComp) {
    super(name, sm, parentComp);
    
    setShortDescription("Save selected SceneGraphComponent as a file");
    setShortCut(KeyEvent.VK_S, InputEvent.SHIFT_MASK, true);
  }

  
  @Override
  public void actionPerformed(ActionEvent e) {
    File file = FileLoaderDialog.selectTargetFile(parentComp, false, ff);
    if (file == null) return;
//    if (!file.getName().endsWith(".jrs")) {
//    	JOptionPane.showMessageDialog(parentComp, "can only safe .jrs files", "unsupported format", JOptionPane.ERROR_MESSAGE);
//    	return;
//    }
    try {
      FileOutputStream fos = new FileOutputStream(file);
      String lc = file.getName().toLowerCase();
      SceneWriter writer = lc.endsWith(".jrs") ? new WriterJRS() : new WriterU3D();
      writer.write(getSelection().getLastComponent(), fos);
      fos.close();
    } catch (IOException ioe) {
      JOptionPane.showMessageDialog(parentComp, "Save failed: "+ioe.getMessage(), "IO Error", JOptionPane.ERROR_MESSAGE);
    }
  }

  
  @Override
  public boolean isEnabled(SelectionEvent e) {
    return e.componentSelected();
  }
  
}