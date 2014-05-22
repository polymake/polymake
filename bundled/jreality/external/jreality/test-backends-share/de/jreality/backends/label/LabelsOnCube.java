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

import java.awt.Color;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.ShaderUtility;

public class LabelsOnCube {

    
    public static void label(IndexedFaceSet ps) {
      int n=ps.getNumPoints();
      String[] labels=new String[n];
      for (int i = 0; i<n; i++) labels[i] = "Point "+i;
      ps.setVertexAttributes(Attribute.LABELS, StorageModel.STRING_ARRAY.createReadOnly(labels));

      n=ps.getNumEdges();
      labels=new String[n];
      for (int i = 0; i<n; i++) labels[i] = "Edge "+i;
      ps.setEdgeAttributes(Attribute.LABELS, StorageModel.STRING_ARRAY.createReadOnly(labels));

      n=ps.getNumFaces();
      labels=new String[n];
      for (int i = 0; i<n; i++) labels[i] = "Face "+i;
      ps.setFaceAttributes(Attribute.LABELS, StorageModel.STRING_ARRAY.createReadOnly(labels));
}

	/**
	 * @param args
	 */
	public static void main(String[] args) {

    IndexedFaceSet ifs = Primitives.cube();
    IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs);
    IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(ifs);
    //ifs = new CatenoidHelicoid(7);
		label(ifs);
    
    SceneGraphComponent cmp = new SceneGraphComponent();
    Appearance a = new Appearance();
    cmp.setAppearance(a);
    cmp.setGeometry(ifs);
    
    DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(a, false);
    DefaultTextShader pts = (DefaultTextShader) ((DefaultPointShader)dgs.getPointShader()).getTextShader();
    DefaultTextShader ets = (DefaultTextShader) ((DefaultLineShader)dgs.getLineShader()).getTextShader();
    DefaultTextShader fts = (DefaultTextShader) ((DefaultPolygonShader)dgs.getPolygonShader()).getTextShader();
    
    pts.setDiffuseColor(Color.blue);
    ets.setDiffuseColor(Color.orange);
    fts.setDiffuseColor(Color.green);
    
    Double scale = new Double(0.01);
    pts.setScale(scale);
    ets.setScale(scale);
    fts.setScale(scale);
    
    double[] offset = new double[]{-.1,0,0.3};
    pts.setOffset(offset);
    ets.setOffset(offset);
    fts.setOffset(offset);
    
    dgs.setShowPoints(true);

    JRViewer v = new JRViewer();
	v.addBasicUI();
	v.setContent(cmp);
	v.registerPlugin(new ContentAppearance());
	v.registerPlugin(new ContentLoader());
	v.registerPlugin(new ContentTools());
	v.startup();
	}

}