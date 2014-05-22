/*
 * Created on Jan 7, 2010
 *
 */
package de.jreality.tutorial.plugin;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.JRViewerUtility;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.Inspector;
import de.jreality.plugin.basic.SimpleAppearancePlugin;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StringArray;
import de.jreality.shader.CommonAttributes;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

public class SimpleAppearanceInspectorExample {

	public static void main(String[] args)	{
		SimpleAppearanceInspectorExample cte = new SimpleAppearanceInspectorExample();
		cte.doit();
	}
	
	Content content;

	private void doit() {
		SceneGraphComponent sgc = Primitives.wireframeSphere();
		final JRViewer v = new JRViewer();
		v.setContent(sgc);
		v.addBasicUI();
		v.addContentUI();
		v.registerPlugin(new ContentLoader());

		v.getPlugin(Inspector.class).setInitialPosition(
		            ShrinkPanelPlugin.SHRINKER_LEFT);
		v.addContentSupport(ContentType.CenteredAndScaled);
		
		SceneGraphComponent outside = new SceneGraphComponent();
		IndexedFaceSet cube = Primitives.cube();
		outside.setGeometry(cube);
		SceneGraphComponent inside = new SceneGraphComponent();
		MatrixBuilder.euclidean().scale(.9).getMatrix().assignTo(inside);
		Appearance app = new Appearance();
//		app.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		app.setAttribute(CommonAttributes.POINT_SHADER + "." + CommonAttributes.DIFFUSE_COLOR, Color.pink);
		app.setAttribute(CommonAttributes.POINT_SHADER + "." + CommonAttributes.POINT_RADIUS, .5);
		cube.setVertexAttributes(Attribute.LABELS, new StringArray(new String[]{"a", "b", "c", "d", "e", "f", "g", "h"}));
		app.setAttribute(CommonAttributes.LINE_SHADER + "." + CommonAttributes.DIFFUSE_COLOR, Color.green);
		app.setAttribute(CommonAttributes.LINE_SHADER + "." + CommonAttributes.TUBES_DRAW, true);
		app.setAttribute(CommonAttributes.LINE_SHADER + "." + CommonAttributes.TUBE_RADIUS, 0.1);
		cube.setEdgeAttributes(Attribute.LABELS, new StringArray(new String[]{"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"}));
		app.setAttribute(CommonAttributes.TUBES_DRAW, true);
		app.setAttribute(CommonAttributes.FACE_DRAW, true);
		app.setAttribute(CommonAttributes.POLYGON_SHADER + "." + CommonAttributes.SMOOTH_SHADING, true);
		cube.setFaceAttributes(Attribute.LABELS, new StringArray(new String[]{"A", "B", "C", "D", "E", "F"}));
		app.setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, true);
		app.setAttribute(CommonAttributes.Z_BUFFER_ENABLED, true);
		app.setAttribute(CommonAttributes.POLYGON_SHADER + "." + CommonAttributes.TRANSPARENCY, .7);
		SimpleAppearancePlugin sap = new SimpleAppearancePlugin(app);
		v.registerPlugin(sap);		
		outside.setAppearance(app);
		v.startup();
		SceneGraphComponent world = new SceneGraphComponent();
		world.addChild(inside);
		inside.setGeometry(Primitives.sharedIcosahedron);
		world.addChild(outside);
		content = JRViewerUtility.getContentPlugin(v.getController());
		content.setContent(world);
		
	}
	
}
