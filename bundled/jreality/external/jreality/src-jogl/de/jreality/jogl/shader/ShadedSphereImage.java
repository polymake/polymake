/*
 * Created on Aug 29, 2008
 *
 */
package de.jreality.jogl.shader;

import java.awt.Color;

import de.jreality.math.Rn;
import de.jreality.shader.ImageData;

public class ShadedSphereImage {
	static byte[] sphereTex;
	static double[][] sphereVertices;
	static final double[] defaultLightDirection = { 0, 0, 1 };
	static final Color defaultDiffuseColor = Color.red;
	static final Color defaultSpecularColor = Color.white;
	static int[] defaultChannels = { 0, 1, 2, 3 };

	public static ImageData shadedSphereImage(double[] lightDirection,
			Color diffuseColor, Color specularColor, double specularExponent,
			int textureSize, boolean lighting, int[] ch) {
		int I = 0, II = 0;
		if (lightDirection == null)
			lightDirection = defaultLightDirection;
		else
			lightDirection = Rn.normalize(null, lightDirection);
		if (diffuseColor == null)
			diffuseColor = defaultDiffuseColor;
		if (specularColor == null)
			specularColor = defaultSpecularColor;
		if (ch == null)
			ch = defaultChannels;
		double[] reflected = new double[3];
		float[] diffuseColorAsFloat = diffuseColor.getRGBComponents(null);
		float[] specularColorAsFloat = specularColor.getRGBComponents(null);
		// if (sphereTex != null) return;
		// TODO check here to see if it's possible to avoid recomputing by
		// comparing to old values
		// for diffuse color, specular color, and exponent.
		if (sphereTex == null
				|| sphereTex.length != 4 * textureSize * textureSize)
			sphereTex = new byte[textureSize * textureSize * 4];
		if (sphereVertices == null
				|| sphereVertices.length != textureSize * textureSize)
			calculateSphereVertices(textureSize);
		double alpha = diffuseColorAsFloat[3];
		for (int i = 0; i < textureSize; ++i) {
			for (int j = 0; j < textureSize; ++j) {
				if (sphereVertices[I][0] != -1) {
					double diffuse = Rn.innerProduct(lightDirection,
							sphereVertices[I]);
					if (diffuse < 0)
						diffuse = 0;
					if (diffuse > 1.0)
						diffuse = 1.0;
					double z = sphereVertices[I][2];
					reflected[0] = 2 * sphereVertices[I][0] * z;
					reflected[1] = 2 * sphereVertices[I][1] * z;
					reflected[2] = 2 * z * z - 1;
					double specular = Rn
							.innerProduct(lightDirection, reflected);
					if (specular < 0.0)
						specular = 0.0;
					if (specular > 1.0)
						specular = 1.0;
					specular = Math.pow(specular, specularExponent);
					for (int k = 0; k < 3; ++k) {
						double f = diffuseColorAsFloat[k];
						if (lighting)
							f = (diffuse * f + specular
									* specularColorAsFloat[k]);
						if (f < 0)
							f = 0;
						if (f > 1)
							f = 1;
						sphereTex[II + ch[k]] = (byte) (255 * f);
					}
					sphereTex[II + ch[3]] = (byte) (sphereVertices[I][2] < .1 ? (byte) (alpha * 2550 * sphereVertices[I][2])
							: (alpha * 255));
				} else {
					sphereTex[II] = sphereTex[II + 1] = sphereTex[II + 2] = sphereTex[II + 3] = 0;
				}
				II += 4;
				I++;
			}
		}
		ImageData id = new ImageData(sphereTex, textureSize, textureSize);
		return id;
	}

	static {
		calculateSphereVertices(128);
	}

	private static void calculateSphereVertices(int ts) {
		if (sphereVertices != null && (ts * ts == sphereVertices.length))
			return;
		double x, y, z;
		int I = 0;
		sphereVertices = new double[ts * ts][3];

		for (int i = 0; i < ts; ++i) {
			y = 2 * (i + .5) / ts - 1.0;
			for (int j = 0; j < ts; ++j) {
				x = 2 * (j + .5) / ts - 1.0;
				double dsq = x * x + y * y;
				if (dsq <= 1.0) {
					z = Math.sqrt(1.0 - dsq);
					sphereVertices[I][0] = x;
					sphereVertices[I][1] = y;
					sphereVertices[I][2] = z;
				} else
					sphereVertices[I][0] = sphereVertices[I][1] = sphereVertices[I][2] = -1;
				I++;
			}
		}
	}

}
