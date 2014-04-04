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
 * 
 */
public abstract class ModularLongPolygonRasterizer implements PolygonRasterizer {
	//For fixpoint math:
	protected static final int FIXP = 14;
	protected final static int FIXPS = 1 << FIXP;
    protected final static int FIXPZ = 30;
    protected final static int FIXPZS = 1 << FIXPZ;
    protected final static int FIXPT = 30;
    protected final static int FIXPTS = 1<<FIXPT;
    
    
	
	//dimensions of the image to render into:
//	private int w;
//	private int h;
	private int xmin = 0;
	private int xmax = 0;
	private int ymin = 0;
	private int ymax = 0;
	
    // store half the size of the screen:
    //mh is the min of wh and hh; 
    private int wh;
    private int hh;
    private int mh;
    
	
	private int pLength = 0;	
	private long[][] polygon = new long[Polygon.MAXPOLYVERTEX][Polygon.VERTEX_LENGTH];
	protected long transparency = 0;
	protected long oneMinusTransparency = 255;

    private Texture texture = null;
    
    private boolean interpolateColor  = true;
    private boolean interpolateTexture= false;
    private static final boolean correctInterpolation =true;
    
	/**
	 * 
	 */
	public ModularLongPolygonRasterizer() {
		super();
	}

	/* (non-Javadoc)
	 * @see de.jreality.soft.Renderer#renderPolygon(de.jreality.soft.Polygon, double[],int,int,int,int)
	 */
	public final void renderPolygon(final Polygon p, final double[] vertexData, final boolean outline /*, final int xmin, final int xmax, final int ymin, final int ymax*/) {
		transparency = (long)(p.getShader().getVertexShader().getTransparency()*255);
		oneMinusTransparency = 255 - transparency;

		pLength = p.length;
		for (int i = 0; i < pLength; i++) {
			int pos = p.vertices[i];
			long[] pi= polygon[i];
			//system.out.println("render Poly"+i+" "+vertexData[pos+Polygon.SX]+" pos"+pos);
            
            double w = 1/vertexData[pos+Polygon.SW];
            double wxy =w*mh;
            //double w = 1;
			pi[Polygon.SX] = (long)( wh + vertexData[pos+Polygon.SX] * wxy);
			pi[Polygon.SY] = (long)(hh - vertexData[pos+Polygon.SY] * wxy);
			pi[Polygon.SZ] = (long)(vertexData[pos+Polygon.SZ] * w * FIXPZS);
            
            if(p.getShader() instanceof SkyboxPolygonShader) pi[Polygon.SZ] =FIXPZS;
            interpolateColor =p.getShader().interpolateColor();
            
            if(correctInterpolation) {
                pi[Polygon.SW] = (long)(w * FIXPTS);
                pi[Polygon.U]  = (long)(vertexData[pos+Polygon.U]  * FIXPTS*w);
                pi[Polygon.V]  = (long)(vertexData[pos+Polygon.V]  * FIXPTS*w);
                
                long iw = interpolateColor? (long)(w*FIXPS): FIXPS;
    			if(transparency ==0) {
    				pi[Polygon.R] = (long) ((vertexData[pos+Polygon.R] > 1 ? 255 : (vertexData[pos+Polygon.R] * 255)) * iw);
    				pi[Polygon.G] = (long) ((vertexData[pos+Polygon.G] > 1 ? 255 : (vertexData[pos+Polygon.G] * 255)) * iw);
    				pi[Polygon.B] = (long) ((vertexData[pos+Polygon.B] > 1 ? 255 : (vertexData[pos+Polygon.B] * 255)) * iw);
    			} else {
    				pi[Polygon.R] = oneMinusTransparency*(long) ((vertexData[pos+Polygon.R] > 1 ? 255 : (vertexData[pos+Polygon.R] * 255)) * iw);
    				pi[Polygon.G] = oneMinusTransparency*(long) ((vertexData[pos+Polygon.G] > 1 ? 255 : (vertexData[pos+Polygon.G] * 255)) * iw);
    				pi[Polygon.B] = oneMinusTransparency*(long) ((vertexData[pos+Polygon.B] > 1 ? 255 : (vertexData[pos+Polygon.B] * 255)) * iw);
    			}
            } else {
                pi[Polygon.U] = (long)(vertexData[pos+Polygon.U] *  FIXPTS);
                pi[Polygon.V] = (long)(vertexData[pos+Polygon.V] *  FIXPTS);
                
                if(transparency ==0) {
                    pi[Polygon.R] = (long) ((vertexData[pos+Polygon.R] > 1 ? 255 : (vertexData[pos+Polygon.R] * 255)) * FIXPS);
                    pi[Polygon.G] = (long) ((vertexData[pos+Polygon.G] > 1 ? 255 : (vertexData[pos+Polygon.G] * 255)) * FIXPS);
                    pi[Polygon.B] = (long) ((vertexData[pos+Polygon.B] > 1 ? 255 : (vertexData[pos+Polygon.B] * 255)) * FIXPS);
                } else {
                    pi[Polygon.R] = oneMinusTransparency*(long) ((vertexData[pos+Polygon.R] > 1 ? 255 : (vertexData[pos+Polygon.R] * 255)) * FIXPS);
                    pi[Polygon.G] = oneMinusTransparency*(long) ((vertexData[pos+Polygon.G] > 1 ? 255 : (vertexData[pos+Polygon.G] * 255)) * FIXPS);
                    pi[Polygon.B] = oneMinusTransparency*(long) ((vertexData[pos+Polygon.B] > 1 ? 255 : (vertexData[pos+Polygon.B] * 255)) * FIXPS);
                }
            }
            /*
            int[]t0 = new int[Polygon.VERTEX_LENGTH];

            t0[Polygon.SX] += pi[Polygon.SX]/pLength;
            t0[Polygon.SY] += pi[Polygon.SY]/pLength;
            t0[Polygon.SZ] += pi[Polygon.SZ]/pLength;
            t0[Polygon.R] += pi[Polygon.R]/pLength;
            t0[Polygon.G] += pi[Polygon.G]/pLength;
            t0[Polygon.B] += pi[Polygon.B]/pLength;
            */
		}
        /*
        t0[Polygon.SX] /= pLength;
        t0[Polygon.SY] /= pLength;
        t0[Polygon.R]  /= pLength;
        t0[Polygon.G]  /= pLength;
        t0[Polygon.B]  /= pLength;
          */  
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

		transparency = (long)(p.getShader().getVertexShader().getTransparency()*255);
        texture = p.getShader().getTexture();
        interpolateTexture = texture != null; 
        interpolateColor =p.getShader().interpolateColor();
		if(interpolateColor) {
            /*
            tri[0] = t0;
            for(int i =1;i<pLength;i++) {
                tri[1] =polygon[i-1]; 
                tri[2] =polygon[i]; 
                scanPolyI(tri,3,xmin,xmax-1,ymin,ymax-1);
            }
            tri[1] =polygon[0]; 
            tri[2] =polygon[pLength-1]; 
            scanPolyI(tri,3,xmin,xmax-1,ymin,ymax-1);
            */
        }
        else {
            /*
            tri[0] = t0;
            for(int i =1;i<pLength;i++) {
                tri[1] =polygon[i-1]; 
                tri[2] =polygon[i]; 
                scanPolyNoColor(tri,3,xmin,xmax-1,ymin,ymax-1);
            }
            tri[1] =polygon[0]; 
            tri[2] =polygon[pLength-1]; 
            scanPolyNoColor(tri,3,xmin,xmax-1,ymin,ymax-1);
            */
        }
		scanPolyI(polygon,pLength,xmin,xmax-1,ymin,ymax-1);
		

	}
	private long[][] tri =new long[3][];

