//**************************************************
// * VRML 1.0 Parser
// */

header {
/*
 *	@author gonska
 *  Nov. october 12 2006
 *
 * TODO: 
 * USE-Problems:
 *  Maybe a DEF statement can be saved as special "State"
 *   instead of "SceneGraphComponent".
 *  Then you need to make a compare betwen the current "State"
 *   and the "DEF-State" in "USE".
 *  Then you generate the geometry from the merged "State".
 * TODO: 
 * mehrerfache Kameras(-Pfade)
 *   hier : die erste Kamera gilt.
 */
package de.jreality.reader.vrml;
import java.awt.Color;
import java.util.*;
import de.jreality.util.*;
import de.jreality.scene.*;
import de.jreality.math.*;
import de.jreality.geometry.*;
import de.jreality.shader.*;
import de.jreality.scene.data.*;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.SpotLight;

}

/*****************************************************************************
 * The VRML version 1.0 Parser
 *****************************************************************************
 */
class VRMLV1Parser extends Parser;
options {
	k = 2;							// two token lookahead
}
{	
	// Entwurfsentscheidung:
	// - hier werden keine Indexed*(-Set) zusammengefast!
	// - es wird auf globale Variablen weitgehend verzichtet
	//		statdessen gibt es die Klasse State die alle wichtigen Daten enthaelt
	// - das einhaengen der Geometrieen geschieht in shapeNode
	//		colorieren und die Normalen werden direkt bei der Geometrie behandelt
	// - die durchgereichten States werden stets direkt geaendert.
	// 		nur beim erschaffen eines neuen Knotens und beim erstellen einer Geometrie 
	// 		(oder DEF & USE) werden die States Kopiert ((als Kopie abgezweigt))
	// - Regeln die den State veraendern haben deshalb KEINEN Rueckgabewert 
	// 		da sie auf dem Original arbeiten.
	// - leider koennen einige Geometrieen nicht direkt erstellt werden:
	//		fuer ihre Richtigen Abmessungen muss man (noch) eine Transformationsmatrix
	//		in ihren Eltern-Knoten legen (Bsp: Sphere mit RADIUS = 3).
	// 		Desshalb gibt es im State eine Matrix extraGeoTrans die dufuer ungleich 
	//		der Identitaet gesetzt wird. Sie wird im ShapeNode behandelt.
	// - sfbitmaskValue werden eine Liste mit zulaessigen Namen uebergeben
	//		zurueckgegeben wird ein neuer BooleanArray der fuer die  
	//		geparsten Namen den entsprechenden Flag auf true gesetzt hat
	// - Erkennungs-Problem zwischen double(float) und int(long):
	//		betrachte filefragment: " 2   .5    1.5 "
	//		der Lexer erkennt entweder [2] [.5] [1.5] als  Double-Double-Double
	//      oder [2] [.] [5] [1] [.] [5] als  int-DOT-int-int-DOT-int
	//		Da die Zeichen nur durch WhiteSpaces getrennt sind,
	//		muessen wir entweder:
	//			- alles als Dezimalzahlen lexen und im Kontext wissen
	//				ob wir ein double oder int erwarten.
	//			- oder beim Lexen aufwendig erkennen ob es ein double oder int ist.
	//				wobei ein int ja stets auch ein double ist!
	//				Dann im Gebrauch erfragen ob int oder Double vorhanden ist.
	//			- Zusammenparsen von (int-Dot-int) fuehrt zu Fehlern!
	//				Bsp oben: [2.5] [1.5] als double double
	// 		Entscheidung: wir lexen nur	positive Dezimalzahlen da hier im Kontext stets
	//				klar ist, ob int, double oder boolean gefordert ist
	//				'+' und '-' und 'e' werden hinzugeparst,
	//				der Exponent wird als double gelext und hinzugeparst 
	//				bsp1: -0.34e+12 
	//				gelext: [-] [0.34] [e] [+] [12] 
	//						als '-' DOUBLE 'e' '+' DOUBLE
	//				bsp2: 12  .3 4 e12.2
	//				gelext:  [12] [.3] [4] [e] [12.2]
	//				geparst: [12] [0.3] [4*10^(12.2)] 
	//		    Ein dezimaler Exponent wird also wirklich einer. Aber mit Warnung.
	//				bsp3: "12.3e4 .3" 
	//				gelext: [12.3] [e] [4] [.3]
	//				geparst: [12.3*10^4] [0.3]
	// 	- Weil '.' als Teil einer Zahl gelext wird kann es nicht als Token zur verfuegung stehen!


	SceneGraphPath camPath = null;
	SceneGraphComponent root = null;

//	int currentNormalBinding = VRMLHelper.DEFAULT;
//	int currentMaterialBinding = VRMLHelper.DEFAULT;
	
	// we use a dynamic allocation scheme, beginning with arrays of length 10000
	final int INITIAL_SIZE = 10000;
	double[] ds = new double[INITIAL_SIZE];			// for storing double arrays
	Color[] cs = new Color[INITIAL_SIZE];			// for storing Color arrays
	int[] ls = new int[INITIAL_SIZE];				// for storing int arrays
	String [] strs = new String [INITIAL_SIZE]; 	// for storing vec2 & vec3 arrays
	double [][] vecs = new double [INITIAL_SIZE][]; // for storing vec2 & vec3 arrays
	boolean generateEdgesFromFaces = true;		// this should be a flag controllable from outside
//	double[] tempVector3 = new double[3];
//	boolean collectingMFVec3 = false;		
	
	DefUseData defs= new DefUseData(); // class that manages the Def & Use
	int numOfSwitchNodes=0;
}

/// __________________________________________________________________________
/// _____________________________START________________________________________
/// __________________________________________________________________________
vrmlFile returns [SceneGraphComponent r=null]
		:
		HEADER
		{
			// Start-State initialisieren:
			root = new SceneGraphComponent();
			SceneGraphPath p = new SceneGraphPath();
			p.push(root);
			Transformation t = null;
			State state = new State();
			state.diffuse=new Color[]{};
			state.trafo=t;
			state.currNode=root;
			state.camPath=p;
			state.transparency= new double[]{1};
			//root.setAppearance(VRMLHelper.defaultApp());
			}
		(statement[state] {state.history = state.history+"*";}
		(statement[state] {state.history = state.history+"*";
						   System.err.println("Warning: multiple basic Nodes"
							 +"found (and processed). This is not VRML-Standard !");})*
		)?
		{ r = root; }
	;

// --------------------- Basic Node Types -------------------------
	
private
statement[State state]:
		defNode [state]
	|	useNode [state]
	|	node[state]
	;

private
defNode [State state]
{if (VRMLHelper.verbose) System.err.print(state.history+"Def Node(");
 String name="";
 State fake = DefUseData.defState(state);
}:		"DEF"	
		name = sfstringValue
		{if (VRMLHelper.verbose) System.err.println(name+"):");}
		statement[fake]
	{
		defs.addDef(fake,name);
		if (VRMLHelper.verbose) System.err.print("DEF End(");
		defs.use(state,name,true); 
		if (VRMLHelper.verbose) System.err.println("DEF used)");
	}
	;

private
useNode [State state]
{if (VRMLHelper.verbose) System.err.print(state.history+"Use Node(");
 String name="";
}:		"USE"	
		name = sfstringValue
		{if (VRMLHelper.verbose) System.err.print(name);}
	{
		defs.use(state,name,false);
		if (VRMLHelper.verbose) System.err.println(")");
		System.out.println("Attention: The USE-Statement is not completely supported and may be read incorrect!");
	}
	;


private
node[State state]
{if (VRMLHelper.verbose) System.err.print(state.history+"Got Node: ");
}
	:(  groupNode					[state]{state.defTyp=DefUseData.KNOT;}
  	  | shapeNode					[state]{state.defTyp=DefUseData.KNOT;}
	  | propertyGeoNAppNode			[state]
	  | propertyMatrixTransformNode	[state]{state.defTyp=DefUseData.TRAFO;}
	  | specialNode				 	[state]
	  | strangeNode 		         // fremd definierter Node (ignorieren)
	 )
	;


