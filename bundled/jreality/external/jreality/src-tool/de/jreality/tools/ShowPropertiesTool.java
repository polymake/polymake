/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.tools;

import java.awt.Color;
import java.awt.Frame;
import java.awt.TextArea;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import de.jreality.math.Matrix;
import de.jreality.math.Rn;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;

/**
 * @author bleicher
 *
 */

public class ShowPropertiesTool extends AbstractTool {
	
	//private List activationSlots;
	private static final InputSlot pointerSlot = InputSlot.getDevice("PointerTransformation");
	
	private Geometry geo;
	private Matrix obj2WorldTrans;
	private int pickIndex;
	private Matrix pointerTrans=new Matrix();
	private boolean geometryMatched;
	
	private ShowPropertiesToolLogger[] log=new ShowPropertiesToolLogger[3];
	private boolean holdLog=false;
	private boolean holdLogFrame=true;
	
	
	
	public ShowPropertiesTool() {
		addCurrentSlot(pointerSlot);
		for(int i=0;i<log.length;i++)  	log[i]=new ShowPropertiesToolLogger();
		initLogFrame();
	}
	
    public void activate(ToolContext tc){
    	showLogFrame(true);
    	perform(tc);
    }    
    
    public void perform(ToolContext tc){  
    	for(int i=0;i<log.length;i++)  	log[i].clear();  
    	tc.getTransformationMatrix(pointerSlot).toDoubleArray(pointerTrans.getArray());
    	
    	evalPointer();
    	
    	PickResult currentPick = tc.getCurrentPick();
		geometryMatched=(!(currentPick == null));
    	if(geometryMatched){
        	geo=currentPick.getPickPath().getLastComponent().getGeometry();
        	pickIndex=currentPick.getIndex();
        	obj2WorldTrans = new Matrix(currentPick.getPickPath().getMatrix(null));
        	
        	if((currentPick.getPickType() == PickResult.PICK_TYPE_POINT)) evalPoint(tc);
        	if((currentPick.getPickType() == PickResult.PICK_TYPE_LINE)) evalEdge(tc);
        	if((currentPick.getPickType() == PickResult.PICK_TYPE_FACE)) evalFace(tc);
        	if((currentPick.getPickType() == PickResult.PICK_TYPE_OBJECT)) evalObject(tc);
    	}
    	print();
    }
    
    public void deactivate(ToolContext tc){
    	if(!holdLog){
    		showLogFrame(false);
    		for(int i=0;i<log.length;i++)  	log[i].clear();
    		print();
    	}
    }
    
    private double[] from;
    private double[] dir;
    private double[] pickedPointOC;
    private double[] pickedPointWC;
    private double[] pickedPointNormalOC;
    private double[] pickedPointNormalWC;
    private double[] pickedPointTexC;
    private int[] pickedEdgeVertexInds;
    private double[][] pickedEdgeVerts;
    private int[] pickedFaceVertexInds;
    private double[][] pickedFaceVerts;
    private double[] pickedObjectWC;
    
    private void evalPointer(){    	
    	from=pointerTrans.getColumn(3);
    	dir=Rn.times(null,-1,pointerTrans.getColumn(2));  
    	
    	log[0].addLine("pointerOnNearClippingPlane",from);
    	log[0].addLine("pointerDirection", dir);
    }    	
    
