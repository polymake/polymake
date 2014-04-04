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

import javax.swing.AbstractAction;

import de.jreality.scene.Viewer;
import de.jreality.ui.viewerapp.SelectionEvent;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.actions.AbstractSelectionListenerAction;
import de.jreality.ui.viewerapp.actions.file.LoadFile;


/**
 * Loads one or several files and adds them as children to the selected scene graph component 
 * (optionally merges indexed face & line sets).
 * 
 * @author msommer
 */
@SuppressWarnings("serial")
public class LoadFileToNode extends AbstractSelectionListenerAction {

  private Viewer viewer;
  
  public LoadFileToNode(String name, SelectionManager sm, Viewer viewer, Component parentComp) {
    super(name, sm, parentComp);
    this.viewer = viewer;

    setShortDescription("Load one or more files into the selected node");
  }

  public LoadFileToNode(String name, SelectionManager sm, Viewer viewer) {
	  this(name, sm, viewer, null);
  }
  
  public LoadFileToNode(String name, SelectionManager sm, Component parentComp) {
	  this(name, sm, null, parentComp);
  }
  
  public LoadFileToNode(String name, SelectionManager sm) {
	  this(name, sm, null, null);
  }
  
  @Override
  public void actionPerformed(ActionEvent e) {
	  new LoadFile((String)getValue(AbstractAction.NAME), getSelection().getLastComponent(), viewer, parentComp).actionPerformed(e);
  }

  @Override
  public boolean isEnabled(SelectionEvent e) {
	  return e.componentSelected();
  }

}