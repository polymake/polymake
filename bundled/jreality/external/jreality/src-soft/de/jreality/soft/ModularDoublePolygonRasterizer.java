/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.soft;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 * @deprecated
 * TODO:Needs update make similar to Integer version again...
 */
public abstract class ModularDoublePolygonRasterizer implements PolygonRasterizer {
	
	//dimensions of the image to render into:
	private int xmin = 0;
	private int xmax = 0;
	private int ymin = 0;
	private int ymax = 0;

    // store half the size of the screen:
    //mh is the min of wh and hh; 
    private double wh;
    private double hh;
    private double mh;
	
	private int pLength = 0;	
	private double[][] polygon = new double[Polygon.MAXPOLYVERTEX][Polygon.VERTEX_LENGTH];
	protected double transparency = 0;
	protected double oneMinusTransparency = 255;

    private Texture texture = null;
    
    private boolean interpolateColor  = true;
    private boolean interpolateTexture= false;
    private static final boolean correctInterpolation =false;
	/**
	 * 
	 */
	public ModularDoublePolygonRasterizer() {
		super();
	}
	
	/**
	 * This method should implement the actual setting of a pixel if could look like the following:<br>
	 *     <code>
	 * 	  int pos = x + w * y;
	 *	  if (apszI >= zBuffer[pos])
	 *	  return;
	 * 	  pixels[pos] =
	 *		  (255 << 24) | (int) (aprI>>FIXP) << 16 | (int) (apgI>>FIXP) << 8 | (int) (apbI>>FIXP);
	 *	  zBuffer[pos] = apszI;
	 *	  gBuffer[pos] = currentNode;
	 *	</code>
	 * @param x x-coordinate to set
	 * @param y y-coordinate to set
	 * 
	 */
    protected abstract void setPixel(final int x, final int y, final double z, double red, double green, double blue, double transparency);
    


	/* (non-Javadoc)
	 * @see de.jreality.soft.Renderer#renderPolygon(de.jreality.soft.Polygon, double[],int,int,int,int)
	 */
	public final void renderPolygon(final Polygon p, final double[] vertexData, final boolean outline) {

		transparency = (p.getShader().getVertexShader().getTransparency());
		oneMinusTransparency = 1 - transparency;

		pLength = p.length;
		for (int i = 0; i < pLength; i++) {
			int pos = p.vertices[i];
			double[] pi= polygon[i];
            
            double w = 1/vertexData[pos+Polygon.SW];
            double wxy =w*mh;
            pi[Polygon.SX] =(wh + vertexData[pos+Polygon.SX] * wxy);
            pi[Polygon.SY] =(hh - vertexData[pos+Polygon.SY] * wxy);
            pi[Polygon.SZ] =(vertexData[pos+Polygon.SZ] * w);
            
            if(p.getShader() instanceof SkyboxPolygonShader) pi[Polygon.SZ] =1.;
            interpolateColor =p.getShader().interpolateColor();
            
            if(correctInterpolation) {
                pi[Polygon.SW] = ( w );
                pi[Polygon.U] = (vertexData[pos+Polygon.U]*w );
                pi[Polygon.V] = (vertexData[pos+Polygon.V]*w );
                
    			//v[R] = (int) ((cdr > 1 ? 255 : (cdr * 255)) * FIXPS);
                if(!interpolateColor) w =1;
    			if( transparency == 0) {
    				pi[Polygon.R] = ((vertexData[pos+Polygon.R] > 1 ? 255 : (255*vertexData[pos+Polygon.R] )))*w;
    				pi[Polygon.G] = ((vertexData[pos+Polygon.G] > 1 ? 255 : (255*vertexData[pos+Polygon.G] )))*w;
    				pi[Polygon.B] = ((vertexData[pos+Polygon.B] > 1 ? 255 : (255*vertexData[pos+Polygon.B] )))*w;
    			} else {
    				pi[Polygon.R] = oneMinusTransparency*((vertexData[pos+Polygon.R] > 1 ? 255 : (255*vertexData[pos+Polygon.R] )))*w;
    				pi[Polygon.G] = oneMinusTransparency*((vertexData[pos+Polygon.G] > 1 ? 255 : (255*vertexData[pos+Polygon.G] )))*w;
    				pi[Polygon.B] = oneMinusTransparency*((vertexData[pos+Polygon.B] > 1 ? 255 : (255*vertexData[pos+Polygon.B] )))*w;
    			}
             } else {
                pi[Polygon.U] = (vertexData[pos+Polygon.U] );
                pi[Polygon.V] = (vertexData[pos+Polygon.V] );
                
                //v[R] = (int) ((cdr > 1 ? 255 : (cdr * 255)) * FIXPS);
                if( transparency == 0) {
                    pi[Polygon.R] = ((vertexData[pos+Polygon.R] > 1 ? 255 : (255*vertexData[pos+Polygon.R] )));
                    pi[Polygon.G] = ((vertexData[pos+Polygon.G] > 1 ? 255 : (255*vertexData[pos+Polygon.G] )));
                    pi[Polygon.B] = ((vertexData[pos+Polygon.B] > 1 ? 255 : (255*vertexData[pos+Polygon.B])));
                } else {
                    pi[Polygon.R] = oneMinusTransparency*((vertexData[pos+Polygon.R] > 1 ? 255 : (255*vertexData[pos+Polygon.R] )));
                    pi[Polygon.G] = oneMinusTransparency*((vertexData[pos+Polygon.G] > 1 ? 255 : (255*vertexData[pos+Polygon.G] )));
                    pi[Polygon.B] =oneMinusTransparency*((vertexData[pos+Polygon.B] > 1 ? 255 : (255*vertexData[pos+Polygon.B])));
                }
                
            }
		}
		

		if (outline) {
			transparency = 0;
			for (int j = 0; j < pLength - 1; j++) {
				line(
					polygon[j][Polygon.SX],
					polygon[j][Polygon.SY],
					polygon[j][Polygon.SZ],
					polygon[j + 1][Polygon.SX],
					polygon[j + 1][Polygon.SY],
					polygon[j + 1][Polygon.SZ],xmin,xmax,ymin,ymax);
			}
			line(
				polygon[pLength - 1][Polygon.SX],
				polygon[pLength - 1][Polygon.SY],
				polygon[pLength - 1][Polygon.SZ],
				polygon[0][Polygon.SX],
				polygon[0][Polygon.SY],
				polygon[0][Polygon.SZ],xmin,xmax,ymin,ymax);
		}
		
		transparency = (p.getShader().getVertexShader().getTransparency());
        texture = p.getShader().getTexture();
        interpolateColor =p.getShader().interpolateColor();
        interpolateTexture = texture != null; 
//		if(p.getShader().interpolateColor())
//            interpolateColor = true;
//		else
//            interpolateColor =false;
		scanPoly(polygon,pLength,xmin,xmax-1,ymin,ymax-1);


	}


