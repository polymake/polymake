package de.jreality.tutorial.geom;

import java.awt.Color;

import de.jreality.geometry.QuadMeshFactory;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.SceneGraphUtility;

/**
 * This example shows how to use a {@link QuadMeshFactory} to generate an instance of 
 * {@link IndexedFaceSet}.  The surface featured here is the same example as in
 * {@link ParametricSurfaceExample} but done with a QuadMeshFactory.
 * 
 * @author gunn
 *
 */
public class QuadMeshExample {

	public static IndexedFaceSet createSurface( int N ) {
		// generate the coordinates for the surface as a 2D array of 3-vectors
		// QuadMeshFactory is the only factory which accepts such a data structure
		// as the argument of its setVertexCoordinates() method!
		double [][][] coords = new double [N][N][3];
		for( int i=0; i<N; i++) {
			double v = -.4 + .8*(i/(N-1.0));
			for (int j = 0; j<N; ++j)	{
				double u = -.3 + .6*(j/(N-1.0));
				coords[i][j][0] = 10*(u-v*v);
				coords[i][j][1]= 10*u*v;
				coords[i][j][2]= 10*(u*u-4*u*v*v);
			}
		}
		
		// QuadMeshFactory knows how to build an IndexedFaceSet from a rectangular array
		// of vectors.  
		QuadMeshFactory factory = new QuadMeshFactory();
		factory.setVLineCount(20);		// important: the v-direction is the left-most index
		factory.setULineCount(20);		// and the u-direction the next-left-most index
		factory.setClosedInUDirection(false);	
		factory.setClosedInVDirection(false);	
		factory.setVertexCoordinates(coords);	
		factory.setGenerateFaceNormals(true);
		factory.setGenerateTextureCoordinates(true);
		factory.setGenerateEdgesFromFaces(true);
		factory.setEdgeFromQuadMesh(true);	// generate "long" edges: one for each u-, v- parameter curve
		
		factory.update();
		
		return factory.getIndexedFaceSet();
	}
	
	public static void main(String[] args) {
		SceneGraphComponent sgc = SceneGraphUtility.createFullSceneGraphComponent("world");
		sgc.setGeometry(createSurface(20));
		Appearance ap = sgc.getAppearance();
		ap.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.yellow);
		JRViewer.display(sgc );

	}

}