	//
	//
	// What follows are the methods for scan converting the polygons
	// the algorithm is in principle the one from "Graphics Gems Vol I"
	// I converted it to fix point arithmetics and droped the flexible selection of
	// what to interpolate...
	//
	//
    private long alsxI, alsyI, alszI, alrI, algI, albI, aluI, alvI, alwI;
    private long adlsxI, adlsyI, adlszI, adlrI, adlgI, adlbI, adluI, adlvI, adlwI;
    private long arsxI, arsyI, arszI, arrI, argI, arbI, aruI, arvI, arwI;
    private long adrsxI, adrsyI, adrszI, adrrI, adrgI, adrbI, adruI, adrvI, adrwI;
    
	private final void scanPolyI(
		final long[][] p,
		final int plength,
		final long wxmin,
		final long wxmax,
		final long wymin,
		final long wymax) {
        int i, li, ri;
		long y, ly, ry, rem, mask;
        int top = 0;
        long ymin;

		if (plength > Polygon.MAXPOLYVERTEX) {
			System.err.println(
				"scanPoly: polygon had to many vertices: " + plength);
			return;
		}
        
        //initialize color if constant
        if(!interpolateColor) {
            aprI = p[0][Polygon.R];
            apgI = p[0][Polygon.G];
            apbI = p[0][Polygon.B];
        }
        
		ymin = Integer.MAX_VALUE;
		for (i = 0; i < plength; i++) {
			if (p[i][Polygon.SY] < ymin) {
				ymin = p[i][Polygon.SY];
				top = i;
			}
		}
		li = ri = top;
		rem = plength;
		//WARNING  was ceil (ymin -.5) !
		y = ((ymin + FIXPS / 2 - 1) >> FIXP);
		ly = ry = y - 1;
        //int up =1;
		while (rem > 0) {
			while (ly <= y && rem > 0) {
            //if (up*(ly - y)<= 0 && rem > 0) {
				rem--;
				i = li - 1;
				if (i < 0)
					i = plength - 1;
				incrementalizeYldlI(p[li], p[i], y);
				ly = (p[i][Polygon.SY] + FIXPS / 2) >> FIXP;
				li = i;
//                int nup = ly>y?1:-1;
//                if(nup!=up) {
//                    up = nup;
//                    incrementalizeYrdrI(p[i], p[ri], y);
//                }
			}

			while (ry <= y && rem > 0) {
            //if (up*(ry - y)<= 0 && rem > 0) {
				rem--;
				i = ri + 1;
				if (i >= plength)
					i = 0;
				incrementalizeYrdrI(p[ri], p[i], y);
				ry = (p[i][Polygon.SY] + FIXPS / 2) >> FIXP;
				ri = i;
//                int nup = ry>y?1:-1;
//                if(nup!=up) {
//                    up = nup;
//                    incrementalizeYldlI(p[i], p[li], y);
//                }
			}

            while (y < ly && y < ry) {
            //while (up*(y - ly)<0 && up*(y - ry)<0) {
				if (y >= wymin && y <= wymax) {
					if (alsxI <= arsxI)
						scanlinelrI(y, wxmin, wxmax, wymin, wymax);
					else
						scanlinerlI(y, wxmin, wxmax, wymin, wymax);
				}
                y++;
				//y+=up;
				incrementldlI();
				incrementrdrI();
				//			System.out.println("d "+((alsxI)>>FIXP)+"   "+((arsxI)>>FIXP));
			}
		}
	}
    




