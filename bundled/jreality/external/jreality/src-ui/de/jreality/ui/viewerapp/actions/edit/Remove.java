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

import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;

import javax.swing.KeyStroke;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.ui.viewerapp.Selection;
import de.jreality.ui.viewerapp.SelectionEvent;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.actions.AbstractSelectionListenerAction;
import de.jreality.util.SceneGraphUtility;


/**
 * Removes selected scene graph nodes (except the scene root) or tools.
 * 
 * @author msommer
 */
public class Remove extends AbstractSelectionListenerAction {

	public Remove(String name, SelectionManager sm) {
		super(name, sm);

		setAcceleratorKey(KeyStroke.getKeyStroke(KeyEvent.VK_DELETE, 0));
		setShortDescription("Delete");
	}


	public void actionPerformed(ActionEvent e) {

		SceneGraphNode node = getSelection().getLastNode();  //the node to be removed or from which to remove a tool
		SceneGraphPath parentPath = getSelection().getSGPath().popNew();  //empty if node==root
		SceneGraphComponent parent;
		if (getSelection().getLength() > 1) parent=parentPath.getLastComponent();
		else parent = getSelection().getLastComponent();  //node==root

		if (getSelection().isNode()) {  //no tool selected
			SceneGraphUtility.removeChildNode(parent, node);
			getSelectionManager().setSelection(new Selection(parentPath));
		}
		else {  //tool selected
			getSelection().getLastComponent().removeTool(getSelection().asTool());
			getSelectionManager().setSelection(getSelection());
		}
	}


	@Override
	public boolean isEnabled(SelectionEvent e) {
		//returns true iff a node!=root or a tool is selected
		return (e.toolSelected() || 
				(e.nodeSelected() && !e.rootSelected()) );
	}

}