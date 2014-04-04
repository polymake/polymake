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

import de.jreality.backends.texture.Texture;

public abstract class AbstractPolygon {

    public static final int WX = 0;
    public static final int WY = 1;
    public static final int WZ = 2;
    public static final int WW = 3;
    public static final int SX = 4;
    public static final int SY = 5;
    public static final int SZ = 6;
    public static final int SW = 7;
    public static final int R = 8;
    public static final int G = 9;
    public static final int B = 10;
    public static final int A = 11;
    public static final int U = 12;
    public static final int V = 13;
    public static final int NX = 14;
    public static final int NY = 15;
    public static final int NZ = 16;
    public static final int VERTEX_LENGTH = 17;

    private double[] center = new double[VERTEX_LENGTH];
    
    public AbstractPolygon() {
        super();
    }

    public double[] getCenter() {
        return center;
    }

    public final Triangle[] triangulate(Triangle[] ta, final ArrayStack stack) {
        final int length = getLength()-2;
        if (length <= 0) return ta == null? new Triangle[0]:ta;
        if(ta == null|| ta.length < length) ta = new Triangle[length];
        double[] start = getPoint(length+1);
        double[] next = getPoint(0);
        for (int i = 0; i < length; i++) {
            Triangle t = stack.pop();
            //if(t== null) t = new Triangle();
            t.setShadingFrom(this);
            ta[i] = t;
            
            t.setPointFrom(0,start);
            t.setPointFrom(1,next);
            next = getPoint((i+1));
            t.setPointFrom(2,next);
        }
        return ta;
    }
    

    
    private double transparency;

    public final void setTransparency(final double t) {
        transparency = t;
    }

    public final double getTransparency() {
        return transparency;
    }

    
    private Texture texture;

    public final Texture getTexture() {
        return texture;
    }

    public final void setTexture(Texture texture) {
        this.texture = texture;
    }
    
    private boolean interpolateColor;
    
    public boolean isInterpolateColor() {
        return interpolateColor;
    }

    public void setInterpolateColor(boolean interpolateColor) {
        this.interpolateColor = interpolateColor;
    }

    private boolean interpolateAlpha;
    
    public boolean isInterpolateAlpha() {
        return interpolateAlpha;
    }

    public void setInterpolateAlpha(boolean interpolateAlpha) {
        this.interpolateAlpha = interpolateAlpha;
    }
    
    public void setShadingFrom(AbstractPolygon p) {
        setInterpolateAlpha(p.isInterpolateAlpha());
        setInterpolateColor(p.isInterpolateColor());
        setTransparency(p.getTransparency());
        setTexture(p.getTexture());
        setSkybox(p.isSkybox());
        setCenterFrom(p.getCenter());
            
    }
    
    public abstract double[] getPoint(int i);
    public abstract int getLength();
    
    public void setPointFrom(int i, double[] data) {
        System.arraycopy(data,0,getPoint(i),0,VERTEX_LENGTH);
    }
    public void setCenterFrom(double[] data) {
        System.arraycopy(data,0,getCenter(),0,VERTEX_LENGTH);
    }
    
    private boolean isSkybox;
    
    public boolean isSkybox() {
        return isSkybox;
    }

    public void setSkybox(boolean isSkybox) {
        this.isSkybox = isSkybox;
    }

}
