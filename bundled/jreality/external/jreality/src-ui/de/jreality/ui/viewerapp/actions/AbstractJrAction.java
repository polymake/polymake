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


package de.jreality.ui.viewerapp.actions;

import java.awt.Component;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;

import javax.swing.AbstractButton;
import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.JMenuItem;
import javax.swing.KeyStroke;


/**
 * Abstract class for actions used in jReality applications. 
 * 
 * @author msommer
 */
@SuppressWarnings("serial")
public abstract class AbstractJrAction extends javax.swing.AbstractAction {


	private static final int CMD_MASK = Toolkit.getDefaultToolkit().getMenuShortcutKeyMask();
	
	protected Component parentComp;


	/**
	 * Default constructor.
	 * @param name the name of the action
	 */
	public AbstractJrAction(String name) {
		this(name, (Component)null);
	}


	/**
	 * Constructor for actions which need a parent component 
	 * e.g. for displaying dialogs.
	 * @param name the name of the action
	 * @param parentComp the parent component
	 */
	public AbstractJrAction(String name, Component parentComp) {
		super(name);
		this.parentComp = parentComp;
	}


	public abstract void actionPerformed(ActionEvent e);


	/**
	 * Set the action's name.
	 * @param name the name of this action
	 */
	public void setName(String name) {
		putValue(NAME, name);
	}


	/**
	 * Set the action's short description.
	 * @param name the short description of this action
	 */
	public void setShortDescription(String desc) {
		putValue(SHORT_DESCRIPTION, desc);
	}


	/**
	 * Set the accelerator key for this action.
	 * @param key the accelerator keystroke
	 */
	public void setAcceleratorKey(KeyStroke key) {
		putValue(ACCELERATOR_KEY, key);
	}

	public JMenuItem createMenuItem() {
		return new JMenuItem(this);
	}

	public AbstractButton createToolboxItem() {
		JButton ret = new JButton(this);
		if (ret.getIcon() != null) {
			String text = ret.getText();
			ret.setToolTipText(text);
			ret.setText(null);
			//ret.setHideActionText(true); this is java 6
		}
		return ret;
	}

	public void setIcon(Icon icon) {
		putValue(SMALL_ICON, icon);
	}

	public Icon getIcon() {
		return (Icon) getValue(SMALL_ICON);
	}

	protected void setShortCut(int key, int mask, boolean cmdKey) {
		setAcceleratorKey(KeyStroke.getKeyStroke(key, mask | (cmdKey ? CMD_MASK : 0)));
	}
}