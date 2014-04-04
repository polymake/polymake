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


package de.jreality.scene.pick;

import de.jreality.examples.CatenoidHelicoid;
import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.Cylinder;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Sphere;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.PickShowTool;

public class TestBruteForcePicking implements Runnable{
	
  static public double[][] icoVerts3 =  {
    {0.850651026, 0, 0.525731027}, 
    {0.850651026, 0, -0.525731027}, 
    {0.525731027, 0.850651026, 0}, 
    {0.525731027, -0.850651026, 0.0}, 
    {0.0, -0.525731027, 0.850651026}, 
    {0.0, 0.525731027, 0.850651026}, 
    {-0.850651026, 0, -0.525731027}, 
    { -0.850651026, 0, 0.525731027}, 
    {-0.525731027, 0.850651026, 0}, 
    { 0.0, 0.525731027, -0.850651026}, 
    {0.0, -0.525731027, -0.850651026}, 
    {-0.525731027, -0.850651026, 0.0}};

// Don't remove: good for testing backend capability to deal with 4D vertices
static private double[][] icoVerts4 =  {
    {0.850651026, 0, 0.525731027, 0.5}, 
    {0.850651026, 0, -0.525731027, 1.0}, 
    {0.525731027, 0.850651026, 0, 1.0}, 
    {0.525731027, -0.850651026, 0.0, 1.0}, 
    {0.0, -0.525731027, 0.850651026, 1.0}, 
    {0.0, 0.525731027, 0.850651026, 1.0}, 
    {-0.850651026, 0, -0.525731027, 1.0}, 
    { -0.850651026, 0, 0.525731027, 1.0}, 
    {-0.525731027, 0.850651026, 0, 1.0}, 
    { 0.0, 0.525731027, -0.850651026, 1.0}, 
    {0.0, -0.525731027, -0.850651026, 1.0}, 
    {-0.525731027, -0.850651026, 0.0, 1.0}};

static private int[][] icoIndices = {
        {0, 1, 2},
        {0, 3, 1},
        {0, 4, 3},
        {0, 5, 4},
        {0, 2, 5},
        {6, 7, 8},
        {6, 8, 9},
        {6, 9, 10},
        {6, 10, 11},
        {6, 11, 7},
        {1, 3, 10},
        {3, 4, 11},
        {4, 5, 7},
        {5, 2, 8},
        {2, 1, 9},
        {7, 11, 4},
        {8, 7, 5},
        {9, 8, 2},
        {10, 9, 1},
        {11, 10, 3}};

  
	private CatenoidHelicoid ch;
	double t=0;
	double speed=0.001;
	int sleeptime=50;

  public static IndexedFaceSet ico(double[][] verts) {
    IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
    ifsf.setVertexCount(12);
    ifsf.setFaceCount(20);
    ifsf.setVertexCoordinates(verts);
    ifsf.setVertexNormals(icoVerts3);
    ifsf.setFaceIndices(icoIndices);
    ifsf.setGenerateEdgesFromFaces(true);
    ifsf.setGenerateFaceNormals(true);
    ifsf.update();
    return ifsf.getIndexedFaceSet();
    }

	public static void main(String[] args) {

		TestBruteForcePicking test=new TestBruteForcePicking();		
		//test.animate();
	}
	
	public TestBruteForcePicking(){
		SceneGraphComponent geoNode=new  SceneGraphComponent();
		
    SceneGraphComponent cmpCH = new SceneGraphComponent();
    SceneGraphComponent cmpIco = new SceneGraphComponent();
		SceneGraphComponent cmpSp = new SceneGraphComponent();
		SceneGraphComponent cmpCyl = new SceneGraphComponent();
		
		ch = new CatenoidHelicoid(20);
		
		Sphere sp = new Sphere();
		Cylinder cyl = new Cylinder();
		
		MatrixBuilder.euclidean().translate(1,1,1).scale(1.5,0.5,2).assignTo(cmpSp);
		MatrixBuilder.euclidean().rotate(Math.PI/2,1,0,0).assignTo(cmpCyl);
		
    MatrixBuilder.euclidean().translate(1,1,-1).scale(1.5,0.5,2).assignTo(cmpIco);
    
    cmpCH.setGeometry(ch);
    cmpIco.setGeometry(ico(icoVerts4));
		cmpSp.setGeometry(sp);
		cmpCyl.setGeometry(cyl);
		
		geoNode.addChild(cmpCH);
    geoNode.addChild(cmpIco);
		geoNode.addChild(cmpSp);
		geoNode.addChild(cmpCyl);
		
	PickShowTool pst = new PickShowTool();
	pst.setRadius(0.07);
    geoNode.addTool(pst);
    geoNode.setAppearance(new Appearance());
    geoNode.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, true);
    geoNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.04);
    JRViewer v = new JRViewer();
	v.addBasicUI();
	v.setContent(geoNode);
	v.registerPlugin(new ContentAppearance());
	v.registerPlugin(new ContentLoader());
	v.registerPlugin(new ContentTools());
	v.startup();	
	}

	private void animate() {
		Thread th = new Thread(this);
		th.start();		
	}
	
	public void run(){
		
		while (true) {
			t=t+speed;
			if(t>=1){ t=0;}					
			ch.setAlpha(t*Math.PI*2);
	    	try {
	    		Thread.sleep(sleeptime);
	    	} catch (InterruptedException e){
	    	}
	        
	     }		
	}
	
	
}