private
specialNode[State state]
 {if (VRMLHelper.verbose) System.err.print("special Node: ");}
	:( camNode	[state]  {state.defTyp=DefUseData.KNOT; System.out.println("def-Cam Problem?");}
	  | lightNode [state]{state.defTyp=DefUseData.KNOT;}
	  | "WWWInline" 		egal	 //[state] TODO3
	 )
	;
	
private
strangeNode
{String s;}
	:	s=id 	egal
		{System.out.println("unknown Node:"+s+" -Node ignored!");}
	;
	

// ------------------------------ group Nodes -----------------------------

private
groupNode [State state] 
 {if (VRMLHelper.verbose) System.err.print("group Node: ");}
	:(  separatorNode   [state]
	  |	switchNode		[state]
	  |	"WWWAnchor"		egal // TODO3
	  |	"LOD"			egal // TODO3
	 )
 	;


private
separatorNode[State state]
{
 if (VRMLHelper.verbose) System.err.println("Separator"); 
 State state2= new State(state);
 Transformation t= state2.trafo;
 Appearance app=null;
 if(state2.materialBinding==State.Binding.OVERALL||
 	state2.materialBinding==State.Binding.DEFAULT){
 	app= new Appearance();
	state2.setColorApp(app,false);
 	state2.materialBinding=State.Binding.NONE;
 }
 state2.trafo=null;
 state2.history=state.history+"|";
 { if (VRMLHelper.verbose) System.err.println(state.history+"\\"); }
		
}:
		g:"Separator"
			{
				SceneGraphComponent sgc = new SceneGraphComponent();
				if (t!=null)
					sgc.setTransformation(t);
				if (app!=null)
					sgc.setAppearance(app);
				sgc.setName("Knot LineNo "+g.getLine()); // for looking up later
				state2.currNode.addChild(sgc);
				state2.currNode=sgc;
				state2.camPath.push(sgc);
			}
		OPEN_BRACE	
			(statement[state2])*
		CLOSE_BRACE	
		{ if (VRMLHelper.verbose) System.err.println(state.history+"/"); }
		
	;
	
private
switchNode[State state]
{
 if (VRMLHelper.verbose) System.err.println("switch"); 
 State state2= new State(state);
 Transformation t= state2.trafo;
 state2.trafo=null;
 state2.history=state.history+"-";
 { if (VRMLHelper.verbose) System.err.println(state.history+"\\"); }
		
}:
		g:"Switch"
			{
				SceneGraphComponent sgc = new SceneGraphComponent();
				if (t!=null)
					sgc.setTransformation(t);
				state2.currNode.addChild(sgc);
				state2.currNode=sgc;
				state2.camPath.push(sgc);
				if (numOfSwitchNodes>0) 
					sgc.setVisible(false);
				numOfSwitchNodes++;
				sgc.setName("Switch Nr "+numOfSwitchNodes);
			}
		OPEN_BRACE	
			(statement[state2])?
		CLOSE_BRACE	
		{ if (VRMLHelper.verbose) System.err.println(state.history+"/"); }
	;

// ------------------------------ shape Nodes ------------------------------

private
shapeNode [State state]
{
if (VRMLHelper.verbose) System.err.print("Shape Node: ");
State state2= new State(state);
Geometry geo=null;
Appearance app= new Appearance();
SceneGraphComponent sgc= new SceneGraphComponent();
}	:
	(
		geo = asciiTextNode			[state2,app]
	|	geo = coneNode				[state2,app]
	|	geo = cubeNode				[state2,app]
	|	geo = cylinderNode			[state2,app]
	|	geo = indexedFaceSetNode 	[state2,app]
	|	geo = indexedLineSetNode 	[state2,app]
	|	geo = pointSetNode			[state2,app]
	|	geo = sphereNode 			[state2,app]
	)
		{ 	
			if (geo==null){System.out.println("failure in geometry. Abort Node!");}
			else{
				sgc.setName(geo.getName());
				state2.currNode.addChild(sgc);
				sgc.setGeometry(geo);
				state2.setColorApp(app,false);
				sgc.setAppearance(app);
				state2.setTrafo(sgc);
			}
		}
	;


private 
//TODO3: material,texture,justification,width,spacing
asciiTextNode [State state, Appearance app] returns[PointSet label=null]
{
  if (VRMLHelper.verbose) System.err.print("Label( ");
  String just="LEFT";
  String [] text = new String[]{""};
  double spacing=1;
  String[] code= new String[]{ 	"LEFT","CENTER","RIGHT"	};
  double[] width =new double[]{0};
}	
	:	"AsciiText" OPEN_BRACE
		(	"string" 		{if (VRMLHelper.verbose) System.err.print("String ");}
				text = mfstringValue
		  | "spacing" 		{if (VRMLHelper.verbose) System.err.print("spacing ");}
		  		spacing = sffloatValue
		  | "justification" {if (VRMLHelper.verbose) System.err.print("justif. ");}
		  		just = sfenumValue
		  | "width" 		{if (VRMLHelper.verbose) System.err.print("width ");}
		  		width = mffloatValue
		  | wrongAttribute
		   )* CLOSE_BRACE
		{
			int justif = VRMLHelper.getEnum(code, just);
			String[] text2= new String[]{VRMLHelper.mergeStrings(text)};
			label = new PointSet();
			label.setNumPoints(1);
			double[][] d=new double[][]{{0,0,0}};
			label.setVertexAttributes(Attribute.COORDINATES,new DoubleArrayArray.Array(d));
			label.setVertexAttributes(Attribute.LABELS, new StringArray(text2));
			label.setName("Label");

			if (VRMLHelper.verbose) System.err.println(")");
			state.extraGeoTrans = new Transformation();		
			state.edgeDraw=2;
			state.vertexDraw=0;
			state.faceDraw=2;
		}
	;
		  
		  
private
coneNode [State state, Appearance app] returns [IndexedFaceSet cone=null]
{//TODO3: material,texture
  if (VRMLHelper.verbose) System.err.print("Cone( ");
  String[]  parts = new String[]{"SIDES","BOTTOM","ALL"};
  boolean[] partsMask=null;
  double r=1;
  double h=2;
  boolean sideDraw=false;
  boolean bottomDraw=false;
}
  	:
	    "Cone"	OPEN_BRACE	(
				("parts"  {if (VRMLHelper.verbose) System.err.print("parts ");}
					partsMask = sfbitmaskValue[parts] )
			| 	("bottomRadius" {if (VRMLHelper.verbose) System.err.print("bottomRadius ");}
					r = sffloatValue)	
			| 	("height"		{if (VRMLHelper.verbose) System.err.print("height ");}
					h = sffloatValue)
			|	wrongAttribute
		)* CLOSE_BRACE
	{	if (partsMask==null)		{bottomDraw=true; sideDraw=true;}
		else{
			if ((partsMask[2])|	!(partsMask[0]|partsMask[1]))
			    	{bottomDraw=true; sideDraw=true;}
			if (partsMask[0])	{sideDraw=true;	 }
			if (partsMask[1])	{bottomDraw=true;}
		}
		cone = VRMLHelper.cone(sideDraw,bottomDraw,20);
		cone.setName("cone"); 
		state.extraGeoTrans = new Transformation();
		state.edgeDraw=2;
		state.vertexDraw=2;
		state.faceDraw=0;
		MatrixBuilder.euclidean().scale(r,h,r).translate(0,-0.5,0).assignTo(state.extraGeoTrans);
		if (VRMLHelper.verbose) System.err.println(")");
		}
	;

