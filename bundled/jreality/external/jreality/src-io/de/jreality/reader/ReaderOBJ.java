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

import static java.io.StreamTokenizer.TT_EOF;
import static java.io.StreamTokenizer.TT_EOL;
import static java.io.StreamTokenizer.TT_NUMBER;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.StreamTokenizer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;

/**
 * 
 * simple reader for the OBJ file format.
 * 
 * smoothening group support is poor (what is a smoothining group?)
 * 
 * @author weissman
 * 
 */
public class ReaderOBJ extends AbstractReader {

	private HashMap<String, Appearance> materials = new HashMap<String, Appearance>();
	private HashMap<String, Group> groups = new HashMap<String, Group>();
	private List<double[]> v, vNorms, vTexs;
	private LinkedList<String> currentGroups;

	public ReaderOBJ() {
		v = new ArrayList<double[]>(1000);
		vNorms = new ArrayList<double[]>(1000);
		vTexs = new ArrayList<double[]>(1000);
		currentGroups = new LinkedList<String>();
		currentGroups.add("default");
		groups.put("default", new Group("default"));
		root = new SceneGraphComponent();
		root.setAppearance(ParserMTL.createDefault());
	}

	public void setInput(Input input) throws IOException {
		super.setInput(input);
		load();
	}

	private StreamTokenizer globalSyntax(StreamTokenizer st) {
		st.resetSyntax();
		st.eolIsSignificant(true);
		st.wordChars('0', '9');
		st.wordChars('A', 'Z');
		st.wordChars('a', 'z');
		st.wordChars('_', '_');
		st.wordChars('.', '.');
		st.wordChars('-', '-');
		st.wordChars('+', '+');
		st.wordChars('\u00A0', '\u00FF');
		st.whitespaceChars('\u0000', '\u0020');
		st.commentChar('#');
		st.ordinaryChar('/');
		st.parseNumbers();
		return st;
	}

	private StreamTokenizer filenameSyntax(StreamTokenizer st) {
		st.resetSyntax();
		st.eolIsSignificant(true);
		st.wordChars('0', '9');
		st.wordChars('A', 'Z');
		st.wordChars('a', 'z');
		st.wordChars('_', '_');
		st.wordChars('.', '.');
		st.wordChars('-', '-');
		st.wordChars('+', '+');
		st.wordChars('\u00A0', '\u00FF');
		st.whitespaceChars('\u0000', '\u0020');
		st.commentChar('#');
		st.ordinaryChar('/');
		st.parseNumbers();
		return st;
	}

	private void load() throws IOException {
		StreamTokenizer st = new StreamTokenizer(input.getReader());
		globalSyntax(st);
		while (st.nextToken() != StreamTokenizer.TT_EOF) {
			if (st.ttype == StreamTokenizer.TT_WORD) {
				String word = st.sval;
				if (word.equalsIgnoreCase("v")) { // vertex
					addVertex(st);
					continue;
				}
				if (word.equalsIgnoreCase("vp")) { // vertex parameter data
					ignoreTag(st);
					continue;
				}
				if (word.equalsIgnoreCase("vn")) { // vertex normal
					addVertexNormal(st);
					continue;
				}
				if (word.equalsIgnoreCase("vt")) { // vertex texture coordinate
					addVertexTextureCoordinate(st);
					continue;
				}
				if (word.equalsIgnoreCase("g")) { // grouping
					addGroup(st);
					continue;
				}
				if (word.equalsIgnoreCase("s")) { // smoothening group
					setSmootheningGroup(st);
					continue;
				}
				if (word.equalsIgnoreCase("p")) { // polygon v1 v2 v3 ...
					ignoreTag(st);
					continue;
				}
				if (word.equalsIgnoreCase("l")) { // lines v1/vt1 v2/vt2 ...
					ignoreTag(st);
					continue;
				}
				if (word.equalsIgnoreCase("f")) { // facet v1/vt1/vn1 v2/vt2/vn2
					addFace(st);
					continue;
				}
				if (word.equalsIgnoreCase("mtllib")) { // facet v1/vt1/vn1
					addMaterial(st);
					continue;
				}
				if (word.equalsIgnoreCase("usemtl")) { // facet v1/vt1/vn1
					setCurrentMaterial(st);
					continue;
				}
				LoggingSystem.getLogger(this).fine("unhandled tag: " + word);
				while (st.nextToken() != StreamTokenizer.TT_EOL) {
					if (st.ttype == StreamTokenizer.TT_NUMBER)
						LoggingSystem.getLogger(this).fine("" + st.nval);
					else
						LoggingSystem.getLogger(this).fine(st.sval);
				}
				LoggingSystem.getLogger(this).fine(
						"unhandled tag: " + word + " end");
			}
		}
		for (Group g : groups.values()) {
			if (g.hasGeometry()) {
				root.addChild(g.createComponent());
			} else {
				LoggingSystem.getLogger(this).fine(
						"Ignoring group " + g.name + " [has no geometry]");
			}
		}
	}

