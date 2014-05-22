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

import java.io.IOException;
import java.io.LineNumberReader;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import de.jreality.geometry.PointSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.Input;


/**
 * Simple reader for borehole measurement data files.
 * 
 * @author msommer
 */
public class ReaderLAS extends AbstractReader {

	//column index starting with 0
	public final int z = 1;  //tvd (total vertical depth)
	public final int x = 4;  //coordNS
	public final int y = 5;  //coordEW
	public final int phi = 2;  //inclination
	public final int theta = 3;  //azimuth
	
	private boolean hasHeader = true;
	

	public ReaderLAS() {
		root = new SceneGraphComponent("borehole");
		Appearance app = new Appearance();
		app.setAttribute(CommonAttributes.SPHERES_DRAW, true);
		app.setAttribute(CommonAttributes.POINT_RADIUS, 2.0);
//		app.setAttribute(CommonAttributes.PICKABLE, false);
//		app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.PICKABLE, false);
//		app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_SIZE, 30.);
		app.setAttribute(CommonAttributes.VERTEX_DRAW, true);
//		app.setAttribute(CommonAttributes.EDGE_DRAW, true);
//		app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.white);
		root.setAppearance(app);
		MatrixBuilder.euclidean().rotate(Math.PI, 0, 1, 0).assignTo(root);
		MatrixBuilder.euclidean(root).rotate(Math.PI/2.0, 0, 0, 1).assignTo(root);
	}

	
	@Override
	public void setInput(Input input) throws IOException {
		super.setInput(input);
		read();
	}

	
	private void read() throws IOException {
		
		LineNumberReader r = new LineNumberReader(input.getReader());
		String line = hasHeader? r.readLine() : "";  //skip header if existing
		
		List<Double> lx = new ArrayList<Double>();
		List<Double> ly = new ArrayList<Double>();
		List<Double> lz = new ArrayList<Double>();
		List<Double> lphi = new ArrayList<Double>();
		List<Double> ltheta = new ArrayList<Double>();
		
		StringTokenizer st = new StringTokenizer(line);
		
//		for (int j = 0; j < 3; j++) {
		while ( (line=r.readLine())!=null ) {

			if (line.equals("")) continue;  //skip empty lines
			
			line = line.replace(',', '.');
			st = new StringTokenizer(line);
		    
			for (int i = 0; st.hasMoreTokens(); i++) {
				String token = st.nextToken();
				switch (i) {
				case x: lx.add(Double.parseDouble(token)); break;
				case y: ly.add(Double.parseDouble(token)); break;
				case z: lz.add(Double.parseDouble(token)); break;
				case phi: lphi.add(Double.parseDouble(token)); break;
				case theta: ltheta.add(Double.parseDouble(token)); break;
				}
			}
		}
		
		double[][] vertices = new double[lx.size()][3];
		for (int i = 1; i < lx.size(); i++)
			vertices[i] = new double[]{lx.get(i), ly.get(i), lz.get(i)};
//		for (int i = 0; i < vertices.length; i++) System.out.println(Arrays.toString(vertices[i]));
		
		//TODO: create cross-section around vertices and rotate with phi&theta
		

		PointSetFactory f = new PointSetFactory();
		f.setVertexCount(vertices.length);
		f.setVertexCoordinates(vertices);
		f.update();
		root.setGeometry(f.getPointSet());
	}
	
	
	public void setHasHeader(boolean hasHeader) {
		this.hasHeader = hasHeader;
	}
	
}