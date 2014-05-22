package de.jreality.tutorial.viewer;

import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridBagLayout;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.ViewShrinkPanelPlugin;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;
import de.jreality.tools.RotateTool;
import de.jreality.util.SceneGraphUtility;

/**
 * This class demonstrates how to put a jReality viewer into a sub-panel rather than creating a new Java Frame for it.
 * The key step is to use {@link JRViewer#startupLocal()} method instead of {@link JRViewer#startup()}.
 * @author gunn
 *
 */
public class AccessoryViewer {

	public static void main(String[] args) {
		JRViewer jrv = new JRViewer();
		jrv.addBasicUI();
		jrv.addContentSupport(ContentType.Raw);
		SceneGraphComponent sgc1 = SceneGraphUtility.createFullSceneGraphComponent("sgc1");
		sgc1.setGeometry(Primitives.icosahedron());
		sgc1.addTool(new RotateTool());
		jrv.setContent(sgc1);
		jrv.encompassEuclidean();
		jrv.startupLocal();
		final Component comp = (Component) jrv.getViewer()
				.getViewingComponent();
		comp.setPreferredSize(new Dimension(300, 300));
		comp.setMaximumSize(new java.awt.Dimension(32768, 32768));
		comp.setMinimumSize(new java.awt.Dimension(300, 300));
		final SceneGraphComponent sgc2 =  SceneGraphUtility.createFullSceneGraphComponent("sgc2");
		sgc2.setGeometry(Primitives.cube());
		jrv = new JRViewer();
		jrv.addBasicUI();
		jrv.addContentUI();
		jrv.addContentSupport(ContentType.Raw);
		jrv.setContent(sgc2);
		jrv.registerPlugin(new ViewShrinkPanelPlugin() {
			{
				shrinkPanel.setLayout(new GridBagLayout());
				shrinkPanel.add(comp);
			}
		});
		jrv.startup();

		// add a transformation listener to the first sgc which writes its
		// current transformation into the second sgc.
		sgc1.getTransformation().addTransformationListener(
				new TransformationListener() {

					public void transformationMatrixChanged(
							TransformationEvent ev) {
						sgc2.getTransformation().setMatrix(ev.getTransformationMatrix());

					}
				});

	}

}