	private boolean smoothShading = false;

	private void setSmootheningGroup(StreamTokenizer st) throws IOException {
		st.nextToken();
		if (st.ttype == StreamTokenizer.TT_NUMBER) {
			smoothShading = (st.nval > 0);
		}
		if (st.ttype == StreamTokenizer.TT_WORD) {
			if ("off".equals(st.sval))
				smoothShading = false;
		}
		while (st.nextToken() != StreamTokenizer.TT_EOL)
			;
		for (Iterator i = currentGroups.iterator(); i.hasNext();) {
			// System.out.println("Appearance ["+matName+"]: "+materials.get(matName));
			Group current = (Group) (groups.get(i.next()));
			current.setSmoothening(smoothShading);
		}
	}

	private void ignoreTag(StreamTokenizer st) throws IOException {
		while (st.nextToken() != StreamTokenizer.TT_EOL)
			;
	}

	private void addVertex(StreamTokenizer st) throws IOException {
		List<Double> cList = new LinkedList<Double>();
		st.nextToken();
		while (st.ttype == TT_NUMBER || st.ttype == '\\') {
			if (st.ttype == '\\') {
				st.nextToken(); // the EOL
				st.nextToken(); // continue parsing in the next line
				continue;
			} 
			st.pushBack();
			cList.add(ParserUtil.parseNumber(st));
			st.nextToken();
		}
		st.pushBack();
		double[] coords = new double[cList.size()];
		for (int i = 0; i < coords.length; i++) {
			coords[i] = cList.get(i);
		}
		v.add(coords);
	}

	private void addVertexTextureCoordinate(StreamTokenizer st) throws IOException {
		List<Double> cList = new LinkedList<Double>();
		st.nextToken();
		while (st.ttype == TT_NUMBER || st.ttype == '\\') {
			if (st.ttype == '\\') {
				st.nextToken(); // the EOL
				st.nextToken(); // continue parsing in the next line
				continue;
			}
			st.pushBack();
			cList.add(ParserUtil.parseNumber(st));
			st.nextToken();
		}
		st.pushBack();
		double[] coords = new double[cList.size()];
		for (int i = 0; i < coords.length; i++) {
			coords[i] = cList.get(i);
		}
		vTexs.add(coords);
	}

	private void addVertexNormal(StreamTokenizer st) throws IOException {
		List<Double> cList = new LinkedList<Double>();
		st.nextToken();
		while (st.ttype == TT_NUMBER || st.ttype == '\\') {
			if (st.ttype == '\\') {
				st.nextToken(); // the EOL
				st.nextToken(); // continue parsing in the next line
				continue;
			}
			st.pushBack();
			cList.add(ParserUtil.parseNumber(st));
			st.nextToken();
		}
		st.pushBack();
		double[] coords = new double[cList.size()];
		for (int i = 0; i < coords.length; i++) {
			coords[i] = cList.get(i);
		}
		if (coords.length > 3) {
			System.err.println("vertex normal " + vNorms.size() + " has " + coords.length + " dimensions");
		}
		vNorms.add(coords);
	}

	private void addMaterial(StreamTokenizer st) throws IOException {
		filenameSyntax(st);
		while (st.nextToken() != StreamTokenizer.TT_EOL) {
			String fileName = st.sval;
			if (fileName == null)
				continue;
			try {
				List app = ParserMTL.readAppearences(input.resolveInput(fileName));
				for (Iterator i = app.iterator(); i.hasNext();) {
					Appearance a = (Appearance) i.next();
					materials.put(a.getName(), a);
				}
			} catch (FileNotFoundException fnfe) {
				LoggingSystem.getLogger(this).info(
						"couldnt find material file: " + fileName);
			}
		}
		globalSyntax(st);
	}

	private Appearance currMat = ParserMTL.createDefault();

