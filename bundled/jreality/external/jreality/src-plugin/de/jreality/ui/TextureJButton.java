package de.jreality.ui;

import static java.awt.RenderingHints.KEY_INTERPOLATION;
import static java.awt.RenderingHints.VALUE_INTERPOLATION_BILINEAR;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Image;
import java.io.IOException;

import javax.imageio.ImageIO;
import javax.swing.JToggleButton;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.util.Input;

public class TextureJButton extends JToggleButton {

	private static final long 
		serialVersionUID = 1L;
	private Image
		texImage = null;
	private double
		ratio = 1.0;
	
	
	public TextureJButton(String texResource) {
		if (texResource == null) {
			setText("");
			return;
		} else {
			try {
				Input texInput = Input.getInput(texResource);
				texImage = ImageIO.read(texInput.getInputStream());
				int w = texImage.getWidth(this);
				int h = texImage.getHeight(this);
				ratio = w / (double)h;
				texImage = ImageHook.scaleImage(texImage, 100, (int)(100 / ratio));
			} catch (IOException e) {}
		}
	}
	
	
	public TextureJButton(Image image) {
		this.texImage = image;
		int w = texImage.getWidth(this);
		int h = texImage.getHeight(this);
		ratio = w / (double)h;
		texImage = ImageHook.scaleImage(texImage, 100, (int)(100 / ratio));
	}
	
	
	@Override
	public void paint(Graphics g) {
		super.paint(g);
		Graphics2D g2d = (Graphics2D)g;
		g2d.setRenderingHint(KEY_INTERPOLATION, VALUE_INTERPOLATION_BILINEAR);
		if (texImage != null) {
			Dimension size = getSize();
			int w = size.width - 10;
			int h = (int)(w / ratio);
			if (h > size.height - 10) {
				h = size.height - 10;
				w = (int)(h * ratio);
			}
			int x = (size.width - w) / 2;
			int y = (size.height - h) / 2;
			g.drawImage(texImage, x, y, w, h, this);
		}
	}
	
	
}
