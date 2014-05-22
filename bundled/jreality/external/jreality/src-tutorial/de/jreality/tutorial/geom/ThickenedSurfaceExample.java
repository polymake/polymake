package de.jreality.tutorial.geom;

import java.awt.Color;

import de.jreality.geometry.SphereUtility;
import de.jreality.geometry.ThickenedSurfaceFactory;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.util.SceneGraphUtility;

public class ThickenedSurfaceExample {

	public static void main(String[] args)	{
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		Appearance ap = world.getAppearance();
		DefaultGeometryShader dgs = (DefaultGeometryShader) 
   		ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		DefaultPolygonShader dps = (DefaultPolygonShader) dgs.getPolygonShader();
		dps.setDiffuseColor(Color.white);
		dps.setSmoothShading(false);
		setupTexture(ap);

		IndexedFaceSet surface = SphereUtility.tessellatedIcosahedronSphere(1); 
		ThickenedSurfaceFactory tsf = new ThickenedSurfaceFactory(surface);		// constructor requires a surface
		tsf.setThickness(.05);				// distance between top and bottom
		tsf.setMakeHoles(true);				// boolean
		tsf.setHoleFactor(.5);				// values smaller than one make the holes bigger
		tsf.setStepsPerEdge(6);				// each original edge is replaced by 6 segments
//		tsf.setCurvedEdges(true);			// force boundary curves to be tanget at vertices
		tsf.setProfileCurve(new double[][]{{0,0}, {0,.4}, {.1,.5},{.9, .5},{1.0, .4}, {1,0}});
		tsf.update();
		IndexedFaceSet thickSurface = tsf.getThickenedSurface();
		world.setGeometry(thickSurface);
		JRViewer.display(world);
	}

	/**
	 * Construct a texture that is half one color, half the other
	 * @param texap
	 */
	private static void setupTexture(Appearance texap) {
		double[] c1 = {1,.8, 0,1}, c2 = {.4,1,0, 1};
		double blend = 0.0;
		byte[] im = new byte[128 * 128 * 4];
		int k1 = 50, k2 = 128-50;
		for (int i = 0; i<128; ++i)	{
			for (int j = 0; j< 128; ++j)	{
				int I = 4*(i*128+j);
				if (j <= k1 ) { blend = 1.0; }
				else if (j >= k2) { blend = 0.0; }
				else {
					blend = 1.0-(1.0*(j-k1))/(k2-k1);
				}
				double[] bc = Rn.linearCombination(null, blend, c1, 1.0-blend, c2);
				for (int k=0; k<4; ++k) im[I+k] = (byte) (255 * bc[k]);
			}
		}
		Texture2D tex2d = (Texture2D) AttributeEntityUtility
		.createAttributeEntity(Texture2D.class, "polygonShader.texture2d", texap, true);	
		ImageData it = new ImageData(im, 128, 128);
		tex2d.setImage(it);
		tex2d.setApplyMode(Texture2D.GL_MODULATE);
		tex2d.setRepeatS(Texture2D.GL_REPEAT);
		tex2d.setRepeatT(Texture2D.GL_REPEAT);
	}
}
