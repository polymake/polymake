package de.jreality.geometry;

import static de.jreality.shader.CommonAttributes.EDGE_DRAW;
import static de.jreality.shader.CommonAttributes.FACE_DRAW;
import static de.jreality.shader.CommonAttributes.TUBES_DRAW;
import static de.jreality.shader.CommonAttributes.VERTEX_DRAW;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;

public class PointSetUtility {

	/**
	 * Generates an instance of {@link IndexedLineSet} which contains one line segment for each vertex normal of the
	 * input <i>ps</i>.  This IndexedLineSet is then stuffed into a {@link SceneGraphComponent} whose appearance
	 * is set with tube drawing <b>disabled</b>, which is then returned.  If normals are not provided in <i>ps</i>,
	 * an exception is thrown.
	 * @param ifs
	 * @param scale
	 * @param metric
	 * @return
	 */
	public static SceneGraphComponent displayVertexNormals(PointSet ps, double scale, int metric)	{
    	SceneGraphComponent sgc = new SceneGraphComponent("displayFaceNormals()");
    	Appearance ap  = new Appearance();
    	ap.setAttribute(EDGE_DRAW, true);
       	ap.setAttribute("lineShader."+TUBES_DRAW, false);
    	ap.setAttribute(FACE_DRAW, false);
    	ap.setAttribute(VERTEX_DRAW, false);
    	sgc.setAppearance(ap);
    	int n = ps.getNumPoints();
    	int[][] edges = new int[n][2];
		double[][] verts = ps.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
    	int fiberlength = verts[0].length;
		double[][] normals = null,
    		nvectors = new double[2*n][fiberlength];
    	normals = ps.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
    	if (normals == null) 
    		throw new IllegalStateException("must have vertex normals");
    	
      	for (int i = 0; i<n; ++i)	{
      		nvectors[i] = verts[i];
    		if (metric == Pn.EUCLIDEAN) {
         		Rn.add(nvectors[i+n], nvectors[i], Rn.times(null, scale, normals[i]));
    		}
    		else {
    			Pn.dragTowards(nvectors[i+n], nvectors[i], normals[i], scale, metric);
    		}
    		if (fiberlength == 4) {
    			if (metric == Pn.EUCLIDEAN) nvectors[i+n][3] = 1.0;
    			else Pn.dehomogenize(nvectors[i+n], nvectors[i+n]);
    		}
     		edges[i][0] = i;
    		edges[i][1] = i+n;
    	}
    	IndexedLineSetFactory ilsf = new IndexedLineSetFactory();
    	ilsf.setVertexCount(2*n);
    	ilsf.setVertexCoordinates(nvectors);
    	ilsf.setEdgeCount(n);
    	ilsf.setEdgeIndices(edges);
    	ilsf.update();
    	sgc.setGeometry(ilsf.getIndexedLineSet());
    	return sgc;
    }

}
