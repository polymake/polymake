package de.jreality.geometry;

import de.jreality.math.P3;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.IndexedFaceSet;

public class TestTriangulate {

	public static void main(String[] args) {
		IndexedFaceSet dumbbell = surfaceOfRevolutionAsIFS(
				new double[][]{
						{0,.01,0},{0,.5,0},{.3333,.5,0},{.3333,.25,0},{.6666,.25,0},{.6666,.5,0},{1,.5,0},{1,.01,0}},
						5, Math.PI*2);
		dumbbell = (IndexedFaceSet)RemoveDuplicateInfo.removeDuplicateVertices(dumbbell, 10E-6);
		
		// following call doesn't terminate in versions before SVN2945
		dumbbell = IndexedFaceSetUtility.triangulate(dumbbell);
		
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(dumbbell);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();

	}
	public static IndexedFaceSet surfaceOfRevolutionAsIFS(double[][] profile, int num, double angle)	{
		QuadMeshFactory qmf = new QuadMeshFactory();//Pn.EUCLIDEAN, profile.length, num, false, false);
		qmf.setULineCount(profile.length);
		qmf.setVLineCount(num);
		double[][] vals = surfaceOfRevolution(profile, num, angle);
		qmf.setVertexCoordinates(vals);
		qmf.setGenerateFaceNormals(true);
		qmf.setGenerateVertexNormals(true);
		qmf.update();
		return qmf.getIndexedFaceSet();
	}

	public static double[][] surfaceOfRevolution(double[][] profile, int num, double angle) {
		if (num <= 1 || profile[0].length < 3) {
			throw new IllegalArgumentException("Bad parameters");
		}
		double[][] vals = new double[num * profile.length][profile[0].length];
		for (int i = 0 ; i < num; ++i)	{
			double a = i * angle/(num-1);
			double[] rot = P3.makeRotationMatrixX(null, a);
			for (int j = 0; j<profile.length; ++j)
				Rn.matrixTimesVector(vals[i*profile.length+j], rot, profile[j]);
		}
		return vals;
	}

}
