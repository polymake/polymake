package de.jreality.reader.vrml;
/**
 * @author gonska
 */

import java.util.LinkedList;

import de.jreality.reader.vrml.State.Binding;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;

public class DefUseData {
	public static final int NO_TYP=-1;
	public static final int KNOT=0;
	public static final int MATERIAL=1;
	public static final int BIND_M=2;
	public static final int BIND_N=3;
	public static final int COORDS=4;
	public static final int NORMALS=5;
	public static final int TRAFO=6;
	public static final int TEXTURE=7;
	public static final int TEXTURE_TRAFO=8;
	public static final int TEXTURE_COORDS=9;
	public static final int SHAPE_HINTS=10;
	
	private static LinkedList types = new LinkedList();
		// ist es ein einzuhaengender Knoten oder wird nur der State geaendert
	private LinkedList names= new LinkedList();// String 
	private LinkedList states =new LinkedList();//State 
	
	
// Setter	
	/**
	 * stores a definition for the USE-command
	 */
	public void addDef(State s, String n){
		for (int i=0;i<names.size();i++){
			if (n.equalsIgnoreCase((String)names.get(i))){
				states.set(i,s);
				return ;
			}
		}
		names.add(n);
		states.add(s);
	}
// Getter
/**
 *returns a State that is to use in a Def Node
 * 
 */
	public static State defState(State givenState){
		State s=new State(givenState);
		// Aufstazpunkt isoloieren:
		s.trafo=null;		
		// entferne Appearance color, sie wird erst bei use eingetragen
		if(s.materialBinding==Binding.OVERALL||s.materialBinding==Binding.DEFAULT)
			s.materialBinding=State.Binding.NONE;
		s.currNode=new SceneGraphComponent();
		s.history="DEF:";
		return s;
	} 
	/**
	 * inserts a, by name defined, Vrml-Node
	 * at the currentKnot of the given state    
	 * @param current state
	 * @param name  	the key
	 */
	public void use(State givenState,String name,boolean definition){
		int n=-1;
		for (int i=0;i<names.size();i++)
			if (name.equalsIgnoreCase((String)names.get(i)))
				n=i;
		if (n==-1) return;
		State defState=(State)states.get(n);
		givenState.defTyp=defState.defTyp;
		switch (defState.defTyp) {
		case KNOT:useKnot(givenState,defState,name,definition);
			break;
		case MATERIAL:useMaterial(givenState,defState); 
			break;
		case BIND_M:givenState.materialBinding= defState.materialBinding;
			break;
		case BIND_N:givenState.normalBinding= defState.normalBinding;
			break;
		case COORDS:givenState.coords=new State(defState).coords;
			break;
		case NORMALS:givenState.normals=new State(defState).normals;
			break;
		case TRAFO:useTrafo(givenState,defState);
			break;
		case TEXTURE: useTexture(givenState,defState);
			break;
		case TEXTURE_COORDS:givenState.textureCoords=
					new State(defState).textureCoords;
			break;
		case TEXTURE_TRAFO: givenState.textureTrafo=
					new State(defState).textureTrafo;
			break;
		case SHAPE_HINTS: useShapeHints(givenState,defState);
			break;
		default:
			break;
		}		
	}	
// privates
	private static void useKnot(State givenState,State defState,String name,boolean definition){
		SceneGraphComponent c=defState.currNode.getChildComponent(0);
		SceneGraphComponent defUseNode =new SceneGraphComponent();
		if(definition)
			defUseNode.setName("defined:"+name);
		else
			defUseNode.setName("used:"+name);
		defUseNode.addChild(c);
		givenState.currNode.addChild(defUseNode);
		if (givenState.trafo!=null)
			defUseNode.setTransformation(new Transformation(givenState.trafo.getMatrix()));
		// set Appearance color
		if(givenState.materialBinding==State.Binding.OVERALL||
				 givenState.materialBinding==State.Binding.DEFAULT){
		 		Appearance app= new Appearance();
				givenState.setColorApp(app,false);
				defUseNode.setAppearance(app);
			 }
	}
	private static void useMaterial(State givenState,State defState){
		State s=new State(defState);
		givenState.diffuse=s.diffuse;
		givenState.emissive=s.emissive;
		givenState.specular=s.specular;
		givenState.ambient=s.ambient;
		givenState.shininess=s.shininess;
		givenState.transparency=s.transparency;
		if(givenState.materialBinding==Binding.NONE)
			givenState.materialBinding=Binding.OVERALL;
	}	
	private static void useTexture(State givenState,State defState){
		State s=new State(defState);
		givenState.textureFile=s.textureFile;
		givenState.textureData=s.textureData;
		givenState.wrapS=s.wrapS;
		givenState.wrapT=s.wrapT;
	} 
	private static void useTrafo(State givenState,State defState){
		State s=new State(defState);
		if (defState.trafo==null) return;
		if (givenState.trafo==null)
			givenState.trafo= new Transformation(defState.trafo.getMatrix());
		else 
		givenState.trafo.multiplyOnRight(
				new State(defState).trafo.getMatrix());	
	} 
	private static void useShapeHints(State givenState,State s){
		givenState.vertOrder=s.vertOrder;
		givenState.shapeType=s.shapeType;
		givenState.faceType=s.faceType;
		givenState.creaseAngle=s.creaseAngle;
	} 

}
