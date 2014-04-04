package de.jreality.tutorial.reader;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.LineNumberReader;
import java.util.ArrayList;
import java.util.StringTokenizer;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.reader.AbstractReader;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;

/**
 * A simple example how to write a file reader for jreality. Parses files that contain
 * a vertex in each row, separated by any white space character. Three subsequent
 * vertices will be interpreted as a triangle. See the file samplefile.demo.
 * 
 * @author Steffen Weissmann
 *
 */
public class DemoReader extends AbstractReader {

	@Override
	public void setInput(Input input) throws IOException {
		super.setInput(input);
		
		// read all vertices into a list:
		ArrayList<double[]> vertices = new ArrayList<double[]>();
		LineNumberReader lnr = new LineNumberReader(new BufferedReader(input.getReader()));
		for (String line = lnr.readLine(); line != null; line = lnr.readLine()) {
			line = line.trim();
			if (line.startsWith("#")) continue;
			StringTokenizer st = new StringTokenizer(line);
			if (st.countTokens() != 3) {
				System.out.println("illegal line: "+line);
				continue;
			}
			double x = Double.parseDouble(st.nextToken());
			double y = Double.parseDouble(st.nextToken());
			double z = Double.parseDouble(st.nextToken());
			vertices.add(new double[]{x, y, z});
		}
		
		// convert vertex list into double[][] array:
		int nVertices = vertices.size();
		double[][] verts = vertices.toArray(new double[nVertices][]);
		
		// create triangle indices
		int nTriangles = vertices.size()/3;
		int[] indices = new int[nTriangles*3];
		for (int i=0, n=nTriangles*3; i<n; i++) indices[i]=i;
		
		// create the geometry:
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setGenerateFaceNormals(true);
		ifsf.setGenerateEdgesFromFaces(true);
		ifsf.setVertexCount(nVertices);
		ifsf.setFaceCount(nTriangles);
		ifsf.setVertexCoordinates(verts);
		ifsf.setFaceIndices(indices, 3);
		ifsf.update();
		
		// assign the root component of the reader and set the geometry:
		root = new SceneGraphComponent("demo-file");
		root.setGeometry(ifsf.getIndexedFaceSet());
	}
	
}
