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
import java.util.LinkedList;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;

/**
 *
 * simple reader for the TXT file format (3d scan data)
 *
 * @author Philipp Beckmann
 *
 */
public class ReaderSTL extends AbstractReader {

  public ReaderSTL() {
    root = new SceneGraphComponent();
  }

  public void setInput(Input input) throws IOException {
    super.setInput(input);
    try {
      load();
    } catch (Exception e) {
      // TODO Auto-generated catch block
      e.printStackTrace();
    }
  }

  private void load() throws Exception {

    LineNumberReader r  = new LineNumberReader(input.getReader());

    PointOctree po = new PointOctree();
    LinkedList<double[]> pts = new LinkedList<double[]>();
    LinkedList<int[]> face_indices = new LinkedList<int[]>();

    String line;

    // read until "solid"
    //while (( !"solid".equals(r.readLine().trim())));
    while ((!r.readLine().trim().startsWith("solid")));

    int faceCount = 0;

    while (true) {
	    line = r.readLine().trim();

	    // handle lines that are split with \
	    while (line.endsWith("\\")) {
	    	line = line.substring(0, line.length()-2);
	    	line += " "+r.readLine().trim();
	    }
	    
	    //if ("endsolid".equals(line)) {
	    if ((line.startsWith("end solid")) || (line.startsWith("endsolid"))) {
	    	System.out.println("solid end: "+faceCount+" facets read.");
	    	break; // file finished
	    }

	    assert (line.startsWith("facet normal"));
	    faceCount++;
		String[] split = line.trim().split("\\s+");
	    // note that "facet normal" is stored at pos 0 and 1!
		double[] normal = new double[3];
		normal[0] = Double.parseDouble(split[2]);
		normal[1] = Double.parseDouble(split[3]);
		normal[2] = Double.parseDouble(split[4]);

	    line = r.readLine().trim();
	    assert (line.startsWith("outer loop"));

	    LinkedList<Integer> face_idx = new LinkedList<Integer>();

	    // read n consecutive lines of vertices: collect each face:
	    while ((line = r.readLine().trim()).startsWith("vertex")) {
	    	String[] numbers = line.trim().split("\\s+");

	        // note that "vertex" is stored at pos 0!
	    	double coord[]  = new double[3];
	    	coord[0] = Double.parseDouble(numbers[1]);
	    	coord[1] = Double.parseDouble(numbers[2]);
	    	coord[2] = Double.parseDouble(numbers[3]);

	        if ( po.insert(coord[0], coord[1], coord[2]) != null ) {
	          pts.add( coord );
	        }
	        PointOctree.Node n;
	        if ( (n = po.find(coord[0], coord[1], coord[2])) == null)
	          throw new RuntimeException();
	        face_idx.add(n.index());
	      } // face finished

	    assert (line.equals("endloop"));

	    line = r.readLine().trim();
	    assert (line.equals("endfacet"));

	    int[] face = new int[face_idx.size()];
	    for (int i=0; i<face.length;i++) face[i]=face_idx.get(i);
	    face_indices.add(face);
    }

    double vertices[][] = pts.toArray(new double[0][0]);
    int faces[][]       = face_indices.toArray(new int[0][0]);

    IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
    ifsf.setVertexCount(vertices.length);
    ifsf.setFaceCount(faces.length);
    ifsf.setVertexCoordinates(vertices);
    ifsf.setFaceIndices(faces);
    ifsf.setGenerateEdgesFromFaces( true );
    ifsf.setGenerateFaceNormals( true );
    ifsf.update();
    IndexedFaceSet ifs = ifsf.getIndexedFaceSet();
    root.setGeometry(ifs);
  }

}