	//
	//
	// What follows are the methods for scan converting the polygons
	// the algorithm is in principle the one from "Graphics Gems Vol I"
	// I dropped the flexible selection of what to interpolate...
	//
	//

	private double alsx, alsy, alsz, alr, alg, alb,alu,alv, alw;
	private double adlsx, adlsy, adlsz, adlr, adlg, adlb, adlu, adlv, adlw;
	private double arsx, arsy, arsz, arr, arg, arb, aru, arv, arw;
	private double adrsx, adrsy, adrsz, adrr, adrg, adrb, adru, adrv, adrw;
	

	protected double apsx, apsy, apsz, apr, apg, apb, apu, apv, apw;
	private double adpsx, adpsy, adpsz, adpr, adpg, adpb, adpu, adpv, adpw;


	private final void scanPoly(
		final double[][] p,
		final int plength,
		final int wxmin,
		final int wxmax,
		final int wymin,
		final int wymax) {
		int i, li, ri, y, ly, ry, rem, mask;
		int top = 0;
		double ymin;
		/*
		double[] l = null;
		double[] r = null;
		double[] dl = null;
		double[] dr = null;;
		*/
		if (plength > Polygon.MAXPOLYVERTEX) {
			System.err.println(
				"scanPoly: polygon had to many vertices: " + plength);
			return;
		}
        
        //initialize color if constant
        if(!interpolateColor) {
            apr = p[0][Polygon.R];
            apg = p[0][Polygon.G];
            apb = p[0][Polygon.B];
        }
        
		ymin = Double.MAX_VALUE;
		for (i = 0; i < plength; i++) {
			if (p[i][Polygon.SY] < ymin) {
				ymin = p[i][Polygon.SY];
				top = i;
			}
		}
		li = ri = top;
		rem = plength;
		//WARNING  was ceil (ymin -.5) !
		y = (int) Math.ceil(ymin - .5);
		ly = ry = y - 1;
		while (rem > 0) {
			while (ly <= y && rem > 0) {
				rem--;
				i = li - 1;
				if (i < 0)
					i = plength - 1;
				incrementalizeYldl(p[li], p[i], y);
				ly = (int) Math.floor(p[i][Polygon.SY] + .5);
				li = i;
			}
			//incrementalizeYldl(p[li],p[i],y);
			while (ry <= y && rem > 0) {
				rem--;
				i = ri + 1;
				if (i >= plength)
					i = 0;
				incrementalizeYrdr(p[ri], p[i], y);
				ry = (int) Math.floor(p[i][Polygon.SY] + .5);
				ri = i;
			}
			//incrementalizeYrdr(p[ri],p[i],y);

			while (y < ly && y < ry) {
				if (y >= wymin && y <= wymax) {
					if (alsx <= arsx)
						scanlinelr(y, wxmin, wxmax, wymin, wymax);
					else
						scanlinerl(y, wxmin, wxmax, wymin, wymax);
//System.out.println("s"+alsx+"   "+arsx);
				}
				y++;
				incrementldl();
				incrementrdr();
			}
		}
	}