    private void evalPoint(ToolContext tc){  
    	PickResult currentPick = tc.getCurrentPick();
		if(currentPick.getPickType() == PickResult.PICK_TYPE_POINT){
        	pickedPointOC=((PointSet)geo).getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray().getValueAt(pickIndex).toDoubleArray(null);
            pickedPointWC=obj2WorldTrans.multiplyVector(pickedPointOC);
            	
            log[2].addLine("pickedPointObjCoords",pickedPointOC);
            log[2].addLine("pickedPointWorldCoords",pickedPointWC);
            	
           	if(((PointSet)geo).getVertexAttributes(Attribute.TEXTURE_COORDINATES)!=null){
        		pickedPointTexC=((PointSet)geo).getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray().getValueAt(pickIndex).toDoubleArray(null);
        		
        		log[2].addLine("pickedPointTexC",pickedPointTexC);
           	}
        	if(((PointSet)geo).getVertexAttributes(Attribute.NORMALS)!=null){
        		pickedPointNormalOC=((IndexedFaceSet)geo).getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray().getValueAt(pickIndex).toDoubleArray(null);
        		pickedPointNormalWC=obj2WorldTrans.multiplyVector(pickedPointNormalOC);
        			
        		log[2].addLine("pickedPointNormalOC",pickedPointNormalOC);
        	    log[2].addLine("pickedPointNormalWC",pickedPointNormalWC);        			
        	}
        }else{
        	pickedPointOC=currentPick.getObjectCoordinates();
            pickedPointWC=obj2WorldTrans.multiplyVector(pickedPointOC);  
            	
            log[2].addLine("pickedPointObjCoords",pickedPointOC);
            log[2].addLine("pickedPointWorldCoords",pickedPointWC);
    	}
        log[1].setLine("pickType: point");    	
    }
    private void evalEdge(ToolContext tc){
    	evalPoint(tc);
    	PickResult currentPick = tc.getCurrentPick();
		if(!(currentPick.getPickType() == PickResult.PICK_TYPE_LINE)) return;
    	if(((IndexedLineSet)geo).getEdgeAttributes(Attribute.INDICES)!=null){
    		pickedEdgeVertexInds=((IndexedLineSet)geo).getEdgeAttributes(Attribute.INDICES).toIntArrayArray().getValueAt(pickIndex).toIntArray(null);
    		pickedEdgeVerts=new double[pickedEdgeVertexInds.length][];
    		for(int i=0;i<pickedEdgeVertexInds.length;i++){
    			pickedEdgeVerts[i]=((PointSet)geo).getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray().getValueAt(pickedEdgeVertexInds[i]).toDoubleArray(null);
    		}
    		
        	log[2].addLine("pickedEdgeVertexIndices",pickedEdgeVertexInds);
        	log[2].addLine("edge contains points:");
        	for(int i=0;i<pickedEdgeVertexInds.length;i++){
    			log[2].addLine("	",pickedEdgeVerts[i]);
    		}	
        	log[1].setLine("pickType: line");
    	}
    }
    private void evalFace(ToolContext tc){
    	evalPoint(tc);
		PickResult currentPick = tc.getCurrentPick();
		if(currentPick.getTextureCoordinates()!=null){
			pickedPointTexC=currentPick.getTextureCoordinates();
			log[2].addLine("pickedPointTexC",pickedPointTexC);
		}
    	if(((IndexedFaceSet)geo).getFaceAttributes(Attribute.INDICES)!=null){
    		pickedFaceVertexInds=((IndexedFaceSet)geo).getFaceAttributes(Attribute.INDICES).toIntArrayArray().getValueAt(pickIndex).toIntArray(null);
    		pickedFaceVerts=new double[pickedFaceVertexInds.length][];
    		for(int i=0;i<pickedFaceVertexInds.length;i++){
    			pickedFaceVerts[i]=((PointSet)geo).getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray().getValueAt(pickedFaceVertexInds[i]).toDoubleArray(null);
    		}
    		
        	log[2].addLine("pickedFaceVertexIndices",pickedFaceVertexInds);
        	log[2].addLine("face contains points:");
        	for(int i=0;i<pickedFaceVertexInds.length;i++){
    			log[2].addLine("	",pickedFaceVerts[i]);
    		}	
        	log[1].setLine("pickType: face");
    	}
    }
    private void evalObject(ToolContext tc){     	
    	pickedObjectWC=obj2WorldTrans.getColumn(3);
    	log[2].addLine("pickedObjectWorldCoords",pickedObjectWC); 
    	log[1].setLine("pick-type: object");
    }    
    
    public double[] getFrom(){
    	return from;	
    }
    public double[] getDir(){
    	return dir;	
    }
    public boolean geometryMatched(){
    	return geometryMatched;
    }
    public Matrix getObj2WorldTrans(){
    	return obj2WorldTrans;
    }
    public double[] getPickedVertOC(){
    	return pickedPointOC;
    }
    public double[] getPickedVertWC(){
    	return pickedPointWC;
    }
    public double[] getPickedVertNormalOC(){
    	return pickedPointNormalOC;
    }
    public double[] getPickedVertNormalWC(){
    	return pickedPointNormalWC;
    }
    public double[] getPickedVertTexC(){
    	return pickedPointTexC;
    }
    public int[] getPickedEdgeVertexInds(){
    	return pickedEdgeVertexInds;
    }
    public double[][] getPickedEdgeVerts(){
    	return pickedEdgeVerts;
    }    
    public int[] getPickedFaceVertexInds(){
    	return pickedFaceVertexInds; 
    }
    public double[][] getPickedFaceVerts(){
    	return pickedFaceVerts;
    }
    public double[] getPickedObjectWC(){
    	return pickedObjectWC;
    }
    
    /////////////////////////////LogFrame
    private Frame logFrame;
    private TextArea ta;
    
    private void initLogFrame(){
    	logFrame=new Frame();
    	logFrame.setSize(800,270);
    	logFrame.setLocation(100, 700);
		if(holdLogFrame) showLogFrame(true);    	
    	
    	logFrame.setBackground(Color.white);
    	logFrame.addWindowListener(
			new WindowAdapter() {
				public void windowClosing(WindowEvent event){
					logFrame.dispose();
				}
			}
		);
    	ta=new TextArea(); 
		ta.setEditable(false);
		ta.setBackground(Color.white);
		logFrame.add(ta);
    }
    private void showLogFrame(boolean show){
    	if(holdLogFrame) show=true;
    	logFrame.setVisible(show);
    }    
    private void print(){    	
    	ta.setText("");
    	for(int i=0;i<log.length;i++){
    		ta.append(log[i].getLog());
    		ta.append("\n");
    		ta.append("\n");
    	}
    }
    
    public void holdLog(boolean hold){
    	holdLog=hold;
    }
    public void holdLogFrame(boolean hold){
    	holdLogFrame=hold;
    }
    public void setHomogeneousLogging(boolean hom){
    	for(int i=0;i<log.length;i++)  	log[i].setHomogeneousLogging(hom);  
    }
}
