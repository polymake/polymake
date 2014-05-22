package de.jreality.renderman;

import java.awt.Color;
import java.io.IOException;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;
import de.jreality.shader.TwoSidePolygonShader;
import de.jreality.ui.viewerapp.ViewerApp;
import de.jreality.util.Input;

public class RendermanTestScene {

	static final int testNr=12;   //testNr: 0..15
	static final int rendererType = RIBViewer.TYPE_PIXAR;
	static String ribPath = "";
	static String ribFileName = "";
	static String globalIncludeFile = "";;

	public static void main(String[] args) {    
		if(args.length>0) ribPath=args[0];
		if(args.length>1) ribFileName=args[1];
		if(args.length>2) globalIncludeFile=args[2];

		if(ribPath.equals("") || ribFileName.equals("")){
			System.err.println("RendermanTestScene: ribPath and/or ribFileName not set");
			return;
		}

		SceneGraphComponent sgc=new SceneGraphComponent();

		IndexedFaceSetFactory ifs=new IndexedFaceSetFactory();
		ifs.setVertexCount(8);
		ifs.setVertexCoordinates(new double[][] {{-2,2,0},{-2,-2,0},{2,-2,0},{2,2,0},{-3,3,1},{-3,0,1},{0,0,1},{0,3,1}});
		ifs.setFaceCount(2);
		ifs.setFaceIndices(new int[][]{{0,1,2,3},{4,5,6,7}});
		ifs.setGenerateEdgesFromFaces(true);
		ifs.setVertexTextureCoordinates(new double[][] {{0,0},{0,1},{1,1},{1,0},{0,0},{0,1},{1,1},{1,0}});
		ifs.setVertexColors(new Color[] {Color.BLACK,Color.BLUE,Color.CYAN,Color.GRAY,Color.GREEN,Color.MAGENTA,Color.ORANGE,Color.PINK});
		ifs.setGenerateFaceNormals(true);
		ifs.setGenerateFaceLabels(true);
		ifs.update();

		SceneGraphComponent faceSetNode=new SceneGraphComponent();
		faceSetNode.setGeometry(ifs.getGeometry());
		faceSetNode.setAppearance(new Appearance());
		faceSetNode.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW,true);
		faceSetNode.getAppearance().setAttribute(CommonAttributes.SPHERES_DRAW, true);
		faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.2);
		faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SIZE, 0.2);
		faceSetNode.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, true);
		faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBES_DRAW, true);
		faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.1);
		faceSetNode.getAppearance().setAttribute(CommonAttributes.LINE_WIDTH, 0.1);
		faceSetNode.getAppearance().setAttribute(CommonAttributes.DIFFUSE_COLOR, Color.WHITE);
		sgc.addChild(faceSetNode);

		IndexedLineSetFactory ils=new IndexedLineSetFactory();
		ils.setVertexCount(8);
		ils.setVertexCoordinates(new double[][] {{-2,2,0},{-2,-2,0},{2,-2,0},{2,2,0},{-3,3,1},{-3,0,1},{0,0,1},{0,3,1}});
		ils.setEdgeCount(8);
		ils.setEdgeIndices(new int[][]{{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4}});
		ils.setVertexTextureCoordinates(new double[][] {{0,0},{0,1},{1,1},{1,0},{0,0},{0,1},{1,1},{1,0}});
		ils.setVertexColors(new Color[] {Color.CYAN,Color.GRAY,Color.BLACK,Color.MAGENTA,Color.BLUE,Color.ORANGE,Color.PINK,Color.GREEN});
		ils.setEdgeColors(new Color[] {Color.RED,Color.BLUE,Color.CYAN,Color.GRAY,Color.GREEN,Color.MAGENTA,Color.ORANGE,Color.PINK});
		ils.update();

		SceneGraphComponent lineSetNode=new SceneGraphComponent();
		lineSetNode.setGeometry(ils.getGeometry());
		MatrixBuilder.euclidean().translate(3.5,0,0).scale(-0.4,0.4,0.4).assignTo(lineSetNode);
		faceSetNode.addChild(lineSetNode);

		ImageData img=null;
		try {
			img = ImageData.load(Input.getInput("textures/outfactory3.png"));
		} catch (IOException e) {e.printStackTrace();}

		switch(testNr){
		case 0: break;
		case 1:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, true);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);
			break;
		}
		case 2:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, true);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, false);
			break;
		}
		case 3:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);
			break;
		}
		case 4:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, false);
			break;
		}
		case 5:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true); 
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.35);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.3);
			TextureUtility.createTexture(faceSetNode.getAppearance(), "polygonShader", img, false);
			break;
		}
		case 6:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, true);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);  
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.35);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.3);
			TextureUtility.createTexture(faceSetNode.getAppearance(), "polygonShader", img, false);
			break;
		}
		case 7:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.35);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.3);
			try {
				TextureUtility.createReflectionMap(faceSetNode.getAppearance(), "polygonShader", "textures/emerald/emerald_", new String[]{"rt","lf","up","dn","bk","ft"},".jpg");
			} catch (IOException e) {e.printStackTrace();}
			break;
		}  
		case 8:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);  
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.35);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.3);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.MAGENTA);
			break;
		}
		case 9:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);  
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.35);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.3);
			TextureUtility.createTexture(faceSetNode.getAppearance(), "lineShader.polygonShader", img, false);
			break;
		}
		case 10:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, true);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, false);  
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.35);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.3);
			TextureUtility.createTexture(faceSetNode.getAppearance(), "lineShader.polygonShader", img, false);
			break;
		}
		case 11:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY, 0.7);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.35);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.3);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.WHITE);
			try {
				TextureUtility.createReflectionMap(faceSetNode.getAppearance(), "lineShader.polygonShader", "textures/emerald/emerald_", new String[]{"rt","lf","up","dn","bk","ft"},".jpg");
			} catch (IOException e) {e.printStackTrace();}
			break;
		}  
		case 12:{
			IndexedFaceSetFactory twoSideFace=new IndexedFaceSetFactory();
			twoSideFace.setVertexCount(8);
			twoSideFace.setVertexCoordinates(new double[][]{{1,0.5,0},{-1,0.5,0},{-1,-0.5,0},{1,-0.5,0},{0,-0.2,1},{-1,-0.2,1},{-1,0.2,1},{0,0.2,1}});
			twoSideFace.setFaceCount(3);
			twoSideFace.setFaceIndices(new int[][]{{0,1,2,3},{2,3,4,5},{4,5,6,7}});
			twoSideFace.setGenerateEdgesFromFaces(true);
			twoSideFace.setVertexTextureCoordinates(new double[][] {{2,2},{0,2},{0,0},{2,0},{0,0},{1,0},{1,0},{0,2}});
			twoSideFace.setGenerateFaceNormals(true);
			twoSideFace.setGenerateVertexNormals(true);
			twoSideFace.update();
			faceSetNode.setGeometry(twoSideFace.getGeometry());
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.05);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.03);