private
cubeNode [State state, Appearance app]returns [IndexedFaceSet cube=null]
{//TODO3: material,texture
  if (VRMLHelper.verbose) System.err.print("Cube( ");
  double w=2;
  double h=2;
  double d=2;
}	
	:
	"Cube"	OPEN_BRACE	(
			"width"	{if (VRMLHelper.verbose) System.err.print("width ");}
				w = 	sffloatValue
		| 	"height" {if (VRMLHelper.verbose) System.err.print("height ");}
				h = 	sffloatValue
		| 	"depth"	 {if (VRMLHelper.verbose) System.err.print("depth ");}
				d = 	sffloatValue
		|	wrongAttribute
		)* CLOSE_BRACE
	{
		cube = Primitives.cube(false);
		cube.setName("cube");
		state.extraGeoTrans = new Transformation();
		state.edgeDraw=2;
		state.vertexDraw=2;
		state.faceDraw=0;
		MatrixBuilder.euclidean().scale(w/2,h/2,d/2).assignTo(state.extraGeoTrans);
		if (VRMLHelper.verbose) System.err.println(")");
	}
	;

private
cylinderNode [State state, Appearance app]returns [IndexedFaceSet cylinder=null]
{//TODO3: material,texture
  if (VRMLHelper.verbose) System.err.print("Cylinder( ");
  String[]  parts = new String[]{"SIDES","TOP","BOTTOM","ALL"};
  boolean[] partsMask=null;
  double r=1;
  double h=2;
}	
	:
	"Cylinder"	OPEN_BRACE	(
				"parts"	{if (VRMLHelper.verbose) System.err.print("parts ");}
					partsMask = sfbitmaskValue[parts] 
			| 	"radius" {if (VRMLHelper.verbose) System.err.print("radius ");}
					r = sffloatValue
			| 	"height" {if (VRMLHelper.verbose) System.err.print("height ");}
					h = sffloatValue
			|	wrongAttribute
		)* CLOSE_BRACE
	{
		if (partsMask==null) 		
			cylinder = VRMLHelper.cylinder(true,true,true,20);
		else{
			if (partsMask[3])
				cylinder = VRMLHelper.cylinder(true,true,true,20);
			else 
				if (!(partsMask[0]|partsMask[1]|partsMask[2]))
					cylinder = VRMLHelper.cylinder(true,true,true,20);
				else
					cylinder = VRMLHelper.cylinder(partsMask[0],partsMask[1],partsMask[2],20);
		}
		cylinder.setName("cylinder");
		state.extraGeoTrans = new Transformation();
		state.edgeDraw=2;
		state.vertexDraw=2;
		state.faceDraw=0;
		MatrixBuilder.euclidean().scale(r,h,r).assignTo(state.extraGeoTrans);
		if (VRMLHelper.verbose) System.err.println(")");
	}
	;

private
indexedFaceSetNode [State state, Appearance app] returns [IndexedFaceSet ifs=null]
{//TODO3: texture
  if (VRMLHelper.verbose) System.err.print("IndexedFaceSet( "); 
  int[] coordIndex	= new int[]{0};
  int[] materialIndex	= new int[]{};
  int[] normalIndex	= new int[]{};
  int[] textureCoordIndex	= new int[]{};
 }
	:
	"IndexedFaceSet"		OPEN_BRACE	
	(   "coordIndex"		{  if (VRMLHelper.verbose) System.err.print("coordIndex "); }
			coordIndex 			= mflongValue 
	  | "materialIndex" 	{  if (VRMLHelper.verbose) System.err.print("materialIndex "); }
	  		materialIndex		= mflongValue
	  | "normalIndex" 		{  if (VRMLHelper.verbose) System.err.print("normalIndex "); }
			normalIndex			= mflongValue
	  | "textureCoordIndex"	{  if (VRMLHelper.verbose) System.err.print("textureCoordIndex "); }
			textureCoordIndex	= mflongValue
	  |	wrongAttribute	)*
	CLOSE_BRACE
	{
	int[][] coordIndex2 = VRMLHelper.convertIndexList(coordIndex);
    int[][] materialIndex2 = VRMLHelper.convertIndexList(materialIndex);
	int[][] normalIndex2 = VRMLHelper.convertIndexList(normalIndex);
	int[][] textureCoordIndex2 = VRMLHelper.convertIndexList(textureCoordIndex);
	
	IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
	ifsf.setVertexCount(state.coords.length);
	ifsf.setFaceCount(coordIndex2.length);
	ifsf.setVertexAttribute(Attribute.COORDINATES,new DoubleArrayArray.Array(state.coords) );
	ifsf.setFaceIndices(coordIndex2);

	if (state.haveToSeparateVertices()){
		int[] reffTab=VRMLHelper.separateVerticesAndVNormals(coordIndex2,state);
		// make new Factory
		ifsf = new IndexedFaceSetFactory();
		ifsf.setVertexCount(reffTab.length);//(new)
		ifsf.setFaceCount(coordIndex2.length);//(new)
		ifsf.setVertexAttribute(Attribute.COORDINATES,new DoubleArrayArray.Array(state.coords) );//new
		ifsf.setFaceIndices(coordIndex2);//new
		// now all indices of texture, color, normals, ect are unique because 
		// they are based on face indices and coords
	}
	VRMLHelper.setNormals(ifsf,coordIndex2,normalIndex2,state);//werden nicht generiert wenn vorhanden
	VRMLHelper.setColors(ifsf,coordIndex2,materialIndex2,state);
	ifsf.setGenerateEdgesFromFaces(generateEdgesFromFaces);
	ifsf.update();
	ifs = ifsf.getIndexedFaceSet();
	state.assignTexture(app, ifs);
	ifs.setName("Face Set");
	state.extraGeoTrans = new Transformation();
//	state.edgeDraw=2;
//	state.vertexDraw=2;
	state.faceDraw=0;
	if (VRMLHelper.verbose) System.err.println(")");
	}
	;
	
private
indexedLineSetNode[State state, Appearance app] returns [IndexedLineSet ils=null]
{ //TODO3: normal,texture
  State state2= new State(state);
  if (VRMLHelper.verbose) System.err.print("IndexedLineSet( "); 
  int[] coordIndex	= new int[]{0};
  int[] materialIndex	= new int[]{};
  int[] normalIndex	= new int[]{};
  int[] textureCoordIndex	= new int[]{};
}
	:
	"IndexedLineSet"		OPEN_BRACE	
	(   "coordIndex" {if (VRMLHelper.verbose) System.err.print("coordIndex ");}
	 		coordIndex 			= mflongValue
	  | "materialIndex" {if (VRMLHelper.verbose) System.err.print("materialIndex ");}
	  	 	materialIndex		= mflongValue
	  | "normalIndex" {if (VRMLHelper.verbose) System.err.print("normalIndex ");}
	  		normalIndex			= mflongValue
	  | "textureCoordIndex"	{if (VRMLHelper.verbose) System.err.print("textureCoordIndex ");}
	  		textureCoordIndex	= mflongValue
	  |	wrongAttribute	)*
	CLOSE_BRACE
	{
	int[][] coordIndex2 = VRMLHelper.convertIndexList(coordIndex);
    int[][] materialIndex2 = VRMLHelper.convertIndexList(materialIndex);
	int[][] normalIndex2 = VRMLHelper.convertIndexList(normalIndex);
	int[][] textureCoordIndex2 = VRMLHelper.convertIndexList(textureCoordIndex);
	
	IndexedLineSetFactory ilsf = new IndexedLineSetFactory();
	ilsf.setVertexCount(state.coords.length);
	ilsf.setLineCount(coordIndex2.length);
	ilsf.setVertexAttribute(Attribute.COORDINATES,new DoubleArrayArray.Array(state2.coords) );
	ilsf.setEdgeIndices(coordIndex2);
	VRMLHelper.setColors(ilsf,coordIndex2,materialIndex2,state2);
	// TODO2: handle Normals
	// Normals:	if (normalIndex2.length>0){}else {}
	// Texture:	if (textureCoordIndex2.length>0){}else {}

	ilsf.update();
	ils = ilsf.getIndexedLineSet();
	ils.setName("Line Set");
	state.extraGeoTrans = new Transformation();
//	state.vertexDraw=2;
	state.edgeDraw=0;
	state.faceDraw=0;
	if (VRMLHelper.verbose) System.err.println(")");
	}
	;

