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


package de.jreality.geometry;

import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphPathObserver;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;


class CoordinateSystemBeautifier extends AbstractTool {

	private static InputSlot evolutionSlot = InputSlot.getDevice("SystemTime");
	
	private CoordinateSystemFactory factory = null;
	
  
	public CoordinateSystemBeautifier(CoordinateSystemFactory factory) {
		this.factory = factory;
		addCurrentSlot(evolutionSlot, "Need notification to add path listeners.");
	}

  
	public void perform(ToolContext tc) {
		//add SceneGraphPathObserver to RootToToolPath
		//and only perform something if TransformationEvent is thrown

    //initialize paths
		final SceneGraphPath rootToToolPath = tc.getRootToToolComponent();
		final SceneGraphPath cameraToRootPath = tc.getViewer().getCameraPath();
		//add path observer to path from root to tool
    SceneGraphPathObserver opObserver = new SceneGraphPathObserver(rootToToolPath);
    opObserver.addTransformationListener(new TransformationListener(){
    	public void transformationMatrixChanged(TransformationEvent ev){
			//get transformation from camera to tool
    		double[] rootToTool = rootToToolPath.getInverseMatrix(null);
			double[] cameraToRoot = cameraToRootPath.getMatrix(null);
			double[] cameraToTool = Rn.times(null, rootToTool, cameraToRoot);
			//update box
			factory.updateBox(cameraToTool);
    	}
    });
    //update box initially
    opObserver.transformationMatrixChanged(null);

    removeCurrentSlot(evolutionSlot);
	}

  
  public String getDescription() {
    return "CoordinateSystemBeautifier";
  }
}