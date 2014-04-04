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

import java.awt.Component;
import java.awt.Dimension;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;

import javax.swing.BorderFactory;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.border.Border;


/**
 * @author msommer
 */
class UIFactory {

	private Component viewer = null;
	private Component beanShell = null;
	private Component navigator = null;
//	private LinkedList<Component> accessory = new LinkedList<Component>();
//	private HashMap<Component, String> accessoryTitles = new HashMap<Component, String>();
	private final Border emptyBorder = BorderFactory.createEmptyBorder();

	private boolean attachNavigator = false;  //default
	private boolean attachBeanShell = false;  //default
	
	private PropertyChangeListener sizeListener;
	
	private JSplitPane beanShellJSP;
	private JSplitPane navigatorJSP;


	protected UIFactory() {
		//needed for SetViewerSize action
		sizeListener = new PropertyChangeListener(){
			public void propertyChange(PropertyChangeEvent evt) {
				if (beanShell != null) 
					beanShell.setPreferredSize( new Dimension(0, beanShell.getSize().height) );
				if (beanShellJSP != null) beanShellJSP.resetToPreferredSizes();
				if (navigator != null) 
					navigator.setPreferredSize( new Dimension(navigator.getSize().width, 0) );				
				if (navigatorJSP != null) navigatorJSP.resetToPreferredSizes();
			}
		};
	}
	
	
	protected Component getDefaultUI() throws UnsupportedOperationException {

		if (viewer == null) 
			throw new UnsupportedOperationException("No viewer instantiated");
		viewer.removePropertyChangeListener("preferredSize", sizeListener);		
		
		if (!attachNavigator && !attachBeanShell) {  //only viewer
			return viewer;
		}
		
		Component right = viewer;
		viewer.addPropertyChangeListener("preferredSize", sizeListener);
		
		
		if (attachBeanShell) {
			int dividerLocation = -1;  //default = honor preferred size of viewer
			if (beanShellJSP != null)	dividerLocation = beanShellJSP.getDividerLocation();
			if (dividerLocation==1) dividerLocation = beanShellJSP.getLastDividerLocation();  //==1 after using (viewer) full screen mode
			
			beanShellJSP = new JSplitPane(JSplitPane.VERTICAL_SPLIT,
					viewer, beanShell);
			beanShellJSP.setContinuousLayout(true);
			beanShellJSP.setOneTouchExpandable(true);
			beanShellJSP.setResizeWeight(1.0);  //use extra space for viewer
			beanShellJSP.setDividerLocation(dividerLocation);
			//jsp.preferredSize = sum of preferred component sizes

			right = beanShellJSP;
		}

		if (attachNavigator) {  //|| !accessory.isEmpty()
			Component left = navigator;
//			JTabbedPane jtb = new JTabbedPane(JTabbedPane.TOP, JTabbedPane.SCROLL_TAB_LAYOUT);
//
//			if (attachNavigator) 
//				jtb.addTab("Navigator", navigator);
//			for (Component c : accessory)  //add accessories 
//				jtb.addTab(accessoryTitles.get(c), scroll(c));
//
//			left = jtb;
//			if (jtb.getTabCount() == 1) left = jtb.getComponentAt(0);

			int dividerLocation = -1;  //default = honor preferred size of scene tree
			if (navigatorJSP != null)	dividerLocation = navigatorJSP.getDividerLocation();
			
			navigatorJSP = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, 
					left, right);
			navigatorJSP.setContinuousLayout(true);
			navigatorJSP.setOneTouchExpandable(true);
			navigatorJSP.setResizeWeight(0.0);  //use extra space for viewer
			navigatorJSP.setDividerLocation(dividerLocation);
			//jsp.preferredSize = sum of preferred component sizes
			
			return navigatorJSP;
		}

		return right;
	}


	protected Component scroll(Component comp) {
		JScrollPane scroll = new JScrollPane(comp);
		scroll.setBorder(emptyBorder);
		return scroll;
	}


	protected void setViewer(Component component) {
		viewer = component;
		
		//initialize sizes
		if ( new Dimension(0,0).equals(viewer.getPreferredSize()) )
			viewer.setPreferredSize(new Dimension(800, 600));
		viewer.setMinimumSize(new Dimension(10, 10));
	}
	
	
	protected void setBeanShell(Component component) {
		beanShell = component;
	}


	protected void setNavigator(Component component) {
		navigator = component;
	}


	protected void setAttachNavigator(boolean b) {
		attachNavigator = b;
	}


	protected void setAttachBeanShell(boolean b) {
		attachBeanShell = b;
	}


//	protected void addAccessory(Component c) {
//		addAccessory(c, null);
//	}
//
//
//	protected void addAccessory(Component c, String title) {
//		accessory.add(c);
//		accessoryTitles.put(c, title);
//	}
//	
//	
//	protected void removeAccessories() {
//		accessory.clear();
//	}

}