private 
pointSetNode [State state, Appearance app] returns[PointSet ps=null]
{//TODO3: material,normal
  if (VRMLHelper.verbose) System.err.print("PointSet( "); 
  int start=0;
  int num=-1;
}
	: "PointSet"		OPEN_BRACE
		( "startIndex" {if (VRMLHelper.verbose) System.err.print("startIndex ");}
				 start = sflongValue
		 |"numPoints"  {if (VRMLHelper.verbose) System.err.print("numPoints ");}
		 		 num   = sflongValue		
		 | wrongAttribute )*
	CLOSE_BRACE
	{
	double[][] coords2 = new double[num][];
	System.arraycopy(state.coords,start,coords2,0,num);
	ps = new PointSet();
	ps.setNumPoints(num);
	ps.setVertexAttributes(Attribute.COORDINATES,new DoubleArrayArray.Array(coords2));
	// TODO2: handle Normals
	VRMLHelper.setColors(ps,state,start,num);
	ps.setName("Point Set");
	state.extraGeoTrans = new Transformation();
	state.vertexDraw=0;
	state.edgeDraw=0;
	state.faceDraw=0;
	{if (VRMLHelper.verbose) System.err.println(")");}
	}
	;

private
sphereNode [State state, Appearance app] returns [Geometry sphere=null]
{//TODO3:texture
 if (VRMLHelper.verbose) System.err.print("Sphere( ");
 double r=1;
}	
	:
	"Sphere"	OPEN_BRACE	(
		"radius" {if (VRMLHelper.verbose) System.err.print("radius ");}
				r = 	sffloatValue
		| wrongAttribute 
		)* CLOSE_BRACE
	{
		sphere = new Sphere(); //Primitives.sphere(20);
		sphere.setName("Sphere");
		state.extraGeoTrans=new Transformation();
		MatrixBuilder.euclidean().scale(r).assignTo(state.extraGeoTrans);
		state.edgeDraw=2;
		state.vertexDraw=2;
		state.faceDraw=0;
		{if (VRMLHelper.verbose) System.err.println(")");}
	}
	;
		
/// ******************* shape Nodes done *****************
// ------------------------------ property Geometry App Nodes ------------------------------

private
propertyGeoNAppNode [State state] 
 {if (VRMLHelper.verbose) System.err.print("prop Geo Node: ");}
	:(	coordinate3Node 		[state]{state.defTyp=DefUseData.COORDS;}
	  |	"FontStyle"				egal 		// TODO2
	  |	infoNode				[state]
	  |	materialNode			[state]
	  		{
	  			state.defTyp=DefUseData.MATERIAL;
				if(state.materialBinding==State.Binding.NONE)
					state.materialBinding=State.Binding.OVERALL;
			}
	  |	baseColorNode			[state]
	  	  		{
	  			state.defTyp=DefUseData.MATERIAL;
				if(state.materialBinding==State.Binding.NONE)
					state.materialBinding=State.Binding.OVERALL;
				}
	  |	materialBindingNode		[state]{state.defTyp=DefUseData.BIND_M;}
	  								// nur fuer IFS & ILS implementiert TODO3: rest
	  |	normalNode				[state]{state.defTyp=DefUseData.NORMALS;}
	  |	normalBindingNode		[state]{state.defTyp=DefUseData.BIND_N;}
	  								// nur fuer IFS implementiert TODO3: rest
	  |	texture2Node			[state]{state.defTyp=DefUseData.TEXTURE;}
	  |	texture2TransformNode	[state]{state.defTyp=DefUseData.TEXTURE_TRAFO;}
	  |	textureCoordinate2Node	[state]{state.defTyp=DefUseData.TEXTURE_COORDS;}
	  |	shapeHintsNode			[state]{state.defTyp=DefUseData.SHAPE_HINTS;}
	  								// eigentlich egal
	 )
	;


private
coordinate3Node [State state] 
{
 if (VRMLHelper.verbose) System.err.println("Coordinate3");
 double[][] d={};
 }
	: "Coordinate3"	OPEN_BRACE	
		("point" 	d = mfvec3fValue
		| wrongAttribute)?
	  CLOSE_BRACE							
	{ state.coords=d;}
	;

//private
// TODO3:
//fontStyleNode[State state]:;

private
infoNode [State state]
{if (VRMLHelper.verbose) System.err.println("Info:{ "); 
 String s = null;}
	: "Info"	OPEN_BRACE
	 ( "string" s=sfstringValue
	  	{if (VRMLHelper.verbose) System.err.println(state.history+"    "+s); }
	   | wrongAttribute )*
	  CLOSE_BRACE
	{if (VRMLHelper.verbose) System.err.println(state.history+"}"); }
  
	;
	
private
materialNode [State state]
{ 
 if (VRMLHelper.verbose) System.err.print("Material( ");
 Color[] c=null;
 double[] d=null;
 Color[] amb=new Color[]{};
 Color[] dif=new Color[]{};
 Color[] spe=new Color[]{};
 Color[] emi=new Color[]{};
 double [] shi= new double[]{};
 double [] tra= new double[]{};
}	:
	"Material" OPEN_BRACE
	  (	 "ambientColor"	{ if (VRMLHelper.verbose) System.err.print("ambientColor ");}
	  			c = mfcolorValue {amb=c;}
	  	|"diffuseColor"		{ if (VRMLHelper.verbose) System.err.print("diffuseColor ");}
	  			c = mfcolorValue {dif=c;}
	  	|"specularColor"	{ if (VRMLHelper.verbose) System.err.print("specularColor ");}
	  			c = mfcolorValue {spe=c;}
	  	|"emissiveColor"	{ if (VRMLHelper.verbose) System.err.print("emissiveColor ");}
	  			c = mfcolorValue {emi=c;}// TODO3 emissive realisieren
	  	|"shininess"		{ if (VRMLHelper.verbose) System.err.print("shininess ");}
	  			d = mffloatValue {shi=d;}
	  	|"transparency"	{ if (VRMLHelper.verbose) System.err.print("transparency ");}
	  			d = mffloatValue {tra=d;}
	  	| wrongAttribute
	  )*
	  CLOSE_BRACE 	{
		  state.ambient=amb;
		  state.diffuse=dif;
		  state.specular=spe;
		  state.emissive=emi; 
		  state.shininess=shi;
		  state.transparency=tra;
	      if (VRMLHelper.verbose) System.err.println(")");
	   }
	;
private
baseColorNode [State state]
{ 
 if (VRMLHelper.verbose) System.err.print("BaseColor( ");
 Color[] c=null;
 Color[] rgb=new Color[]{new Color(0.8f,0.8f,0.8f)};
}	:
	"BaseColor" OPEN_BRACE
	  (	"rgb"		{ if (VRMLHelper.verbose) System.err.print("rgb ");}
	  			c = mfcolorValue {rgb=c;}
	  	| wrongAttribute
	  )*
	  CLOSE_BRACE 	{
		  state.diffuse=rgb;
	      if (VRMLHelper.verbose) System.err.println(")");
	   }
	;

private
materialBindingNode [State state]
{if (VRMLHelper.verbose) System.err.println("Material Binding");
 String mb="OVERALL"; }
	:
	"MaterialBinding"	OPEN_BRACE
		("value"  mb = sfenumValue 
		 |wrongAttribute )?
	CLOSE_BRACE
		{ state.materialBinding = State.getBinding(mb);	}
	;

private
normalNode [State state]
{ if (VRMLHelper.verbose)	System.err.println("Normals"); 
  double[][] normals={};
}
	:
	"Normal"	OPEN_BRACE	
	("vector" 	normals=mfvec3fValue
	| wrongAttribute)?
	CLOSE_BRACE
	{ state.normals=normals; }	
	;

