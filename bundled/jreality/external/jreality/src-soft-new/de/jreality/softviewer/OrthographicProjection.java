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


package de.jreality.softviewer;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class OrthographicProjection extends CameraProjection {

	private double fieldOfView =  60. / 180.* Math.PI;
	private double focalLength = 	1. /Math.tan(15. / 360. * Math.PI);
	//TODO: customize This
	private int width= 640;
	private int height = 480;
	private double wh = width/2.;
	private double hh = height/2.;
    private double mh = Math.min(wh,hh);
	private double nearclip = 3;
	private double farclip = 50;
	private double focus = 3.;
	/**
	 * 
	 */
	public OrthographicProjection() {
		super();
		setFrustumZmin(-1);
		setFrustumZmax(1);
		
	}

	/* (non-Javadoc)
	 * @see de.jreality.soft.Perspective#perspective(double[], int)
	 */
	public final void perspective(final double[] v) {

         double d =focalLength/focus;
        v[Polygon.SX] = v[Polygon.WX] *d;//( 1.2);
        v[Polygon.SY] = v[Polygon.WY] *d;//( 1.2);
        v[Polygon.SZ] = -((nearclip + farclip)*v[Polygon.WW]+ 2*v[Polygon.WZ] )/(farclip - nearclip );
        v[Polygon.SW] = v[Polygon.WW];
	}

	/**
	 * @return double
	 */
	public double getFieldOfView() {
		return fieldOfView;
	}

	/**
	 * @return double
	 */
	public double getFocalLength() {
		return focalLength;
	}

	/**
	 * Sets the fieldOfView in degrees.
	 * @param fieldOfView The fieldOfView to set
	 */
	public void setFieldOfViewDeg(double fieldOfView) {
		this.fieldOfView = fieldOfView / 180.* Math.PI;
        this.focalLength = /*(width/(double)height)*/ 1/Math.tan(this.fieldOfView/2);
		//TODO fix this: fieldOfView <-> focalLength
	}

	/**
	 * Sets the focalLength.
	 * @param focalLength The focalLength to set
	 */
	public void setFocalLength(double focalLength) {
		this.focalLength = focalLength;
		//TODO fix this: fieldOfView <-> focalLength
	}

	/**
	 * @return int
	 */
	public int getHeight() {
		return height;
	}

	/**
	 * @return int
	 */
	public int getWidth() {
		return width;
	}

	/**
	 * Sets the height.
	 * @param height The height to set
	 */
	public void setHeight(int height) {
		this.height = height;
		this.hh = height/2.;
		//frustum.y1 = height;
        mh =Math.min(wh,hh);
        setFrustumYmax(Math.max(1,hh/mh));
        setFrustumYmin(-getFrustumYmax());
        setFrustumXmax(Math.max(1,wh/mh));
        setFrustumXmin(-getFrustumXmax());
	}

	/**
	 * Sets the width.
	 * @param width The width to set
	 */
	public void setWidth(int width) {
		this.width = width;
		this.wh = width/2.;
		//frustum.x1 = width;
        mh =Math.min(wh,hh);
        
        setFrustumYmax(Math.max(1,hh/mh));
        setFrustumYmin(-getFrustumYmax());
        setFrustumXmax(Math.max(1,wh/mh));
        setFrustumXmin(-getFrustumXmax());
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.Perspective#getFrustum()
     */
    

    public void setNear(double d) {
        nearclip =d; 
        
    }

    
    public void setFar(double d) {
        farclip =d;
        
    }

    public double getFocus() {
        return focus;
    }

    public void setFocus(double focus) {
        this.focus = focus;
    }

}
