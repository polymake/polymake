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


package de.jreality.renderman;

import java.util.HashMap;

import de.jreality.math.Matrix;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.SpotLight;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;

/**
 * This is a utility for the {@link RIBVisitor}. It collects all the lights in the scene
 * and writes them using the {@link de.jreality.renderman.Ri} instance <i>ri</i>. Currently supports {@link de.jreality.scene.DirectionalLight},
 * {@link de.jreality.scene.PointLight}, and {@link de.jreality.scene.SpotLight}.  If the
 * current effective appearance {@link de.jreality.shader.EffectiveAppearance} evaluates 
 * {@link de.jreality.shader.CommonAttributes#RMAN_SHADOWS_ENABLED} as <code>true</code> then
 * corresponding shadow-casting shaders are written out into the RIB file instead of the 
 * standard RenderMan lights.
 * <p>
 * Todo: look at the {@link de.jreality.scene.Appearance} attached to the SceneGraphComponent 
 * containing the light for the key {@link de.jreality.shader.CommonAttributes#RMAN_LIGHT_SHADER}. If 
 * found, its value should be an instance of {@link de.jreality.renderman.shader.SLShader}, and this 
 * should be substituted for the standard light as generated now by this visitor.
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>, Charles Gunn
 *
 */
public class LightCollector extends SceneGraphVisitor {
    double[]  initialTrafo,   currentTrafo;
    RIBVisitor ribv = null;
    float[] zdirection = new float[] {0f,0f,-1f};
    float[] fCurrentTrafo = null;
    protected LightCollector reclaimableSubcontext;
    EffectiveAppearance eAppearance = null;
    boolean shadowEnabled = false;
    int metric = Pn.EUCLIDEAN;
    SceneGraphPath currentPath = null;
    Ri ri = null;
    String lightname;
	private double[] dfrom, dto;
   /**
     * 
     */
    public LightCollector(SceneGraphComponent root, RIBVisitor v) {
        super();
        ribv = v;
        this.ri = v.ri;
        currentTrafo = new double[16];
        eAppearance=EffectiveAppearance.create();
        currentPath = new SceneGraphPath();
        visit(root);
    }
    protected LightCollector(LightCollector parentContext) {
        initializeFromParentContext(parentContext);
    }
    
    LightCollector subContext() {
        if (reclaimableSubcontext != null) {
            reclaimableSubcontext.initializeFromParentContext(this);
            return reclaimableSubcontext;
        } else
            return reclaimableSubcontext= new LightCollector(this);
    }
    
    protected void initializeFromParentContext(LightCollector parentContext) {
        currentTrafo=parentContext.currentTrafo;
        ribv = parentContext.ribv;
    }
    
    public void visit(SceneGraphComponent c) {
        EffectiveAppearance tmp =eAppearance;
        Appearance a = c.getAppearance();
        if (!c.isVisible()) return;
        if(a!= null ) eAppearance = eAppearance.create(a);
        currentPath.push(c);
        shadowEnabled = eAppearance.getAttribute(CommonAttributes.RMAN_SHADOWS_ENABLED, false);
        metric = eAppearance.getAttribute(CommonAttributes.METRIC, Pn.EUCLIDEAN);
        c.childrenAccept(this); //subContext());
        eAppearance= tmp;
        currentPath.pop();
    }
    
    public void visit(DirectionalLight l) {
        ri.transformBegin();
        HashMap<String, Object> map =new HashMap<String, Object>();
        handleCommon(l, map);
//        ri.concatTransform(fCurrentTrafo);
//        map.put("to",zdirection);
        map.put("to",new float[] {(float)dto[0], (float) dto[1], (float) dto[2]});
        if (metric == Pn.EUCLIDEAN)
        	lightname = shadowEnabled ? "shadowdistant": "distantlight";
         ri.lightSource(lightname,map);
         ri.transformEnd();
     }
	private void handleCommon(Light l, HashMap<String, Object> map) {
		currentPath.getMatrix(currentTrafo);
		// Unfortunately, Renderman PRMan doesn't transform lights correctly when
		// non-euclidean matrices are involved. We explicitly multiply out. -gunn
	    Matrix mm = new Matrix(currentTrafo);
	    mm.multiplyOnLeft(ribv.world2Camera);
	    dfrom = mm.getColumn(3);
	    Pn.dehomogenize(dfrom, dfrom);
	    dto = mm.getColumn(2);
	    Rn.times(dto, -1, dto);
	    Pn.dehomogenize(dto, dto);
		fCurrentTrafo = RIBHelper.fTranspose(currentTrafo);
		map.put("intensity",new Float(l.getIntensity()));
        map.put("lightcolor",l.getColor().getRGBColorComponents(null));
//        map.put("from",new float[] {0f,0f,0f});
        if (metric != Pn.EUCLIDEAN)	{
        	map.put("signature", new Float(metric));
     	   lightname = "noneuclideanlight";
        }
        else if (shadowEnabled){
        	map.put("string shadowname", "raytrace");
        }
	}
    public void visit(PointLight l) {
       ri.transformBegin();
       HashMap<String, Object> map =new HashMap<String, Object>();
       handleCommon(l, map);
       map.put("from",new float[] {(float)dfrom[0], (float) dfrom[1], (float) dfrom[2]});
//       ri.concatTransform(fCurrentTrafo);
       if (metric == Pn.EUCLIDEAN)	
    	   lightname = shadowEnabled ? "shadowpoint": "pointlight";
       ri.lightSource(lightname,map);
        
       ri.transformEnd();
    }
    public void visit(SpotLight l) {
        ri.transformBegin();
        HashMap<String, Object> map =new HashMap<String, Object>();
        handleCommon(l, map);
//        ri.concatTransform(fCurrentTrafo);
//        map.put("from",new float[] {0f,0f,0f});
        map.put("from",new float[] {(float)dfrom[0], (float) dfrom[1], (float) dfrom[2]});
        map.put("to",new float[] {(float)dto[0], (float) dto[1], (float) dto[2]});
        map.put("coneangle",new Float(l.getConeAngle()));
        map.put("conedeltaangle",new Float(l.getConeDeltaAngle()));
        map.put("beamdistribution",new Float(l.getDistribution()));
        if(ribv.fullSpotLight ) {
            map.put("float a0", new Float(l.getFalloffA0()));
            map.put("float a1", new Float(l.getFalloffA1()));
            map.put("float a2", new Float(l.getFalloffA2()));
            ri.lightSource("spotlightFalloff",map);
        } else
        if (metric == Pn.EUCLIDEAN)	
        	lightname = shadowEnabled ? "shadowspot" : "spotlight";
        ri.lightSource(lightname,map);
         
        ri.transformEnd();
        //super.visit(l);
    }

}
