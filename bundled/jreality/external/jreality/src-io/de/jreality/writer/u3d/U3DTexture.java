package de.jreality.writer.u3d;

import java.util.Arrays;

import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.writer.u3d.texture.SphereMapGenerator;

public class U3DTexture {

	private ImageData
		image[] = null;
	private CubeMap 
		cm = null;
	private ImageData 
		imageData = null;

	
	public U3DTexture(Texture2D tex) {
		this.image = new ImageData[]{tex.getImage()};
	}
	
	public U3DTexture(ImageData image) {
		this.image = new ImageData[]{image};
	}
	
	public U3DTexture(CubeMap cm) {
		image = new ImageData[]{
				cm.getLeft(), cm.getRight(),
				cm.getFront(), cm.getBack(),
				cm.getTop(), cm.getBottom()
			};
		this.cm = cm;
	}
	
	public ImageData getImage() {
		if (imageData == null)
			imageData = image.length == 1 ? image[0] : new ImageData(SphereMapGenerator.create(cm, 768, 768));
		return imageData;
	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + Arrays.hashCode(image);
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		final U3DTexture other = (U3DTexture) obj;
		if (!Arrays.equals(image, other.image))
			return false;
		return true;
	}
	
}
