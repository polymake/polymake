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
import java.util.HashMap;
import java.util.List;
import java.util.StringTokenizer;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;

/**
 *
 * simple reader for the Evolver FE file format. 
 * 
 * 
 * @author weissman
 *
 */
public class ReaderFE extends AbstractReader {

	  private List<double[]> v=new ArrayList<double[]>(1000);
	  private HashMap<Integer, Integer> vertexIndices=new HashMap<Integer, Integer>();
	  private List<int[]> edges=new ArrayList<int[]>(1000);
	  private HashMap<Integer, Integer> edgeIndices=new HashMap<Integer, Integer>();
	  private List<int[]> faces=new ArrayList<int[]>(1000);

	  IndexedFaceSetFactory ifsf=new IndexedFaceSetFactory();
	  
  public ReaderFE() {
    root = new SceneGraphComponent();
    root.setGeometry(ifsf.getGeometry());
    ifsf.setGenerateFaceNormals(true);
    ifsf.setGenerateVertexNormals(true);
  }

  public void setInput(Input input) throws IOException {
    super.setInput(input);
    load();
  }

  private void load() throws IOException {
    LineNumberReader lnr = new LineNumberReader(input.getReader());
    String line = null;
    while ((line=lnr.readLine()) != null) {
    	if (line.startsWith("vertices")) break;
    }
    // read vertices
    while ((line=lnr.readLine()) != null && !line.trim().equals("")) {
    	StringTokenizer tok = new StringTokenizer(line);
    	int index=Integer.parseInt(tok.nextToken());
    	double x=Double.parseDouble(tok.nextToken());
    	double y=Double.parseDouble(tok.nextToken());
    	double z=Double.parseDouble(tok.nextToken());
    	vertexIndices.put(index, v.size());
    	double[] vertex=new double[]{x,y,z};
    	v.add(vertex);
    }
    
    while ((line=lnr.readLine()) != null) {
    	if (line.startsWith("edges")) break;
    }
    // read edges
    while ((line=lnr.readLine()) != null && !line.trim().equals("")) {
    	StringTokenizer tok = new StringTokenizer(line);
    	int index=Integer.parseInt(tok.nextToken());
    	int sv=Integer.parseInt(tok.nextToken());
    	int ev=Integer.parseInt(tok.nextToken());
    	edgeIndices.put(index, edges.size());
    	edges.add(new int[]{vertexIndices.get(sv),vertexIndices.get(ev)});
    }
    
    while ((line=lnr.readLine()) != null) {
    	if (line.startsWith("faces") || line.startsWith("facets")) break;
    }
    // read faces
    while ((line=lnr.readLine()) != null && !line.trim().equals("")) {
    	StringTokenizer tok = new StringTokenizer(line);
    	int index=Integer.parseInt(tok.nextToken());
    	List<Integer> edgs = new ArrayList<Integer>();
    	loop: while (tok.hasMoreTokens()) {
    		String token = tok.nextToken();
    		try {
    			int ei=Integer.parseInt(token);
    			edgs.add(ei);
    		} catch (NumberFormatException nfe) {
    			break loop;
    		}
    	}
    	int[] face = new int[edgs.size()];
    	int cnt=0;
    	for (int ei : edgs) {
    		boolean orientation = ei > 0;
    		int evolverEdgeIndex=orientation ? ei : -ei;
    		int edgeIndex = edgeIndices.get(evolverEdgeIndex);
    		int[] edge = edges.get(edgeIndex);
    		face[cnt++] = edge[orientation ? 1 : 0];
    	}
    	faces.add(face);
    }
    
    ifsf.setVertexCount(v.size());
    ifsf.setEdgeCount(edges.size());
    ifsf.setFaceCount(faces.size());
    ifsf.setVertexCoordinates(v.toArray(new double[0][]));
    ifsf.setEdgeIndices(edges.toArray(new int[0][]));
    ifsf.setFaceIndices(faces.toArray(new int[0][]));
    ifsf.update();
  }

}
