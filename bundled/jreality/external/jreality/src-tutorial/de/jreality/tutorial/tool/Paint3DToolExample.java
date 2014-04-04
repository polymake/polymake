package de.jreality.tutorial.tool;

import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.io.IOException;

import de.jreality.examples.CatenoidHelicoid;
import de.jreality.jogl.shader.ShadedSphereImage;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.Tool;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.util.SceneGraphUtility;

/**
 * This tutorial demonstrates how  to write a tool to paint on a 3D surface.  It uses the texture coordinates of the
 * surface to paint into a texture map associated to the surface.  
 * <p>
 * It also shows how a tool can use different activation slots
 * to lead to different behavior.  In this case, painting by dragging with the left button down paints with a brush which is a shaded blue
 * sphere on the surface. The same with the shift-key depressed erases what has been drawn, by drawing with a brush that is 
 * identical to the original texture color.
 * 
 * @author gunn
 *
 */
public class Paint3DToolExample {

	static Viewer viewer;
	static Graphics2D g2D;
	public static void main(String[] args) throws IOException {
	  	IndexedFaceSet geom = new CatenoidHelicoid(40);
		SceneGraphComponent sgc = SceneGraphUtility.createFullSceneGraphComponent("TextureExample");
		sgc.setGeometry(geom);
		Appearance ap = sgc.getAppearance();
		DefaultGeometryShader dgs = (DefaultGeometryShader) ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		DefaultPolygonShader dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.white);
		// set up the texture image and Graphics2D objects for the tool
		final int imageSize = 512;
		BufferedImage bi = new BufferedImage(imageSize, imageSize, BufferedImage.TYPE_INT_ARGB);
		ImageData id = new ImageData(bi);
		// this is not pretty: the original bi was trashed to get a different byte order.
		bi = (BufferedImage) id.getImage();
		g2D =  bi.createGraphics();
		// make a grey background to start with
		final Color initialColor = new Color(.5f,.5f,.5f,1f);
		g2D.setColor(initialColor);
		g2D.fillRect(0, 0, imageSize, imageSize);
		
		// set up the texture object
		final Texture2D tex2d = TextureUtility.createTexture(sgc.getAppearance(), POLYGON_SHADER,id);
		tex2d.setRepeatS(Texture2D.GL_CLAMP_TO_EDGE);
		tex2d.setRepeatT(Texture2D.GL_CLAMP_TO_EDGE);
 		tex2d.setAnimated(true);
 		// not a good idea to set false: no picture shows up!
 		tex2d.setMipmapMode(true);
 		tex2d.setApplyMode(Texture2D.GL_MODULATE);
// 		tex2d.setPixelFormat(Texture2D.GL_BGRA);
  		final int brushSize = 16;
  		// make a transparent brush using utility method in jogl backend
 		Color brushColor = new Color(0,0,255,70);
 		ImageData bid = ShadedSphereImage.shadedSphereImage(
 				Rn.setToLength(null, new double[]{0,1,1}, 1.0),
 				brushColor, 
 				Color.white, 
 				10.0, 
 				brushSize, 
 				true,
 				new int[]{1,0,3,2});
 		final BufferedImage brush1 = (BufferedImage) bid.getImage();
		final BufferedImage brush2 = new BufferedImage(brushSize, brushSize, BufferedImage.TYPE_INT_ARGB);
		Graphics2D tmpG = (Graphics2D) brush2.getGraphics();
		tmpG.setColor(initialColor);
		tmpG.fillRect(0, 0, brushSize, brushSize);

	    Tool paintTool = new AbstractTool(
	    		InputSlot.LEFT_BUTTON,
	    		InputSlot.SHIFT_LEFT_BUTTON) {	// left button and shift left button

	    	BufferedImage brush;
			public void activate(ToolContext tc) {
				// express interest in mouse moves (so perform() gets called)
		   		addCurrentSlot(InputSlot.getDevice("PointerTransformation"));
		   		if (tc.getSource()== InputSlot.LEFT_BUTTON) {
		   			System.err.println("left mouse activate");
		   			brush = brush1;
		   		} else brush = brush2;
			}

			public void perform(ToolContext tc) {
				PickResult currentPick = tc.getCurrentPick();
				if (currentPick == null) return;
				double[] texCoords = currentPick.getTextureCoordinates();
				// that the following can happen is ... odd
				if (texCoords == null  || texCoords.length < 2) return;
				int ix = (int) (texCoords[0] * imageSize);
				int iy = (int) (texCoords[1] * imageSize);
				g2D.drawImage(brush, ix - brushSize/2, iy - brushSize/2, null);
				// we have to explicitly trigger render since the scene graph isn't changed by the painting
				viewer.renderAsync();
			}

			public String getDescription(InputSlot slot) {
				return null;
			}

			public String getDescription() {
				return "A tool which paints on a 3D surface";
			}

			@Override
			public void deactivate(ToolContext tc) {
		   		removeCurrentSlot(InputSlot.getDevice("PointerTransformation"));
			}
	    	
	    };
		sgc.addTool(paintTool);		

		viewer = JRViewer.display(sgc);

	  }
}
