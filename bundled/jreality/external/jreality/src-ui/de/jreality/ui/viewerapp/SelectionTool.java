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


package de.jreality.ui.viewerapp;

import java.util.logging.Level;

import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.util.LoggingSystem;


/**
 * Allows selections within the displayed JrScene via picking.
 * 
 * @author msommer
 */
public class SelectionTool extends AbstractTool {

	final static InputSlot activationSlot = InputSlot.getDevice("SelectionActivation");
	private SelectionManager sm;
	private Selection selection;


	public SelectionTool(SelectionManager sm) {
		addCurrentSlot(activationSlot);
		this.sm = sm;
//		LoggingSystem.getLogger(SelectionTool.class).setLevel(Level.INFO);
	}

	public SelectionTool() {
		this((SelectionManager) null);
	}

	public SelectionTool(ViewerApp v) {
		this(v.getSelectionManager());
	}


	public void perform(ToolContext tc) {
		//only perform when activationSlot is pressed
		if (tc.getAxisState(activationSlot).isReleased()) return;

		PickResult pr = tc.getCurrentPick();

//		if (pr != null) {
//		double[] d = pr.getObjectCoordinates();
//		for (int i = 0; i < d.length; i++) {
//		System.out.print(d[i] + "  ");
//		}
//		System.out.println();
//		}

		if (pr == null) //nothing picked
			return;  //do nothing
		else selection = new Selection(pr.getPickPath().popNew());  //select component instead of its geometry

		if (sm != null) sm.setSelection(selection);

		LoggingSystem.getLogger(SelectionTool.class).log(Level.INFO, 
				"SELECTED COMPONENT: " + (selection!=null ? selection.getLastComponent().getName() : "default") );
	}

}