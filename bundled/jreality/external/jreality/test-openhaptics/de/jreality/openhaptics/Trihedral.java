package de.jreality.openhaptics;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.shader.CommonAttributes;


public class Trihedral extends SceneGraphComponent {
	final static public double points[][] = new double[][]{
		{0.0, 0.0, 0.0,}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0},
	};
	
	public Trihedral(String lx, String ly, String lz){
		IndexedLineSetFactory ifsf = new IndexedLineSetFactory();
		
		ifsf.setVertexCount(points.length);
		ifsf.setVertexCoordinates(points);
		

		ifsf.setEdgeCount(3);
		ifsf.setEdgeIndices(new int [][]{{0,1}, {0,2}, {0,3}});
		ifsf.setEdgeLabels(new String[]{ lx, ly, lz});
		
		
		ifsf.update();

		Appearance ap;
		setAppearance(ap =new Appearance());			
		
		ap.setAttribute(CommonAttributes.LINE_SHADER+".textShader.scale", 0.005);
		setGeometry(ifsf.getGeometry());
	}
	
	public Trihedral translate(double x, double y, double z){
		setTransformation(new Transformation(MatrixBuilder.euclidean().translate(x, y, z).getMatrix().getArray()));
		return this;
	}
}