//			DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(faceSetNode.getAppearance(), true);
//			TwoSidePolygonShader tsps = (TwoSidePolygonShader) dgs.createPolygonShader("twoSide");  	
//			((DefaultPolygonShader)tsps.getFront()).setDiffuseColor(Color.BLUE);
//			((DefaultPolygonShader)tsps.getBack()).setDiffuseColor(Color.RED);
			
			faceSetNode.getAppearance().setAttribute("polygonShader",TwoSidePolygonShader.class);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+".front."+CommonAttributes.DIFFUSE_COLOR,Color.BLUE);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+".back."+CommonAttributes.DIFFUSE_COLOR,Color.RED);

//			faceSetNode.getAppearance().setAttribute("lineShader.polygonShader",DefaultPolygonShader.class);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,Color.PINK);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+".front."+CommonAttributes.DIFFUSE_COLOR,Color.YELLOW);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+".back."+CommonAttributes.DIFFUSE_COLOR,Color.GREEN);

//			faceSetNode.getAppearance().setAttribute("pointShader.polygonShader",DefaultPolygonShader.class);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,Color.PINK);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+".front."+CommonAttributes.DIFFUSE_COLOR,Color.RED);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+".back."+CommonAttributes.DIFFUSE_COLOR,Color.GRAY);

			MatrixBuilder.euclidean().scale(3).assignTo(faceSetNode);
			faceSetNode.removeChild(lineSetNode);
			break;
		}
		case 13:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.SPHERES_DRAW, false);
			//faceSetNode.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, true);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SIZE, 400); 
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.2);     
			faceSetNode.getAppearance().setAttribute(CommonAttributes.ATTENUATE_POINT_SIZE,false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS, 0.03);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,Color.MAGENTA);
			break;
		}
		case 14:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TRANSPARENCY_ENABLED, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.SPHERES_DRAW, false);
			//faceSetNode.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, true);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SIZE, 400); 
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, 0.2);     
			faceSetNode.getAppearance().setAttribute(CommonAttributes.ATTENUATE_POINT_SIZE,false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBES_DRAW, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,Color.MAGENTA);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,Color.GREEN);
			break;
		}
		case 15:{
			faceSetNode.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,Color.GREEN);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.TUBES_DRAW, false);

			faceSetNode.getAppearance().setAttribute(CommonAttributes.SPHERES_DRAW, false);
			faceSetNode.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,Color.RED);

			break;
		}
		}

		ViewerApp va=new ViewerApp(sgc);
		va.update();
		va.display();
		
//		if(va.getSceneRoot().getAppearance()==null) va.getSceneRoot().setAppearance(new Appearance());
//		va.getSceneRoot().getAppearance().setAttribute(CommonAttributes.FOG_ENABLED, true);

		RIBViewer ribv=new RIBViewer();
		ribv.initializeFrom(va.getViewer());
		ribv.setRendererType(rendererType);
		ribv.setFileName(ribPath+ribFileName);
		if(ribv.getSceneRoot().getAppearance()==null)  ribv.getSceneRoot().setAppearance(new Appearance());
		if(!globalIncludeFile.equals(""))  ribv.getSceneRoot().getAppearance().setAttribute(CommonAttributes.RMAN_GLOBAL_INCLUDE_FILE, globalIncludeFile);
		ribv.render();
	}
}
