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

import java.awt.Image;
import java.io.IOException;

import javax.imageio.ImageIO;

import de.jreality.backends.texture.SimpleTexture;
import de.jreality.backends.texture.Texture;
import de.jreality.shader.ImageData;

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class EtchTexture implements Texture {
    
    static SimpleTexture etch1;
    static SimpleTexture etch2;
    static SimpleTexture etch3;
    
    
    static {
        Image img;
        try {
            img = ImageIO.read(EtchTexture.class.getResource("hatch4.png"));            
            etch1 = new SimpleTexture(new ImageData(img),false,false);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        try {
            img = ImageIO.read(EtchTexture.class.getResource("hatch5.png"));
            etch2 = new SimpleTexture(new ImageData(img),false,false);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        try {
            img = ImageIO.read(EtchTexture.class.getResource("hatch6.png"));
            etch3 = new SimpleTexture(new ImageData(img),false,false);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
    
    double scaleU = .02;
    double scaleV = .02;
    public final int transparency = 255;
    /**
     * 
     */
    public EtchTexture() {
        super();
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.Texture#getColor(double, double, int[])
     */
    public void getColor(double u, double v, double nx, double ny, double nz, int x, int y, double[] color) {
        double value  = (color[0] + color[1] + color[2])/3;
        color[0] = color[1] = color[2] = 255;
        color[3] = 255;
        
        u = 4.*x/314.;
        v = 4.*y/314.;
//        u = 4.*u;
//        v = 4.*v;
        if(value>250) {
            //color[0] = color[1] = color[2] = 255;
            //color[3] = 255;
            return;
        } else if(value >160) {
            etch1.getColor(u, v, nx, ny, nz, x, y, color);
            return;
        } else if(value >80) {
            etch2.getColor(u, v, nx, ny, nz, x, y, color);
            return;
        } else if( value > 5) {
            etch3.getColor(u, v, nx, ny, nz, x, y, color);
            return;
        } else
            color[0] = color[1] = color[2] = 0;
        color[3] = 255;
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
