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


package de.jreality.examples;

import java.awt.Color;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ShaderUtility;

public class SimpleExample {

  /**
   * @param args
   */
  public static void main(String[] args) {
    double[][] points = {{0, 1, 0},{1, 0, 0},{0, -1, 0}, {-1, 0, 0}};
    int[][] faces = {{0, 1, 2, 3}};

    IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();

    ifsf.setGenerateEdgesFromFaces(true); // or false
    ifsf.setGenerateFaceNormals(true);
    ifsf.setGenerateVertexNormals(false);

    ifsf.setVertexCount(4);
    ifsf.setFaceCount(1);
    ifsf.setVertexCoordinates(points);
    ifsf.setFaceIndices(faces);

    ifsf.update();

    IndexedFaceSet faceSet = ifsf.getIndexedFaceSet();
    
    //ViewerApp.display(faceSet);
    
    Appearance app = new Appearance();
//    app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.green);
//    app.setAttribute(CommonAttributes.VERTEX_DRAW, true);
//    app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_RADIUS, 0.07);
//    app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.yellow);
//    app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBE_RADIUS, 0.07);
    
    DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(app, true);
    dgs.setShowPoints(Boolean.TRUE);
    DefaultPolygonShader polyShader = (DefaultPolygonShader) dgs.getPolygonShader();
    DefaultPointShader pointShader = (DefaultPointShader) dgs.getPointShader();
    DefaultLineShader lineShader = (DefaultLineShader) dgs.getLineShader();
    
    polyShader.setDiffuseColor(Color.green);
    lineShader.setDiffuseColor(Color.yellow);
    lineShader.setTubeRadius(new Double(0.07));
    pointShader.setPointRadius(new Double(0.07));
    
    SceneGraphComponent cmp = new SceneGraphComponent();
    
    cmp.setAppearance(app);
    cmp.setGeometry(faceSet);
    
    //System.setProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_SOFT);
	JRViewer v = new JRViewer();
	v.addBasicUI();
	v.setContent(cmp);
	v.registerPlugin(new ContentAppearance());
	v.registerPlugin(new ContentLoader());
	v.registerPlugin(new ContentTools());
	v.startup();
  }

}