private
normalBindingNode [State state]
{ if (VRMLHelper.verbose) System.err.print("normalBinding( ");
 String nb ="DEFAULT";}
	:
	"NormalBinding"	OPEN_BRACE
		("value"  {if (VRMLHelper.verbose) System.err.print("value ");}
				nb = sfenumValue 
		| wrongAttribute)?
	 CLOSE_BRACE
	 	{ state.normalBinding = State.getBinding(nb);
	 	 if (VRMLHelper.verbose) System.err.println(")");
	 	}
	;

private
texture2Node[State state]
{   if (VRMLHelper.verbose) System.err.print("texture2( ");
	String file="";
	int[][][] image = new int[][][]{{{}}};
	String[] code = new String[]{"REPEAT","CLAMP"};
	String wrapS="REPEAT";
	String wrapT="REPEAT";
}	: "Texture2" OPEN_BRACE
		("filename" { if (VRMLHelper.verbose) System.err.print("filename ");}
				file=sfstringValue
		| "image" 	{ if (VRMLHelper.verbose) System.err.print("image ");}
				image=sfimageValue
		| "wrapS"	{ if (VRMLHelper.verbose) System.err.print("wrapS ");}
				wrapS=sfenumValue
		| "wrapT"	{ if (VRMLHelper.verbose) System.err.print("wrapT ");}
				wrapT=sfenumValue
		| wrongAttribute )*
	CLOSE_BRACE
	{
	 state.textureFile=file;
	 state.textureData=image;
	 // wrap: 0=repeat 1= clamp
	 state.wrapS=VRMLHelper.getEnum(code,wrapS);
	 state.wrapT=VRMLHelper.getEnum(code,wrapT);
	 if (VRMLHelper.verbose) System.err.println(")");
	 }
;

private
texture2TransformNode[State state]
{   if (VRMLHelper.verbose) System.err.print("texture2Transform( ");
	double[] trans= new double[]{0,0};
	double 	 rot =	0;
	double[] scale= new double[]{1,1};
	double[] center=new double[]{0,0};
}
	: "Texture2Transform" OPEN_BRACE
		( "translation" { if (VRMLHelper.verbose) System.err.print("translation ");}
				trans=sfvec2fValue
		| "rotation" 	{ if (VRMLHelper.verbose) System.err.print("rotation ");}
				rot=sffloatValue
		| "scaleFactor"	{ if (VRMLHelper.verbose) System.err.print("scaleFactor ");}
				scale=sfvec2fValue
		| "center"	{ if (VRMLHelper.verbose) System.err.print("center ");}
				center=sfvec2fValue
		| wrongAttribute )*
	CLOSE_BRACE
	{
	 MatrixBuilder.euclidean()
		.translate(trans[0],trans[1],0)
		.translate(center[0],center[1],0)
		.rotate(rot,0,0,1)
		.scale(scale[0],scale[1],1)
		.translate(-center[0],-center[1],0)
		.assignTo(state.textureTrafo);
	 if (VRMLHelper.verbose) System.err.println(")");
	 }
;

private
textureCoordinate2Node[State state]
{
 if (VRMLHelper.verbose) System.err.print("TextureCoordinate2( ");
 double [][] point= new double[][]{{0,0}};
}
	: "TextureCoordinate2" OPEN_BRACE 
	  ( "point"		{ if (VRMLHelper.verbose) System.err.print("point ");}
	  			point= mfvec2fValue
		 | wrongAttribute )?
		CLOSE_BRACE
		{ state.textureCoords=point;
		  if (VRMLHelper.verbose) System.err.println(")"); 
		}
	;

private
shapeHintsNode [State state]
{ if (VRMLHelper.verbose) System.err.println("ShapeHints");
 int vertOrd=0;
 int shape=0;
 int face=0;
 double crease=0.5;
 String s;
}:
		"ShapeHints"	OPEN_BRACE
		( "vertexOrdering"  s = sfenumValue 
			{state.vertOrder=VRMLHelper.getEnum(State.VERTORDER,s);}
		 |"shapeType"		s = sfenumValue
 			{state.shapeType=VRMLHelper.getEnum(State.SHAPETYPE,s);}
		 |"faceType"		s = sfenumValue
			{state.faceType=VRMLHelper.getEnum(State.FACETYPE,s);}
		 |"creaseAngle"		crease = sffloatValue
		 	{state.creaseAngle=crease;}
		 | wrongAttribute
		)*
		CLOSE_BRACE		
		{// TODO3: handle Hints;
		}
	;

// ************* prop Geo Nodes done *********************
// ------------------------------ property Matrix transform Nodes ------------------------------

private
propertyMatrixTransformNode [State state]
{
if (VRMLHelper.verbose) System.err.print("Prop Matrix Node: ");
Transformation m= new Transformation();
}	:
	(	m = matrixTransformNode
	|	m = rotationNode
	|	m = scaleNode
	| 	m = transformNode
	| 	m = translationNode	)
		{ 	
		 if (state.trafo==null)
		 	state.trafo= new Transformation();
		 state.trafo.multiplyOnRight(m.getMatrix());
		}
	;

private
matrixTransformNode returns[Transformation t= new Transformation()]
{if (VRMLHelper.verbose) System.err.println("Matrix Transform");
double[] mat = null;}
	: "MatrixTransform"		OPEN_BRACE
	   ( "matrix"	mat = sfmatrixValue
	   | wrongAttribute )?
	  CLOSE_BRACE
	{t= new Transformation(Rn.transpose(null, mat));}
	;

private
rotationNode returns [Transformation m =new Transformation() ]
{if (VRMLHelper.verbose) System.err.println("Rotation");
double[] d=new double[]{0,0,1,0};
}	:
	"Rotation"	OPEN_BRACE	
	("rotation" d=sfrotationValue
	| wrongAttribute	)?
	CLOSE_BRACE
	{
		double[] axis=new double[3];
		System.arraycopy(d,0,axis,0,3);
		MatrixBuilder.euclidean().rotate(d[3],axis).assignTo(m);
	}
	;

private
scaleNode returns [Transformation m =new Transformation() ]
{ if (VRMLHelper.verbose) System.err.println("Scale");
double[] d = new double[]{1,1,1};
}	: "Scale"	OPEN_BRACE
	   ( "scaleFactor" d=sfvec3fValue 
	   | wrongAttribute	)?
	 CLOSE_BRACE
	{ MatrixBuilder.euclidean().scale(d).assignTo(m); 
	}
	;

private
transformNode returns [Transformation m =new Transformation() ]
{	
 if (VRMLHelper.verbose) System.err.println("transform Node");
 double[] trans=new 	double[]{0,0,0};
 double[] rot=new 		double[]{0,0,1,0};
 double[] scaleF=new 	double[]{1,1,1};
 double[] scaleO=new 	double[]{0,0,1,0};
 double[] center=new 	double[]{0,0,0};
}	: "Transform"	OPEN_BRACE
	  ( "translation"		trans = sfvec3fValue	
	   |"rotation"			rot = sfrotationValue	
	   |"scaleFactor"		scaleF = sfvec3fValue	
	   |"scaleOrientation"	scaleO = sfrotationValue	
	   |"center"			center = sfvec3fValue
	   | wrongAttribute 
	  )*
	  CLOSE_BRACE
	{
	 MatrixBuilder.euclidean()
	  .translate(trans)
	  .translate(center)
	  .rotate(rot[3],rot[0],rot[1],rot[2])
	  .rotate(scaleO[3],scaleO[0],scaleO[1],scaleO[2])
	  .scale(scaleF)
	  .rotate(-scaleO[3],scaleO[0],scaleO[1],scaleO[2])
	  .translate(-center[0],-center[1],-center[2])
	  .assignTo(m);
	}
	;

