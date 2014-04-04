package de.jreality.geometry;

import junit.framework.TestCase;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.data.Attribute;

public class ParametricSurfaceFactoryTest extends TestCase {

	
	public void testBasic() {
		ParametricSurfaceFactory self=new de.jreality.geometry.ParametricSurfaceFactory();

		self.setImmersion(
		  new ParametricSurfaceFactory.DefaultImmersion() {
		    public void evaluate( double u, double v ) {

		      x=u;
		      y=v;
		      z=u*v;
		   }
		  }
		);

		
		self.setULineCount( 13 );
		self.setVLineCount( 7 );

		self.setGenerateVertexNormals( true );
		//self.setGenerateFaceNormals( true );
		self.setGenerateEdgesFromFaces( true );

		self.update();
		
//		pause();
//		
//		self.debug = false;
//		ViewerApp.display(self.getIndexedFaceSet());
//		self.debug = true;
		
		//pause();
	
		self.setGenerateEdgesFromFaces( false );

		self.setVLineCount( 19 );
		self.update();
			
		//pause();
		
		self.setGenerateEdgesFromFaces( true );
		self.update();
		
		//pause();
		
		final double sR = 1/Math.sqrt(2);
		final double bR= 2;
		
		ParametricSurfaceFactory.Immersion immersion =
	        new ParametricSurfaceFactory.Immersion() {

                public int getDimensionOfAmbientSpace() {
                    return 3;
                }

                public void evaluate(double x, double y, double[] targetArray, int arrayLocation) {
                    // TODO Auto-generated method stub
                    double sRMulSinY=sR*Math.sin(y);
                    targetArray[arrayLocation  ] = Math.cos(-x)*(bR+sRMulSinY);
                    targetArray[arrayLocation+1] = sR*Math.cos(y);
                    targetArray[arrayLocation+2] = Math.sin(-x)*(bR+sRMulSinY);   
                }

				public boolean isImmutable() {
					return true;
				}
	        
	    };
	    
	    self.setImmersion( immersion );
	    self.update();
	    
	    //pause();
	    
	    self.setUMin(-Math.PI);
	    self.setUMax( Math.PI);
	    self.setVMin(-Math.PI);
	    self.setVMax( Math.PI);
	    
	    self.update();
	    
	    //pause();
	    
	    self.setGenerateEdgesFromFaces(false);
	    
	    self.update();
	    
	    //pause();
	    
	    self.setULineCount( 20 );
		self.setVLineCount( 20 );
		
		self.update();
		
		double [][] xyz;
		          
		xyz = self.getIndexedFaceSet().getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		//pause();
		self.update();
		
		xyz = self.getIndexedFaceSet().getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		//pause();
		

		self.setClosedInUDirection(true);
		self.setClosedInVDirection(true);
		
		
		self.update();
		
		self.setGenerateFaceNormals(true);
		
		self.update();
		//pause();
		self.setGenerateFaceNormals(false);
		
		self.update();
		//pause();
		self.setGenerateFaceNormals(true);
		self.update();
		//pause();
		self.update();
		////pause();
	}
	
	public static void pause() {
		pause( 2000 );
	}
	
	public static void pause( long millis ) {
		System.out.println("pause");
		
		long time = System.currentTimeMillis();
		while( System.currentTimeMillis()<time+millis);
	}
	
	public static void main(String[] args) {

		ParametricSurfaceFactory self=new de.jreality.geometry.ParametricSurfaceFactory();
		self.debug = true;
		self.setImmersion(
		  new ParametricSurfaceFactory.DefaultImmersion() {
		    public void evaluate( double u, double v ) {

		      x=u;
		      y=v;
		      z=u*v;
		   }
		  }
		);

		
		self.setULineCount( 13 );
		self.setVLineCount( 7 );

		self.setGenerateVertexNormals( true );
		//self.setGenerateFaceNormals( true );
		self.setGenerateEdgesFromFaces( true );

		self.update();
		
		pause();
		
		self.debug = false;
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(self.getIndexedFaceSet());
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
		self.debug = true;
		
		pause();
	
		self.setGenerateEdgesFromFaces( false );

		self.setVLineCount( 19 );
		self.update();
			
		pause();
		
		self.setGenerateEdgesFromFaces( true );
		self.update();
		
		pause();
		
		final double sR = 1/Math.sqrt(2);
		final double bR= 2;
		
		ParametricSurfaceFactory.Immersion immersion =
	        new ParametricSurfaceFactory.Immersion() {

                public int getDimensionOfAmbientSpace() {
                    return 3;
                }

                public void evaluate(double x, double y, double[] targetArray, int arrayLocation) {
                    // TODO Auto-generated method stub
                    double sRMulSinY=sR*Math.sin(y);
                    targetArray[arrayLocation  ] = Math.cos(-x)*(bR+sRMulSinY);
                    targetArray[arrayLocation+1] = sR*Math.cos(y);
                    targetArray[arrayLocation+2] = Math.sin(-x)*(bR+sRMulSinY);   
                }

				public boolean isImmutable() {
					return true;
				}
	        
	    };
	    
	    self.setImmersion( immersion );
	    self.update();
	    
	    pause();
	    
	    self.setUMin(-Math.PI);
	    self.setUMax( Math.PI);
	    self.setVMin(-Math.PI);
	    self.setVMax( Math.PI);
	    
	    self.update();
	    
	    pause();
	    
	    self.setGenerateEdgesFromFaces(false);
	    
	    self.update();
	    
	    pause();
	    
	    self.setULineCount( 20 );
		self.setVLineCount( 20 );
		
		self.update();
		
		double [][] xyz;
		          
		xyz = self.getIndexedFaceSet().getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		pause();
		self.update();
		
		xyz = self.getIndexedFaceSet().getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		pause();
		

		self.setClosedInUDirection(true);
		self.setClosedInVDirection(true);
		
		
		self.update();
		
		self.setGenerateFaceNormals(true);
		
		self.update();
		pause();
		self.setGenerateFaceNormals(false);
		
		self.update();
		pause();
		self.setGenerateFaceNormals(true);
		self.update();
		pause();
		self.update();
		pause();
	}
}
