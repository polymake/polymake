package de.jreality.swing.jrwindows;

import java.awt.Color;
import java.awt.Font;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.image.BufferedImage;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;

import de.jreality.backends.label.LabelUtility;
import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.Tool;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;
import de.jreality.swing.JFakeFrame;
import de.jreality.tools.ActionTool;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;

/**
 * 
 * Manages a JFrame that may be immersed in the scene or an external JFrame on the
 * Desktop. Do not hold any reference to the given JFrame, but always get the reference
 * via getFrame().
 * 
 * @author bleicher, Steffen Weissmann
 *
 */

public class JRWindow {

	private int windowNumber;

	private boolean enableVertexPopUpTool=false;

	private IndexedFaceSet frameFace;
	private IndexedLineSet borders;
	private IndexedFaceSet decoControlFace;
	private IndexedFaceSet decoDragFace;
	private JFakeFrame frame;  
	private JFrame externalFrame = new JFrame();

	private boolean inScene=true;
	ActionTool myActionTool=new ActionTool("PanelActivation");
	private Tool mouseEventTool;
	private Tool vertexPopUpTool;

	private SceneGraphComponent sgc;
	private SceneGraphComponent positionSgc; 
	private SceneGraphComponent frameSgc;
	private SceneGraphComponent borderSgc;
	private SceneGraphComponent decoControlSgc;  
	private SceneGraphComponent decoDragSgc;
	private final double[][] startCornerPos={{1,0,0},{1,-1,0},{-1,-1,0},{-1,0,0}};
	private double[][] cornerPos;
	private double[][] cornerPosBak;
	private boolean isSmall=false;

	private double borderRadius=0.01;
	private double cornerRadius;
	private double cornerRadiusPopUpFactor=3;
	private double translateFactor;

	private double decoHight=0.08;
	private final double decoControlWidthFactor=4;	//decoControlWidth=decoHeight*decoControlWidthFactor
	private double decoControlWidth;
	private double aspectRatio=1;
	private double[][] faceCorners;
	private double[][] decoControlCorners;
	private double[][] decoDragCorners; 
	private double[] smallCenter;

	private JPanel panel;
	private JButton killButton;
	private JButton maxButton;
	private JButton minButton;
	private final Color activeColor=new Color(62,139,210);
	private final Color inactiveColor=new Color(138,182,225);
	private static final Font TITLE_FONT = new Font("Sans Serif", Font.BOLD, 24);

	private static final boolean FORBID_EXTERNAL_FRAME = "portal-remote".equals(Secure.getProperty(SystemProperties.ENVIRONMENT));

