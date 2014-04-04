package de.jreality.jogl;

import java.awt.Dimension;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferByte;
import java.io.File;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.logging.Level;

import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLDrawableFactory;
import javax.media.opengl.GLPbuffer;
import javax.media.opengl.GLProfile;

import com.jogamp.opengl.util.awt.ImageUtil;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.Texture2D;
import de.jreality.util.ImageUtility;

public class JOGLOffscreenRenderer {

	transient private GLPbuffer offscreenPBuffer;
	transient private Buffer offscreenBuffer;
	BufferedImage offscreenImage, bi;
	boolean preMultiplied = false; // not sure about this!
	boolean useColorBuffer = true;
	boolean asTexture = false;
	transient private int tileSizeX = 1024, tileSizeY = 768, numTiles = 4;
	private JOGLRenderer jr;
	int[] maxrbuffer = new int[4];

	public JOGLOffscreenRenderer(JOGLRenderer jr) {
		this.jr = jr;
	}

	public void renderOffscreen(int imageWidth, int imageHeight, File file,
			GLAutoDrawable canvas) {
		BufferedImage img = renderOffscreen(imageWidth, imageHeight, canvas);
		ImageUtility.writeBufferedImage(file, img);
	}

	public BufferedImage renderOffscreen(int imageWidth, int imageHeight,
			GLAutoDrawable canvas) {
		return renderOffscreen(imageWidth, imageHeight, 1.0, canvas);
	}

	public BufferedImage renderOffscreen(int imageWidth, int imageHeight,
			double aa, GLAutoDrawable canvas) {
		return renderOffscreen(null, imageWidth, imageHeight, aa, canvas);
	}

	HashMap<Long, GLPbuffer> pbuffers = new HashMap<Long, GLPbuffer>();
	Boolean useFBO = true;
	JOGLFBO joglFBO = null, joglFBOSlow;
	int[] fbo = { -1 }, rbuffer = { -1 }, cbuffer = { -1 }, txt = { -1 },
			normalFBO = { -1 }, normalCBuffer = { -1 };
	int imageWidth, imageHeight;
	static Matrix flipY = new Matrix();
	{
		MatrixBuilder.euclidean().scale(1, -1, 1).assignTo(flipY);
	}

	public JOGLFBO renderOffscreen(JOGLFBO fbo, Texture2D dst, int imageWidth,
			int imageHeight) {
		if (fbo == null)
			joglFBO = new JOGLFBO(imageWidth, imageHeight);
		else
			joglFBO = fbo;
		// by setting the texture we get the image copied into the right place
		// (in case asTexture = false)
		joglFBO.setTexture(dst);
		joglFBO.setAsTexture(asTexture);
		joglFBO.setSize(new Dimension(imageWidth, imageHeight));
		if (asTexture) {
			dst.setTextureMatrix(flipY);
			// dst.setMipmapMode(false);
		}
		jr.setTheFBO(joglFBO);
		jr.setFboMode(true);
		jr.theViewer.render();
		jr.setFboMode(false);
		return joglFBO;
	}

	public BufferedImage renderOffscreen(BufferedImage dst, int w, int h,
			double aa, GLAutoDrawable canvas) {
		return renderOffscreen(dst, w, h, aa, canvas, null);
	}

