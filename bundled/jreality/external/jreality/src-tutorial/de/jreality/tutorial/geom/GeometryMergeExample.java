package de.jreality.tutorial.geom;

import java.awt.Color;
import java.util.LinkedList;
import java.util.List;

import de.jreality.geometry.GeometryMergeFactory;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.SceneGraphUtility;

/**
 * This example shows how to use the {@link GeometryMergeFactory}, which combines all geometries in a scene graph into
 * a single {@link IndexedFaceSet}.
 * 
 * @author Bernd Gonska
 *
 */
public class GeometryMergeExample {

	public static void main(String[] args) {
		// a little Scene (two boxes and a bangle, transfomation, appearance)
        IndexedFaceSet box= Primitives.box(2, .5, .5, false);
        IndexedFaceSet box2= Primitives.box(2, .6, 0.4, true);
        IndexedFaceSet zyl= Primitives.cylinder(20,1,0,.5,5);
        SceneGraphComponent original= new SceneGraphComponent();
        SceneGraphComponent childNode1= new SceneGraphComponent();
        MatrixBuilder.euclidean().translate(0,0,1).assignTo(childNode1);
        SceneGraphComponent childNode2= new SceneGraphComponent();
        Appearance app= new Appearance();
        app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, new Color(255,255,0));
        childNode2.setAppearance(app);
        original.addChild(childNode1);
        original.addChild(childNode2);
        original.setGeometry(box2);
        childNode1.setGeometry(box);
        childNode2.setGeometry(zyl);
// the Factory:
        GeometryMergeFactory mergeFact= new GeometryMergeFactory();             
// play with the following 3 optional settings (by default they are true)
        mergeFact.setRespectFaces(true);
        mergeFact.setRespectEdges(true);
        mergeFact.setGenerateVertexNormals(true);                       
// you can set some defaults:
        List defaultAtts= new LinkedList();
        List defaultAttValue= new LinkedList();
        List value= new LinkedList();
        defaultAtts.add(Attribute.COLORS);
        defaultAttValue.add(value);
        value.add(new double[]{0,1,0,1});// remember: only 4d colors
        mergeFact.setDefaultFaceAttributes(defaultAtts,defaultAttValue );
// merge a list of geometrys:
        //IndexedFaceSet[] list= new IndexedFaceSet[]{box2,zyl};
        //IndexedFaceSet result=mergeFact.mergeIndexedFaceSets(list);
// or  a complete tree:
        IndexedFaceSet result=mergeFact.mergeGeometrySets(original);
        SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
       SceneGraphComponent merged =  SceneGraphUtility.createFullSceneGraphComponent("merged");
       merged.setGeometry(result);
       MatrixBuilder.euclidean().translate(3,0,0).assignTo(merged);
       world.addChildren(merged, original);
        JRViewer.display(world);
	}

}
