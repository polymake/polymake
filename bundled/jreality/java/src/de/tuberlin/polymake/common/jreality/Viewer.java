/* Copyright (c) 1997-2014
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

package de.tuberlin.polymake.common.jreality;

import java.awt.event.ActionEvent;
import java.io.File;
import java.util.LinkedList;
import java.util.List;

import javax.swing.AbstractAction;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JPanel;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.PickShowTool;
import de.jreality.tools.PointDragListener;
import de.jreality.util.Rectangle3D;
import de.jreality.util.SystemProperties;
import de.jtem.jrworkspace.plugin.Plugin;
import de.tuberlin.polymake.common.PolymakeFrame;
import de.tuberlin.polymake.common.PolymakeViewer;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;

/**
 * @author Thilo Rörig
 * 
 */
public class Viewer extends PolymakeViewer {

	protected JRViewer jrviewer = new JRViewer();
	
	/** The root node of all displayed polymake geometries */
	protected SceneGraphComponent polymakeRoot = new SceneGraphComponent();

	protected DragEventTool dragTool = new DragEventTool("PrimaryMenu");

	protected ViewMenuBar viewerMenu = null;

	private ExplodePlugin explodePlugin = new ExplodePlugin();
	
    // private SunflowPlugin sunflowPlugin = new SunflowPlugin();
	
    public Viewer(EmbeddedGeometries eg) {
        this(eg, (Transformation)null, new Plugin[]{});
    }

    public Viewer(EmbeddedGeometries eg, Transformation transform) {
        this(eg, transform, new Plugin[]{});
    }

    public Viewer(EmbeddedGeometries eg, Plugin... plugins) {
        this(eg, (Transformation)null, plugins);
    }

    public Viewer(EmbeddedGeometries eg, Transformation transform, Plugin... plugins) {
        polymakeRoot.setName("Polymake Root");
        polymakeRoot.addTool(new PickShowTool());
        polymakeRoot.addTool(dragTool);
        Appearance rootAppearance = new Appearance();
		rootAppearance.setAttribute(CommonAttributes.RADII_WORLD_COORDINATES, true);
		rootAppearance.setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);
		rootAppearance.setAttribute(CommonAttributes.LEVEL_OF_DETAIL, 0.4);
        polymakeRoot.setAppearance(rootAppearance);
		
        List<SceneGraphComponent> componentList = new LinkedList<SceneGraphComponent>();
        
        for (int i = 0; i < eg.getNumberOfGeometries(); ++i) {
        	SceneGraphComponent comp = ((Geometry) eg.getGeometry(i)).getSceneGraphComponent();
        	componentList.add(comp);
        	polymakeRoot.addChild(comp);
        }
        

        String userdir=System.getProperty("polymake.userdir");
        System.setProperty(
				SystemProperties.VIEWER,
				SystemProperties.VIEWER_DEFAULT_JOGL + " " + 
				SystemProperties.VIEWER_DEFAULT_SOFT
			);
        jrviewer = new JRViewer();
        if (userdir != null) {
            jrviewer.setPropertiesFile(userdir + "/JReality.jrw");
        } else {
            jrviewer.setPropertiesFile((File)null);
        }		
        jrviewer.addBasicUI();
        jrviewer.addContentUI();
        jrviewer.setShowMenuBar(true);
        jrviewer.setShowToolBar(true);
        jrviewer.addContentSupport(ContentType.Raw);
		
        jrviewer.setContent(polymakeRoot);
        if (transform != null) {
            jrviewer.getPlugin(de.jreality.plugin.basic.Scene.class).getContentPath().getLastComponent().setTransformation(transform);
        }

        if(eg.getNumberOfGeometries() > 1) {
            jrviewer.registerPlugin(explodePlugin);
            explodePlugin.setSceneGraphComponent(polymakeRoot);
        }
        jrviewer.registerPlugin(new SplitGeometriesPlugin(polymakeRoot));
        VisualizationPlugin visualizationPlugin = new VisualizationPlugin(polymakeRoot);
        if(eg.getName().contains("fan:")) {
            visualizationPlugin.setSphericalRefineEnabled(true);
            eg.getName().replace("fan:", "");
        } else {
        	visualizationPlugin.setSphericalRefineEnabled(false);
        }
        
        jrviewer.registerPlugin(visualizationPlugin);
        for (Plugin p : plugins) {
            jrviewer.registerPlugin(p);
        }

        viewerMenu = jrviewer.getPlugin(ViewMenuBar.class);

        viewerMenu.addMenuItem(Viewer.class, 2, new HelpAction(), "Polymake");
		
        viewingPanel.add(jrviewer.startupLocal());

//		JMenuBar menuBar = jrp.getJMenuBar();
//		for(int i = 0; i < menuBar.getMenuCount(); ++i) {
//			SelectorThread.newErr.println(menuBar.getMenu(i).getText());
//			if(menuBar.getMenu(i).getText().equalsIgnoreCase("Help")) {
//				menuBar.getMenu(i).add(new JMenuItem("polymake"));
//			}
//		}
		
    }

	 @Override
	public void encompass() {
		MatrixBuilder mb = MatrixBuilder.euclidean();
		Transformation rootTransform = new Transformation("Normalization");
		rootTransform.setMatrix(mb.getArray());
		Rectangle3D bbox = BoundingBoxUtility.calculateBoundingBox(polymakeRoot);
		double maxExtend = bbox.getMaxExtent();		
		mb.scale(10 / maxExtend);
		rootTransform.setMatrix(mb.getArray());
		polymakeRoot.setTransformation(rootTransform);
		jrviewer.encompassEuclidean();
	}

	/*
	 * @see de.tuberlin.polymake.common.PolymakeViewer#getMenuBar()
	 */
	 @Override
	public Object getMenuBar() {
		JMenuBar jmb = new JMenuBar();
		for (JMenu menu : viewerMenu.getMenus()) {
			jmb.add(menu);
		}
		return jmb; 
	}

	/*
	 * @see de.tuberlin.polymake.common.PolymakeViewer#getViewerAppearancePanel()
	 */
	 @Override
	public JPanel getViewerAppearancePanel() {
			return new JPanel();
	}

	/**
	 * Add a listener to the viewer dealing with events caused by dragging
	 * points.
	 * 
	 * @param pdl the listener to be notified
	 */
	public void addPointDragListener(PointDragListener pdl) {
		dragTool.addPointDragListener(pdl);
	}

	private class HelpAction extends AbstractAction {

		private static final long serialVersionUID = 1L;

		public HelpAction() {
			putValue(NAME, "Help");
		}
		
		@Override
		public void actionPerformed(ActionEvent e) {
			PolymakeFrame.helpFrame.setVisible(true);
		}
		
	}

    public double[] getTransformationMatrix() {
        return jrviewer.getPlugin(de.jreality.plugin.basic.Scene.class).getContentPath().getLastComponent().getTransformation().getMatrix();
    }
}
