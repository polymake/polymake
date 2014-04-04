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


package de.jreality.backends.label;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.SystemProperties;

public class LabelsOnPointSet {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		int numPoints = 100;
		double[] points = new double[3*numPoints];
		String[] labels = new String[numPoints];
		for (int i = 0; i < numPoints; i++) {
			points[3*i] = Math.random()*10;
			points[3*i+1] = Math.random()*10;
			points[3*i+2] = Math.random()*10;
			labels[i] = "This is point "+i; 
		}
		PointSet pSet = new PointSet();
		pSet.setVertexCountAndAttributes(Attribute.COORDINATES, StorageModel.DOUBLE3_INLINED.createReadOnly(points));
		pSet.setVertexAttributes(Attribute.LABELS, StorageModel.STRING_ARRAY.createReadOnly(labels));
		
		Appearance a = new Appearance();
		a.setAttribute(CommonAttributes.VERTEX_DRAW,true);
		a.setAttribute(CommonAttributes.EDGE_DRAW,false);
		a.setAttribute("pointShader.scale", .01);
		a.setAttribute("pointShader.offset", new double[]{.2,0,0});
		SceneGraphComponent sgc = new SceneGraphComponent();
		sgc.setAppearance(a);
		sgc.setGeometry(pSet);
		
		System.setProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_SOFT+" "+SystemProperties.VIEWER_DEFAULT_JOGL+" "+SystemProperties.VIEWER_DEFAULT_JOGL3); // de.jreality.portal.DesktopPortalViewer");

	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(sgc);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
	}

}
