/*
 * Created on 19.11.2006
 *
 * This file is part of the de.jreality.softviewer package.
 * 
 * This program is free software; you can redistribute and/or modify 
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation; either version 2 of the license, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITTNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the 
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307
 * USA 
 */
package de.jreality.backends.texture;

import java.awt.Color;

import de.jreality.shader.CubeMap;

public class EnvironmentTexture implements Texture {
    private final Texture texture;
    
    private final SimpleTexture top;
    private final SimpleTexture bot;
    private final SimpleTexture front;
    private final SimpleTexture back;
    private final SimpleTexture left;
    private final SimpleTexture right;
    
    private int br, bg,bb,ba;
    
    public EnvironmentTexture(CubeMap cm, Texture texture2) {
        texture = texture2;
        top = new SimpleTexture(cm.getTop());
        bot = new SimpleTexture(cm.getBottom());
        left = new SimpleTexture(cm.getLeft());
        right = new SimpleTexture(cm.getRight());
        front = new SimpleTexture(cm.getFront());
        back = new SimpleTexture(cm.getBack());
        Color blend = cm.getBlendColor();
        br = blend.getRed();
        bg = blend.getGreen();
        bb = blend.getBlue();
        ba = blend.getAlpha();
    }

    public EnvironmentTexture(CubeMap cm, Texture texture2, Color blend) {
        this(cm, texture2);
        br = blend.getRed();
        bg = blend.getGreen();
        bb = blend.getBlue();
        ba = blend.getAlpha();
    }
    
    
    public void getColor(double u, double v, double nx, double ny, double nz,
            int x, int y, double[] color) {
        
        if(texture!=null) 
            texture.getColor(u, v, nx, ny, nz, x, y, color);
        if(ba == 0) return;
        
        double r = color[0];
        double g = color[1];
        double b = color[2];
        double a = color[3];
        color[0] = br;
        color[1] = bg;
        color[2] = bb;
        color[3] = ba;
        cubeMapColor(u,v,nx,ny,nz,color);
        double t = ba/(255.);
        double omt = 1-t;
        color[0] = (r*omt + t*color[0]);
        color[1] = (g*omt + t*color[1]);
        color[2] = (b*omt + t*color[2]);
        color[3] = a;
    }
    
    private void cubeMapColor(double u, double v, double nx, double ny, double nz,
            double[] color) {
       
        int s = (int) Math.signum(nx);
        double p = s*nx;
        int i = 1;
        if(p<Math.abs(ny)) {
            s = (int) Math.signum(ny);
            p = s*ny;
            i=2;
        }
        if(p<Math.abs(nz)) {
            s = (int) Math.signum(nz);
            i=3;
        }
        switch (s*i) {
        case -3: //z<0
            double xx = 0.5*(1+nx/nz);
            double yy = 0.5*(1+ny/nz);
            right.getColor(xx, yy, 0, 0, 0, 0, 0, color);
            return;
        case -2: //y<0
            xx = 0.5*(1-nx/ny);
            yy = 0.5*(1+nz/ny);
            top.getColor(xx, yy, 0, 0, 0,0,0, color);
            return;
        case -1: // x<0
            xx = 0.5*(1-nz/nx);
            yy = 0.5*(1+ny/nx);
            front.getColor(xx, yy, 0, 0, 0, 0,0,color);
            return;
        case 1: //x>0
            xx = 0.5*(1-nz/nx);
            yy = 0.5*(1-ny/nx);
            back.getColor(xx, yy, 0, 0, 0, 0, 0, color);
            return;
        case 2: //y>0
            xx = 0.5*(1+nx/ny);
            yy = 0.5*(1+nz/ny);
            bot.getColor(xx, yy, 0, 0, 0, 0,0,color);
            return;
        default:
            xx = 0.5*(1+nx/nz);
            yy = 0.5*(1-ny/nz);
            left.getColor(xx, yy, 0, 0, 0, 0, 0, color);
            return;
        }
        /*
        if(nz>0) {
            double xx = 0.5*(1+nx/nz);
            double yy = 0.5*(1-ny/nz);
            if(xx>=0 && yy>=0 && xx<=1 && yy <=1) {
                left.getColor(xx, yy, 0, 0, 0, 0, 0, color);
                return;
            }
        }
        
        if(nz<-0) {
            double xx = 0.5*(1+nx/nz);
            double yy = 0.5*(1+ny/nz);
            if(xx>=0 && yy>=0 && xx<=1 && yy <=1) {
                right.getColor(xx, yy, 0, 0, 0, 0, 0, color);
                return;
            }
        }
        if(nx > 0) {
            double xx = 0.5*(1-nz/nx);
            double yy = 0.5*(1-ny/nx);
            if(xx>=0 && yy>=0 && xx<=1 && yy <=1) {
                back.getColor(xx, yy, 0, 0, 0, 0, 0, color);
                return;
            }
        }
        if(nx < -0) {
            double xx = 0.5*(1-nz/nx);
            double yy = 0.5*(1+ny/nx);
            if(xx>=0 && yy>=0 && xx<=1 && yy <=1) {
                front.getColor(xx, yy, 0, 0, 0, 0,0,color);
                return;
            }
        }
        if(ny > 0) {
            double xx = 0.5*(1+nx/ny);
            double yy = 0.5*(1+nz/ny);
            if(xx>=0 && yy>=0 && xx<=1 && yy <=1) {
                bot.getColor(xx, yy, 0, 0, 0, 0,0,color);
                return;
            }
        }
        if(ny < -0) {
            double xx = 0.5*(1-nx/ny);
            double yy = 0.5*(1+nz/ny);
            if(xx>=0 && yy>=0 && xx<=1 && yy <=1) {
                top.getColor(xx, yy, 0, 0, 0,0,0, color);
                return;
            }
        }
        */
         
    }
    public boolean needsNormals() {
        return true;
    }

    public boolean isTransparent() {
        return texture ==null || texture.isTransparent();
    }
        
}