package de.jreality.tutorial.geom;

import static de.jreality.shader.CommonAttributes.ALIGNMENT;
import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.LINE_SHADER;
import static de.jreality.shader.CommonAttributes.OFFSET;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.SCALE;

import java.awt.Color;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.SwingConstants;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tools.ActionTool;
import de.jreality.util.Input;

/**
 * Tutorial on uwrapping a geometry using IndexedFaceSetFactory. 
 * 
 *  <p>A geometry in the jReality scene graph does not allow per 
 *  face vertex attributes. So to unwrap a geometry e.g. 
 *  to put a texture on it, one has to introduce multiple vertices. 
 *  The IndexedFaceSetFactory supports this as follows: set vertex 
 *  coordinates for all vertices (setVertexCount(), setVertexCoordinates()), 
 *  set the face indices to the wrapped faces (leave out the extra vertices) 
 *  (setFaceCount(), setFaceIndices), and use setUnwrapFaceIndices(int[][]) 
 *  or its variants to introduce the unwrapped faces (same number of faces).
 *	
 *  <p>The unwrapFaceIndices are written into the created IndexedFaceSet. 
 *  The faceIndices are used to generate attributes, in particular to 
 *  generate correct vertex normals. The vertices that represent the same 
 *  vertex are detected from correspondence in faceIndices and unwrapFaceIndices, 
 *  and not from the vertex coordinates.
 *  
 *  See also <a href=http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/unwrap_a_geometry_using_a_factory>the tutorial</a>.
 *  
 * 
 * @author G. Paul Peters, 11.04.2009
 *
 */
public class CubeUnwrapped {
	
	public static void main(String[] args) {
		
		final double [][] vertices  = new double[][] {
			//The 8 vertices of the cube
			{0,  0,  0}, {1,  0,  0}, {1,  1,  0}, {0,  1,  0}, //0-3
			{0,  0,  1}, {1,  0,  1}, {1,  1,  1}, {0,  1,  1}, //4-7
		};
		//The 6 faces of the "wrapped" cube in space
		final int [][] indices = new int [][] {
			{ 0, 1, 2, 3 }, { 7, 6, 5, 4 },	{ 0, 1, 5, 4 }, 
			{ 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } 
		};
		//The 6 faces of the usual unwrapping of the cube 
		final int [][] unwrapIndices = new int [][] {
			//The first 3 faces are connected to each other as on the cube,
			//so their indices stay the same
			{ 0, 1, 2, 3 }, { 7, 6, 5, 4 }, { 0, 1, 5, 4 },
			//The following 3 faces are not connected
			//to some of the faces they are connected on the cube.
			//This is done using vertex indices 8-13.
			{ 1, 8, 9, 5 }, { 8, 10, 11, 9 }, { 12, 0, 4, 13 }
		};
		//The texture coordinates of the vertices. Make a sketch of them and
		//you will see the unwrapped cube
		final double [][] unwrapTextureCoordinates = new double[][] {
			{ .25, .5},{ .5, .5},{ .5, .75},{ .25, .75}, //0-3
			{ .25, .25 },{ .5, .25 },{ .5, .0 },{ .25, .0 },//4-7
			{ .75, .5 },{ .75, .25 },{ 1., .5 },{ 1., .25 },{ 0., .5 },{ 0., .25 },//8-13
		};
	
		final IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		
		// set all 14 vertices and their coordinates
		ifsf.setVertexCount( 14 );
		// We need to unwrap the vertex coordinates for the 14 vertices
		// This is done by unwrapVertexAttributes, which determines the correspondence of vertices
		// from correspondence in indices and unwrapIndices. For the cube:
		// 8->2, 9->6, 10->3, 11->7, 12 -> 3, 13 -> 7	 

		ifsf.setVertexCoordinates( IndexedFaceSetFactory.unwrapVertexAttributes(vertices, indices, unwrapIndices, 14));
		ifsf.setVertexTextureCoordinates(unwrapTextureCoordinates);
		
		// The 6 faces of the cube, use the "wrapped" face indices
		ifsf.setFaceCount( 6 );	
		ifsf.setFaceIndices( indices );

		// Generation of vertexNormals and edges is according to the wrapped indices rather than
		// the unwrapped
		ifsf.setGenerateVertexNormals( true );
		ifsf.setGenerateEdgesFromFaces( true );
		// The edge labels indicate that edges are doubled only in mode 2 below
		ifsf.setGenerateEdgeLabels(true);
		// Crude documentation 
		ifsf.setVertexLabels(new String[]{"","","","","","    Click with middle mouse","","","","","","","",""});
		ifsf.update();
		
		SceneGraphComponent sgc = new SceneGraphComponent("scene");
		
		/* An action tool that cycles through 3 modes when the cube 
		 * is clicked with the middle mouse button 
		 * mode0: the texture cube with texture jumps, because no unwrapped indices are set
		 * mode1: the nicely textured unwrapped cube
		 * mode3: also nicely textured unwrapped cube, but now edges are doubled 
		 * and broken vertex normals
		 */
		ActionTool tool = new ActionTool("PrimaryMenu");
		tool.addActionListener(new ActionListener(){
			int mode=0;
			public void actionPerformed(ActionEvent e) {
				mode = (mode +1) % 3;
				if (mode==0) {
					ifsf.setFaceIndices( indices ); 
					ifsf.setUnwrapFaceIndices((int[][]) null); 
					ifsf.setVertexLabels(new String[]{"","","","","","    NO unwrapped face indices","","","","","","","",""});
				}
				if (mode==1) {
					ifsf.setFaceIndices( indices ); 
					ifsf.setUnwrapFaceIndices( unwrapIndices ); 
					ifsf.setVertexLabels(new String[]{"","","","","","    wrapped and UNWRAPPED face indices","","","","","","","",""});
					}
				if (mode==2) {
					ifsf.setFaceIndices( unwrapIndices ); 
					ifsf.setUnwrapFaceIndices( (int[][]) null ); 
					ifsf.setVertexLabels(new String[]{"","","","","","    DOUBLED edges and BROKEN vertex normals, unwrapped indices","","","","","","","",""});
					}
				ifsf.update();
			}
		});
		sgc.addTool(tool);
		sgc.setGeometry(ifsf.getIndexedFaceSet());
		
		// Add the texture
		sgc.setAppearance(new Appearance());
		sgc.getAppearance().setAttribute(POLYGON_SHADER+"."+DIFFUSE_COLOR, Color.white);
		sgc.getAppearance().setAttribute(SCALE, .003);
		sgc.getAppearance().setAttribute(OFFSET, new double[]{.1,0,.1});
		sgc.getAppearance().setAttribute(ALIGNMENT, SwingConstants.CENTER);
		sgc.getAppearance().setAttribute(DIFFUSE_COLOR, Color.GREEN);
		sgc.getAppearance().setAttribute(LINE_SHADER+"."+DIFFUSE_COLOR, Color.blue);
		Texture2D tex;
		try{
			tex=TextureUtility.createTexture(
				sgc.getAppearance(),       
				"polygonShader", 
				ImageData.load(Input.getInput("de/jreality/tutorial/geom/black_cross.png")),
				false);
			tex.setTextureMatrix(MatrixBuilder.euclidean().scale(12).getMatrix());
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		JRViewer.display(sgc);
	}

}