	protected JRWindow(int windowNumber){    
		myActionTool.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				getFrame().setVisible(!getFrame().isVisible());
				if(!isSmall && vertexPopUpTool!=null && !borderSgc.getTools().contains(vertexPopUpTool))
					borderSgc.addTool(vertexPopUpTool);
			}
		});
		this.windowNumber=windowNumber; 
		setBorderRadius(borderRadius);   
		setDecoSize(decoHight);
		cornerPos=new double[startCornerPos.length][startCornerPos[0].length];
		for(int n=0;n<cornerPos.length;n++)
			for(int c=0;c<cornerPos[0].length;c++)
				cornerPos[n][c]=startCornerPos[n][c];
		cornerPosBak=new double[cornerPos.length][cornerPos[0].length];
		initSgc();
		initFrame();
		initDecoration();
	}

	private void initSgc(){
		positionSgc=new SceneGraphComponent("frame ["+windowNumber+"]");
		positionSgc.setVisible(false); // until the frame is visible
		positionSgc.setAppearance(new Appearance());
		positionSgc.getAppearance().setAttribute("pointShader.pickable", true);
		positionSgc.getAppearance().setAttribute("lineShader.pickable", true);
		positionSgc.getAppearance().setAttribute("polygonShader.pickable", true); 
		sgc = new SceneGraphComponent();
		positionSgc.addChild(sgc);
	}
	
	private void initFrame(){
		frameSgc=new SceneGraphComponent("content");
		sgc.addChild(frameSgc);    
		faceCorners=new double[cornerPos.length][cornerPos[0].length];
		calculateFaceCorners(faceCorners,cornerPos);    
		IndexedFaceSetFactory face=new IndexedFaceSetFactory();
		face.setVertexCount(4);
		face.setVertexCoordinates(faceCorners);
		face.setVertexTextureCoordinates(new double[][] {{1,0},{1,1},{0,1},{0,0}});
		face.setFaceCount(1);
		face.setFaceIndices(new int[][] {{0,1,2,3}});
		face.setGenerateEdgesFromFaces(false);
		face.setGenerateFaceNormals(true);
		face.setGenerateVertexNormals(true);
		face.update();
		this.frameFace=face.getIndexedFaceSet();     
		frame=new JFakeFrame();

		ComponentListener componentListener = new ComponentListener() {
			public void componentHidden(ComponentEvent e) {
				getSgc().setVisible(false);
			}
			public void componentMoved(ComponentEvent e) {
			}
			public void componentResized(ComponentEvent e) {
				updateAspectRatio();
				setCorner(0, cornerPos[0]);
			}
			public void componentShown(ComponentEvent e) {
				updateAspectRatio();
				if (!isSmall) setCorner(0, cornerPos[0]);
				else setSmall(true);
				getSgc().setVisible(true);
			}
			void updateAspectRatio() {
				double newAspectRatio=(double)frame.getWidth()/(double)frame.getHeight();
				if (newAspectRatio == 0 || Double.isNaN(newAspectRatio) || Double.isInfinite(newAspectRatio)) {
					System.out.println("ignoring new aspectRatio: "+newAspectRatio);				
					return;
				}
				aspectRatio=newAspectRatio;
			}
		};
		frame.addComponentListener(componentListener);

		frame.addPropertyChangeListener("title", new PropertyChangeListener() {
			public void propertyChange(PropertyChangeEvent evt) {
				updateFrameTitle();
				externalFrame.setTitle(frame.getTitle());
			}
		});

		frameSgc.setGeometry(frameFace);
		frameSgc.addTool(frame.getTool());
		frameSgc.setAppearance(frame.getAppearance());

		frameSgc.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW,false);
		frameSgc.getAppearance().setAttribute(CommonAttributes.SPHERES_DRAW,false);
		frameSgc.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW,false);
		frameSgc.getAppearance().setAttribute(CommonAttributes.TUBES_DRAW,false);
		frameSgc.getAppearance().setAttribute(CommonAttributes.LIGHTING_ENABLED, false);
	}

	private void initDecoration(){
		decoControlSgc=new SceneGraphComponent("controls");    
		sgc.addChild(decoControlSgc);      
		decoControlCorners=new double[cornerPos.length][cornerPos[0].length];
		for(int i=0; i<cornerPos.length;i++)
			decoControlCorners[i][2]=0;
		calculateDecoControlCorners(decoControlCorners,cornerPos);    
		IndexedFaceSetFactory decoControlFace=new IndexedFaceSetFactory();
		decoControlFace.setVertexCount(4);
		decoControlFace.setVertexCoordinates(decoControlCorners);
		decoControlFace.setVertexTextureCoordinates(new double[][] {{1,0},{1,1},{0,1},{0,0}});
		decoControlFace.setFaceCount(1);
		decoControlFace.setFaceIndices(new int[][] {{0,1,2,3}});
		decoControlFace.setGenerateEdgesFromFaces(false);
		decoControlFace.setGenerateFaceNormals(true);
		decoControlFace.setGenerateVertexNormals(true);
		decoControlFace.update();
		this.decoControlFace=decoControlFace.getIndexedFaceSet();     
		JFakeFrame decoControlFrame=new JFakeFrame();    
		decoControlSgc.addTool(decoControlFrame.getTool());
		decoControlSgc.setAppearance(decoControlFrame.getAppearance());  
		decoControlSgc.setGeometry(this.decoControlFace);  
		decoControlSgc.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW,false);
		decoControlSgc.getAppearance().setAttribute(CommonAttributes.SPHERES_DRAW,false);
		decoControlSgc.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW,false);
		decoControlSgc.getAppearance().setAttribute(CommonAttributes.TUBES_DRAW,false); 
		decoControlSgc.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,activeColor);
		decoControlSgc.getAppearance().setAttribute(CommonAttributes.LIGHTING_ENABLED, false);

		panel=new JPanel();
		panel.setBackground(activeColor);
		killButton=new JButton("X");
		killButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				frame.setVisible(false);
				if(vertexPopUpTool!=null && !borderSgc.getTools().isEmpty() && borderSgc.getTools().contains(vertexPopUpTool))
					borderSgc.removeTool(vertexPopUpTool);
			}
		});
		maxButton=new JButton("O");
		maxButton.setEnabled(false);
		minButton=new JButton("_");
		minButton.setEnabled(true);
		panel.add(minButton);
		panel.add(maxButton);
		panel.add(killButton);
		decoControlFrame.getContentPane().add(panel);
		decoControlFrame.pack();
		decoControlFrame.setVisible(true);

		////decoDragFace////////////////////////////////////
		decoDragSgc=new SceneGraphComponent("title");
		sgc.addChild(decoDragSgc);     
		decoDragCorners=new double[cornerPos.length][cornerPos[0].length];
		for(int i=0; i<cornerPos.length;i++)
			decoDragCorners[i][2]=0;
		calculateDecoDragCorners(decoDragCorners,cornerPos);    
		IndexedFaceSetFactory decoDragFace=new IndexedFaceSetFactory();
		decoDragFace.setVertexCount(4);
		decoDragFace.setVertexCoordinates(decoDragCorners);
		decoDragFace.setVertexTextureCoordinates(new double[][] {{1,0},{1,1},{0,1},{0,0}});
		decoDragFace.setFaceCount(1);
		decoDragFace.setFaceIndices(new int[][] {{0,1,2,3}});
		decoDragFace.setGenerateEdgesFromFaces(false);
		decoDragFace.setGenerateFaceNormals(true);
		decoDragFace.setGenerateVertexNormals(true);
		decoDragFace.update();
		this.decoDragFace=decoDragFace.getIndexedFaceSet();      
		decoDragSgc.setGeometry(this.decoDragFace);  
		decoDragSgc.setAppearance(new Appearance());
		decoDragSgc.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW,false);
		decoDragSgc.getAppearance().setAttribute(CommonAttributes.SPHERES_DRAW,false);
		decoDragSgc.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW,false);
		decoDragSgc.getAppearance().setAttribute(CommonAttributes.TUBES_DRAW,false);
		decoDragSgc.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,activeColor);
		decoDragSgc.getAppearance().setAttribute(CommonAttributes.LIGHTING_ENABLED, false);

		////borders///////////////////////
		borderSgc=new SceneGraphComponent("border");
		sgc.addChild(borderSgc);
		IndexedLineSetFactory borders=new IndexedLineSetFactory();
		borders.setVertexCount(4);
		borders.setVertexCoordinates(cornerPos);
		borders.setEdgeCount(4);
		borders.setEdgeIndices(new int[][] {{0,1},{1,2},{2,3},{3,0}});
		borders.update();
		this.borders=borders.getIndexedLineSet();
		borderSgc.setGeometry(this.borders);
		borderSgc.setAppearance(new Appearance());
		borderSgc.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW,true);
		borderSgc.getAppearance().setAttribute(CommonAttributes.SPHERES_DRAW,true);
		borderSgc.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW,true);
		borderSgc.getAppearance().setAttribute(CommonAttributes.TUBES_DRAW,true); 
		borderSgc.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,activeColor);
		borderSgc.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,activeColor);
		borderSgc.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS,cornerRadius);
		borderSgc.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS,borderRadius);
		borderSgc.getAppearance().setAttribute(CommonAttributes.LIGHTING_ENABLED, true);

		if(enableVertexPopUpTool)
			enableVertexPopUpTool(true);
	}   

	protected void updateFrameTitle() {
		BufferedImage img = LabelUtility.createImageFromString(frame.getTitle(), TITLE_FONT, Color.black, Color.white);

		double w = img.getWidth();
		double h = img.getHeight()*1.5;

		double width = decoDragCorners[0][0]-decoDragCorners[2][0];
		double height = decoDragCorners[0][1]-decoDragCorners[2][1];

		double lambda = h/height;

		double effW = lambda*width;

		if (effW <= w+h/3) effW=w+h/3;
		BufferedImage effImg = new BufferedImage((int)effW, (int)h, BufferedImage.TYPE_INT_ARGB);
		effImg.getGraphics().fillRect(0, 0, effImg.getWidth(), effImg.getHeight());
		effImg.getGraphics().drawImage(img, (int) (int)(h/6), (int)(h/6), null);

		TextureUtility.createTexture(decoDragSgc.getAppearance(), "polygonShader", new ImageData(effImg));
	}

	protected void addActionListeners(ActionListener actionListener){
		//killButton.addActionListener(actionListener);    
		maxButton.addActionListener(actionListener);    
		minButton.addActionListener(actionListener);
		updateActionCommands();
	}
	private void updateActionCommands(){
		killButton.setActionCommand("X"+windowNumber);
		maxButton.setActionCommand("O"+windowNumber);
		minButton.setActionCommand("_"+windowNumber);
	}

	protected void setCornerPos(double[][] newCornerPos){ 
		for (int i=0;i<newCornerPos.length;i++)
			newCornerPos[i][2]=0;
		if(newCornerPos[0][0]-newCornerPos[3][0]>=decoControlWidth+borderRadius&&newCornerPos[0][1]-newCornerPos[1][1]>=decoHight+borderRadius){
			cornerPos=newCornerPos;
		}else if(newCornerPos[0][0]-newCornerPos[3][0]>=decoControlWidth+borderRadius||newCornerPos[0][1]-newCornerPos[1][1]>=decoHight+borderRadius){ 
			if(newCornerPos[0][0]-newCornerPos[3][0]>decoControlWidth+borderRadius){ //copy x   
				for(int n=0;n<cornerPos.length;n++)
					cornerPos[n][0]=newCornerPos[n][0];
			}else if(newCornerPos[0][1]-newCornerPos[1][1]>decoHight+borderRadius){  //copy y   
				for(int n=0;n<cornerPos.length;n++)
					cornerPos[n][1]=newCornerPos[n][1];
			}
		}else return;
		
		// rotate window (Ulrich)
		double centerX = (cornerPos[3][0]+ cornerPos[0][0])/2;
		//double centerZ = (cornerPos[3][3]+ cornerPos[0][3])/2;
		MatrixBuilder.euclidean().
			//translate(centerX,0,centerZ).
			rotateY(-.2*centerX).
			translate(0, 0,-.3 * centerX*centerX).
			assignTo(sgc);

		calculateFaceCorners(faceCorners,cornerPos);
		frameFace.setVertexAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(faceCorners));
		calculateDecoControlCorners(decoControlCorners,cornerPos);
		decoControlFace.setVertexAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(decoControlCorners));  
		calculateDecoDragCorners(decoDragCorners,cornerPos);
		decoDragFace.setVertexAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(decoDragCorners));    
		borders.setVertexAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(cornerPos));
		updateFrameTitle();
	}    

	public void resize(double factor) {
		double[] c1 = cornerPos[0];
		double[] c2 = cornerPos[2];
		double[] diag = Rn.subtract(null, c2, c1);
		Rn.times(diag, factor, diag);
		Rn.add(diag, diag, c1);
		setCorner(2, diag);
	}
	
	void setCorner(int cornerIndex, double[] newPoint) {
		int oppositeCorner = (cornerIndex+2)%4;
		newPoint[1]-=decoHight; newPoint[2]=0;
		double[] diag = Rn.subtract(null, newPoint, cornerPos[oppositeCorner]);

		double newAsp = Math.abs(diag[0]/diag[1]);
		if (newAsp > aspectRatio)  // adapt width to height:			
			diag[0] = Math.signum(diag[0])*aspectRatio*Math.abs(diag[1]);
		else   //adapt height to width:            
			diag[1] = Math.signum(diag[1])*Math.abs(diag[0])/aspectRatio;

		// do not allow less width than the deco needs:
		double decoWidth = Math.abs(decoControlCorners[2][0]-decoControlCorners[0][0]);
		if (Math.abs(diag[0]) < decoWidth) {
			double f = decoWidth/Math.abs(diag[0]);
			diag[0]*=f;
			diag[1]*=f;
		}

		// cancel if window would be flipped:
		if ((cornerIndex == 0) && (diag[0]<=0 || diag[1]<=0)) return;
		if ((cornerIndex == 1) && (diag[0]<=0 || diag[1]>=0)) return;
		if ((cornerIndex == 2) && (diag[0]>=0 || diag[1]>=0)) return;
		if ((cornerIndex == 3) && (diag[0]>=0 || diag[1]<=0)) return;

		Rn.add(cornerPos[cornerIndex], cornerPos[oppositeCorner], diag);

		if (cornerIndex == 3 || cornerIndex == 0) cornerPos[cornerIndex][1]+=decoHight;
		else cornerPos[cornerIndex][1]-=decoHight;

		// adjust other two corners:
		int nextCorner = (cornerIndex+1)%4;
		int prevCorner = (cornerIndex-1+4)%4;
		if (cornerIndex%2 == 0) {
			cornerPos[nextCorner][0]=cornerPos[cornerIndex][0];
			cornerPos[prevCorner][1]=cornerPos[cornerIndex][1];
		} else {
			cornerPos[nextCorner][1]=cornerPos[cornerIndex][1];
			cornerPos[prevCorner][0]=cornerPos[cornerIndex][0];
		}

		setCornerPos(cornerPos);
	}  

	private void calculateFaceCorners(double[][] faceCorners, double[][] cornerPos){ 
		faceCorners[0][0]=cornerPos[0][0];faceCorners[0][1]=cornerPos[0][1]-decoHight; 
		faceCorners[1][0]=cornerPos[1][0];faceCorners[1][1]=cornerPos[1][1]; 
		faceCorners[2][0]=cornerPos[2][0];faceCorners[2][1]=cornerPos[2][1]; 
		faceCorners[3][0]=cornerPos[3][0];faceCorners[3][1]=cornerPos[3][1]-decoHight; 
	} 
	private void calculateDecoControlCorners(double[][] decoControlCorners,double[][] cornerPos){      
		decoControlCorners[0][0]=cornerPos[0][0]; decoControlCorners[0][1]=cornerPos[0][1]; 
		decoControlCorners[1][0]=cornerPos[0][0]; decoControlCorners[1][1]=cornerPos[0][1]-decoHight; 
		decoControlCorners[2][0]=cornerPos[0][0]-decoControlWidth; decoControlCorners[2][1]=cornerPos[0][1]-decoHight; 
		decoControlCorners[3][0]=cornerPos[0][0]-decoControlWidth; decoControlCorners[3][1]=cornerPos[0][1]; 
	}
	private void calculateDecoDragCorners(double[][] decoDragCorners,double[][] cornerPos){ 
		decoDragCorners[0][0]=cornerPos[0][0]-decoControlWidth; decoDragCorners[0][1]=cornerPos[0][1]; 
		decoDragCorners[1][0]=cornerPos[1][0]-decoControlWidth; decoDragCorners[1][1]=cornerPos[0][1]-decoHight;
		decoDragCorners[2][0]=cornerPos[2][0]; decoDragCorners[2][1]=cornerPos[3][1]-decoHight; 
		decoDragCorners[3][0]=cornerPos[3][0]; decoDragCorners[3][1]=cornerPos[3][1];
	}

	protected double[][] getCornerPos(){
		return cornerPos;
	}  

	protected void setSmall(boolean setSmall){    
		if(setSmall&&!isSmall){
			for(int i=0;i<cornerPos.length;i++)
				Rn.copy(cornerPosBak[i],cornerPos[i]); 
			if(smallCenter==null)
				smallCenter=getCenter(cornerPos);

			double factorX = (decoControlWidth+borderRadius)/2;
			double factorY = factorX*Rn.euclideanDistance(cornerPos[0], cornerPos[1])/Rn.euclideanDistance(cornerPos[0], cornerPos[3]); 	  
			cornerPos[0][0]=smallCenter[0]+factorX;  cornerPos[0][1]=smallCenter[1]+factorY;
			cornerPos[1][0]=smallCenter[0]+factorX;  cornerPos[1][1]=smallCenter[1]-factorY;
			cornerPos[2][0]=smallCenter[0]-factorX;  cornerPos[2][1]=smallCenter[1]-factorY;
			cornerPos[3][0]=smallCenter[0]-factorX;  cornerPos[3][1]=smallCenter[1]+factorY;

			mouseEventTool=frameSgc.getTools().get(0);
			frameSgc.removeTool(mouseEventTool);
			if(vertexPopUpTool!=null)
				borderSgc.removeTool(vertexPopUpTool);

			setActiveColor(false);
			minButton.setEnabled(false);
			maxButton.setEnabled(true);
			isSmall=true;
		}else if((!setSmall)&&isSmall){
			smallCenter=getCenter(cornerPos);      
			for(int i=0;i<cornerPos.length;i++)
				Rn.copy(cornerPos[i],cornerPosBak[i]);		  

			if(mouseEventTool!=null)
				frameSgc.addTool(mouseEventTool);
			if(vertexPopUpTool!=null)
				borderSgc.addTool(vertexPopUpTool);

			setActiveColor(true);
			minButton.setEnabled(true);
			maxButton.setEnabled(false);
			isSmall=false;
		}
		setCornerPos(cornerPos);
	}
	private double[] getCenter(double[][] box){
		double[] center={0,0,0};
		for(int n=0;n<box.length;n++)
			Rn.add(center,center,box[n]);
		Rn.times(center,1/(double)box.length,center);  
		return center;
	}
	protected boolean isSmall(){
		return isSmall;
	}

	protected void setInFront(boolean setInFront){
		if(setInFront){
			if(!isSmall)
				setActiveColor(true);	
			MatrixBuilder.euclidean().assignTo(positionSgc); 
		}else{
			if(!isSmall)
				setActiveColor(false);	
			MatrixBuilder.euclidean().translate(0,0,-translateFactor*(windowNumber+1)).assignTo(positionSgc); 
		}
	}

	private void setActiveColor(boolean active){
		if(active){
			decoDragSgc.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,activeColor);
			borderSgc.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,activeColor);
			borderSgc.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,activeColor);
			panel.setBackground(activeColor);
		}else{
			decoDragSgc.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,inactiveColor);
			borderSgc.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,inactiveColor);
			borderSgc.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,inactiveColor);
			panel.setBackground(inactiveColor);
		}
	}

	protected void setBorderRadius(double r) {
		borderRadius=r;
		cornerRadius=borderRadius*1.75;
		translateFactor=1.1*cornerRadius;
		if(borderSgc!=null){
			borderSgc.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS,cornerRadius);
			borderSgc.getAppearance().setAttribute(CommonAttributes.TUBE_RADIUS,borderRadius);
			setCornerPos(cornerPos);
		} 
	}  
	protected void setDecoSize(double s){
		decoHight=s;
		decoControlWidth=s*decoControlWidthFactor; 
		if(borderSgc!=null){
			setCornerPos(cornerPos);
		}
	}

	protected void setWindowNumber(int windowNumber){
		this.windowNumber=windowNumber;
		updateActionCommands();
	}

	protected int getWindowNumber(){
		return windowNumber;
	}  
	protected SceneGraphComponent getSgc(){
		return positionSgc;
	}  
	public JFrame getFrame() {
		return inScene ? frame : externalFrame;
	}
	protected IndexedFaceSet getFrameFace(){
		return frameFace;
	}  
	protected IndexedFaceSet getDecoControlFace(){
		return decoControlFace;
	}
	protected IndexedFaceSet getDecoDragFace(){
		return decoDragFace;
	}
	protected IndexedLineSet getBorders(){
		return borders;
	}
	public void setInScene(boolean b) {
		if (FORBID_EXTERNAL_FRAME) return;
		if (b==inScene) return;
		if (b) {
			inScene = b;
			boolean visible = externalFrame.isVisible();
			externalFrame.setVisible(false);
			frame.setContentPane(externalFrame.getContentPane());
			externalFrame.remove(externalFrame.getContentPane());
			frame.pack();
			frame.setVisible(visible);
		} else {
			getSgc().setVisible(false);
			boolean visible = frame.isVisible();
			frame.setVisible(false);
			inScene = b;
			externalFrame.setContentPane(frame.getContentPane());
			frame.remove(frame.getContentPane());
			externalFrame.pack();
			externalFrame.setVisible(visible);
		}
	}

	public ActionTool getPanelTool() {
		return myActionTool;
	}

	protected void popUpDragVertices(boolean popUp){
		borderSgc.getAppearance().setAttribute(CommonAttributes.POINT_RADIUS, popUp ? cornerRadius*cornerRadiusPopUpFactor : cornerRadius);	
	}
	private boolean vertexDragged=false;
	protected void setVertexDragged(boolean isDragged){
		this.vertexDragged=isDragged;
	}
	
	protected void enableVertexPopUpTool(boolean enableVertexPopUpTool){	
		if(enableVertexPopUpTool){
			if(vertexPopUpTool==null) vertexPopUpTool=new VertexPopUpTool();
			if(!isSmall && !borderSgc.getTools().contains(vertexPopUpTool))
				borderSgc.addTool(vertexPopUpTool);
		}else{
			if(vertexPopUpTool!=null && !borderSgc.getTools().isEmpty() && borderSgc.getTools().contains(vertexPopUpTool))
				borderSgc.removeTool(vertexPopUpTool);		
		}
		this.enableVertexPopUpTool=enableVertexPopUpTool;
	}

	private class VertexPopUpTool extends AbstractTool{

		private final InputSlot pointer = InputSlot.getDevice("PointerTransformation");

		protected VertexPopUpTool(){
			super(new InputSlot[] {null});
			addCurrentSlot(pointer);
		}

		public void perform(ToolContext tc){
			if(!isSmall){
				PickResult currentPick = tc.getCurrentPick();
				if(currentPick!=null){
					if(currentPick.getPickPath().getLastComponent()==borderSgc){
						if(currentPick.getPickType()==PickResult.PICK_TYPE_POINT){
							popUpDragVertices(true);
						}else if(!vertexDragged) popUpDragVertices(false);
					}else if(!vertexDragged) popUpDragVertices(false);
				}else if(!vertexDragged) popUpDragVertices(false);
			}
		}
	}  
}
