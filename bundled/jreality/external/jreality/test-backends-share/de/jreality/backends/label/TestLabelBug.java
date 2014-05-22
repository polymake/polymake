package de.jreality.backends.label;
import java.io.IOException;

import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.TextureUtility;
import de.jreality.util.SystemProperties;


public class TestLabelBug {

	public static void main(String[] args)	{
		SceneGraphComponent root =  new SceneGraphComponent();
		SceneGraphComponent geom = new SceneGraphComponent();
		geom.setName("GRAPH of 1");
		root.addChild(geom);
		/*----------------------------------------*/
		/*--------------------1--------------------*/
		double[][] pts = new double[][]{
		{-1.27497,1.63014,0.54799},
		{-0.189161,1.34561,-1.65432},
		{0.846426,1.03742,1.67049},
		{1.93224,0.752891,-0.531814},
		{-1.93224,-0.752893,0.531812},
		{-0.846424,-1.03742,-1.6705},
		{0.18916,-1.34561,1.65432},
		{1.27497,-1.63014,-0.547988}
		};//pts
		String[] labels = new String[]{"0","1","2","3","4","5","6","7"};
		int[][] lines = new int[][]{ { 0,1 }
		,{ 0,2 }
		,{ 1,3 }
		,{ 2,3 }
		,{ 0,4 }
		,{ 1,5 }
		,{ 4,5 }
		,{ 2,6 }
		,{ 4,6 }
		,{ 3,7 }
		,{ 5,7 }
		,{ 6,7 }
		 };
		Appearance appearance = new Appearance();
		appearance.setAttribute(de.jreality.shader.CommonAttributes.VERTEX_DRAW, true);
		appearance.setAttribute(de.jreality.shader.CommonAttributes.SPHERES_DRAW, true);
		geom.setAppearance(appearance);
		appearance.setAttribute(CommonAttributes.TUBES_DRAW,true);
		appearance.setAttribute(CommonAttributes.EDGE_DRAW,true);
		appearance.setAttribute(
		  "lineShader.diffuseColor",
		  new java.awt.Color((float)0 ,(float)0,(float)0)
		);
		   appearance.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, java.awt.Color.WHITE);
		   //TODO check textures/desertstorm/desertstorm_ path 
		   try {
		  			CubeMap rm = TextureUtility.createReflectionMap(
			                  appearance,
			                  "polygonShader",
			                  "textures/desertstorm/desertstorm_",
			                  new String[]{"rt","lf","up", "dn","bk","ft"},
			                  ".JPG");
					rm.setBlendColor(new java.awt.Color(1f, 1f, 1f, .6f));
		    } catch (IOException e) {
		      e.printStackTrace();
		    }
		IndexedLineSetFactory ilsf = new IndexedLineSetFactory();
		ilsf.setEdgeCount(12);
		ilsf.setVertexCount(8);
		ilsf.setEdgeIndices(lines);
		ilsf.setVertexCoordinates(pts);
		if(labels.length == 8) ilsf.setVertexLabels(labels);
		double[] colors = new double[]{ 1,0,0,
		1,0,0,
		1,0,0,
		1,0,0,
		1,0,0,
		1,0,0,
		1,0,0,
		1,0,0 };
		ilsf.setVertexColors(new DoubleArrayArray.Inlined(colors, 3) );
		double[] lineColors = new double[]{ 0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0 };
		ilsf.setEdgeColors( new DoubleArrayArray.Inlined(lineColors, 3) );
		ilsf.update();
		SceneGraphComponent part1=new SceneGraphComponent();
		part1.setGeometry(ilsf.getGeometry());
		part1.setAppearance(appearance);
		part1.setName("LineSet 0");
		geom.addChild(part1);
		
		System.setProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_SOFT+" "+SystemProperties.VIEWER_DEFAULT_JOGL+" "+SystemProperties.VIEWER_DEFAULT_JOGL3); // de.jreality.portal.DesktopPortalViewer");

	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(root);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();

		}
}