	public BufferedImage renderOffscreen(BufferedImage dst, int w, int h,
			double aa, GLAutoDrawable canvas, SceneGraphPath cp) {
		imageHeight = (int) (h / aa);
		imageWidth = (int) (w / aa);
		if (useFBO) {
			if (joglFBOSlow == null)
				joglFBOSlow = new JOGLFBO(imageWidth, imageHeight);
			else
				joglFBOSlow.setSize(new Dimension(imageWidth, imageHeight));
			jr.setTheFBO(joglFBOSlow);
			joglFBOSlow.setAsTexture(false);
			jr.setFboMode(true);
			jr.setAlternateCameraPath(cp);
			jr.theViewer.render();
			// canvas.display();
			dst = joglFBOSlow.getImage();
			jr.setAlternateCameraPath(null);
			jr.setFboMode(false);
		} else { // use pbuffers
			jr.offscreenMode = true;
			if (!GLDrawableFactory.getFactory(canvas.getGLProfile())
					.canCreateGLPbuffer(
							canvas.getNativeSurface()
									.getGraphicsConfiguration().getScreen()
									.getDevice(), canvas.getGLProfile())) {
				JOGLConfiguration.getLogger().log(Level.WARNING,
						"PBuffers not supported");
				return null;
			}

			double oldaa = jr.renderingState.globalAntiAliasingFactor;
			jr.renderingState.globalAntiAliasingFactor = aa;
			// System.err.println("setting global aa factor to "+aa);
			jr.lightsChanged = true;
			numTiles = Math.max(imageWidth / 1024, imageHeight / 1024);
			if (numTiles == 0)
				numTiles = 1;
			tileSizeX = (imageWidth / numTiles);
			tileSizeY = (imageHeight / numTiles);
			tileSizeX = 4 * (tileSizeX / 4);
			tileSizeY = 4 * (tileSizeY / 4);
			imageWidth = (tileSizeX) * numTiles;
			imageHeight = (tileSizeY) * numTiles;
			// System.err.println("Tile size x = "+tileSizeX);
			// System.err.println("Tile sizey = "+tileSizeY);
			// System.err.println("Image size = "+imageWidth+":"+imageHeight);
			long hashkey = 16384 * tileSizeY + tileSizeX;
			offscreenPBuffer = pbuffers.get(hashkey);
			if (offscreenPBuffer == null) {
				// if (offscreenPBuffer == null ||
				// lastWidth != tileSizeX ||
				// lastHeight != tileSizeY ) {
				System.err.println("Allocating new pbuffer");
				GLCapabilities caps = new GLCapabilities(GLProfile.get("GL2"));
				caps.setDoubleBuffered(false);
				caps.setAlphaBits(8);
				// if (offscreenPBuffer != null)
				// offscreenPBuffer.destroy();
				offscreenPBuffer = GLDrawableFactory.getFactory(
						GLProfile.get("GL2")).createGLPbuffer(
						canvas.getNativeSurface().getGraphicsConfiguration()
								.getScreen().getDevice(), caps, null,
						tileSizeX, tileSizeY, canvas.getContext());
				pbuffers.put(hashkey, offscreenPBuffer);
			} else {
				jr.renderingState.clearColorBuffer = true;
			}
			if (offscreenImage == null
					|| offscreenImage.getWidth() != imageHeight
					|| offscreenImage.getHeight() != imageHeight) {
				offscreenImage = new BufferedImage(imageWidth, imageHeight,
						BufferedImage.TYPE_4BYTE_ABGR); // TYPE_3BYTE_BGR); //
				offscreenBuffer = ByteBuffer
						.wrap(((DataBufferByte) offscreenImage.getRaster()
								.getDataBuffer()).getData());
			}
			jr.lightListDirty = true;
			// offscreenPBuffer.setGL(new DebugGL(offscreenPBuffer.getGL()));
			// System.err.println("Calling canvas.display()");
			canvas.display();
			jr.renderingState.globalAntiAliasingFactor = oldaa;
			dst = ImageUtility.rearrangeChannels(dst, offscreenImage);
			ImageUtil.flipImageVertically(dst);
			// a magic incantation to get the alpha channel to show up correctly
			dst.coerceData(true);
		}
		return dst;
	}

	public GLPbuffer getOffscreenPBuffer() {
		return offscreenPBuffer;
	}

	public Buffer getOffscreenBuffer() {
		return offscreenBuffer;
	}

	public int getNumTiles() {
		return numTiles;
	}

	public int getTileSizeX() {
		return tileSizeX;
	}

	public int getTileSizeY() {
		return tileSizeY;
	}

	public boolean isAsTexture() {
		return asTexture;
	}

	public void setAsTexture(boolean asTexture) {
		this.asTexture = asTexture;
	}

