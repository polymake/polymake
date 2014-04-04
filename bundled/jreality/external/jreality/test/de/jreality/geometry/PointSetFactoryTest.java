package de.jreality.geometry;

import junit.framework.TestCase;
import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;

public class PointSetFactoryTest extends TestCase {

	PointSetFactory factory;
	
	static double [] vertices  = new double[] {

	 0,  0,  0,
	 1,  0,  0,
	 1,  1,  0,
	 0,  1,  0,

	 0,  0,  1,
	 1,  0,  1,
	 1,  1,  1,
	 0,  1,  1,

	};

	public void setUp() {
		factory = new PointSetFactory();
	}
	
	public void tearDown() {
	}
	
	public void testPointLabels()	{
		
		//factory.debug = true;
		
		factory.setVertexCount( vertices.length );
		factory.setVertexCoordinates( vertices );	
		
		factory.setGenerateVertexLabels( true );
		factory.update();
		
		PointSet ps = factory.getPointSet();
		
		String [] labels = ps.getVertexAttributes(Attribute.LABELS).toStringArray(null);
		
		for( int i=0; i<labels.length; i++ ) {
			assertEquals( labels[i], new Integer( i ).toString());
		}
		
		factory.setGenerateVertexLabels( false );
		
		factory.update();
		
		assertEquals( ps.getVertexAttributes(Attribute.LABELS), null );
		
		labels[0] = "gaga";
		
		factory.setVertexLabels( labels );
		
		factory.update();
		
		labels = ps.getVertexAttributes(Attribute.LABELS).toStringArray(null);
		
		assertEquals( labels[0],  "gaga" );
		for( int i=1; i<labels.length; i++ ) {
			assertEquals( labels[i], new Integer( i ).toString());
		}
		
		// this should work
		factory.setGenerateVertexLabels( false );
		
		//this should fail
		try {
			factory.setGenerateVertexLabels( true );
		} catch( UnsupportedOperationException e ) {
		}
		
		factory.setVertexLabels( (String[])null );
		factory.setGenerateVertexLabels( true );
	}
	
	public void testPointColors()	{
		
		factory.debug = true;
		
		
		double[][] vertices = new double[][] {{0,0,0,1}};
		
		
		factory.setVertexCount( vertices.length );
		factory.setVertexCoordinates( vertices );	
		
		factory.setVertexColors(new double[][]{{0,1,0}});
		
		factory.update();
		
		PointSet ps = factory.getPointSet();
			
		assertEquals( ps.getVertexAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][0], 0, 0);
		assertEquals( ps.getVertexAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][1], 1, 0);
		assertEquals( ps.getVertexAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][2], 0, 0);
	
		// now we try to change the alpha channel of the face color
		// just to be safe, we don't use the old array but create a new one.
		
		factory.setVertexColors(new double[][]{{1,0,0}});
		
		factory.update();
		
		assertEquals( ps.getVertexAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][0], 1, 0);
		assertEquals( ps.getVertexAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][1], 0, 0);
		assertEquals( ps.getVertexAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][2], 0, 0);
	}
	public void test() {

		factory.setVertexCount( 8 );
		factory.setVertexCoordinates( vertices );
		
		factory.update();
		
		
	}
	
}
