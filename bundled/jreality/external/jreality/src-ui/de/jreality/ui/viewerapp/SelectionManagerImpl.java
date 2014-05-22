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

import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Vector;
import java.util.WeakHashMap;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.LoggingSystem;
import de.jreality.util.Rectangle3D;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;


/**
 * Manages selections within a jReality scene.
 * 
 * @author msommer, Charles Gunn
 */
public class SelectionManagerImpl implements SelectionManager {

	private Selection defaultSelection;
	private Selection selection;

	private Vector<SelectionListener> listeners;

	private boolean renderSelection = false;  //default
	private SceneGraphComponent selectionKit;
	private SceneGraphComponent selectionKitOwner;

	
	static WeakHashMap<Viewer, SelectionManager> globalTable = new WeakHashMap<Viewer, SelectionManager>();
	
	public static SelectionManager selectionManagerForViewer(Viewer viewer)	{
		SelectionManager sm = globalTable.get(viewer);
		
		if (sm != null)	{
			LoggingSystem.getLogger(SelectionManagerImpl.class).fine("Selection manager is "+sm);
			return sm;
		}
		//get all depending viewers in hierarchy and check if SelectionManager already exists
		List<Viewer> viewers = new LinkedList<Viewer>();
		Viewer v = viewer;
		viewers.add(v);
		if (v instanceof ViewerSwitch) {
			Viewer[] vs = ((ViewerSwitch)v).getViewers();
			sm = globalTable.get((ViewerSwitch)v);  //should be null
			for (int i = 0; i < vs.length; i++) {
				viewers.add(vs[i]);
				if (globalTable.get(vs[i]) != null) {  //SelectionManager exists for vs[i]
					if (sm!=null && sm!=globalTable.get(vs[i])) 
						System.err.println("Distinct SelectionManagers used in viewer hierarchy of "+v);
					sm = globalTable.get(vs[i]);
				}
			}
		} else {
			if (globalTable.get(v) != null) {  //SelectionManager exists for vs[i]
				if (sm!=null && sm!=globalTable.get(v)) 
					System.err.println("Distinct SelectionManagers used in viewer hierarchy of "+v);
				sm = globalTable.get(v);
			}
		}
		
		if (sm == null) {  //create new SelectionManager
			String selectionManager = Secure.getProperty(SystemProperties.SELECTION_MANAGER, SystemProperties.SELECTION_MANAGER_DEFAULT);		
			try { 
				sm = (SelectionManager) Class.forName(selectionManager).newInstance();	
				sm.setDefaultSelection(new Selection(new SceneGraphPath(v.getSceneRoot())));
				} 
			catch (Exception e) {	e.printStackTrace(); } 
		}
		
		//add mapping for viewer depending viewers
		for (Viewer vw : viewers) globalTable.put(vw, sm);
		
		return sm;
	}
	
	public static void disposeForViewer(Viewer viewer) {
		if (viewer instanceof ViewerSwitch) {
			for (Viewer v : ((ViewerSwitch)viewer).getViewers()) {
				disposeForViewer(v);
			}	
		}
		globalTable.remove(viewer);
	}
	
	public SelectionManagerImpl() {
		this(null);
	}
	
	
	public SelectionManagerImpl(Selection defaultSelection) {

		listeners = new Vector<SelectionListener>();

		//set default selection and select it
		if (defaultSelection!=null) {
			setDefaultSelection(defaultSelection);
		}
	}


	public Selection getDefaultSelection() {
		return defaultSelection;
	}


	public void setDefaultSelection(Selection defaultSelection) {
		this.defaultSelection = new Selection(defaultSelection);
		if (selection == null) selection = defaultSelection;
	}


	public Selection getSelection() {
		return (selection == null ? defaultSelection : selection);
	}


	/**
	 * Set the current selection.
	 * @param selection the current Selection object 
	 */
	public void setSelection(Selection selection) {
		if (this.selection!=null && this.selection.equals(selection)) return;  //already selected

		if (selection == null)  //nothing selected
			this.selection = new Selection(defaultSelection);
		else this.selection = new Selection(selection);

		// a convenience  to help selection tools cycle through a fixed selection path
		if (!cycling) {
			previousFullSelection = new SceneGraphPath( this.selection.sgPath);
			truncatedSelection = null;
		}
		selectionChanged();
	}


	public void addSelectionListener(SelectionListener listener)  {
		if (listeners.contains(listener)) return;
		listeners.add(listener);
	}


	public void removeSelectionListener(SelectionListener listener) {
		listeners.remove(listener);
	}


	public void selectionChanged() {

		if (!listeners.isEmpty()) {
			for (int i = 0; i<listeners.size(); i++)  {
				SelectionListener l = listeners.get(i);
				l.selectionChanged(new SelectionEvent(this, selection));
			}
		}

		if (renderSelection) {
			updateBoundingBox();
		}
	}


