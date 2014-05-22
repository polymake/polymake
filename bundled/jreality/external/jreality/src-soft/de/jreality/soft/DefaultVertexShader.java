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

import java.awt.Color;

import de.jreality.scene.Geometry;
import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class DefaultVertexShader implements VertexShader {
	private boolean vertexColors;
    private boolean interpolateAlpha;
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
	
	public DefaultVertexShader(double color[], double phong, double phongSize, double transparency) {
		red= color[0];
		green = color[1];
		blue = color[2];
		this.phong = phong;
		this.phongSize = phongSize;
		this.transparency = transparency; 
	}

	/* (non-Javadoc)
	 * @see de.jreality.soft.VertexShader#shadeVertex(double[], de.jreality.soft.Environment)
	 */
	public final void shadeVertex(final double[] vd,final int pos, final Environment environment) {
		double nx = vd[pos+Polygon.NX];
		double ny = vd[pos+Polygon.NY];
		double nz = vd[pos+Polygon.NZ];
		double px = vd[pos+Polygon.SX];
		double py = vd[pos+Polygon.SY];
		double pz = vd[pos+Polygon.SZ];
		double cdr = 0;
		double cdg = 0;
		double cdb = 0;
		double csr = 0;
		double csg = 0;
		double csb = 0;

		//double normalflip = nz<0?1:-1;
        double normalflip = (px * nx + py * ny + pz * nz) <= 0 ? 1 : -1;
        //normalflip = 1;
		//
		// iterate over all directional lights
		//
        //System.out.println("num Dir L "+environment.getNumDirectionalLights());
        final int dirLightCount = environment.getNumDirectionalLights();
        final DirectionalLightSoft directionalLights[] = environment.getDirectionalLights();
		for (int i = 0; i < dirLightCount; i++) {
			DirectionalLightSoft light = directionalLights[i];
			double[] dl = light.direction;
			double lx = dl[0];
			double ly = dl[1];
			double lz = dl[2];
			//Color3d lcolor = light.color;
			double lr = light.red;
			double lg = light.green;
			double lb = light.blue;
			double intensity = light.intensity;
			//diffuse:
            double ntl = (nx * lx + ny * ly + nz * lz);// * 2;
            //double fac = -normalflip * (nx * lx + ny * ly + nz * lz) * 2;
            double fac = normalflip * ntl;

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
			double[] dl = light.direction;
			// light direction:
			double dx = dl[0];
			double dy = dl[1];
			double dz = dl[2];
			// point to light vector:
			double[] src = light.source;
			double lx = src[0] - px;
			double ly = src[1] - py;
			double lz = src[2] - pz;
			double dst = Math.sqrt(lx * lx + ly * ly + lz * lz);
			lx /= dst;
			ly /= dst;
			lz /= dst;

			// check if we are inside the cone:
			double cosCone = light.cosConeAngle;
			double cosAngle = (lx * dx + ly * dy + lz * dz);
			if (cosAngle < cosCone)
				continue;

			//Color3d lcolor = spotLights[i].color;
			double lr = light.red;
			double lg = light.green;
			double lb = light.blue;
			double intensity = 1 * light.intensity/ (light.a0
						+ (light.a1 + light.a2 * dst) * dst);
			double softFrac = light.softEdgeFraction;
			if (cosAngle - cosCone < softFrac)
				intensity *= (cosAngle - cosCone) / softFrac;
			//diffuse:
            double ntl = (nx * lx + ny * ly + nz * lz) ;
            //double fac = -normalflip * (nx * lx + ny * ly + nz * lz) * 2;
            double fac = normalflip *ntl;
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
            cdr *= vd[pos+Polygon.R];
            cdg *= vd[pos+Polygon.G];
            cdb *= vd[pos+Polygon.B];
            a    = vd[pos+Polygon.A];
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
		
		vd[pos+Polygon.R] = cdr;
		vd[pos+Polygon.G] = cdg;
        vd[pos+Polygon.B] = cdb;
        vd[pos+Polygon.A] = a;
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

    public boolean interpolateAlpha() {
        return interpolateAlpha;
    }

    public void setup(EffectiveAppearance eAppearance, String name) {
      transparency = eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.TRANSPARENCY), transparency);
      Color c = (Color)eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.DIFFUSE_COLOR), CommonAttributes.DIFFUSE_COLOR_DEFAULT);
      float[] rgb=c.getComponents(null);
      red=rgb[0];
      green=rgb[1];
      blue=rgb[2];
      //pending
      phong = eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.SPECULAR_COEFFICIENT), CommonAttributes.SPECULAR_COEFFICIENT_DEFAULT);
      phongSize = eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.SPECULAR_EXPONENT), CommonAttributes.SPECULAR_EXPONENT_DEFAULT);
    }
    public void startGeometry(Geometry geom)
    {
        DataList colors=null;
        vertexColors=(geom instanceof PointSet)
          &&(colors=((PointSet)geom).getVertexAttributes(Attribute.COLORS))!=null;
//        System.out.println(geom.getName()+": colors: "+vertexColors);
        interpolateAlpha=vertexColors&&colors.getStorageModel().getDimensions()[1]!=3;
    }
}
