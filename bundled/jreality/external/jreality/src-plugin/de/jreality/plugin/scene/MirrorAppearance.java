package de.jreality.plugin.scene;

import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.TextureUtility.createTexture;
import static java.awt.image.BufferedImage.TYPE_INT_RGB;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.image.BufferedImage;

import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;

public class MirrorAppearance extends Appearance implements Runnable {

	public int 
		resolution = 128;
	public BufferedImage
		image = new BufferedImage(resolution, resolution, TYPE_INT_RGB);
	public ImageData
		imageData = new ImageData(image);
	public Texture2D
		mirrorTex = createTexture(this, POLYGON_SHADER, imageData);
	
	public MirrorAppearance() {
		super("Mirror Appearance");
		setAttribute(CommonAttributes.FACE_DRAW, true);
		setAttribute(CommonAttributes.EDGE_DRAW, false);
		setAttribute(CommonAttributes.VERTEX_DRAW, false);
	    mirrorTex.setRepeatS(Texture2D.GL_CLAMP_TO_EDGE);
	    mirrorTex.setRepeatT(Texture2D.GL_CLAMP_TO_EDGE);
	    mirrorTex.setMagFilter(Texture2D.GL_NEAREST);
	    mirrorTex.setMinFilter(Texture2D.GL_NEAREST);
	    mirrorTex.setAnimated(true);
	    mirrorTex.setMipmapMode(false);
	    
	    image = (BufferedImage)imageData.getImage();
	    run();
	    mirrorTex.setRunnable(this);
	}
	
	@Override
	public void run() {
		Graphics2D g = (Graphics2D)image.createGraphics();
		g.setBackground(Color.WHITE);
		g.clearRect(0, 0, resolution, resolution);
		g.setColor(Color.RED);
		g.setStroke(new BasicStroke(2.0f));
		g.drawLine(0,0, resolution, resolution);
	}
	
}
