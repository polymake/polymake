package de.jreality.jogl3;

import java.awt.Color;

import de.jreality.math.Matrix;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;

/**
 * This class is essentially a cache for the Texture2D used in the constructor.
 * 
 * WARNING: the set methods are not supported.  Call update() to read out ALL values of the
 * associated Texture2D into the JOGLTexture2D.
 * @author Charles Gunn
 *
 */
public class JOGLTexture2D implements Texture2D {
	Texture2D proxy;
	protected int applyMode,
		combineModeAlpha,
		combineModeColor,
		source0Color,
		source0Alpha,
		source1Color,
		source1Alpha,
		source2Color,
		source2Alpha,
		operand0Color,
		operand0Alpha,
		operand1Color,
		operand1Alpha,
		operand2Color,
		operand2Alpha;
	protected Color blendColor;
	protected String externalSource;
	protected ImageData image;
	protected Integer magFilter;
	protected Integer minFilter;
	protected Matrix textureMatrix;
	protected Integer repeatS;
	protected Integer repeatT;
	protected Boolean animated, mipmapMode;
	protected Runnable runnable;
	protected Integer pixelFormat;
	protected Integer texID = -1;
	public JOGLTexture2D(Texture2D t)	{
		super();
		proxy = t;
		update();
	}
	
	public void update()	{
		applyMode = proxy.getApplyMode();
		blendColor = proxy.getBlendColor();
		combineModeColor = proxy.getCombineModeColor();
		combineModeAlpha = proxy.getCombineModeAlpha();
		operand0Color = proxy.getOperand0Color();
		operand1Color = proxy.getOperand1Color();
		operand2Color = proxy.getOperand2Color();
		operand0Alpha = proxy.getOperand0Alpha();
		operand1Alpha = proxy.getOperand1Alpha();
		operand2Alpha = proxy.getOperand2Alpha();
		source0Color = proxy.getSource0Color();
		source1Color = proxy.getSource1Color();
		source2Color = proxy.getSource2Color();
		source0Alpha = proxy.getSource0Alpha();
		source1Alpha = proxy.getSource1Alpha();
		source2Alpha = proxy.getSource2Alpha();
		image = proxy.getImage();
		textureMatrix = proxy.getTextureMatrix();
		minFilter = proxy.getMinFilter();
		magFilter = proxy.getMagFilter();
		repeatS = proxy.getRepeatS();
		repeatT = proxy.getRepeatT();
		externalSource = proxy.getExternalSource();
		animated = proxy.getAnimated();
		mipmapMode = proxy.getMipmapMode();
		runnable = proxy.getRunnable();
		pixelFormat = proxy.getPixelFormat();
		if (source0Alpha == 23) {
			if (source1Alpha != Texture2D.SOURCE1_ALPHA_DEFAULT)
				texID = source1Alpha;
			System.err.println("Got texid = "+texID);
		}
	}
	
	public Integer getApplyMode() {
		return applyMode;
	}

	public Color getBlendColor() {
		return blendColor;
	}

	public Integer getCombineMode() {
		return combineModeColor;
	}

	public Integer getCombineModeAlpha() {
		return combineModeAlpha;
	}

	public Integer getCombineModeColor() {
		return combineModeColor;
	}

	public String getExternalSource() {
		return externalSource;
	}

	public ImageData getImage() {
		return image;
	}

	public Integer getMagFilter() {
		return magFilter;
	}

	public Integer getMinFilter() {
		return minFilter;
	}

	public Integer getOperand0Alpha() {
		return operand0Alpha;
	}

	public Integer getOperand0Color() {
		return operand0Color;
	}

	public Integer getOperand1Alpha() {
		return operand1Alpha;
	}

	public Integer getOperand1Color() {
		return operand1Color;
	}

	public Integer getOperand2Alpha() {
		return operand2Alpha;
	}

	public Integer getOperand2Color() {
		return operand2Color;
	}

	public Integer getRepeatS() {
		return repeatS;
	}

	public Integer getRepeatT() {
		return repeatT;
	}

	public Integer getSource0Alpha() {
		return source0Alpha;
	}

	public Integer getSource0Color() {
		return source0Color;
	}

	public Integer getSource1Alpha() {
		return source1Alpha;
	}

	public Integer getSource1Color() {
		return source1Color;
	}

	public Integer getSource2Alpha() {
		return source2Alpha;
	}

	public Integer getSource2Color() {
		return source2Color;
	}

	public Matrix getTextureMatrix() {
		return textureMatrix;
	}
	
	public Boolean getAnimated() {
		return animated;
	}

	public void setApplyMode(Integer applyMode) {
		this.applyMode = applyMode;
	}

	public void setBlendColor(Color blendColor) {
		this.blendColor = blendColor;
	}

	public void setCombineMode(Integer combineMode) {
	}
	// TODO: finish implementing the set() methods
	public void setCombineModeAlpha(Integer i) {
	}

	public void setCombineModeColor(Integer i) {
	}

	public void setExternalSource(String b) {
	}

	public void setImage(ImageData image) {
		this.image = image;
	}

	public void setMagFilter(Integer i) {
	}

	public void setMinFilter(Integer i) {
	}

	public void setOperand0Alpha(Integer i) {
	}

	public void setOperand0Color(Integer i) {
	}

	public void setOperand1Alpha(Integer i) {
	}

	public void setOperand1Color(Integer i) {
	}

	public void setOperand2Alpha(Integer i) {
	}

	public void setOperand2Color(Integer i) {
	}

	public void setRepeatS(Integer repeatS) {
	}

	public void setRepeatT(Integer repeatT) {
	}

	public void setSource0Alpha(Integer i) {
	}

	public void setSource0Color(Integer i) {
	}

	public void setSource1Alpha(Integer i) {
	}

	public void setSource1Color(Integer i) {
	}

	public void setSource2Alpha(Integer i) {
	}

	public void setSource2Color(Integer i) {
	}

	public void setTextureMatrix(Matrix matrix) {
	}

	public void setAnimated(Boolean b) {
	}

	public Boolean getMipmapMode() {
		return mipmapMode;
	}

	public void setMipmapMode(Boolean b) {
		mipmapMode = b;
	}

	public Runnable getRunnable() {
		return runnable;
	}

	public void setRunnable(Runnable r) {
		runnable = r;
	}

	public Integer getPixelFormat() {
		return pixelFormat;
	}

	public void setPixelFormat(Integer i) {
		pixelFormat = i;
	}

	public Integer getTexID() {
		return texID;
	}

	public void setTexID(Integer texID) {
		this.texID = texID;
	}

}
