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


package de.jreality.tools;

import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.EDGE_DRAW;
import static de.jreality.shader.CommonAttributes.METRIC;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.TRANSPARENCY;
import static de.jreality.shader.CommonAttributes.TRANSPARENCY_ENABLED;
import static de.jreality.shader.CommonAttributes.VERTEX_DRAW;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Sphere;
import de.jreality.scene.Transformation;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.util.SceneGraphUtility;

public class TrackballRotateTool extends AbstractTool {

    static InputSlot activationSlot = InputSlot.getDevice("RotateActivation");
    static InputSlot evolutionSlot = InputSlot.getDevice("TrackballTransformation");
    static InputSlot camPath = InputSlot.getDevice("WorldToCamera");

    public TrackballRotateTool() {
        super(activationSlot);
        addCurrentSlot(evolutionSlot);
        addCurrentSlot(camPath);
    }

    transient SceneGraphComponent comp;
    int type = 0;
    transient EffectiveAppearance eap;
    boolean allesInOrdnung = true, positiveOrientation = true;
    transient private int metric;
    transient Matrix result = new Matrix();
    transient Matrix evolution = new Matrix();
    static double[][] fixedAxes = {{0,0,1},{1,0,0},{0,1,0}};

    public void activate(ToolContext tc) {
    	System.err.println("Activating trackball rotate tool");
        comp = tc.getRootToToolComponent().getLastComponent();
        if (comp.getTransformation() == null)
        	comp.setTransformation(new Transformation());
        if (eap == null || !EffectiveAppearance.matches(eap, tc.getRootToToolComponent())) {
            eap = EffectiveAppearance.create(tc.getRootToToolComponent());
        }
        allesInOrdnung = true;
        SceneGraphComponent leaf = tc.getRootToLocal().getLastComponent();
        if (leaf.getName() == "band") {
        	SceneGraphPath shorter = tc.getRootToLocal().popNew();
        	//shorter.pop();
        	leaf = shorter.getLastComponent();
        }
        String name = leaf.getName();
        if (name.equals("ball")) type = 0;
        else if (name.equals("xyBand")) type = 1;
        else if (name.equals("yzBand")) type = 2;
        else if (name.equals("zxBand")) type = 3;
        else {
        	allesInOrdnung = false;
        }
        System.err.println("Type is "+type);
        metric = eap.getAttribute(METRIC, Pn.EUCLIDEAN);
    }
    
    public void perform(ToolContext tc) {
    	if (!allesInOrdnung) return;
        Matrix root2Tool = new Matrix(tc.getRootToToolComponent().getInverseMatrix(null)); //(moveChildren ? tc.getRootToLocal():tc.getRootToToolComponent()).getInverseMatrix(null)); 
        root2Tool.assignFrom(P3.extractOrientationMatrix(null, root2Tool.getArray(), P3.originP3, metric));
        evolution.assignFrom(tc.getTransformationMatrix(evolutionSlot));
        evolution.conjugateBy(root2Tool);
       if (type > 0)	{
        	FactoredMatrix fm = new FactoredMatrix(evolution.getArray());
        	fm.update();
        	double angle = fm.getRotationAngle();
        	double[] axis = fm.getRotationAxis();
        	double[] newAxis = new double[3];
        	double[] fa = fixedAxes[type-1];
            // now clean up the rotation based on the type field
           	Rn.projectOnto(newAxis, axis, fa);
            MatrixBuilder.euclidean().rotate(angle, newAxis).assignTo(evolution);
        }
        comp.getTransformation().multiplyOnRight(evolution.getArray());
        tc.getViewer().renderAsync();
    }

    double radius =  1;
    SceneGraphComponent allSGC, ballSGC, xyBand, yzBand, zxBand;
    public SceneGraphComponent getTrackball()		{
    	allSGC = SceneGraphUtility.createFullSceneGraphComponent("trackball repn");
    	MatrixBuilder.euclidean().scale(radius).assignTo(allSGC);
    	allSGC.getAppearance().setAttribute(EDGE_DRAW, false);
    	allSGC.getAppearance().setAttribute(VERTEX_DRAW, false);
    	ballSGC = SceneGraphUtility.createFullSceneGraphComponent("ball");
       	ballSGC.setGeometry(new Sphere());
       	ballSGC.getAppearance().setAttribute(TRANSPARENCY_ENABLED, true);
       	ballSGC.getAppearance().setAttribute(TRANSPARENCY, 0.95);
       	MatrixBuilder.euclidean().scale(.8).assignTo(ballSGC);
       	SceneGraphComponent band = new SceneGraphComponent("band");
       	MatrixBuilder.euclidean().scale(1,1,.1).assignTo(band);
       	band.setGeometry(Primitives.cylinder(50));
    	xyBand = SceneGraphUtility.createFullSceneGraphComponent("xyBand");
    	xyBand.getAppearance().setAttribute(POLYGON_SHADER+"."+DIFFUSE_COLOR, Color.blue);
      	xyBand.addChild(band);
       	yzBand = SceneGraphUtility.createFullSceneGraphComponent("yzBand");
       	yzBand.getAppearance().setAttribute(POLYGON_SHADER+"."+DIFFUSE_COLOR, Color.red);
       	MatrixBuilder.euclidean().rotateY(Math.PI/2).assignTo(yzBand);
       	yzBand.addChild(band);
       	zxBand = SceneGraphUtility.createFullSceneGraphComponent("zxBand");
       	zxBand.getAppearance().setAttribute(POLYGON_SHADER+"."+DIFFUSE_COLOR, Color.green);
      	MatrixBuilder.euclidean().rotateX(Math.PI/2).assignTo(zxBand);
      	zxBand.addChild(band);
      	allSGC.addChildren(ballSGC, xyBand, yzBand, zxBand);
     	return allSGC;
     }

 
}