	private final void incrementldl() {
		alsx += adlsx;
//		alsy += adlsy;
		alsz += adlsz;
        if(interpolateColor) {
    		alr += adlr;
    		alg += adlg;
            alb += adlb;
        }
        if(interpolateTexture) {
            alu += adlu;
            alv += adlv;
        }
        if(correctInterpolation) {
            alw += adlw;
        }
	}
	
	private final void incrementrdr() {
		arsx += adrsx;
//		arsy += adrsy;
		arsz += adrsz;
        if(interpolateColor) {
            arr += adrr;
            arg += adrg;
            arb += adrb;
        }
        if(interpolateTexture) {
            aru += adru;
            arv += adrv;
        }
        if(correctInterpolation) {
            arw += adrw;
        }
    }

	private final void incrementpdp() {
//		apsx += adpsx;
//		apsy += adpsy;
		apsz += adpsz;
        if(interpolateColor) {
    		apr += adpr;
    		apg += adpg;
            apb += adpb;
        }
        if(interpolateTexture) {
            apu += adpu;
            apv += adpv;
        }
        if(correctInterpolation) {
            apw += adpw;
        }
	}

//	protected int apsxI, apsyI, apszI, aprI, apgI, apbI;
//	protected int adpsxI, adpsyI, adpszI, adprI, adpgI, adpbI;


	private final void scanlinelr(
		final int y,
		final int wxmin,
		final int wxmax,
		final int wymin,
		final int wymax) {
		int x = 0;
		int lx = 0;
		int rx = 0;
		//WARNING original reads ceil(l->sx-.5)!
		lx = (int) Math.ceil(alsx - .5);
		if (lx < wxmin)
			lx = wxmin;
		rx = (int) Math.floor(arsx - .5);
		if (rx > wxmax)
			rx = wxmax;
		if (lx > rx)
			return;
		//System.out.println(l+" "+r+" "+p+" "+dp+"   "+lx);

		incrementalizeXlrpdp(lx);
		for (x = lx; x <= rx; x++) {
			colorize(x, y);
			incrementpdp();
            //if(apsz<-1||apsz>1) System.out.println("z "+apsz+" dz "+adpsz);
		}
	}

	private final void scanlinerl(
		final int y,
		final int wxmin,
		final int wxmax,
		final int wymin,
		final int wymax) {
		int x = 0;
		int lx = 0;
		int rx = 0;
		//WARNING original reads ceil(l->sx-.5)!
		lx = (int) Math.ceil(arsx - .5);
		if (lx < wxmin)
			lx = wxmin;
		rx = (int) Math.floor(alsx - .5);
		if (rx > wxmax)
			rx = wxmax;
		if (lx > rx)
			return;
		//System.out.println(l+" "+r+" "+p+" "+dp+"   "+lx);

		incrementalizeXrlpdp(lx);
		for (x = lx; x <= rx; x++) {
			colorize(x, y);
			incrementpdp();
		}
	}

	private final void incrementalizeXrlpdp(final int x) {
		double dx, frac;
		dx = alsx - arsx;
		if (dx == 0)
			dx = 1;
		double dxInv = 1/dx;
		frac = x + .5 - arsx;

//		adpsx = (alsx - arsx) / dx;
//		apsx = arsx + adpsx * frac;

//		adpsy = (alsy - arsy) / dx;
//		apsy = arsy + adpsy * frac;

		adpsz = (alsz - arsz) * dxInv;
		apsz = arsz + adpsz * frac;

        if(interpolateColor) {
    		adpr = (alr - arr) * dxInv;
    		apr = arr + adpr * frac;
    
    		adpg = (alg - arg) * dxInv;
    		apg = arg + adpg * frac;
    
    		adpb = (alb - arb) * dxInv;
    		apb = arb + adpb * frac;
        }
        if(interpolateTexture) {
            adpu = (alu - aru) * dxInv;
            apu = aru + adpu * frac;
            
            adpv = (alv - arv) * dxInv;
            apv = arv + adpv * frac;
        }
        if(correctInterpolation) {
            adpw = (alw - arw) * dxInv;
            apw = arw + adpw * frac;
        }
	}
	
