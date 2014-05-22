package light;

import de.jreality.geometry.PointSetFactory;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;

public class ScalingTest {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		SceneGraphComponent sgc = new SceneGraphComponent("test");
		
		
		PointSetFactory psf = new PointSetFactory();
		psf.setVertexCount(1);
		psf.setVertexCoordinates(new double[]{0, 0, 0, 1});
		psf.setVertexColors(new double[]{1, 1, 0, 1});
		psf.update();
		sgc.setGeometry(psf.getGeometry());
		
		Appearance a = new Appearance();
		a.setAttribute(CommonAttributes.RADII_WORLD_COORDINATES, true);
		sgc.setAppearance(a);
		
		//MatrixBuilder.euclidean().scale(2, 2, 2).assignTo(sgc);
		
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addContentUI();
		v.addVRSupport();
		v.setContent(sgc);
		v.startup();
	}

}
