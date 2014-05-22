package de.jreality.util;

import java.awt.AWTException;
import java.awt.Component;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.Robot;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferByte;
import java.awt.image.WritableRaster;
import java.awt.image.renderable.ParameterBlock;
import java.beans.Statement;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.logging.Level;

import javax.imageio.ImageIO;

import de.jreality.scene.Viewer;
import de.jreality.shader.ImageData;

/**
 * Static methods for handling various image-related tasks.
 * @author gunn
 *
 */
public class ImageUtility {

	private ImageUtility() {}

	public static void writeBufferedImage(File file, BufferedImage img) {
		String suffix = getFileSuffix(file);
		if ("tiff".equals(suffix) || "tif".equals(suffix)) {
			try {
				  Class encParamClass = Class.forName("com.sun.media.jai.codec.TIFFEncodeParam");
				  
			      Object encodeParam = encParamClass.newInstance();
			      Object compField = encParamClass.getField("COMPRESSION_DEFLATE").get(null);
			      
			      new Statement(encodeParam, "setCompression", new Object[]{compField}).execute();
			      new Statement(encodeParam, "setDeflateLevel", new Object[]{9}).execute();
			      
			      ParameterBlock pb = new ParameterBlock();
			      pb.addSource(img);
			      pb.add(new FileOutputStream(file));
			      pb.add("tiff");
			      pb.add(encodeParam);
			      
				  new Statement(Class.forName("javax.media.jai.JAI"), "create", new Object[]{"encode", pb}).execute();
			} catch(Throwable e) {
				throw new RuntimeException("need JAI for tiff writing", e);
			}
		} else {
			try {
				if (suffix != "")
				    if (!ImageIO.write(img, getFileSuffix(file), file)) {
					    LoggingSystem.getLogger(ImageUtility.class).log(Level.WARNING,"Error writing file using ImageIO (unsupported file format?)");
				}
			} catch (IOException e) {
				throw new RuntimeException("image writing failed", e);
			}
		}
	}

	public static BufferedImage resizeToPowerOfTwo(BufferedImage input)	{
		BufferedImage output;
		int w = input.getWidth();
		int h = input.getHeight();
		int w2 = 1;
		while (w2 < w)	w2 *= 2;
		int h2 = 1;
		while (h2 < h) h2 *= 2;
		System.err.println("input type is "+input.getType());
		output = new BufferedImage(w2, h2, BufferedImage.TYPE_INT_ARGB);
		Graphics2D g = (Graphics2D) output.getGraphics();
		g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BICUBIC);
		output.getGraphics().drawImage(input, 0, 0, w2, h2, null);
		return output;
	}
	private static String getFileSuffix(File file) {
		int lastDot = file.getName().lastIndexOf('.');
		if (lastDot == -1) return "png";
		return file.getName().substring(lastDot+1);
	}

	/**
	 * I need this when i do offscreen rendering in the JOGL backend .. don't really understand why
	 * since it appears I'm just copying from one image to the other.  -gunn
	 * @param img
	 * @return
	 *
	 */
	public static BufferedImage rearrangeChannels(BufferedImage img)	{
		return rearrangeChannels(null, img);
	}
	public static BufferedImage rearrangeChannels(BufferedImage bi, BufferedImage img)	{
		 if (! (img.getRaster().getDataBuffer() instanceof DataBufferByte)) return img;
		int imageHeight = img.getHeight();
		int imageWidth = img.getWidth();
	    if (bi == null || bi.getWidth() != imageWidth || bi.getHeight() != bi.getHeight())
	    	bi = new BufferedImage(imageWidth, imageHeight, BufferedImage.TYPE_INT_ARGB);
		WritableRaster raster = bi.getRaster();
		byte[] byteArray = ((DataBufferByte) img.getRaster().getDataBuffer()).getData();
		int[] dst = new int[4];
	    for (int y = 0, ptr = 0; y < imageHeight; y++)
	          for (int x = 0; x < imageWidth; x++, ptr += 4) {
	            dst[3] =  byteArray[ptr+3];  //(byte) (px & 255); //
	            if (dst[3] < 0) dst[3] += 256;
	            double d = dst[3]/255.0;
	            for (int j = 0; j<3; ++j)	{
		            dst[j] = (int) (byteArray[ptr+j]); //(byte) ((px >> 8) & 255); //
	            	if (dst[j] < 0) dst[j] += 256;
	            }
	            raster.setPixel(x, y, dst);
	      }
	    return bi;
	}


	public static BufferedImage getValidBufferedImage(ImageData data)	{
		   byte[] byteArray = data.getByteArray();
		   int dataHeight = data.getHeight();
		   int dataWidth = data.getWidth();
		   BufferedImage img = new BufferedImage(dataWidth, dataHeight, BufferedImage.TYPE_INT_ARGB);
		   WritableRaster raster = img.getRaster();
		   int[] pix = new int[4];
	         for (int y = 0, ptr = 0; y < dataHeight; y++) {
	           for (int x = 0; x < dataWidth; x++, ptr += 4) {             
	             pix[0] = byteArray[ptr];
	             pix[1] = byteArray[ptr + 1];
	             pix[2] = byteArray[ptr + 2];
	             pix[3] = byteArray[ptr + 3]; 
	             raster.setPixel(x, y, pix);
	           }
	         }                      
		return img;
	}
	
	/**
	 * Captures a screen shot from the viewer's viewing component. This requires
	 * a viewer that has an AWT component as viewing component, and it requires
	 * a AWT Robot, which may not be available when running as a webstart or applet.
	 * 
	 * @param v the viewer to capture
	 * @return the screen capture of the viewer's viewing component, or null
	 */
	public static BufferedImage captureScreenshot(Viewer v) {
		if (v.getViewingComponent() instanceof Component) {
			Component viewingComponent = (Component) v.getViewingComponent();
			if (!viewingComponent.isShowing()) return null;
			Point loc = viewingComponent.getLocationOnScreen();
			Robot r;
			try {
				r = new Robot();
				return r.createScreenCapture(new Rectangle(loc, viewingComponent.getSize()));
			} catch (AWTException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		return null;
	}

}
