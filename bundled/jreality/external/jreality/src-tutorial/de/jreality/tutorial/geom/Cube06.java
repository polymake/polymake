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


package de.jreality.tutorial.geom;

import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.LINE_SHADER;
import static de.jreality.shader.CommonAttributes.POINT_SHADER;

import java.awt.Color;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;

/**
 * This example builds on {@link Cube05}. It shows how to set the edge list by hand, and
 * also a JOGL backend feature to allow display of only certain vertices
 * (in this case, only the vertices appearing in the edge list).
 * 
 * @author Charles Gunn
 *
 */public class Cube06 {
  
  public static void main(String[] args) {
    
    IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
    
    double [][] vertices  = new double[][] {
      {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
      {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
    };
    
    int [][] faceIndices = new int [][] {
      {0, 1, 2, 3}, {7, 6, 5, 4}, {0, 1, 5, 4}, 
      {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7} 
    };
    
    int [][] edgeIndices = new int [][] {
    		{0,1},{1,2},{2,6},{6,7},{7,4},{4,0}
    };
    
    int [] pointIndices = {1,1,1,0,1,0,1,1};
    Color[] faceColors = new Color[]{
      Color.BLUE, Color.BLUE, Color.GREEN, Color.GREEN, Color.RED, Color.RED 
    };
    ifsf.setVertexCount( vertices.length );
    ifsf.setVertexCoordinates( vertices );
    ifsf.setFaceCount( faceIndices.length);
    ifsf.setFaceIndices( faceIndices ); 
    ifsf.setFaceColors(faceColors);    
    ifsf.setEdgeCount(edgeIndices.length);
    ifsf.setEdgeIndices(edgeIndices);
    ifsf.setGenerateFaceNormals( true );
    ifsf.update();
    
    ifsf.getIndexedFaceSet().setVertexAttributes(Attribute.INDICES,
    		StorageModel.INT_ARRAY.createReadOnly(pointIndices));
    SceneGraphComponent sgc = new SceneGraphComponent();
    sgc.setGeometry(ifsf.getIndexedFaceSet());
    
    Appearance app = new Appearance();
//    app.setAttribute(VERTEX_DRAW, false);
    app.setAttribute(LINE_SHADER+"."+DIFFUSE_COLOR, Color.yellow);
    app.setAttribute(POINT_SHADER+"."+DIFFUSE_COLOR, Color.yellow);
    sgc.setAppearance(app);
    
    JRViewer.display(sgc);
  }
}