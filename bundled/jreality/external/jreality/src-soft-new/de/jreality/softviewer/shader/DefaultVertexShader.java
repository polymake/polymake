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

import java.awt.Color;

import de.jreality.softviewer.DirectionalLightSoft;
import de.jreality.softviewer.Environment;
import de.jreality.softviewer.Polygon;
import de.jreality.softviewer.SpotLightSoft;

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class DefaultVertexShader extends VertexShader {
    private double phongSize = 8;
	private double phong = .7;
	private double red = .4;
	private double green = .4;
	private double blue = .6;
	private double transparency = 0.0;
	
	/**
	 * 
	 */
	public DefaultVertexShader() {
		super();
	}
	
	public DefaultVertexShader(Color c, double phong, double phongSize, double transparency) {
        float[] rgb=c.getComponents(null);
        red=rgb[0];
        green=rgb[1];
        blue=rgb[2];
		this.transparency = transparency; 
		this.phong = phong;
		this.phongSize = phongSize;
	}

	/* (non-Javadoc)
	 * @see de.jreality.soft.VertexShader#shadeVertex(double[], de.jreality.soft.Environment)
	 */
	public final void shadeVertex(final double[] vd, final Environment environment, boolean vertexColors) {
        vertexColors |= this.vertexColors;
		double nx = vd[Polygon.NX];
		double ny = vd[Polygon.NY];
		double nz = vd[Polygon.NZ];
		double px = vd[Polygon.WX];
		double py = vd[Polygon.WY];
        double pz = vd[Polygon.WZ];
        double pw = vd[Polygon.WW];
		double cdr = 0;
		double cdg = 0;
		double cdb = 0;
		double csr = 0;
		double csg = 0;
		double csb = 0;

		//double normalflip = nz<0?-1:1;
        double normalflip = (px * nx + py * ny + pz * nz)*pw <= 0 ? 1 : -1;
        //double normalflip = 1;
		//
		// iterate over all directional lights
		//
        //System.out.println("num Dir L "+environment.getNumDirectionalLights());
        final int dirLightCount = environment.getNumDirectionalLights();
        final DirectionalLightSoft directionalLights[] = environment.getDirectionalLights();
		for (int i = 0; i < dirLightCount; i++) {
			DirectionalLightSoft light = directionalLights[i];
			double[] dl = light.getDirection();
			double lx = dl[0];
			double ly = dl[1];
			double lz = dl[2];
			//Color3d lcolor = light.color;
			double lr = light.getRed();
			double lg = light.getGreen();
			double lb = light.getBlue();
			double intensity = light.getIntensity();
			//diffuse:
            double ntl = (nx * lx + ny * ly + nz * lz);// * 2;
            //double fac = -normalflip * (nx * lx + ny * ly + nz * lz) * 2;
            double fac = normalflip * ntl;

            //fac =1-  normalflip *(ntl);
            
            if (fac < 0)
				continue;
			fac *= intensity;
			cdr += lr * fac;
			cdg += lg * fac;
			cdb += lb * fac;

			//hilite:
			//double t =/* 1/Math.sqrt(2*(1+lz))* */ ((-normalflip*(x * lx + y * ly + z * (lz + 1))));
            
            //double t = ((2 * (nx * lx + ny * ly + nz * lz)) * nz - lz);
            
            double t = (2*ntl * nz - lz);
            // this would be a better specular component 
            // (but needs somehow phongSize->4*phongSize)
//            double hx,hy,hz;
//            double pl =1/Math.sqrt(px*px+py*py+pz*pz);
//            hx =+lx-px*pl;
//            hy =+ly-py*pl;
//            hz =+lz-pz*pl;
//            double n =1/Math.sqrt(hx*hx+hy*hy+hz*hz);
//            double t = n*normalflip*(hx*nx+hy*ny+hz*nz);
            
            if (t > 0) {
				t = intensity * Math.pow(t, phongSize /* *4*/ );
                //TODO: Add support for specular color?
				csr += lr * t;
				csg += lg * t;
				csb += lb * t;
			}
		}

		//
		// now do the same for all spot lights:
		//
		//
		// iterate over all spot lights lights
		//
        final int spotLightCount = environment.getNumSpotLights();
        final SpotLightSoft spotlights[] = environment.getSpotLights();
		for (int i = 0; i < spotLightCount; i++) {
			SpotLightSoft light = spotlights[i];
			double[] dl = light.getDirection();
			// light direction:
			double dx = dl[0];
			double dy = dl[1];
			double dz = dl[2];
			// point to light vector:
			double[] src = light.getSource();
			double lx = src[0] - px;
			double ly = src[1] - py;
			double lz = src[2] - pz;
			double dst = Math.sqrt(lx * lx + ly * ly + lz * lz);
			lx /= dst;
			ly /= dst;
			lz /= dst;

			// check if we are inside the cone:
			double cosCone = light.getCosConeAngle();
			double cosAngle = (lx * dx + ly * dy + lz * dz);
			if (cosAngle < cosCone)
				continue;

			//Color3d lcolor = spotLights[i].color;
			double lr = light.getRed();
			double lg = light.getGreen();
			double lb = light.getBlue();
			double intensity = 1 * light.getIntensity()/ (light.getA0()
						+ (light.getA1() + light.getA2() * dst) * dst);
			double softFrac = light.getSoftEdgeFraction();
			if (cosAngle - cosCone < softFrac)
				intensity *= (cosAngle - cosCone) / softFrac;
			//diffuse:
            double ntl = (nx * lx + ny * ly + nz * lz) ;
            //double fac = -normalflip * (nx * lx + ny * ly + nz * lz) * 2;
            double fac = normalflip *ntl;
            
            //fac = 1- normalflip * (ntl);

            
            if (fac < 0)
				continue;
			fac *= intensity;
			cdr += lr * fac;
			cdg += lg * fac;
			cdb += lb * fac;

			//hilite:
			//double t =/* 1/Math.sqrt(2*(1+lz))* */ ((-normalflip*(x * lx + y * ly + z * (lz + 1))));
            
			//double t = ((2 * (nx * lx + ny * ly + nz * lz)) * nz - lz);
            double t = (2*ntl * nz - lz);
            if (t > 0) {
				t = intensity * Math.pow(t, phongSize /* * 4*/);
				csr += lr * t;
				csg += lg * t;
				csb += lb * t;
			}
		}

        double a;
        if(vertexColors)
        {
            cdr *= vd[Polygon.R];
            cdg *= vd[Polygon.G];
            cdb *= vd[Polygon.B];
            a    = vd[Polygon.A];
        }
        else
        {
            cdr *= red;
            cdg *= green;
            cdb *= blue;
            a    = transparency;
        }

		cdr += phong * csr;
		cdg += phong * csg;
		cdb += phong * csb;

		//double omt = 1-transparency;
        
        double ff = environment.getFogfactor();
        if(ff != 0) {
            double d = Math.exp(-Math.sqrt(px*px + py*py + pz*pz)*ff);
            double dm = 1-d;
            double[] fc = environment.getFogColor();
            cdr = cdr * d + dm*fc[0];
            cdg = cdg * d + dm*fc[1];
            cdb = cdb * d + dm*fc[2];
        }
		
		vd[Polygon.R] = cdr;
		vd[Polygon.G] = cdg;
        vd[Polygon.B] = cdb;
        vd[Polygon.A] = a;
	}

	public final double getTransparency() {
		return transparency;
	}

	/**
	 * Sets the transparency.
	 * @param transparency The transparency to set
	 */
	public final void setTransparency(double transparency) {
		this.transparency = transparency;
	}

    public void setColor(double r, double g, double b) {
        red = r;
        green = g;
        blue = b;
        
    }

    @Override
    public double getBlue() {
        return blue;
    }

    @Override
    public double getGreen() {
        return green;
    }

    @Override
    public double getRed() {
        return red;
    }

//    public void setup(EffectiveAppearance eAppearance, String name) {
//      transparency = eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.TRANSPARENCY), transparency);
//      Color c = (Color)eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.DIFFUSE_COLOR), CommonAttributes.DIFFUSE_COLOR_DEFAULT);
//      float[] rgb=c.getComponents(null);
//      red=rgb[0];
//      green=rgb[1];
//      blue=rgb[2];
//      //pending
//      phong = eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.SPECULAR_COEFFICIENT), CommonAttributes.SPECULAR_COEFFICIENT_DEFAULT);
//      phongSize = eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.SPECULAR_EXPONENT), CommonAttributes.SPECULAR_EXPONENT_DEFAULT);
//    }
  
}