private
translationNode returns [Transformation m =new Transformation() ]
{if (VRMLHelper.verbose) System.err.println("Translation");
 double[] d = new double[]{0,0,0};
}	:
	"Translation"	OPEN_BRACE 
	 ("translation" d = sfvec3fValue 
	 | wrongAttribute )?
	CLOSE_BRACE
	{ MatrixBuilder.euclidean().translate(d).assignTo(m);}
	;

// ******************** prop Matr TransFNode Done *****************
// ------------------------------ cam & light Nodes ------------------------------
private
camNode[State state]
{ if (VRMLHelper.verbose) System.err.print("Cam Node: ");
  Camera c=null;
  State state2= new State(state);
}
	:( c = orthographicCameraNode	[state2]
	  |c = perspectiveCameraNode	[state2]	
	 )
		{ 
			SceneGraphComponent sgc= new SceneGraphComponent();
			state.currNode.addChild(sgc);
			sgc.setName(c.getName());
			if (c!=null){
				//TODO3 mehrere KameraPfade
				if (camPath==null) 	camPath=state.camPath;
				sgc.setCamera(c);
				state2.setTrafo(sgc);
			}
		}
	;

private
lightNode [State state]
{ if (VRMLHelper.verbose) System.err.print("Light Node: ");
  Light l= null;
  State state2= new State(state);
}	:(	l= directionalLightNode [state2]
	 |	l= pointLightNode		[state2]
	 |	l= spotLightNode		[state2]
	 )
	{
		SceneGraphComponent sgc= new SceneGraphComponent();
		state2.currNode.addChild(sgc);
		sgc.setName(l.getName());
		sgc.setLight(l);
		state2.setTrafo(sgc);
		
	}
	;


private
perspectiveCameraNode [State state] returns [Camera cam=null]
{	
    if (VRMLHelper.verbose) System.err.print("perspective Cam( ");
	double[] pos = new double[]{0,0,1};
	double[] orient = new double[]{0,0,1,0};
	double fDist = 5;
	double heightA = 0.785398; // defaults
}	:
	"PerspectiveCamera"	OPEN_BRACE	
	(		"position"		{if (VRMLHelper.verbose) System.err.print("position ");}
				pos =sfvec3fValue
		|	"orientation"	{if (VRMLHelper.verbose) System.err.print("Orientation ");}
				orient =sfrotationValue
		|	"focalDistance"	{if (VRMLHelper.verbose) System.err.print("focalDistance ");}
				fDist =sffloatValue
		|	"heightAngle"	{if (VRMLHelper.verbose) System.err.print("heightAngle ");}
				heightA =sffloatValue
		|   wrongAttribute
	)*	CLOSE_BRACE 
	{
		double[] rotAx=new double[3];
	  	System.arraycopy(orient, 0, rotAx, 0, 3);	
		cam= new Camera();
		cam.setPerspective(true);
		double hAngle=heightA*180/Math.PI;
		cam.setFieldOfView(hAngle);// default=45
		cam.setFocus(3);
		cam.setName("perspective Cam");
		cam.setFocalLength(fDist);
		state.extraGeoTrans = new Transformation();
		MatrixBuilder.euclidean()
			.rotate(orient[3],rotAx)
			.translate(pos)
			.assignTo(state.extraGeoTrans);
		if (VRMLHelper.verbose) System.err.println(")");
	}
	;

private
orthographicCameraNode 	[State state] returns[Camera cam=null]
{
    if (VRMLHelper.verbose) System.err.print("orthographic Cam( ");
	double[] pos = new double[]{0,0,1};
	double[] orient = new double[]{0,0,1,0};
	double fDist = 5;
	double height = 2; // defaults
}:	"OrthographicCamera"	OPEN_BRACE	
	(		"position"		{if (VRMLHelper.verbose) System.err.print("position ");}
					pos =sfvec3fValue
		|	"orientation"	{if (VRMLHelper.verbose) System.err.print("Orientation ");}
					orient =sfrotationValue
		|	"focalDistance"	{if (VRMLHelper.verbose) System.err.print("focalDistance ");}
					fDist =sffloatValue
		|	"height"	    {if (VRMLHelper.verbose) System.err.print("height ");}
					height =sffloatValue
		|   wrongAttribute
	)*	CLOSE_BRACE 
	{ 
		cam= new Camera();
		cam.setPerspective(false);
		cam.setFieldOfView(45);
		cam.setFocus((height*6)/5);
		cam.setName("orthographic Cam");
		cam.setFocalLength(fDist);
		state.extraGeoTrans = new Transformation();
		MatrixBuilder.euclidean()
			.rotate(orient[3],orient[0],orient[1],orient[2])
			.translate(pos)
			.assignTo(state.extraGeoTrans);
		if (VRMLHelper.verbose) System.err.println(")");	
	}

;

private	pointLightNode	[State state] returns[PointLight l=null]
{if (VRMLHelper.verbose) System.err.print("Point Light( ");
  boolean on = true;
  double intens = 1;
  Color c = new Color(1f,1f,1f);
  double[] loc = new double[]{0,0,1};
}
: "PointLight"	OPEN_BRACE
		( "on" 	{if (VRMLHelper.verbose) System.err.print("on ");}
				on = sfboolValue
		 |"intensity"	{if (VRMLHelper.verbose) System.err.print("intensity ");}
		 		intens = sffloatValue
		 |"color"	{if (VRMLHelper.verbose) System.err.print("color ");}
		 		c = sfcolorValue
		 |"location" {if (VRMLHelper.verbose) System.err.print("location ");}
		 		loc = sfvec3fValue
		 | wrongAttribute
		)*
	  CLOSE_BRACE
	  {l = new PointLight();
		if (on)	l.setIntensity(intens);
		else 	l.setIntensity(0);
		l.setColor(c);
		l.setGlobal(false);
		l.setName("Point Light");
		state.extraGeoTrans = new Transformation();
		MatrixBuilder.euclidean().translate(loc)
			.assignTo(state.extraGeoTrans);
		if (VRMLHelper.verbose) System.err.println(")");
		}
	;
	
private
spotLightNode	[State state] returns[SpotLight l=null]
{if (VRMLHelper.verbose) System.err.print("Spot Light( ");
  boolean on = true;
  double intens = 1;
  Color c = new Color(1f,1f,1f);
  double[] loc = new double[]{0,0,1};
  double[] dir = new double[]{0,0,-1};
  double dropR=0;
  double cutA=0.785398;
}
: "SpotLight"	OPEN_BRACE
		( "on" 	{if (VRMLHelper.verbose) System.err.print("on ");}
				on = sfboolValue
		 |"intensity"	{if (VRMLHelper.verbose) System.err.print("intensity ");}
		 		intens = sffloatValue
		 |"color"	{if (VRMLHelper.verbose) System.err.print("color ");}
		 		c = sfcolorValue
		 |"location" {if (VRMLHelper.verbose) System.err.print("location ");}
		 		loc = sfvec3fValue
		 |"direction" {if (VRMLHelper.verbose) System.err.print("direction ");}
		 		dir = sfvec3fValue
		 |"dropOffRate" {if (VRMLHelper.verbose) System.err.print("dropOffRate ");}
		 		dropR = sffloatValue
		 |"cutOffAngle" {if (VRMLHelper.verbose) System.err.print("cutOffAngle ");}
		 		cutA  = sffloatValue
		 | wrongAttribute
		)*
	  CLOSE_BRACE
	  {
	    l = new SpotLight();
		if (on)	l.setIntensity(intens);
		else 	l.setIntensity(0);
		l.setColor(c);
		l.setGlobal(false);
		l.setName("Spot Light");
		state.extraGeoTrans = new Transformation();
		MatrixBuilder.euclidean()
			.translate(loc)
			.rotateFromTo(new double[]{0,0,1},dir)
			.assignTo(state.extraGeoTrans);
		l.setConeAngle(cutA);
		l.setDistribution(dropR);
		l.setFalloffA0(1);
		l.setFalloffA1(0);
		l.setFalloffA2(0);
		if (VRMLHelper.verbose) System.err.println(")");
		}
	;


