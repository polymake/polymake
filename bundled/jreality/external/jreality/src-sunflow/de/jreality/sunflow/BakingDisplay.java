package de.jreality.sunflow;

import java.awt.image.BufferedImage;

import org.sunflow.core.Display;
import org.sunflow.image.Color;
import org.sunflow.system.UI.Module;
import org.sunflow.system.UI.PrintLevel;
import org.sunflow.system.UserInterface;
import org.sunflow.system.ui.ConsoleInterface;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;

public class BakingDisplay implements Display, UserInterface {
	
    private BufferedImage image;
    private int[] pixels;
    private ConsoleInterface console;
    private Appearance app;
    private int min;
    private int max;
    private float invP;
    private int lastP;
    private int frameCount = 10;

    public BakingDisplay(Appearance app) {
		this.app = app;
		console = new ConsoleInterface();
	}

	public void print(Module m, PrintLevel level, String s) {
		console.print(m, level, s);
	}

	public void taskStart(String s, int min, int max) {
		this.min = min;
        this.max = max;
        //lastP = -1;
        invP = ((float)frameCount) / (max - min);
        console.taskStart(s, min, max);
	}

	public void taskStop() {
		console.taskStop();
	}

	public void taskUpdate(int current) {
		int p = (min == max) ? 0 : (int) ((current - min) * invP);
		if (image != null && p != lastP) {
			lastP = p;
			updateTexture();
		}
		console.taskUpdate(current);
	}

	private void updateTexture() {
		image.setRGB(0, 0, image.getWidth(), image.getHeight(), pixels, 0, image.getWidth());
		ImageData img = new ImageData(image);
		Texture2D tex = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "lightMap", app, false);
		tex.setApplyMode(Texture2D.GL_MODULATE);
	    tex.setImage(img);
		Matrix texMat = MatrixBuilder.euclidean().scale(1,-1,1).getMatrix();
		tex.setTextureMatrix(texMat);
	}
	
    public synchronized void imageBegin(int w, int h, int bucketSize) {
        if (image != null && w == image.getWidth() && h == image.getHeight()) {
            // nothing to do
        } else {
            // allocate new framebuffer
            pixels = new int[w * h];
            image = new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB);
        }
    }

    public void imagePrepare(int x, int y, int w, int h, int id) {
    }

    public void imageUpdate(int x, int y, int w, int h, Color[] data) {
        int iw = image.getWidth();
        int off = x + iw * y;
        iw -= w;
        for (int j = 0, index = 0; j < h; j++, off += iw)
            for (int i = 0; i < w; i++, index++, off++)
                pixels[off] = 0xFF000000 | data[index].toRGB();
    }

    public void imageFill(int x, int y, int w, int h, Color c) {
        int iw = image.getWidth();
        int off = x + iw * y;
        iw -= w;
        int rgb = 0xFF000000 | c.toRGB();
        for (int j = 0, index = 0; j < h; j++, off += iw)
            for (int i = 0; i < w; i++, index++, off++)
                pixels[off] = rgb;
    }

    public synchronized void imageEnd() {
    }

	public int getFrameCount() {
		return frameCount;
	}

	public void setFrameCount(int frameCount) {
		this.frameCount = frameCount;
	}
}
