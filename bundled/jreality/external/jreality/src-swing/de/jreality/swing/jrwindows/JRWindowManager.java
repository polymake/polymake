package de.jreality.swing.jrwindows;

import java.awt.Color;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;

import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.FaceDragEvent;
import de.jreality.tools.FaceDragListener;
import de.jreality.tools.PointDragEvent;
import de.jreality.tools.PointDragListener;

/**
 * @author bleicher
 *
 */

public class JRWindowManager implements ActionListener{
  
  private final double[] defaultDesktopWindowPos={0,0,-2};
  
  private SceneGraphComponent sgc;
  private ArrayList<JRWindow> windowList=new ArrayList<JRWindow>();
  private DragEventTool dragTool;
  
  private boolean windowsInScene=true;
  
  public JRWindowManager(SceneGraphComponent avatar){
    sgc=new SceneGraphComponent("window manager");
    sgc.setAppearance(new Appearance());
    sgc.getAppearance().setAttribute(CommonAttributes.LIGHTING_ENABLED, false);    
    sgc.getAppearance().setAttribute(CommonAttributes.METRIC, Pn.EUCLIDEAN);
    sgc.getAppearance().setAttribute("polygonShader.reflectionMap:blendColor", new Color(255,255,255,65));
    sgc.getAppearance().setAttribute("lineShader.polygonShader.reflectionMap:blendColor", new Color(255,255,255,255));
    avatar.addChild(sgc);
    setPosition(defaultDesktopWindowPos);
    initDragTool();
  } 
  
  private void initDragTool(){
    dragTool=new DragEventTool("PrimaryAction");
    dragTool.addPointDragListener(new PointDragListener(){
    	private int windowNum;
    	private int cornerIndex;
    	public void pointDragStart(PointDragEvent e) {
    		windowNum=searchWindowNum(e.getPointSet());
    		if(windowNum==-1) return;
    		if(windowList.get(windowNum).isSmall()) return;
    		cornerIndex=e.getIndex();
    		setWindowInFront(windowNum);
    		windowList.get(windowNum).setVertexDragged(true);
    	}
    	public void pointDragged(PointDragEvent e) { 
    		if(windowNum==-1) return;
    		if(windowList.get(windowNum).isSmall()) return;
    		double[] point=new double[3];
    		point[0]=e.getPosition()[0];
    		point[1]=e.getPosition()[1];
    		windowList.get(windowNum).setCorner(cornerIndex, point);
    	}
    	public void pointDragEnd(PointDragEvent e) { 
    		windowList.get(windowNum).popUpDragVertices(false);
    		windowList.get(windowNum).setVertexDragged(false);
    	}
    }); 
//    dragTool.addLineDragListener(new LineDragListener(){
//      private int windowNum;
//      private double[][] points;
//      private int[] lineIndices;
//      private double[][] line;      
//      public void lineDragStart(LineDragEvent e) {
//        windowNum=searchWindowNum(e.getIndexedLineSet());
//        if(windowNum==-1) return;
//        if(windowList.get(windowNum).isSmall()) return;
//        setWindowInFront(windowNum);
//        points=windowList.get(windowNum).getCornerPos();        
//        lineIndices=e.getLineIndices();
//        line=e.getLineVertices();        
//      }
//      public void lineDragged(LineDragEvent e) {
//        if(windowNum==-1) return;
//        if(windowList.get(windowNum).isSmall()) return;       
//        double[][] newPoints=new double[points.length][points[0].length];
//        for(int n=0;n<newPoints.length;n++)
//          for(int c=0;c<newPoints[0].length;c++)
//            newPoints[n][c]=points[n][c];
//        double[] translation=Pn.dehomogenize(new double[]{0,0,0},e.getTranslation());
//        if(Math.abs(points[lineIndices[0]][0]-points[lineIndices[1]][0])==0){
//          for(int i=0;i<lineIndices.length;i++){
//            newPoints[lineIndices[i]][0]=line[i][0]+translation[0];
//          }
//        }else if(Math.abs(points[lineIndices[0]][1]-points[lineIndices[1]][1])==0){
//          for(int i=0;i<lineIndices.length;i++){
//            newPoints[lineIndices[i]][1]=line[i][1]+translation[1];
//          }
//        }
//        windowList.get(windowNum).setCornerPos(newPoints);
//        }
//      public void lineDragEnd(LineDragEvent e) {
//        windowList.get(windowNum).updateFrameSize();
//    }});    
    dragTool.addFaceDragListener(new FaceDragListener(){ 
      private int windowNum;
      private double[][] points;   
      public void faceDragStart(FaceDragEvent e) { 
        windowNum=searchWindowNum(e.getIndexedFaceSet()); 
        if(windowNum==-1) return;        
        setWindowInFront(windowNum);      
        points=windowList.get(windowNum).getCornerPos();
      }
      public void faceDragged(FaceDragEvent e) {
        if(windowNum==-1) return;
        double[] translation=e.getTranslation();
        double[][] newPoints=new double[points.length][points[0].length];
        Pn.dehomogenize(translation,translation);        
        translation[2]=0; //!no z-dragging! use sgc-draggingtool instead (middle mouse-button)        
        if(translation.length==4) translation[3]=0;        
        for(int i=0;i<points.length;i++){
          Rn.add(newPoints[i],points[i],translation);
        }          
        windowList.get(windowNum).setCornerPos(newPoints);
      }
      public void faceDragEnd(FaceDragEvent e) { 
      }
    });
    
    sgc.addTool(dragTool);
  }
  
