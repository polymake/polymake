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


package de.jreality.reader;

import java.awt.Color;
import java.io.IOException;
import java.io.LineNumberReader;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.geometry.GeometryUtility;
import de.jreality.geometry.PointSetFactory;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.Input;
import de.jreality.util.Rectangle3D;

/**
 *
 * simple reader for the PTS file format. 
 * 
 * @author Steffen Weissmann
 *
 */
public class ReaderPTS extends AbstractReader {

  public ReaderPTS() {
    root = new SceneGraphComponent();
    Appearance app = new Appearance();
    app.setAttribute(CommonAttributes.SPHERES_DRAW, false);
    app.setAttribute(CommonAttributes.PICKABLE, false);
    app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.PICKABLE, false);
    app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_SIZE, 30.);
    app.setAttribute(CommonAttributes.VERTEX_DRAW, true);
    app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.white);
    root.setAppearance(app);
  }

  public void setInput(Input input) throws IOException {
    super.setInput(input);
    load();
  }

  private void load() throws IOException {
	int skip=2;
    LineNumberReader r = new LineNumberReader(input.getReader());
    
    String l = null;
    while ((l=r.readLine().trim()).startsWith("#"));
    
    int pointCount = Integer.parseInt(l)/(skip+1);
    double[] points = new double[pointCount*3];
    double[] colors = new double[pointCount*3];
    
    int index=0;
    while ((l=r.readLine())!=null) {
    	if (index==pointCount) break;
    	for (int i = 0; i < skip; i++) r.readLine(); 
    	String[] split = l.split(" ");
    	if (split.length!=7) continue;
    	points[3*index]=Double.parseDouble(split[0]);
    	points[3*index+1]=Double.parseDouble(split[1]);
    	points[3*index+2]=Double.parseDouble(split[2]);
    	
    	colors[3*index]=Double.parseDouble(split[4])/255;
    	colors[3*index+1]=Double.parseDouble(split[5])/255;
    	colors[3*index+2]=Double.parseDouble(split[6])/255;
    	index++;
    }
    
    PointSetFactory psf = new PointSetFactory();
    psf.setVertexCount(pointCount);
    psf.setVertexCoordinates(points);
    psf.setVertexColors(colors);
    psf.update();
    Rectangle3D bb = BoundingBoxUtility.calculateBoundingBox(psf.getPointSet());
    psf.getPointSet().setGeometryAttributes(GeometryUtility.BOUNDING_BOX, bb);
    root.setGeometry(psf.getPointSet());
  }

}
