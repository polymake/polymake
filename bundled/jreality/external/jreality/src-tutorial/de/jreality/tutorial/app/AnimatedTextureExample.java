package de.jreality.tutorial.app;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;

import javax.swing.Timer;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.tutorial.util.GameOfLife;

/**
 * This example shows an animated 2d texture which maps a <i>live</i> game of life onto a quadrilateral.
 * @author Charles Gunn
 *
 */
public class AnimatedTextureExample {
	static int count = 0;
	public static void main(String[] args)		{
		SceneGraphComponent worldSGC = new SceneGraphComponent("AnimatedTextureExample");
		worldSGC.setGeometry(Primitives.texturedQuadrilateral());
		Appearance ap = new Appearance();
		worldSGC.setAppearance(ap);
		DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		DefaultPolygonShader dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.white);
		
		final Texture2D tex2d = (Texture2D) AttributeEntityUtility
	       .createAttributeEntity(Texture2D.class, "polygonShader.texture2d", ap, true);
		final int width = 128, height = 128;
		BufferedImage bi = new BufferedImage(width, height, BufferedImage.TYPE_4BYTE_ABGR);
		final ImageData id = new ImageData(bi);
		tex2d.setImage(id);
		// shouldn't have to do this; but if I comment it out, updating the texture doesn't work
		bi = (BufferedImage) id.getImage();
		tex2d.setRepeatS(Texture2D.GL_CLAMP_TO_EDGE);
		tex2d.setRepeatT(Texture2D.GL_CLAMP_TO_EDGE);
		tex2d.setMagFilter(Texture2D.GL_NEAREST);
		tex2d.setMinFilter(Texture2D.GL_NEAREST);
 		tex2d.setAnimated(true);
 		tex2d.setMipmapMode(false);
 		final Graphics2D g = bi.createGraphics();
		final GameOfLife gol = new GameOfLife(bi);

		tex2d.setRunnable(new Runnable() {
 		
			public void run() {
				gol.update();
				Image current = gol.currentValue();
				g.drawImage(current, 0, 0, null);
			}
 			
 		});
		
// 		final ViewerApp va = ViewerApp.display(worldSGC);
//		CameraUtility.encompass(va.getCurrentViewer());
		final Viewer v = JRViewer.display(worldSGC);
		// force regular rendering; each render will invoke the above runnable
		// to update the texture
 		Timer timer = new Timer(20, new ActionListener()	{

			public void actionPerformed(ActionEvent e) {
				v.renderAsync();		
			}
 			
 		});
		timer.start();

	}
}
