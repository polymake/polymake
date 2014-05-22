package de.jreality.geometry;

import java.awt.Color;

import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.shader.CommonAttributes;

public class TestMerge {

	static SceneGraphComponent root= new SceneGraphComponent();

	public static void main(String[] args) {
		IndexedFaceSet ico= Primitives.sharedIcosahedron;
		ico.setVertexAttributes(Attribute.NORMALS,null);
		ico.setFaceAttributes(Attribute.NORMALS,null);
		IndexedFaceSet box= Primitives.box(10, .5, .5, true);
		IndexedFaceSet box2= Primitives.box(10, .6, 0.4, true);
		IndexedFaceSet zyl= Primitives.cylinder(20,1,0,.5,5);
		IndexedLineSet arr=Primitives.arrow(0, 0, 1, 1, 1, false);
		SceneGraphComponent root= new SceneGraphComponent();
		Appearance app=new Appearance();
		app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, new Color(255,255,0));
		app.setAttribute(CommonAttributes.VERTEX_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, new Color(0,255,255));
		root.setAppearance(app);
		SceneGraphComponent A= new SceneGraphComponent();
		SceneGraphComponent B= new SceneGraphComponent();

		SceneGraphComponent C= new SceneGraphComponent();
		SceneGraphComponent A1= new SceneGraphComponent();
		MatrixBuilder.euclidean().translate(0,1,0).assignTo(A1);
		SceneGraphComponent A11= new SceneGraphComponent();
		MatrixBuilder.euclidean().rotate(Math.PI/2,0,0,1 ).assignTo(A11);
		SceneGraphComponent B1= new SceneGraphComponent();
		SceneGraphComponent B2= new SceneGraphComponent();
		Appearance app2=new Appearance();
		app2.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, new Color(255,0,255));
		B2.setAppearance(app2);

		root.addChild(A); 	A.addChild(A1); A1.addChild(A11);
		root.addChild(B); 	B.addChild(B1);
		B.addChild(B2);
		root.addChild(C);

			root.setGeometry(new PointSet());
			A1.setGeometry(box);
			A11.setGeometry(box2);
			B1.setGeometry(zyl);
			B2.setGeometry(ico);
			C.setGeometry(arr);
		//IndexedLineSet[] list= new IndexedLineSet[]{			};
		PointSet[] list= new PointSet[]{zyl,box2,box};
		//IndexedLineSet[] list= new IndexedLineSet[]{ico,box};
		//IndexedFaceSet i=mergeIndexedFaceSets(list,new Attribute[]{Attribute.COLORS},new double[][][]{{{0,1,1}}},null,null,null,null );

		GeometryMergeFactory t= new GeometryMergeFactory();
		//t.respectFaces=false;
		//t.generateFaceNormals=false;
		//t.generateVertexNormals=false;
		//t.respectEdges=false;
		
		IndexedFaceSet i=t.mergeIndexedFaceSets(list);
		//IndexedFaceSet i=t.mergeGeometrySets(root);
		
		System.out.println("Report:"+i);
		
		
		
		
		//IndexedFaceSet i= Primitives.torus(20,10 , 20, 10);
		//System.out.println("TestMerge.main(i)"+i);
		IndexedFaceSet j= (IndexedFaceSet)RemoveDuplicateInfo.removeDuplicateVertices(i,new Attribute[]{Attribute.COLORS} );
		//System.out.println("TestMerge.main(j)"+j);
	    
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(j);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
		
	}

}