	private void setCurrentMaterial(StreamTokenizer st) throws IOException {
		while (st.nextToken() != StreamTokenizer.TT_EOL) {
			String matName = st.sval;
			currMat = (Appearance) materials.get(matName);
			if (currMat == null)
				System.err.println("Warning: " + matName
						+ " [Material name] is null");
			else
				for (Iterator i = currentGroups.iterator(); i.hasNext();) {
					Group current = (Group) (groups.get(i.next()));
					current.setMaterial(currMat);
				}
		}
	}

	private int[][] temp = new int[3][1000];
	{
		for (int i = 0; i < 1000; i++) {
			temp[0][i] = -1;
			temp[1][i] = -1;
			temp[2][i] = -1;
		}

	}
	int count = 0;

	private void addFace(StreamTokenizer st) throws IOException {
		int ix = 0; // side counter
		int jx = 0; // vertex/vertex-texture/vertex-normal index
		boolean lastWasNumber = false;
		st.nextToken();
		while (st.ttype != TT_EOL && st.ttype != TT_EOF) {
			if (st.ttype == '/') {
				jx++;
				lastWasNumber = false;
				st.nextToken();
				continue;
			} 
			else if (st.ttype == '\\') {
				st.nextToken(); // the EOL
				st.nextToken(); // continue parsing in the next line
				continue;
			} 
			else if (st.ttype == StreamTokenizer.TT_NUMBER) {
				if (lastWasNumber) {
					ix++;
					jx = 0;
				}
				// System.out.println("adding ["+jx+"]["+ix+"]="+(int)st.nval);
				if (st.nval > 0)
					temp[jx][ix] = (int) (st.nval - 1);
				else {
					// count backwards
					System.err.println("OBJReader.addFace() negative face");
					temp[jx][ix] = v.size() + (int) st.nval;
				}
				lastWasNumber = true;
			}
			else {
				System.out.println("unknown tag " + st.sval + " " + st.ttype);
			}
			st.nextToken();
		}
		ix++;
		int[] faceV = new int[ix];
		int[] faceVT = new int[ix];
		int[] faceVN = new int[ix];
		System.arraycopy(temp[0], 0, faceV, 0, ix);
		System.arraycopy(temp[1], 0, faceVT, 0, ix);
		System.arraycopy(temp[2], 0, faceVN, 0, ix);

		// clean dirty entries in temp
		for (int i = 0; i < ix; i++) {
			temp[0][i] = -1;
			temp[1][i] = -1;
			temp[2][i] = -1;
		}

		// TODO what means that? adding face to all groups?
		for (Iterator i = currentGroups.iterator(); i.hasNext();) {
			Group g = ((Group) groups.get(i.next()));
			g.addFace(faceV, faceVT, faceVN);
		}
	}

	private void addGroup(StreamTokenizer st) throws IOException {
		// till now only the first groupname gets parsed
		currentGroups.clear();
		st.nextToken();
		String gName = "default";
		if (st.ttype == StreamTokenizer.TT_EOL) {
			LoggingSystem.getLogger(this).fine("Warning: empty group name");
			st.pushBack();
		} else
			gName = st.sval;
		// System.out.println("adding "+gName+" to current groups. ["+st.nval+","+st.sval+","+st.ttype+"]");
		currentGroups.add(gName);
		if (groups.get(gName) == null) {
			Group g = new Group(gName);
			groups.put(gName, g);
		}
		while (st.nextToken() != StreamTokenizer.TT_EOL) {
		}
	}

	private class Group {

		final List<int[]> faces;
		final String name;
		final Appearance material;
		boolean smooth;

		boolean hasTex, hasNorms;

		FaceData fd;

		Group(String name) {
			this.name = name;
			faces = new ArrayList<int[]>(11);
			material = ParserMTL.createDefault();
			setSmoothening(smoothShading);
			setMaterial(currMat);
			fd = new FaceData();
		}

		void addFace(int[] verts, int[] texs, int[] norms) {
			int[] face = new int[verts.length];
			for (int i = 0; i < verts.length; i++) {
				face[i] = fd.getID(verts[i], texs[i], norms[i]);
			}
			faces.add(face);
		}

		void setSmoothening(boolean smoothShading) {
			// TODO: check what smoothening should do...
			if (true)
				return;
			smooth = smoothShading;
			material.setAttribute(CommonAttributes.POLYGON_SHADER + "."
					+ CommonAttributes.SMOOTH_SHADING, smooth);
		}

		public boolean hasGeometry() {
			return faces.size() > 0 || 
			(groups.size() == 1 && v.size() > 0); // vertices only
		}