	/**
	 * @param l
	 * @param dl
	 */
	private final void incrementldlI() {
		alsxI += adlsxI;
		//	  alsyI += adlsyI;
		alszI += adlszI;
        if(interpolateColor) {
            alrI += adlrI;
    		algI += adlgI;
    		albI += adlbI;
        }
        if(interpolateTexture) {
            aluI += adluI;
            alvI += adlvI;
        }
        if(correctInterpolation) {
            alwI += adlwI;
        }
	}
	private final void incrementrdrI() {
		arsxI += adrsxI;
		//	  arsyI += adrsyI;
		arszI += adrszI;
        if(interpolateColor) {
            arrI += adrrI;
            argI += adrgI;
            arbI += adrbI;
        }
        if(interpolateTexture) {
            aruI += adruI;
            arvI += adrvI;
        }
        if(correctInterpolation) {
            arwI += adrwI;
        }
	}
	private final void incrementpdpI() {
		//	  apsxI += adpsxI;
		//	  apsyI += adpsyI;
		apszI += adpszI;
        if(interpolateColor) {
            aprI += adprI;
            apgI += adpgI;
            apbI += adpbI;
        }
        if(interpolateTexture) {
            apuI += adpuI;
            apvI += adpvI;
        }
        if(correctInterpolation) {
            apwI += adpwI;
        }
	}