  private int searchWindowNum(Geometry matchedGeo){
    int matchedWindowNum=0;
    for(JRWindow win : windowList){      
      if(matchedGeo.equals(win.getFrameFace())||matchedGeo.equals(win.getBorders())||matchedGeo.equals(win.getDecoDragFace())){
        return matchedWindowNum; 
      }
      matchedWindowNum++;
    }
    return -1; 
  }
  private void setWindowInFront(int windowNum){
    int i=0;
    for(JRWindow win : windowList){
      if(i==windowNum) win.setInFront(true);
      else win.setInFront(false);   
      i++;
    }
  }
  
  public void actionPerformed(ActionEvent e) {    
    if(e.getActionCommand().startsWith("O")){
      String command=e.getActionCommand();
      command=command.replaceFirst(String.valueOf(command.charAt(0)),"");
      int windowNum=Integer.parseInt(command);    
      if(windowList.get(windowNum).isSmall()){
        windowList.get(windowNum).setSmall(false);
        setWindowInFront(windowNum);
      }
    } 
    else if(e.getActionCommand().startsWith("_")){
      String command=e.getActionCommand();
      command=command.replaceFirst(String.valueOf(command.charAt(0)),"");
      int windowNum=Integer.parseInt(command);    
      if(!windowList.get(windowNum).isSmall())
        windowList.get(windowNum).setSmall(true);
    } 
  }  
  
  public JRWindow createFrame(){ 
    JRWindow window=new JRWindow(getWindowCount());
    window.addActionListeners(this);
    window.setInScene(getWindowsInScene());
    sgc.addChild(window.getSgc());
    windowList.add(window);
    setWindowInFront(getWindowCount()-1);
    return window;
  }   
  
  public boolean getWindowsInScene() {
	  return windowsInScene;
  }

  public void setWindowsInScene(boolean b) {
	  windowsInScene = b;
	  for (JRWindow w : windowList) w.setInScene(windowsInScene);
  }
  public int getWindowCount(){
    return windowList.size();
  }
  
  public void setPosition(double[] pos){
    MatrixBuilder.euclidean().translate(pos).assignTo(sgc);
  }
  
  public void enableVertexPopUpTool(boolean enableVertexPopUpTool){
	  for (JRWindow w : windowList) w.enableVertexPopUpTool(enableVertexPopUpTool);
  }
  
  public SceneGraphComponent getSceneGraphRepresentation() {
  	return sgc;
  }
}
