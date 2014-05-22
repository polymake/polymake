/*
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

public abstract class CameraProjection {

    private double frustumXmin,frustumXmax,frustumYmin,frustumYmax,frustumZmin,frustumZmax;
    
    public CameraProjection() {
        super();
        // TODO Auto-generated constructor stub
    }
    public abstract void perspective(double[] data);

    /**
     * @param w
     */
    public abstract void setWidth(int w);

    /**
     * @param h
     */
    public abstract void setHeight(int h);
    public double getFrustumXmax() {
        return frustumXmax;
    }
    public void setFrustumXmax(double frustumXmax) {
        this.frustumXmax = frustumXmax;
    }
    public double getFrustumXmin() {
        return frustumXmin;
    }
    public void setFrustumXmin(double frustumXmin) {
        this.frustumXmin = frustumXmin;
    }
    public double getFrustumYmax() {
        return frustumYmax;
    }
    public void setFrustumYmax(double frustumYmax) {
        this.frustumYmax = frustumYmax;
    }
    public double getFrustumYmin() {
        return frustumYmin;
    }
    public void setFrustumYmin(double frustumYmin) {
        this.frustumYmin = frustumYmin;
    }
    public double getFrustumZmax() {
        return frustumZmax;
    }
    public void setFrustumZmax(double frustumZmax) {
        this.frustumZmax = frustumZmax;
    }
    public double getFrustumZmin() {
        return frustumZmin;
    }
    public void setFrustumZmin(double frustumZmin) {
        this.frustumZmin = frustumZmin;
    }
    
}
