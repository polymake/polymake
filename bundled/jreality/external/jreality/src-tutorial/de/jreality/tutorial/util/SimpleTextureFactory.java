package de.jreality.tutorial.util;

import java.awt.Color;

import de.jreality.shader.ImageData;

public class SimpleTextureFactory {
	public enum TextureType {
		WEAVE,
		GRAPH_PAPER,
		DISK,
		ANTI_DISK, GRADIENT
	};
	TextureType type = TextureType.WEAVE;
	ImageData id;
	int size = 64;
	
	public SimpleTextureFactory() {
		super();
		updatebcolors();
	}
	
	public void setType(TextureType foo)	{
		type = foo;
	}
	
	public ImageData getImageData()	{
		return id;
	}
	
	int[] channels = {0,1,2,3};
	Color[] colors = {Color.white, Color.LIGHT_GRAY, Color.yellow, new Color(0,0,0,0)};
	byte[][] bcolors = new byte[4][4];

	public void setColors(Color[] colors) {
		this.colors = colors;
	}

	public void setColor(int i, Color c)	{
		colors[i] = c;
		updatebcolors();
	}
	
	private void updatebcolors() {
		float[] cc = new float[4];
		for (int i = 0; i<colors.length; ++i)	{
			cc = colors[i].getRGBComponents(cc);
			for (int j = 0; j<4; ++j)	bcolors[i][channels[j]] = (byte) (cc[j] * 255.0);
		}
	}
	public void update()	{
		byte[] im = new byte[size*size* 4];
		switch(type)	{
		case ANTI_DISK:
		    for (int i = 0; i<size; ++i)	{
		        for (int j = 0; j< size; ++j)	{
					int I = 4*(i*size+j);
					int sq = (i-(size/2))*(i-(size/2)) + (j-(size/2))*(j-(size/2));
					sq = i*i + j*j;
					if (sq <= size*size)	
						{im[I] =  im[I+1] = im[I+2] = im[I+3] = (byte) 255; }
					else
						{im[I] =  im[I+1] = im[I+2] = im[I+3]  = 0;  }
			    }
			}
			break;
		case DISK:
		    for (int i = 0; i<size; ++i)	{
		        for (int j = 0; j< size; ++j)	{
					int I = 4*(i*size+j);
					int sq = (i-(size/2))*(i-(size/2)) + (j-(size/2))*(j-(size/2));
					sq = i*i + j*j;
					if (sq > size*size)	
						{im[I] =  im[I+1] = im[I+2] = im[I+3] = (byte) 255; }
					else
						{im[I] =  im[I+1] = im[I+2] = im[I+3]  = 0;  }
			    }
			}
			break;
		case WEAVE:
			int margin = size/16;
			int bandwidth = 16;
			int gapwidth =16;
			int shwd = 2;
			int onewidth = 32;
			int iband, jband, imod, jmod;
			int which = 0;
		    for (int i = 0; i<size; ++i)	{
		        iband = i/onewidth;
		        imod = i%onewidth;
		        for (int j = 0; j< size; ++j)	{
				int where = 4*(i*size+j);
					jband = j /onewidth;
					jmod = j%onewidth;
					int q = 2*(iband)+jband;
					if (imod > bandwidth && jmod > bandwidth) which = 0;
					else {
					    if (imod <= bandwidth && jmod <= bandwidth)	{
					        if (q == 0 || q == 3) which = 1;
					        else which = 2;
					    } else if (jmod > bandwidth) {
					        which = 1;
					        if ((q == 0 || q == 3)&& jmod > (onewidth - shwd)) which = 3;
					        if ((q == 1 || q == 2) && jmod < (bandwidth + shwd)) which = 3;
					    } else if (imod > bandwidth) {
				 	        which = 2;
					        if ((q == 1 || q == 2)&& imod > (onewidth - shwd)) which = 3;
					        if ((q == 0 || q ==3) && imod < (bandwidth + shwd)) which = 3;
					    }
					}
					System.arraycopy(bcolors[which],0,im,where,4);
				}
		    }
			break;
		case GRAPH_PAPER:
			int bands = 4;
			int[] widths = {4,2,2,2};

			onewidth = size/bands;
		    for (int i = 0; i<size; ++i)	{
		        iband = i/onewidth;
		        imod = i%onewidth;
		        for (int j = 0; j< size; ++j)	{
				int where = 4*(i*size+j);
				jband = j /onewidth;
				jmod = j%onewidth;
				which = 0;
				if (jmod <= widths[jband]) {
				    if (jband == 0) which = 2;
				    else which = 1;
				} 
				if (imod <=widths[iband]) {
				    if (iband == 0) which = 2;
				    else which = 1; }
					System.arraycopy(bcolors[which],0,im,where,4);
				}
		    }			
		    break;
		
		case GRADIENT:
			double blend = 0.0;
			int k1 = 0, k2 = size-k1;
			float[] bcf = new float[4];
			for (int i = 0; i<size; ++i)	{
				for (int j = 0; j< size; ++j)	{
					int I = 4*(j*size+i);
					if (j <= k1 ) { blend = 1.0; }
					else if (j >= k2) { blend = 0.0; }
					else {
						blend = 1.0-(1.0*(j-k1))/(k2-k1);
					}
					Color bc = linearInterpolation(colors[0], colors[1], blend);
					bc.getRGBComponents(bcf);
					for (int k=0; k<4; ++k) im[I+k] = (byte) (255 * bcf[k]);
				}
			}
			break;
		}
		id = new de.jreality.shader.ImageData(im, size, size);
	}
	
	private static Color linearInterpolation(Color c1, Color c2, double d) {
		float[] fc1 = c1.getRGBComponents(null);
		float[] fc2 = c2.getRGBComponents(null);
		float fd = (float) d;
		return new Color((1-fd)*fc1[0]+fd*fc2[0],(1-fd)*fc1[1]+fd*fc2[1],(1-fd)*fc1[2]+fd*fc2[2], (1-fd)*fc1[3]+fd*fc2[3]);
	}

}
