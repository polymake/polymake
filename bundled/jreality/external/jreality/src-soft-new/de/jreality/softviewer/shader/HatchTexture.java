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


package de.jreality.softviewer.shader;

import de.jreality.backends.texture.Texture;

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class HatchTexture implements Texture {
    double scaleU = .02;
    double scaleV = .02;
    public final int transparency = 255;
    /**
     * 
     */
    public HatchTexture() {
        super();
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.Texture#getColor(double, double, int[])
     */
    public void getColor(double u, double v, double nx, double ny, double nz, int x, int y, double[] color) {
        double value  = (color[0] + color[1] + color[2])/3;

        if(value>90) {
            color[0] = color[1] = color[2] = 255;
            color[3] = 255;
            return;
        }
        
        int m = value >20? 4:(value>5? 2:1);
        //int iu = ((int) ((x+y)/4))%m;
        int iu = x%m;
        //int iu = ((int) ((v/scaleV)+(u/scaleU)))%m;
        if(iu ==0 ) {
            color[0] = color[1] = color[2] = 0;
            color[3] = 255;
        } else {
            color[0] = color[1] = color[2] = 255;
            color[3] = 255;
        }

    }

 
    public boolean isTransparent() {
        return false;
    }

    public boolean needsNormals() {
        return false;
    }

    public void getMipMapedColor(double u, double dxu, double dyu, double v, double dxv, double dyv, double nx, double ny, double nz, int x, int y, double[] color) {
        getColor(u, v, nx, ny, nz, x, y, color);
        
    }

}
