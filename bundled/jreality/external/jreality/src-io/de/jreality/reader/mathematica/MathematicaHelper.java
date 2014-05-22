package de.jreality.reader.mathematica;

import java.awt.Color;
import java.util.Iterator;
import java.util.Set;

import de.jreality.geometry.GeometryMergeFactory;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.shader.CommonAttributes;

public class MathematicaHelper {
	static Appearance copyApp(Appearance appOld){
		// kopiert eine Appearance um doppelt-Verzeigerung zu vermeiden
		Appearance appNew= new Appearance();
		Set s=appOld.getStoredAttributes();
		Iterator ite= s.iterator();
		while (ite.hasNext()){
			String key=(String)ite.next();
			appNew.setAttribute(key,appOld.getAttribute(key));
		}
	 	return appNew;
	}
	static Appearance setPLColor(Appearance app,Object c){
		Appearance app2= copyApp(app);
		app2.setAttribute(CommonAttributes.POINT_SHADER+"."+
			 	CommonAttributes.POLYGON_SHADER+"."+
			 	CommonAttributes.DIFFUSE_COLOR, c);
	 	app2.setAttribute(CommonAttributes.POINT_SHADER+"."+
			 	CommonAttributes.DIFFUSE_COLOR, c);			 	
		app2.setAttribute(CommonAttributes.LINE_SHADER+"."+
			 	CommonAttributes.POLYGON_SHADER+"."+
			 	CommonAttributes.DIFFUSE_COLOR, c);
	 	app2.setAttribute(CommonAttributes.LINE_SHADER+"."+
			 	CommonAttributes.DIFFUSE_COLOR, c);
		return app2;
	}
	static Color getPLColor(Appearance app,Color plCDefault){
		Color c= null;
		try{
			c=(Color)app.getAttribute(CommonAttributes.POINT_SHADER+"."+
			 	CommonAttributes.DIFFUSE_COLOR);
			}
		catch(Exception e){
			c=plCDefault;
		}
		return c;
	}
	static Color getFColor(Appearance app,Color fCDefault){
		Color c= null;
		try{
			c=(Color)app.getAttribute(CommonAttributes.POLYGON_SHADER+"."+
			 	CommonAttributes.DIFFUSE_COLOR);
			}
		catch(Exception e){
			c=fCDefault;
		}
		return c;
	}
	static Color copyColor(Color c){
		// kopiert eine Farbe um doppelt-Verzeigerung zu vermeiden
		return new Color(c.getRed(),c.getGreen(),c.getBlue()) ;
	}
	static double[] getRGBColor(Color c){
	// retuns a array that represents a color (needed for color-Arrays as double[][])
		double[] fl= new double[3];
		fl[0]=c.getRed()/255.0;
		fl[1]=c.getGreen()/255.0;
		fl[2]=c.getBlue()/255.0;
		return fl ;
	}
	public static SceneGraphComponent getDefaultLightNode (){
		SceneGraphComponent lightNode= new SceneGraphComponent();
		SceneGraphComponent light1Node= new SceneGraphComponent();
		SceneGraphComponent light2Node= new SceneGraphComponent();
		SceneGraphComponent light3Node= new SceneGraphComponent();
		SceneGraphComponent light4Node= new SceneGraphComponent();
		SceneGraphComponent light5Node= new SceneGraphComponent();
		SceneGraphComponent light6Node= new SceneGraphComponent();
		Light light1 = new PointLight();
		Light light2 = new PointLight();
		Light light3 = new PointLight();
		Light light4 = new PointLight();
		Light light5 = new PointLight();
		Light light6 = new PointLight();
		lightNode.addChild(light1Node);
		lightNode.addChild(light2Node);
		lightNode.addChild(light3Node);
		lightNode.addChild(light4Node);
		lightNode.addChild(light5Node);
		lightNode.addChild(light6Node);
		light1.setColor(new Color(255,0,0));
		light2.setColor(new Color(0,255,0));
		light3.setColor(new Color(0,0,255));
		light4.setColor(new Color(255,0,0));
		light5.setColor(new Color(0,255,0));
		light6.setColor(new Color(0,0,255));
		light1.setIntensity(1);
		light2.setIntensity(1);
		light3.setIntensity(1);
		light4.setIntensity(1);
		light5.setIntensity(1);
		light6.setIntensity(1);
		light1Node.setLight(light1);
		light2Node.setLight(light2);
		light3Node.setLight(light3);
		light4Node.setLight(light4);
		light5Node.setLight(light5);
		light6Node.setLight(light6);
		MatrixBuilder.euclidean().translate(1,0,1).assignTo(light1Node);
		MatrixBuilder.euclidean().translate(1,1,1).assignTo(light2Node);
		MatrixBuilder.euclidean().translate(0,1,1).assignTo(light3Node);
		MatrixBuilder.euclidean().translate(-1,0,-1).assignTo(light4Node);
		MatrixBuilder.euclidean().translate(-1,-1,-1).assignTo(light5Node);
		MatrixBuilder.euclidean().translate(0,-1,-1).assignTo(light6Node);
		return lightNode; 
	} 
	
	static IndexedFaceSet makeCylinder(double[] start, double[] end, double radius){
		GeometryMergeFactory gem= new GeometryMergeFactory();
		SceneGraphComponent sgc= new SceneGraphComponent();
		double len=Rn.euclideanDistance(start, end);
		sgc.addChild(Primitives.closedCylinder(20, radius,0, len, Math.PI*2));
		double[] diff=Rn.subtract(null, end, start);
		MatrixBuilder.euclidean().rotateFromTo(new double[]{0,0,1}, diff).assignTo(sgc);
		IndexedFaceSet ifs=gem.mergeGeometrySets(sgc);
		ifs.setVertexAttributes(Attribute.COLORS, null);
		ifs.setFaceAttributes(Attribute.COLORS, null);
		return ifs;
	}
	
}