		void setMaterial(Appearance a) {
			if (a == null) {
				System.err.println("Warning: current app==null");
				return;
			}
			Set lst = a.getStoredAttributes();
			for (Iterator i = lst.iterator(); i.hasNext();) {
				String aName = (String) i.next();
				material.setAttribute(aName, a.getAttribute(aName));
			}
			setSmoothening(smooth);
		}

		Geometry createGeometry() {
			ArrayList vertices = extractVertices();
			IndexedFaceSet ifs = new IndexedFaceSet();
			// data is definitly available
			ifs.setVertexCountAndAttributes(Attribute.COORDINATES,
					StorageModel.DOUBLE3_ARRAY.createReadOnly(vertices
							.toArray(new double[vertices.size()][])));
			ifs.setFaceCountAndAttributes(Attribute.INDICES,
					StorageModel.INT_ARRAY_ARRAY.createReadOnly(
						faces.toArray(new int[faces.size()][]))
					);

			// check if texture coordinates are available and if size fits
			if (fd.size() > 0) {
				ArrayList vertexTex = extractTexCoords();
				ArrayList vertexNorms = extractNormals();
				if (vertexTex != null) {
					double[][] vTexArray = new double[vertexTex.size()][];
					vertexTex.toArray(vTexArray);
					int numPerEntry = 2;
					if (vTexArray.length != 0) {
						numPerEntry = vTexArray[0].length;
					}
					ifs.setVertexAttributes(
							Attribute.TEXTURE_COORDINATES,
							StorageModel.DOUBLE_ARRAY.array(numPerEntry).createReadOnly(vTexArray));
				}
				if (vertexNorms != null) {
					ifs.setVertexAttributes(Attribute.NORMALS,
							StorageModel.DOUBLE3_ARRAY.createReadOnly(vertexNorms
									.toArray(new double[vertexNorms.size()][]))
							);
				}
			}
			boolean hasVertexNormals = ifs
					.getVertexAttributes(Attribute.NORMALS) != null;
			if (!hasVertexNormals && smooth) {
				IndexedFaceSetUtility.calculateAndSetVertexNormals(ifs);
			}
			if (!smooth && !hasVertexNormals) {
				IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs);
			}
			if (fd.size() > 0) {
				IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(ifs);
			}
			return ifs;
		}

		private ArrayList<double[]> extractNormals() {
			if (fd.normalId(0) == -1)
				return null;
			ArrayList<double[]> list = new ArrayList<double[]>(fd.size());
			for (int i = 0; i < fd.size(); i++) {
				list.add(i, vNorms.get(fd.normalId(i)));
			}
			return list;
		}

		private ArrayList<double[]> extractTexCoords() {
			if (fd.texId(0) == -1)
				return null;
			ArrayList<double[]> list = new ArrayList<double[]>(fd.size());
			for (int i = 0; i < fd.size(); i++) {
				list.add(i, vTexs.get(fd.texId(i)));
			}
			return list;
		}

		private ArrayList<double[]> extractVertices() {
			if (fd.size() == 0) {
				return new ArrayList<double[]>(v);
			}
			ArrayList<double[]> list = new ArrayList<double[]>(fd.size());
			for (int i = 0; i < fd.size(); i++) {
				list.add(i, v.get(fd.vertexId(i)));
			}
			return list;
		}

		SceneGraphComponent createComponent() {
			SceneGraphComponent ret = new SceneGraphComponent();
			ret.setName(name);
			ret.setAppearance(material);
			ret.setGeometry(createGeometry());
			return ret;
		}
	}

	/**
	 * creates indices for triples of vertex/tex/normal
	 */
	private static class FaceData {
		private HashMap<String, Integer> storedData = new HashMap<String, Integer>();
		private ArrayList<Triple> list = new ArrayList<Triple>();

		private class Triple {
			int v, t, n;

			Triple(int vId, int tId, int nId) {
				v = vId;
				t = tId;
				n = nId;
			}
		}

		private FaceData() {
		}

		private int idCounter;

		int getID(int vertexIndex, int texIndex, int normalIndex) {
			final String key = vertexIndex + "::" + texIndex + "::"
					+ normalIndex;
			Integer ret = storedData.get(key);
			if (ret == null) {
				ret = new Integer(idCounter++);
				storedData.put(key, ret);
				list.add(new Triple(vertexIndex, texIndex, normalIndex));
			}
			return ret.intValue();
		}

		void reset() {
			storedData.clear();
		}

		int size() {
			return storedData.size();
		}

		int vertexId(int id) {
			return list.get(id).v;
		}

		int texId(int id) {
			return list.get(id).t;
		}

		int normalId(int id) {
			return list.get(id).n;
		}
	}
}