private
directionalLightNode [State state] returns[DirectionalLight l=null]
{if (VRMLHelper.verbose) System.err.print("Dir Light( ");
  boolean on = true;
  double intens = 1;
  Color c = new Color(1f,1f,1f);
  double[] dir = new double[]{0,0,-1};
}
	: "DirectionalLight"	OPEN_BRACE
		( "on" 	{if (VRMLHelper.verbose) System.err.print("on ");}
				on = sfboolValue
		 |"intensity"	{if (VRMLHelper.verbose) System.err.print("intensity ");}
		 		intens = sffloatValue
		 |"color"	{if (VRMLHelper.verbose) System.err.print("color ");}
		 		c = sfcolorValue
		 |"direction" {if (VRMLHelper.verbose) System.err.print("direction ");}
		 		dir = sfvec3fValue
		 | wrongAttribute
		)*
	  CLOSE_BRACE
	  {
	    l = new DirectionalLight();
		if (on)	l.setIntensity(intens);
		else 	l.setIntensity(0);
		l.setColor(c);
		l.setGlobal(false);
		l.setName("Directional Light");
		state.extraGeoTrans = new Transformation();
		MatrixBuilder.euclidean().rotateFromTo(new double[]{0,0,-1},dir)
			.assignTo(state.extraGeoTrans);
		if (VRMLHelper.verbose) System.err.println(")");
		}
	;
        
// ---------------------------- Nodes ende -----------------------------------
// ---------------------------------------------------------------------------
/// ---------- Daten **** Done **** ------------------------------

// entwurfswentscheidung fuer Werte:
// - Achtung: ein Long ist hier ein 32Integer
// - keine allgemeinen Values (man weiss ja was man parst)
// --> Long lexen
//	-> Bool nur TRUE & False Lexen
//		-> Bool aus 0,1 und gelextem TRUE,FALSE Parsen
//		-> Float aus Long parsen

//		-> Field als LinkedList[Object] parsen? 
//			-> geht nicht da nicht klar ist was geparst wird!!
//				=> mus Fields fuer jeden MF neu schreiben
//		-> koennte eine FloatList[Anzahl={1,2,3,4,16}] machen ({Float,Vec2,Vec3&Color,Rotatieon,Matrix})
//		-> koennte Fields dann wenigstens fuer Color,Float,Vec2,vec3 machen
//				dann bliebe nur Long und String

private
id returns [String s]
{ s = ""; }
	:	
	n:ID  {s=n.getText();}	
	;

private 
sfbitmaskValue [String [] code] returns [boolean[] mask]
{ mask= new boolean[code.length];
  for(int i=0;i<code.length;i++)	mask[i]=false;
  String b="";
}
	:  	b=id  {VRMLHelper.checkFlag(code,mask,b);}
	|	LPAREN  b=id  {VRMLHelper.checkFlag(code,mask,b);}
		( T1	b=id  {VRMLHelper.checkFlag(code,mask,b);})*
		( T1 )? 
		RPAREN
	;
		
private
sfboolValue returns [boolean b]
{ b = false;
  int n=0; }
	:(n=intThing {
			if (n==0) b=false;
			else b=true; // TODO3: hier Fehler ausgeben bei n!=1
	 })
    |("TRUE" {b = true;})
    |("FALSE"{b = false;})
	;
	
private
sfcolorValue returns [Color c]
{ c = null;
  double r, g, b;
  int ro, ge, bl;
}
	:	r=doubleThing g=doubleThing b=doubleThing	
		{ro= (int)Math.round(r*255); ge=(int)Math.round(g*255); bl=(int)Math.round(b*255);
		 c = new Color(ro,ge,bl); }
	;
	
private
mfcolorValue returns [Color[] cl=null]
{Color c = null; 
 cs= new Color[INITIAL_SIZE];
 int i=0;
 }
	:(OPEN_BRACKET
		(   ( CLOSE_BRACKET{cl=new Color[]{};})
		  | ( c=sfcolorValue { cs[0]=c;i++;}
			  ( COLON c=sfcolorValue {	if (i==cs.length)	cs=VRMLHelper.reallocate(cs);
		  								cs[i]=c;i++;})*
			  ( COLON )?
			  CLOSE_BRACKET {
			  	cl=new Color[i];
			  	System.arraycopy(cs, 0, cl, 0, i);}
			)
		)
	  )
	 |(c = sfcolorValue	{cl = new Color[]{c};} )	
	;

private
sfenumValue returns[String s=""]
	:	s=id
	;

private
sffloatValue returns [double d=0]
	: d = doubleThing 
	;

private
mffloatValue returns [double[] dl=null]
{double d; 
 ds= new double[INITIAL_SIZE];
 int i=0;
 }
	:(OPEN_BRACKET
		(   ( CLOSE_BRACKET{dl=new double[]{};})
		  | ( d=sffloatValue { ds[0]=d;i++;}
			  ( COLON d=sffloatValue {	if (i==ds.length)	ds=VRMLHelper.reallocate(ds);
		  								ds[i]=d;i++;})*
			  ( COLON )?
			  CLOSE_BRACKET {
			  	dl=new double[i];
			  	System.arraycopy(ds, 0, dl, 0, i);}
			)
		)
	  )
	 |(d = sffloatValue	{dl = new double[]{d};} )	
	;

private 
sfimageValue returns[int[][][] colors = new int[][][]{{{}}} ]
{int width=0;
 int height=0;
 int colorDim=0;
 int[][] colL=null;
 int size=0;}
 	:	width=intThing height=intThing colorDim=intThing 
		{size=width*height;}
		colL=hexList[size,colorDim]
		{
		 colors=new int[width][height][colorDim];
		 for (int i=0;i<width;i++)
		 	for (int j=0;j<height;j++)
		 		for (int k=0;k<colorDim;k++)
		 			colors[i][j][k]=colL[i*height+j][k];
		}
	;

private 
hexList [int size, int dim] returns [int[][] cL= new int[size][dim]]
{int i=0;
 String s="";}
	:
	((g : HEXDEC {s=g.getText();}
	 |k : NUMBER {s=k.getText();} )
	 {	if(i<size)
		 	cL[i]=VRMLHelper.decodeColorFromString(dim,s);
	 	i++; }
	)*
	;

private
sflongValue returns [int i]
{ i = 0;}
	: i=intThing
	;

private
mflongValue returns [int[] ll=null]
{int l; 
 ls= new int[INITIAL_SIZE];
 int i=0;
 }
	:(OPEN_BRACKET
		(   ( CLOSE_BRACKET{ll=new int[]{};})
		  | ( l=sflongValue { ls[0]=l;i++;}
			  ( COLON l=sflongValue {	if (i==ls.length)	ls=VRMLHelper.reallocate(ls);
		  								ls[i]=l;i++;})*
			  ( COLON )?
			  CLOSE_BRACKET {
			  	ll= new int[i];
			  	System.arraycopy(ls, 0, ll, 0, i);}
			)
		)
	  )
	 |(l = sflongValue	{ll = new int[]{l};} )	
	;

private 
sfmatrixValue returns [double[] m]
{ m=new double[16];
  double d;
  int i=0;
}
	:    d=doubleThing{m[i]=d;i++;}  d=doubleThing{m[i]=d;i++;}
	     d=doubleThing{m[i]=d;i++;}  d=doubleThing{m[i]=d;i++;}
	     d=doubleThing{m[i]=d;i++;}  d=doubleThing{m[i]=d;i++;}
	     d=doubleThing{m[i]=d;i++;}  d=doubleThing{m[i]=d;i++;}
	     d=doubleThing{m[i]=d;i++;}  d=doubleThing{m[i]=d;i++;}
	     d=doubleThing{m[i]=d;i++;}  d=doubleThing{m[i]=d;i++;}
	     d=doubleThing{m[i]=d;i++;}  d=doubleThing{m[i]=d;i++;}
	     d=doubleThing{m[i]=d;i++;}  d=doubleThing{m[i]=d;i++;}
	;