    protected long apsxI, apsyI, apszI, aprI, apgI, apbI, apuI, apvI, apwI;
    protected long adpsxI, adpsyI, adpszI, adprI, adpgI, adpbI, adpuI, adpvI, adpwI;

	/**
	 * @param y
	 * @param l
	 * @param r
	 * @param wxmin
	 * @param wxmax
	 * @param wymin
	 * @param wymax
	 */
	private final void scanlinelrI(
		final long y,
		final long wxmin,
		final long wxmax,
		final long wymin,
		final long wymax) {
		long x = 0;
		long lx = 0;
		long rx = 0;
		//WARNING original reads ceil(l->sx-.5)!
		lx = ((alsxI + FIXPS / 2 - 1) >> FIXP);
		if (lx < wxmin)
			lx = wxmin;
		rx = (arsxI - FIXPS / 2) >> FIXP;
		if (rx > wxmax)
			rx = wxmax;
		if (lx > rx)
			return;
		//System.out.println(l+" "+r+" "+p+" "+dp+"   "+lx);

//HERE:
		incrementalizeXlrpdpI(lx);
		//	  blackPixelI(lx,y,apszI);
		for (x = lx; x <= rx; x++) {
			colorize(x,y);
			incrementpdpI();
		}
		//	  blackPixelI(rx,y,apszI);
	}
    private int[] color = new int[4];
    private static final int FIXPHS =1<<(FIXPT-FIXP);
    private final void colorize(final long x, final long y) {
        if(interpolateTexture) {
            if(correctInterpolation)
                texture.getColor((double)apuI/(double)apwI,(double)apvI/(double)apwI,
                    //adpuI/(2.*FIXPTS) + adruI/(4.*FIXPTS) +adluI/(4.*FIXPTS),
                    //adpvI/(2.*FIXPTS) + adrvI/(4.*FIXPTS) +adlvI/(4.*FIXPTS),
                        (int)x,(int)y,
                    color);
            else
                texture.getColor((double)apuI/(double)FIXPTS,(double)apvI/(double)FIXPTS,
                    //adpuI/(2.*FIXPTS) + adruI/(4.*FIXPTS) +adluI/(4.*FIXPTS),
                    //adpvI/(2.*FIXPTS) + adrvI/(4.*FIXPTS) +adlvI/(4.*FIXPTS),
                        (int)x,(int)y,
                    color);
            int t =color[3];
            if(correctInterpolation&&interpolateColor) {
                long ww =apwI>>(FIXPT-FIXP);
                if(t<255)
                    setPixel((int)x,(int) y,(int) apszI,
                            (int) (255&(((aprI>>8)*(color[0]*t))/ww)),
                            (int) (255&(((apgI>>8)*(color[1]*t))/ww)),
                            (int) (255&(((apbI>>8)*(color[2]*t))/ww)),
                            (int) ((255-t)) );
                else 
                    setPixel((int)x,(int) y,(int) apszI,
                            (int) (255&(((aprI>>8)*(color[0]))/ww)),
                            (int) (255&(((apgI>>8)*(color[1]))/ww)),
                            (int) (255&(((apbI>>8)*(color[2]))/ww)),
                            (int) transparency );
            } else {
                if(t<255)
                    setPixel((int)x,(int) y, (int)apszI,
                            (int)  ((aprI>>8)*(color[0]*t))>>FIXP,
                            (int)  ((apgI>>8)*(color[1]*t))>>FIXP,
                            (int)  ((apbI>>8)*(color[2]*t))>>FIXP,
                            (int) ((255-t)) );
                else 
                    setPixel((int)x,(int) y,(int) apszI,
                            (int) ((aprI>>8)*(color[0]))>>FIXP,
                            (int)((apgI>>8)*(color[1]))>>FIXP,
                            (int)((apbI>>8)*(color[2]))>>FIXP,
                            (int)transparency );
            }
      }
        else
            if(correctInterpolation&&interpolateColor) {
                //TODO
                //XXX:the division sometimes gives 256!!
                long ww =((apwI)>>(FIXPT-FIXP))-1;
                //if(apgI/ww>255) System.out.println("WARNING COLOR G"+(apgI/ww));
                //if(aprI/ww<0) System.out.println("WARNING COLOR R"+(aprI/ww));
                setPixel((int)x, (int)y, (int)apszI, (int)(255&(aprI/ww)),(int) (255&(apgI/ww)), (int)(255&(apbI/ww)),(int)transparency);
            } else 
                setPixel((int)x,(int) y,(int) apszI,(int)( aprI>>FIXP),(int)(apgI>>FIXP),(int)(apbI>>FIXP),(int)transparency);
        
    }
	private final void scanlinerlI(
		final long y,
		final long wxmin,
		final long wxmax,
		final long wymin,
		final long wymax) {
		long x = 0;
		long lx = 0;
		long rx = 0;
		//WARNING original reads ceil(l->sx-.5)!
		lx = ((arsxI + FIXPS / 2 - 1) >> FIXP);
		if (lx < wxmin)
			lx = wxmin;
		rx = (alsxI - FIXPS / 2) >> FIXP;
		if (rx > wxmax)
			rx = wxmax;
		if (lx > rx)
			return;
		//System.out.println(l+" "+r+" "+p+" "+dp+"   "+lx);

		incrementalizeXrlpdpI(lx);
		//	  blackPixelI(lx,y,apszI);
		for (x = lx; x <= rx; x++) {
			colorize(x,y);
			incrementpdpI();
		}
		//	  blackPixelI(rx,y,apszI);
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
	 *	if the pixel is tansparent the colors are premultiplied with 255*(1-transtarency).
	 * @param x x-coordinate to set
	 * @param y y-coordinate to set
	 * 
	 */
	protected abstract void setPixel(final int x, final int y, final int z, int red, int green, int blue, int transparency);

	private final void incrementalizeXrlpdpI(final long x) {
		long dx, frac;
		dx = (alsxI - arsxI) >> (FIXP / 2);
		if (dx == 0)
			dx = FIXPS >> (FIXP / 2);
		frac = ((x << FIXP) + FIXPS / 2 - arsxI) >> (FIXP / 2);

		//	  adpsxI = ((alsxI - arsxI) )/ dx;
		//	  apsxI = arsxI + adpsxI * frac;

		//	  adpsyI = ((alsyI - arsyI)) / dx;
		//	  apsyI = arsyI + adpsyI * frac;

		adpszI = ((alszI - arszI)) / dx;
		apszI = arszI + adpszI * frac;
		adpszI <<= FIXP / 2;

        if(interpolateColor) {
    		adprI = ((alrI - arrI)) / dx;
    		aprI = arrI + adprI * frac;
    		adprI <<= FIXP / 2;
    
    		adpgI = ((algI - argI)) / dx;
    		apgI = argI + adpgI * frac;
    		adpgI <<= FIXP / 2;
    
    		adpbI = ((albI - arbI)) / dx;
    		apbI = arbI + adpbI * frac;
    		adpbI <<= FIXP / 2;
        }
        if(interpolateTexture) {
            adpuI = ((aluI - aruI)) / dx;
            apuI = aruI + adpuI * frac;
            adpuI <<= FIXP / 2;
            
            adpvI = ((alvI - arvI)) / dx;
            apvI = arvI + adpvI * frac;
            adpvI <<= FIXP / 2;
        }
        if(correctInterpolation) {
            adpwI = ((alwI - arwI)) / dx;
            apwI = arwI + adpwI * frac;
            adpwI <<= FIXP / 2;
        }
	}
    
	private final void incrementalizeXlrpdpI(final long x) {
		long dx, frac;
		dx = (arsxI - alsxI) >> (FIXP / 2);
		if (dx == 0)
			dx = FIXPS >> (FIXP / 2);

		frac = ((x << FIXP) + FIXPS / 2 - alsxI) >> (FIXP / 2);

		//	  adpsxI =( (ars//xI - alsxI))/ dx;
		//	  apsxI = als//x//I + adpsxI * frac;

		//	  adpsyI = //((arsyI - alsyI)) / dx;
		//	  apsyI// //= alsyI + adpsyI * frac;

		adpszI = ((arszI - alszI)) / dx;
		apszI = alszI + adpszI* frac;
		adpszI <<= FIXP / 2;
        
		if(interpolateColor) {
    		adprI = ((arrI - alrI)) / dx;
    		aprI = alrI + adprI * frac;
    		adprI <<= FIXP / 2;
    
    		adpgI = ((argI - algI)) / dx;
    		apgI = algI + adpgI * frac;
    		adpgI <<= FIXP / 2;
    
    		adpbI = ((arbI - albI)) / dx;
    		apbI = albI + adpbI * frac;
    		adpbI <<= FIXP / 2;
            }
        if(interpolateTexture) {
            adpuI = ((aruI - aluI)) / dx;
             apuI = aluI + adpuI * frac;
            adpuI <<= FIXP / 2;
            
            adpvI = ((arvI - alvI)) / dx;
            apvI = alvI + adpvI * frac;
            adpvI <<= FIXP / 2;
        }
        if(correctInterpolation) {
            adpwI = ((arwI - alwI)) / dx;
            apwI = alwI + adpwI * frac;
            adpwI <<= FIXP / 2;
        }
	}



	private final void incrementalizeYldlI(
		final long[] p1,
		final long[] p2,
		final long y) {
		long dy, frac;
		dy = (p2[Polygon.SY] - p1[Polygon.SY]) >> (FIXP / 2);
		if (dy == 0) 
			dy = FIXPS >> (FIXP / 2);
		frac = ((y << FIXP) + FIXPS / 2 - p1[Polygon.SY]) >> (FIXP / 2);

		adlsxI = ((p2[Polygon.SX] - p1[Polygon.SX])) / dy;
		alsxI = p1[Polygon.SX] + adlsxI * frac;
		adlsxI <<= FIXP / 2;

		//	  adlsyI = ((p2[SY] - p1[SY])) / dy;
		//	  alsyI = p1[SY] + adlsyI * frac;

		adlszI = ((p2[Polygon.SZ] - p1[Polygon.SZ])) / dy;
		alszI = p1[Polygon.SZ] + adlszI * frac;
		adlszI <<= FIXP / 2;

        if(interpolateColor) {
    		adlrI = ((p2[Polygon.R] - p1[Polygon.R])) / dy;
    		alrI = p1[Polygon.R] + adlrI * frac;
    		adlrI <<= FIXP / 2;
    
    		adlgI = ((p2[Polygon.G] - p1[Polygon.G])) / dy;
    		algI = p1[Polygon.G] + adlgI * frac;
    		adlgI <<= FIXP / 2;
    
    		adlbI = ((p2[Polygon.B] - p1[Polygon.B])) / dy;
    		albI = p1[Polygon.B] + adlbI * frac;
    		adlbI <<= FIXP / 2;
        }
        if(interpolateTexture) {
            adluI = ((p2[Polygon.U] - p1[Polygon.U])) / dy;
            aluI = p1[Polygon.U] + adluI * frac;
            adluI <<= FIXP / 2;
            
            adlvI = ((p2[Polygon.V] - p1[Polygon.V])) / dy;
            alvI = p1[Polygon.V] + adlvI * frac;
            adlvI <<= FIXP / 2;
        }
        if(correctInterpolation) {
            adlwI = ((p2[Polygon.SW] - p1[Polygon.SW])) / dy;
            alwI = p1[Polygon.SW] + adlwI * frac;
            adlwI <<= FIXP / 2;
        }
	}
	private final void incrementalizeYrdrI(
		final long[] p1,
		final long[] p2,
		final long y) {
		long dy, frac;
		dy = (p2[Polygon.SY] - p1[Polygon.SY]) >> (FIXP / 2);
		if (dy == 0)
			dy = FIXPS >> (FIXP / 2);

		frac = ((y << FIXP) + FIXPS / 2 - p1[Polygon.SY]) >> (FIXP / 2);

		adrsxI = ((p2[Polygon.SX] - p1[Polygon.SX])) / dy;
		arsxI = p1[Polygon.SX] + adrsxI * frac;
		adrsxI <<= FIXP / 2;

		//	  adrsyI = ((p2[SY] - p1[SY]))  / dy;
		//	  arsyI = p1[SY] + adrsyI * frac;

		adrszI = ((p2[Polygon.SZ] - p1[Polygon.SZ])) / dy;
		arszI = p1[Polygon.SZ] + adrszI * frac;
		adrszI <<= FIXP / 2;

        if(interpolateColor) {
            adrrI = ((p2[Polygon.R] - p1[Polygon.R])) / dy;
    		arrI = p1[Polygon.R] + adrrI * frac;
    		adrrI <<= FIXP / 2;
    
    		adrgI = ((p2[Polygon.G] - p1[Polygon.G])) / dy;
    		argI = p1[Polygon.G] + adrgI * frac;
    		adrgI <<= FIXP / 2;
    
    		adrbI = ((p2[Polygon.B] - p1[Polygon.B])) / dy;
    		arbI = p1[Polygon.B] + adrbI * frac;
    		adrbI <<= FIXP / 2;
        }
        if(interpolateTexture) {
            adruI = ((p2[Polygon.U] - p1[Polygon.U])) / dy;
            aruI = p1[Polygon.U] + adruI * frac;
            adruI <<= FIXP / 2;
            
            adrvI = ((p2[Polygon.V] - p1[Polygon.V])) / dy;
            arvI = p1[Polygon.V] + adrvI * frac;
            adrvI <<= FIXP / 2;
        }
        if(correctInterpolation) {
            adrwI = ((p2[Polygon.SW] - p1[Polygon.SW])) / dy;
            arwI = p1[Polygon.SW] + adrwI * frac;
            adrwI <<= FIXP / 2;
        }
	}


 


	//
	//
	// line with z
	//
	//
	private static final int ZEPS = (int)(-0.0001*FIXPZS);
	private final void line(
		final long x1,
		final long y1,
		final long z1,
		final long x2,
		final long y2,
		final long z2, final long xmin, final long xmax, final long ymin, final long ymax) {
			long dirX = x2-x1;
			long dirY = y2-y1;
			long dirZ = z2 -z1;
			double l = ((double)dirX)*dirX + ((double)dirY)*dirY;
			l = Math.sqrt(l);
			long il = 1*( ((long) l)>>FIXP);
			if(il ==0) return;
			dirX /= il;
			dirY /= il;
			dirZ /= il;

			long zz1 =z1-dirZ+ZEPS;
			long zz2 = z2+dirZ+ZEPS;
            
            interpolateColor   = false;
            interpolateTexture = false;
            scanPolyI(new long[][] {
				{x1-dirX-dirY,y1-dirY+dirX,zz1,0,0,0,0,0,1},
				{x1-dirX+dirY,y1-dirY-dirX,zz1,0,0,0,0,0,1},
				{x2+dirX+dirY,y2+dirY-dirX,zz2,0,0,0,0,0,1},
				{x2+dirX-dirY,y2+dirY+dirX,zz2,0,0,0,0,0,1}},4,xmin,xmax-1,ymin,ymax-1);

		}

    public void setWindow(int xmin, int xmax, int ymin, int ymax) {
        this.xmin = xmin;
        this.xmax = xmax ;
        this.ymin = ymin;
        this.ymax = ymax ;
//        w = xmax - xmin;
//        h = ymax -ymin;

    }
    public void setSize(double width, double height) {
        wh =((int)width)*FIXPS/2;
        hh =((int)height)*FIXPS/2;
        mh =Math.min(wh,hh);
        
    }
}