	private void updateBoundingBox() {

		if (selection.getLastComponent() == selectionKit) 
			return;  //bounding box selected
		if (selectionKit == null) {
			//set up representation of selection in scene graph
			selectionKit = new SceneGraphComponent("boundingBox");
			selectionKit.setOwner(this);
			boundingboxApp.setAttribute(CommonAttributes.EDGE_DRAW,true);
			boundingboxApp.setAttribute(CommonAttributes.FACE_DRAW,false);
			boundingboxApp.setAttribute(CommonAttributes.VERTEX_DRAW,false);
			boundingboxApp.setAttribute(CommonAttributes.LIGHTING_ENABLED,false);
			boundingboxApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_STIPPLE,true);
			boundingboxApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_FACTOR, 1.0);
			boundingboxApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_STIPPLE_PATTERN, 0x6666);
			boundingboxApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_WIDTH, 2.0);
			boundingboxApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DEPTH_FUDGE_FACTOR, 1.0);
			boundingboxApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBES_DRAW, false);
			boundingboxApp.setAttribute(CommonAttributes.LEVEL_OF_DETAIL,0.0);
			boundingboxApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, java.awt.Color.WHITE);
			selectionKit.setAppearance(boundingboxApp);
		}
		System.err.println("Updating bb");

		Rectangle3D bbox = BoundingBoxUtility.calculateChildrenBoundingBox( selection.getLastComponent() ); 

		IndexedFaceSet box = IndexedFaceSetUtility.representAsSceneGraph(null, bbox);
		selectionKit.setPickable(false);

		selectionKit.setGeometry(box);
		if (selectionKitOwner!=null) selectionKitOwner.removeChild(selectionKit);
		selectionKitOwner = selection.getLastComponent();
		boundingboxApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_STIPPLE,selectionKitOwner.getTransformation() == null);
		selectionKitOwner.addChild(selectionKit);
	}


	public boolean isRenderSelection() {
		return renderSelection;
	}


	public void setRenderSelection(boolean renderSelection) {
		this.renderSelection = renderSelection;
		if (renderSelection) updateBoundingBox();
		if (selectionKit != null) selectionKit.setVisible(renderSelection);
	}


	public SceneGraphPath getDefaultSelectionPath() {
		return getDefaultSelection().getSGPath();
	}


	public SceneGraphPath getSelectionPath() {
		return getSelection().getSGPath();
	}


	public void setDefaultSelectionPath(SceneGraphPath defaultSelection) {
		setDefaultSelection(new Selection(defaultSelection));
	}


	public void setSelectionPath(SceneGraphPath selection) {
		if (selection == null || selection.getLength() == 0)
			setSelection(defaultSelection);
		else setSelection(new Selection(selection));
	}

	SceneGraphPath truncatedSelection = null, previousFullSelection;
	boolean cycling;
	public void cycleSelectionPath()	{
		if (truncatedSelection == null || truncatedSelection.getLength()<=2) {
			truncatedSelection = new SceneGraphPath(previousFullSelection);
			LoggingSystem.getLogger(this).info("reached end");
		}
		else truncatedSelection = truncatedSelection.popNew();
		LoggingSystem.getLogger(this).info("truncated selection is "+truncatedSelection);
		// use of cycling here probably makes this un-thread-safe but it's also very unlikely ...
		cycling = true;
		setSelectionPath(truncatedSelection);
		cycling = false;
	}

	
	// add functionality to create a list of selections and to cycle through them
	private SceneGraphPath currentCycleSelection;
	private Vector<SceneGraphPath> selectionList = new Vector<SceneGraphPath>();
	private Appearance boundingboxApp  = new Appearance("app");
;
	
	public void addSelection(SceneGraphPath p)	{
		Iterator iter = selectionList.iterator();
		while (iter.hasNext())	{
			SceneGraphPath sgp = (SceneGraphPath) iter.next();
			if (sgp.isEqual(p)) return;
		}
		selectionList.add(p);
		LoggingSystem.getLogger(this).fine("Adding path "+p.toString());
	}
		
	public void removeSelection(SceneGraphPath p)	{
		Iterator iter = selectionList.iterator();
		while (iter.hasNext())	{
			SceneGraphPath sgp = (SceneGraphPath) iter.next();
			if (sgp.isEqual(p)) {
				if (currentCycleSelection != null && 
						currentCycleSelection.equals(sgp)) cycleSelection();
				selectionList.remove(sgp);
				LoggingSystem.getLogger(this).info("Removing path "+p.toString());
				return;
			}
		}
	}
	
	public void clearSelections()	{
		selectionList.clear();
	}
		
	public void cycleSelection()	{
		int target = 0;
		if (selectionList == null || selectionList.size() == 0)		return;
		if (currentCycleSelection != null) {
			int which = selectionList.indexOf(currentCycleSelection);
			if (which != -1)  {
				target = (which + 1) % selectionList.size();
			}
		}
		currentCycleSelection = (SceneGraphPath) selectionList.get(target);
		LoggingSystem.getLogger(this).info("Cycling selection to "+currentCycleSelection.toString());
		setSelectionPath(currentCycleSelection);
	}


}