	private final void incrementalizeXlrpdp(final int x) {
		double dx, frac;
		dx = arsx - alsx;
		if (dx == 0)
			dx = 1;
		frac = x + .5 - alsx;
		double dxInv = 1/dx;
//		adpsx = (arsx - alsx) / dx;
//		apsx = alsx + adpsx * frac;

//		adpsy = (arsy - alsy) / dx;
//		apsy = alsy + adpsy * frac;

		adpsz = (arsz - alsz) * dxInv;
		apsz = alsz + adpsz * frac;

        if(interpolateColor) {
    		adpr = (arr - alr) * dxInv;
    		apr = alr + adpr * frac;
    
    		adpg = (arg - alg) * dxInv;
    		apg = alg + adpg * frac;
    
    		adpb = (arb - alb) * dxInv;
    		apb = alb + adpb * frac;
        }
        if(interpolateTexture) {
            adpu = (aru - alu) * dxInv;
            apu = alu + adpu * frac;
            
            adpv = (arv - alv) * dxInv;
            apv = alv + adpv * frac;
        }
        if(correctInterpolation) {
            adpw = (arw - alw) * dxInv;
            apw = alw + adpw * frac;
        }
	}

	private final void incrementalizeYldl(
		final double[] p1,
		final double[] p2,
		final int y) {
		double dy, frac;
		dy = p2[Polygon.SY] - p1[Polygon.SY];
		if (dy == 0)
			dy = 1;
		frac = y + .5 - p1[Polygon.SY];
//		double dyInv = 1/dy;
		adlsx = (p2[Polygon.SX] - p1[Polygon.SX]) / dy;
		alsx = p1[Polygon.SX] + adlsx * frac;

//		System.out.println(" s"+alsx+"  "+adlsx + " "+frac+ " "+dy);

//		adlsy = (p2[SY] - p1[SY]) / dy;
//		alsy = p1[SY] + adlsy * frac;

		adlsz = (p2[Polygon.SZ] - p1[Polygon.SZ]) / dy;
		alsz = p1[Polygon.SZ] + adlsz * frac;

		if(interpolateColor) {
            adlr = (p2[Polygon.R] - p1[Polygon.R]) / dy;
    		alr = p1[Polygon.R] + adlr * frac;
    
    		adlg = (p2[Polygon.G] - p1[Polygon.G]) / dy;
    		alg = p1[Polygon.G] + adlg * frac;
    
    		adlb = (p2[Polygon.B] - p1[Polygon.B]) / dy;
    		alb = p1[Polygon.B] + adlb * frac;
        }
        
        if(interpolateTexture) {
            adlu = (p2[Polygon.U] - p1[Polygon.U]) / dy;
            alu = p1[Polygon.U] + adlu * frac;
            
            adlv = (p2[Polygon.V] - p1[Polygon.V]) / dy;
            alv = p1[Polygon.V] + adlv * frac;
        }
        if(correctInterpolation) {
            adlw = (p2[Polygon.SW] - p1[Polygon.SW]) / dy;
            alw = p1[Polygon.SW] + adlw * frac;
        }
	}

	private final void incrementalizeYrdr(
		final double[] p1,
		final double[] p2,
		final int y) {
		double dy, frac;
		dy = p2[Polygon.SY] - p1[Polygon.SY];
		if (dy == 0)
			dy = 1;
		frac = y + .5 - p1[Polygon.SY];

		adrsx = (p2[Polygon.SX] - p1[Polygon.SX]) / dy;
		arsx = p1[Polygon.SX] + adrsx * frac;

//		adrsy = (p2[SY] - p1[SY]) / dy;
//		arsy = p1[SY] + adrsy * frac;

		adrsz = (p2[Polygon.SZ] - p1[Polygon.SZ]) / dy;
		arsz = p1[Polygon.SZ] + adrsz * frac;

        if(interpolateColor) {
    		adrr = (p2[Polygon.R] - p1[Polygon.R]) / dy;
    		arr = p1[Polygon.R] + adrr * frac;
    
    		adrg = (p2[Polygon.G] - p1[Polygon.G]) / dy;
    		arg = p1[Polygon.G] + adrg * frac;
    
    		adrb = (p2[Polygon.B] - p1[Polygon.B]) / dy;
    		arb = p1[Polygon.B] + adrb * frac;
        }
        
        if(interpolateTexture) {
            adru = (p2[Polygon.U] - p1[Polygon.U]) / dy;
            aru = p1[Polygon.U] + adru * frac;
            
            adrv = (p2[Polygon.V] - p1[Polygon.V]) / dy;
            arv = p1[Polygon.V] + adrv * frac;
        }
        if(correctInterpolation) {
            adrw = (p2[Polygon.SW] - p1[Polygon.SW]) / dy;
            arw = p1[Polygon.SW] + adrw * frac;
        }
	}
    
