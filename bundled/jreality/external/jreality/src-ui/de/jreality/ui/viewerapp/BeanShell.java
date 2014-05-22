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

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.lang.reflect.Proxy;

import javax.swing.BorderFactory;
import javax.swing.JScrollPane;
import javax.swing.text.SimpleAttributeSet;
import javax.swing.text.StyleConstants;

import bsh.EvalError;
import de.jreality.scene.SceneGraphNode;
import de.jtem.jterm.BshEvaluator;
import de.jtem.jterm.JTerm;
import de.jtem.jterm.Session;


/**
 * @author msommer
 */
public class BeanShell implements SelectionListener {

	private BshEvaluator bshEval;
	private JTerm jterm;
	private SimpleAttributeSet infoStyle;

	private Component beanShell;


	public BeanShell() {
		this(null);
	}

	public BeanShell(SelectionManager sm) {

		bshEval = new BshEvaluator();

		jterm = new JTerm(new Session(bshEval));
		jterm.setMaximumSize(new Dimension(10, 10));

		infoStyle = new SimpleAttributeSet();
		StyleConstants.setForeground(infoStyle, new Color(65, 166, 48));  //from SGC icon
		StyleConstants.setFontFamily(infoStyle, "Monospaced");
		StyleConstants.setBold(infoStyle, true);
		StyleConstants.setFontSize(infoStyle, 12);

		if ( sm!= null) {
			sm.addSelectionListener(this);
			setSelf(sm.getSelection().getLastElement());  //select current selection
		}

	}


	public void selectionChanged(SelectionEvent e) {
		setSelf(e.getSelection().getLastElement());
	}


	public void eval(String arg) {

		try { bshEval.getInterpreter().eval(arg); } 
		catch (EvalError error) { error.printStackTrace(); }
	}


	/**
	 * Get the bean shell as a Component.
	 * @return the bean shell
	 */
	public Component getComponent() {

		if (beanShell == null) {
			beanShell = new JScrollPane(jterm);
			((JScrollPane)beanShell).setBorder(BorderFactory.createEmptyBorder());
//			beanShell.setPreferredSize(new Dimension(0,0));  //let user set the size
		}

		return beanShell;
	}


	public void setSelf(Object obj) {

		if (obj == null) return;

		try {
			if (obj.equals(bshEval.getInterpreter().get("self"))) return;
			bshEval.getInterpreter().set("self", obj);
			String name = (obj instanceof SceneGraphNode) ? ((SceneGraphNode)obj).getName() : "";
			String type = Proxy.isProxyClass(obj.getClass()) ? obj.getClass().getInterfaces()[0].getName() : obj.getClass().getName();
			String info="\nself="+name+"["+type+"]\n";
			try {
				jterm.getSession().displayAndPrompt(info, infoStyle);
				jterm.setCaretPosition(jterm.getDocument().getLength());
			} 
			catch (Exception exc) {
				// unpatched jterm 
			}
		} 
		catch (EvalError error) { 
			error.printStackTrace();
		}
	}


	public BshEvaluator getBshEval() {
		return bshEval;
	}

}