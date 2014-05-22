package de.jreality.tutorial.app;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;

/**
 * This example shows how to use vertex colors on a point set.
 * <p>
 * Not that in contrast to the case of the default line shader, the attribute
 * {@link CommonAttributes#VERTEX_COLORS_ENABLED} plays no role in the point shader.
 * If vertex colors are present in the PointSet instance they are used when rendering.
 * If you don't want vertex colors, you should remove the attribute.
 * 
 * @author gunn  (from code donated by forum member STRESS).
 *
 */
public class VertexColorsExample {

	public static void main(String[] args)	{
		SceneGraphComponent sgc = new SceneGraphComponent();
		IndexedFaceSet icosahedron = Primitives.icosahedron();
		sgc.setGeometry(icosahedron);
		// set vertex colors based on coordinates xyz->rgb: 
		// have to make make values positive
		double[][] vc = icosahedron.getVertexAttributes(Attribute.COORDINATES).
			toDoubleArrayArray(null);
		for (double[] c : vc) for (int i = 0; i<3; ++i) c[i] = .5 + .5*c[i];
		icosahedron.setVertexAttributes(Attribute.COLORS, 
				StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(vc));
		sgc.setAppearance(new Appearance());
		Appearance ap = sgc.getAppearance();
		ap.setAttribute(CommonAttributes.LIGHTING_ENABLED,false);
		ap.setAttribute(CommonAttributes.ATTENUATE_POINT_SIZE, false);
		ap.setAttribute(CommonAttributes.EDGE_DRAW,false);
		ap.setAttribute(CommonAttributes.FACE_DRAW,false);
		ap.setAttribute(CommonAttributes.VERTEX_DRAW,true);
		ap.setAttribute(CommonAttributes.POINT_SHADER+"."+
				CommonAttributes.SPHERES_DRAW,false);
		ap.setAttribute(CommonAttributes.POINT_SHADER+"."+
				CommonAttributes.POINT_SIZE,10.0);
		JRViewer.display(sgc);
	}
}