	// IMPORTANT:  This code did the PBuffer tiling approach in the old JOGLRenderer.  It's been replaced now by the FBO's but may
	// need to be revived since not all machines can  create big FBO's.
	// The code was grabbed from SVN revision 5453 of JOGLRenderer.java
//	if (offscreenMode) {
//		if (true)	{
//			offscreenRenderer.preRenderOffscreen(gl);
//			render();
//			offscreenRenderer.postRenderOffscreen(gl);
//		}
//		if (theCamera.isStereo() && renderingState.stereoType != de.jreality.jogl.Viewer.CROSS_EYED_STEREO) {
//			theLog.warning("Invalid stereo mode: Can only save cross-eyed stereo offscreen");
//			offscreenMode = false;
//			return;
//		}
//		GLContext context = offscreenRenderer.getOffscreenPBuffer().getContext();
//		if (context.makeCurrent() == GLContext.CONTEXT_NOT_CURRENT) {
//			JOGLConfiguration.getLogger().log(Level.WARNING,"Error making pbuffer's context current");
//			offscreenMode = false;
//			return;
//		}
//		globalGL = offscreenRenderer.getOffscreenPBuffer().getGL();
//		if (true || !JOGLConfiguration.sharedContexts)  forceNewDisplayLists();
//		renderingState.initializeGLState();
//		// we need another camera to avoid alerting the listeners when
//		// we change the camera while doing the offscreen render
//		CopyVisitor copier = new CopyVisitor();
//		copier.visit(theCamera);			
//		Camera offscreenCamera = theCamera = (Camera) copier.getCopy();
//		
//		Color[] bg=null;
//		float[][] bgColors=null;
//		int tileSizeX = offscreenRenderer.getTileSizeX(),
//		tileSizeY = offscreenRenderer.getTileSizeY(),
//		numTiles = offscreenRenderer.getNumTiles();
//		if (numTiles > 1) {
//			if (theRoot.getAppearance() != null && theRoot.getAppearance().getAttribute(BACKGROUND_COLORS, Color[].class) != Appearance.INHERITED) {
//				bg = (Color[]) theRoot.getAppearance().getAttribute(BACKGROUND_COLORS, Color[].class);
//				bgColors=new float[4][];
//				bgColors[0]=bg[0].getRGBComponents(null);
//				bgColors[1]=bg[1].getRGBComponents(null);
//				bgColors[2]=bg[2].getRGBComponents(null);
//				bgColors[3]=bg[3].getRGBComponents(null);
//			}
//		}
////		double[] c2ndc = CameraUtility.getCameraToNDC(offscreenCamera, 
////				CameraUtility.getAspectRatio(theViewer),
////				CameraUtility.MIDDLE_EYE);
////		System.err.println("c2ndc is "+Rn.matrixToString(c2ndc));
//		int numImages = offscreenCamera.isStereo() ? 2 : 1;
//		tileSizeX = tileSizeX / numImages;
//		myglViewport(0,0,tileSizeX, tileSizeY);
//		Rectangle2D vp = CameraUtility.getViewport(theCamera, getAspectRatio()); //CameraUtility.getAspectRatio(theViewer));
//		double dx = vp.getWidth()/numTiles;
//		double dy = vp.getHeight()/numTiles;
//		offscreenCamera.setOnAxis(false);
//		for (int st = 0; st < numImages; ++st)	{
//			if (offscreenCamera.isStereo()) {
//				if (st == 0) whichEye = CameraUtility.RIGHT_EYE;
//				else whichEye =  CameraUtility.LEFT_EYE;
//			}
//			else whichEye = CameraUtility.MIDDLE_EYE;
//			for (int i = 0; i<numTiles; ++i)	{
//				for (int j = 0; j<numTiles; ++j)	{
//					whichTile[0] = j; whichTile[1] = i;
//					renderingState.clearBufferBits = clearColorBits | GL.GL_DEPTH_BUFFER_BIT;
//					Rectangle2D lr = new Rectangle2D.Double(vp.getX()+j*dx, vp.getY()+i*dy, dx, dy);
////					System.err.println("Setting vp to "+lr.toString());
//					offscreenCamera.setViewPort(lr);
////					c2ndc = CameraUtility.getCameraToNDC(offscreenCamera, 
////							CameraUtility.getAspectRatio(theViewer),
////							CameraUtility.MIDDLE_EYE);
////					System.err.println(i+j+"c2ndc is "+Rn.matrixToString(c2ndc));
//					
//					if (bgColors != null) {
//						Color[] currentBg = new Color[4];
//						currentBg[1]=interpolateBG(bgColors, i+1, j, numTiles);
//						currentBg[2]=interpolateBG(bgColors, i, j, numTiles);
//						currentBg[3]=interpolateBG(bgColors, i, j+1, numTiles);
//						currentBg[0]=interpolateBG(bgColors, i+1, j+1, numTiles);
//						theRoot.getAppearance().setAttribute(BACKGROUND_COLORS, currentBg);
//					}
//					
//					render();
//					if (i == 0 && j == 0) render();	// ?? rerender the first t
//					globalGL.glPixelStorei(GL.GL_PACK_ROW_LENGTH,numImages*numTiles*tileSizeX);
//					globalGL.glPixelStorei(GL.GL_PACK_SKIP_ROWS, i*tileSizeY);
//					globalGL.glPixelStorei(GL.GL_PACK_SKIP_PIXELS, (st*numTiles+j)*tileSizeX);
//					globalGL.glPixelStorei(GL.GL_PACK_ALIGNMENT, 1);
//
//					globalGL.glReadPixels(0, 0, tileSizeX, tileSizeY,
//							GL.GL_RGBA, GL.GL_UNSIGNED_BYTE, offscreenRenderer.getOffscreenBuffer());
//	//						GL.GL_RGB, GL.GL_UNSIGNED_BYTE, offscreenBuffer);
//				}
//			}
//			
//		}
//
//		if (bgColors != null) theRoot.getAppearance().setAttribute(BACKGROUND_COLORS, bg);
//		
//		context.release();
		// restore the state of non-offscreen mode
//		theCamera = CameraUtility.getCamera(theViewer);
//private Color interpolateBG(float[][] bgColors, int i, int j, int numTiles) {
//	float[] col = new float[bgColors[0].length];
//	float alpha = ((float)j)/numTiles;
//	float beta = 1-((float)i)/numTiles;
//	//col = alpha*(1-beta)*bgColors[0]+(1-alpha)*(1-beta)*bgColors[1]+beta*(1-alpha)*bgColors[2]+alpha*beta*bgColors[3]
//	for (int k = 0; k < col.length; k++) {
//		col[k] = alpha*(1-beta)*bgColors[0][k]+(1-alpha)*(1-beta)*bgColors[1][k]+beta*(1-alpha)*bgColors[2][k]+alpha*beta*bgColors[3][k];
//	}
//	if (col.length == 3) return new Color(col[0], col[1], col[2]);
//	else return new Color(col[0], col[1], col[2], col[3]);
//}
//

}