private
sfrotationValue returns [double[] rv=null]
{ double x,y,z,ang; }
	:
	 x=doubleThing y=doubleThing z=doubleThing ang=doubleThing
		{rv = new double[]{x,y,z,ang}; }
	;

private
sfstringValue returns [String s=""]
	:     g:ID 	  	{ s = g.getText();}
		| h:STRING	{ s = h.getText();}
	;

private
mfstringValue returns [String [] strl={}]
{String str; 
 strs= new String[INITIAL_SIZE];
 int i=0;
 }
	:(OPEN_BRACKET
		(   ( CLOSE_BRACKET{strl=new String[]{};})
		  | ( str=sfstringValue { strs[0]=str;i++;}
			  ( COLON str=sfstringValue {
			  		if (i==strs.length)	strs=VRMLHelper.reallocate(strs);
			  			strs[i]=str;i++;
		  		}
		  	  )*
			  ( COLON )?
			  CLOSE_BRACKET {
			  	strl= new String[i];
			 	System.arraycopy(strs, 0, strl, 0, i);}
			)
		)
	  )
	 |(str = sfstringValue	{strl = new String[]{str};} )	
	;

private
sfvec2fValue returns [double[] vec=null]
{ double fx,fy;}
	: fx=doubleThing fy=doubleThing 
	  {vec= new double[]{fx,fy};}
	;

private
sfvec3fValue returns [double[] vec=null]
{ double fx,fy,fz;}
	: fx=doubleThing fy=doubleThing fz=doubleThing 
	  {vec= new double[]{fx,fy,fz};}
	;
	
private
mfvec2fValue returns [double[][] vecl=null]
{double[] vec; 
 vecs= new double[INITIAL_SIZE][];
 int i=0;
 }
	:(OPEN_BRACKET
		(   ( CLOSE_BRACKET{vecl=new double[][]{};})
		  | ( vec=sfvec2fValue { vecs[0]=vec;i++;}
			  ( COLON vec=sfvec2fValue { if (i==vecs.length)	vecs=VRMLHelper.reallocate(vecs);
		  								 vecs[i]=vec;i++;})*
			  ( COLON )?
			  CLOSE_BRACKET {
			  		vecl= new double[i][];
			  		System.arraycopy(vecs, 0, vecl, 0, i);}
			)
		)
	  )
	 |(vec = sfvec2fValue	{vecl = new double[][]{vec};} )	
	;

private
mfvec3fValue returns [double[][] vecl=null]
{double[] vec; 
 vecs= new double[INITIAL_SIZE][];
 int i=0;
 }
	:(OPEN_BRACKET
		(   ( CLOSE_BRACKET{vecl=new double[][]{};})
		  | ( vec=sfvec3fValue { vecs[0]=vec;i++;}
			  ( COLON vec=sfvec3fValue { if (i==vecs.length)	vecs=VRMLHelper.reallocate(vecs);
		  								 vecs[i]=vec;i++;})*
			  ( COLON )?
			  CLOSE_BRACKET {
			  		vecl= new double[i][];
			   		System.arraycopy(vecs, 0, vecl, 0, i);
			  		}
			)
		)
	  )
	 |(vec = sfvec3fValue	{vecl = new double[][]{vec};} )	
	;

// _________________________________________________
// ---------------- einfache Zahlen ----------------

private 
hexValue returns[int hexVal]
{hexVal=0;}
	: g:HEXDEC
	{ hexVal= Integer.parseInt(g.getText());}
	;

private
intThing returns[int i]
// liest ein Integer aus
{i=0; String sig="";}
	: (PLUS | MINUS {sig="-";} )?
	  s:NUMBER 
	   {double d=Double.parseDouble(sig + s.getText());
	    i=(int)Math.round(d);}
	;
	
private
doubleThing returns[double d=0]
// liest ein double aus
	{double e=0; String sig="";}
    : 
    (PLUS | MINUS {sig="-";} )?
     s:NUMBER	{d=Double.parseDouble(sig + s.getText());}
    (e=expThing {d=d*Math.pow(10,e);})?
    ;
    
private 
expThing returns[double e]
// liest den exponenten fuer floatThing
{e=0; String sig="";}
    : ("E"|"e") (PLUS | MINUS {sig="-";} )?
		s:NUMBER
     	{e=Double.parseDouble(sig + s.getText() );}
	;

// ___________________________________________________
// -------------- Ueberlesen: { ... } ----------------

private
wrongAttribute
	:	g:ID 	(dumb)*
		{System.out.println("unknown Attribute:"+g.getText()+" -following Attributes ignored!");}
	;

private 
egal :
	  (LPAREN			(dumb)*		RPAREN			)
	| (OPEN_BRACE	 	(dumb)*		CLOSE_BRACE		)
	| (OPEN_BRACKET	 	(dumb)*		CLOSE_BRACKET	)
	;

private
dumb
// ueberliset alles bis zum Klammerende auch mit Unterklammern
	:(  (~(	 OPEN_BRACE | OPEN_BRACKET | LPAREN | RPAREN |CLOSE_BRACKET | CLOSE_BRACE ))+
		(     OPEN_BRACE	 	(dumb)*	CLOSE_BRACE
			| LPAREN			(dumb)* RPAREN
			| OPEN_BRACKET	 	(dumb)*	CLOSE_BRACKET	)?  )
	 |  (     OPEN_BRACE	 	(dumb)*	CLOSE_BRACE 
	 		| OPEN_BRACKET	 	(dumb)*	CLOSE_BRACKET	
	 		| LPAREN 			(dumb)*	RPAREN		)
  ;



/************************************************************************************
 * The VRML Lexer
 ************************************************************************************
 */
class VRMLV1Lexer extends Lexer;
options {
	charVocabulary = '\3'..'\377';
	k=2;
	testLiterals=false;
//	filter=IGNORE;
}
	/* Terminal Symbols */
OPEN_BRACE:		'{';
CLOSE_BRACE:	'}';
OPEN_BRACKET:	'[';
CLOSE_BRACKET:	']';
LPAREN:			'(';
RPAREN:			')';
MINUS:			'-';
PLUS:			'+';	
COLON:			',';
T1:				'|';

ID
options {
	paraphrase = "an identifier";
	testLiterals=true;
}
	:	('a'..'z'|'A'..'Z'|'_') (ID_LETTER)*
	;

protected 
ID_LETTER:
	('!'|'$'..'&'|'*'|','|'/'..'Z'|'^'..'z'|'~')
	;

HEXDEC
	:'0'
	 ('x'|'X')
	 (HEXDIGIT)*
	;
protected
HEXDIGIT
	: ('0'..'9')
	| ('A'..'F')
	| ('a'..'f')
	;

protected
DIGIT:	
	('0'..'9')
	;

NUMBER:
 	(DIGIT)+ ('.' (DIGIT)* )?
 	|'.' (DIGIT)+	
	;

STRING:	
//  ".*" ... double-quotes must be \", backslashes must be \\... 
		'"'! (ESC | ~('"'|'\\'))* '"'!
	;

protected
ESC:
		'\\' ('\\' | '"')
	;

protected
RESTLINE:
	 (~('\n'))* ('\n')
	 ;
	
protected
HEADER1:	"#VRML V1.0 ascii" | "#Inventor V2.1 ascii";


HEADER:	{getLine()==1}?	HEADER1 RESTLINE
	{System.err.println("Got header");}
	;
	
protected
COMMENT:
	'#' (~('\n'))* ('\n')
	;

WS_:
		( ' '
		| '\t'
		| '\f'
		| COMMENT
		// handle newlines
		|	(options {
					generateAmbigWarnings=false;
				}
		: "\r\n"		// DOS
			| '\r'		// Mac
			| '\n'		// Unix
			{newline(); } )	
		)+ { $setType(Token.SKIP); }
	;
	