    private int[] color = new int[4];
    
    private final void colorize(final int x, final int y) {
        if(interpolateTexture) {
            if(correctInterpolation)
                texture.getColor(apu/apw,apv/apw,
                    //adpu/(2.) + adru/(4.) +adlu/(4.),
                    //adpv/(2.) + adrv/(4.) +adlv/(4.),
                        x,y,
                    color);
            else 
                texture.getColor(apu,apv,
                    //adpu/(2.) + adru/(4.) +adlu/(4.),
                    //adpv/(2.) + adrv/(4.) +adlv/(4.),
                        x,y,
                    color);
            double t =(255&color[3])/255.;
            double d;
            if(correctInterpolation&&interpolateColor)
                d = 1/(255*apw);
            else 
                d =1/(255.);
            if(color[3]<255)
                setPixel(x, y, apsz,
                        ((apr)*(color[0]*t))*d,
                        ((apg)*(color[1]*t))*d,
                        ((apb)*(color[2]*t))*d,
                        ((1-t)) );
            else 
                setPixel(x, y, apsz,
                        ((apr)*(color[0]))*d,
                        ((apg)*(color[1]))*d,
                        ((apb)*(color[2]))*d,
                        transparency );

            
        }
        else {
            if( (correctInterpolation&&interpolateColor) ) {
                setPixel(x, y, apsz, apr/apw,apg/apw,apb/apw,transparency);
            } else {
                setPixel(x, y, apsz, apr,apg,apb,transparency);
            }
        }
    }
    
	//
	//
	// line with z
	//
	//
	private static final double ZEPS = -.0001;
	private final void line(
		final double x1,
		final double y1,
		final double z1,
		final double x2,
		final double y2,
		final double z2, final int xmin, final int xmax, final int ymin, final int ymax) {
			double dirX = x2-x1;
			double dirY = y2-y1;
			double dirZ = z2-z1;
			double l = ((double)dirX)*dirX + ((double)dirY)*dirY;
			l = Math.sqrt(l);
			if(l <1) return;
			dirX /= l;
			dirY /= l;
			dirZ/= l;

//			scanPolyNoColor(new double[][] {{x1-dirY,y1+dirX,z1+ZEPS,0,0,0,0,0,1},{x1+dirY,y1-dirX,z1-ZEPS,0,0,0,0,0,1},
//				{x2+dirY,y2-dirX,z2-ZEPS,0,0,0,0,0,1},{x2-dirY,y2+dirX,z2+ZEPS,0,0,0,0,0,1}},4,xmin,xmax-1,ymin,ymax-1);
			double zz1 =z1-dirZ+ZEPS;
			double zz2 = z2+dirZ+ZEPS;
            
            interpolateColor   = false;
            interpolateTexture = false;
            
			scanPoly(new double[][] {
				{x1-dirX-dirY,y1-dirY+dirX,zz1,0,0,0,0,0,1},
				{x1-dirX+dirY,y1-dirY-dirX,zz1,0,0,0,0,0,1},
				{x2+dirX+dirY,y2+dirY-dirX,zz2,0,0,0,0,0,1},
				{x2+dirX-dirY,y2+dirY+dirX,zz2,0,0,0,0,0,1}},4,xmin,xmax-1,ymin,ymax-1);

		}
	/* (non-Javadoc)
	 * @see de.jreality.soft.PolygonRasterizer#setWindow(int, int, int, int)
	 */
	public void setWindow(int xmin, int xmax, int ymin, int ymax) {
		this.xmin = xmin;
		this.xmax = xmax ;
		this.ymin = ymin;
		this.ymax = ymax ;
	}
    public void setSize(double width, double height) {
        wh =(width)/2;
        hh =(height)/2;
        mh =Math.min(wh,hh);
        
    }
  public abstract void setBackground(int argb);

}
