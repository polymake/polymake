package de.jreality.tools;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
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


public class DragEventTool extends AbstractTool {
	

	protected PointDragListener pointDragListener;
	protected LineDragListener lineDragListener;
	protected FaceDragListener faceDragListener;
	protected PrimitiveDragListener primitiveDragListener;
	
  private static final InputSlot pointerSlot = InputSlot.getDevice("PointerTransformation");
  private static InputSlot alongZPointerSlot = InputSlot.getDevice("DragAlongViewDirection");
  
	public DragEventTool(String dragSlotName){
		super(InputSlot.getDevice(dragSlotName));
		addCurrentSlot(pointerSlot, "triggers drag events");
    addCurrentSlot(alongZPointerSlot);
	}

	public DragEventTool(InputSlot... activationSlots) {
		super(activationSlots);
		addCurrentSlot(pointerSlot, "triggers drag events");
	    addCurrentSlot(alongZPointerSlot);
	}
	
	public DragEventTool(){
		  this("AllDragActivation");      
	}
	
  protected boolean active;
  private boolean dragInViewDirection;
  protected Geometry geom;
  protected PointSet pointSet;
  protected IndexedLineSet lineSet;
  protected IndexedFaceSet faceSet;
  protected int index=-1;
  protected double[] pickPoint=new double[4];;
  private int pickType=PickResult.PICK_TYPE_OBJECT;
    
  private Matrix pointerToPoint = new Matrix();
    
	public void activate(ToolContext tc) {
		active = true;    
    try {
      if (tc.getAxisState(alongZPointerSlot).isPressed()) 
        dragInViewDirection = true;
      else 
        dragInViewDirection = false;
    }catch (Exception me) {dragInViewDirection = false;}  
    
    tc.getTransformationMatrix(pointerSlot).toDoubleArray(pointerToPoint.getArray());     
    pointerToPoint.invert();     
    pointerToPoint.multiplyOnRight(tc.getRootToLocal().getMatrix(null));    
    
    Matrix root2cam=new Matrix(tc.getViewer().getCameraPath().getMatrix(null));
    root2cam.setColumn(3,new double[]{0,0,0,1});  
    distDir[0]=Rn.normalize(null,root2cam.multiplyVector(dir2ScaleZDrag[0]));
    distDir[1]=Rn.normalize(null,root2cam.multiplyVector(dir2ScaleZDrag[1]));
    PickResult currentPick = tc.getCurrentPick();
	if (currentPick == null ){
//    	throw new IllegalStateException("null pick");
		tc.reject();
		active=false;
		return;
	}
    if (currentPick.getPickType() == PickResult.PICK_TYPE_OBJECT) {
        if (primitiveDragListener == null) {
          active=false;
          tc.reject();
          return;
        }
  	    pickType=PickResult.PICK_TYPE_OBJECT;
  	    geom = (Geometry) currentPick.getPickPath().getLastElement();
  	  double[] pickPointTemp=currentPick.getObjectCoordinates();
      if(pickPointTemp.length==3) Pn.homogenize(pickPoint,pickPointTemp);
      else Pn.dehomogenize(pickPoint,pickPointTemp);
      MatrixBuilder.euclidean(pointerToPoint).translate(pickPoint); 	            
	  firePrimitiveDragStart(new double[]{0,0,0,1});        

    } else if (currentPick.getPickType() == PickResult.PICK_TYPE_POINT) {
      if (pointDragListener == null) {
        active=false;
        tc.reject();
        return;
      }
	    pickType=PickResult.PICK_TYPE_POINT;
	    pointSet = (PointSet) currentPick.getPickPath().getLastElement();
	    index=currentPick.getIndex();  
      double[] pickPointTemp = pointSet.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray().getValueAt(index).toDoubleArray(null);
      if(pickPointTemp.length==3) Pn.homogenize(pickPoint,pickPointTemp);
      else Pn.dehomogenize(pickPoint,pickPointTemp);
      MatrixBuilder.euclidean(pointerToPoint).translate(pickPoint);  
      // timh: should be replaced by the folowing if the tool should respect trafos of the object while dragging...
      //Matrix m = new Matrix(tc.getRootToLocal().getMatrix(null));
      //m.invert();      
      //MatrixBuilder.euclidean(pointerToPoint).translate(pickPoint).times(m);  
      //
	    firePointDragStart(pickPoint);        
	  }else if (currentPick.getPickType() == PickResult.PICK_TYPE_LINE) {	            
	    if (lineDragListener == null) {
	      active=false;
        tc.reject();
        return;
      }
	    pickType=PickResult.PICK_TYPE_LINE;
	    lineSet = (IndexedLineSet) currentPick.getPickPath().getLastElement();
	    index=currentPick.getIndex();	            
	    double[] pickPointTemp=currentPick.getObjectCoordinates();
      if(pickPointTemp.length==3) Pn.homogenize(pickPoint,pickPointTemp);
      else Pn.dehomogenize(pickPoint,pickPointTemp);
      MatrixBuilder.euclidean(pointerToPoint).translate(pickPoint);	            
      // timh: as above 
      //Matrix m = new Matrix(tc.getRootToLocal().getMatrix(null));
      //m.invert();      
      //MatrixBuilder.euclidean(pointerToPoint).translate(pickPoint).times(m);  
      //
	    fireLineDragStart(new double[]{0,0,0,1},pickPoint);        
	  }else if (currentPick.getPickType() == PickResult.PICK_TYPE_FACE) {
      if (faceDragListener == null) {
        active=false;
        tc.reject();
        return;
      }
      pickType=PickResult.PICK_TYPE_FACE;
	    faceSet = (IndexedFaceSet) currentPick.getPickPath().getLastElement();
	    index=currentPick.getIndex();
	    double[] pickPointTemp=currentPick.getObjectCoordinates();
      if(pickPointTemp.length==3) Pn.homogenize(pickPoint,pickPointTemp);
      else Pn.dehomogenize(pickPoint,pickPointTemp);
      MatrixBuilder.euclidean(pointerToPoint).translate(pickPoint);
      // timh as above
      //Matrix m = new Matrix(tc.getRootToLocal().getMatrix(null));
      //m.invert();      
      //MatrixBuilder.euclidean(pointerToPoint).translate(pickPoint).times(m);  
      //
	    fireFaceDragStart(new double[]{0,0,0,1},pickPoint);
	  }else {
      active=false;
      tc.reject();
    }
	}

  
  private final double[][] dir2ScaleZDrag=new double[][]{{1,0,0},{0,1,0}};   //Richtung in Weltkoordinaten, die die Staerke des drag in z-Richtung bestimmt
  private double[][] distDir=new double[dir2ScaleZDrag.length][dir2ScaleZDrag[0].length];
  //private double f=2*Math.sin(Math.PI/4);
  private Matrix result=new Matrix();
	
	public void perform(ToolContext tc) {		
 		if (!active) return;    
    tc.getTransformationMatrix(pointerSlot).toDoubleArray(result.getArray());     
    result.multiplyOnRight(pointerToPoint);
    // timh : get root to local on every event instead of the beginning only: 
    //result.multiplyOnRight(tc.getRootToLocal().getMatrix(null));
    result.multiplyOnLeft(tc.getRootToLocal().getInverseMatrix(null));
    
    double[] newPoint3=new double[3];   Pn.dehomogenize(newPoint3,result.getColumn(3));
    double[] pickPoint3=new double[3];  Pn.dehomogenize(pickPoint3,pickPoint); 
    double[] translation3=Rn.subtract(null,newPoint3,pickPoint3);
    if(dragInViewDirection){       
      double[] dir=new double[3];
      Pn.dehomogenize(dir,pointerToPoint.getInverse().getColumn(2));
      Rn.normalize(dir,dir);
      Matrix root2local=new Matrix(tc.getRootToLocal().getMatrix(null));
      root2local.setColumn(3,new double[]{0,0,0,1});      
      Pn.dehomogenize(translation3,root2local.multiplyVector(Pn.homogenize(null,translation3)));        
      double factor=(Rn.innerProduct(distDir[0],translation3)+Rn.innerProduct(distDir[1],translation3));// /f;
     //double factor=Rn.innerProduct(distDir[0],translation3);
      factor=factor/Rn.euclideanNorm(root2local.multiplyVector(dir)); 
      Rn.times(translation3,factor,dir);
    }   
    double[] translation={translation3[0],translation3[1],translation3[2],1};
    double[] position = new double[4];
	  if (pickType == PickResult.PICK_TYPE_OBJECT) {
		    firePrimitiveDragged(Rn.add(translation,translation,pickPoint));
	  }else if (pickType == PickResult.PICK_TYPE_POINT) {
	    firePointDragged(Rn.add(translation,translation,pickPoint));
	  }else if (pickType == PickResult.PICK_TYPE_LINE) {
	    fireLineDragged(translation,Rn.add(position,translation,pickPoint));    	
	  }else if (pickType == PickResult.PICK_TYPE_FACE) {
      fireFaceDragged(translation,Rn.add(position,translation,pickPoint));	    	
	  }
	}

	public void deactivate(ToolContext tc) {
		  if (!active) return;   
	      if (pickType == PickResult.PICK_TYPE_OBJECT) firePrimitiveDragEnd(pickPoint);
	      else if (pickType == PickResult.PICK_TYPE_POINT) firePointDragEnd(pickPoint);
	      
	      // TODO: this needs to be fixed:
	      else if (pickType == PickResult.PICK_TYPE_LINE) fireLineDragEnd(new double[]{0,0,0,1}, new double[]{0,0,0,1});
	      else if (pickType == PickResult.PICK_TYPE_FACE) fireFaceDragEnd(new double[]{0,0,0,1}, new double[]{0,0,0,1});
	      index=-1;
	      pointSet=null;
	      lineSet=null;
	      faceSet=null;
	      active = false;	
	      result=new Matrix();
	      pickType=PickResult.PICK_TYPE_OBJECT;
	}	
	
    public void addPrimitiveDragListener(PrimitiveDragListener listener) {
        primitiveDragListener = PrimitiveDragEventMulticaster.add(primitiveDragListener, listener);
    }
    public void removePrimitiveDragListener(PrimitiveDragListener listener) {
    	primitiveDragListener = PrimitiveDragEventMulticaster.remove(primitiveDragListener, listener);
    }    
    public void addPointDragListener(PointDragListener listener) {
        pointDragListener = PointDragEventMulticaster.add(pointDragListener, listener);
    }
    public void removePointDragListener(PointDragListener listener) {
    	pointDragListener = PointDragEventMulticaster.remove(pointDragListener, listener);
    }    
    public void addLineDragListener(LineDragListener listener) {
        lineDragListener = LineDragEventMulticaster.add(lineDragListener, listener);
    }
    public void removeLineDragListener(LineDragListener listener) {
    	lineDragListener = LineDragEventMulticaster.remove(lineDragListener, listener);
    }    
    public void addFaceDragListener(FaceDragListener listener) {
    	faceDragListener = FaceDragEventMulticaster.add(faceDragListener, listener);
    }
    public void removeFaceDragListener(FaceDragListener listener) {
    	faceDragListener = FaceDragEventMulticaster.remove(faceDragListener, listener);
    }
	
	protected void firePrimitiveDragStart(double[] position) {
	    final PrimitiveDragListener l=primitiveDragListener;
		if (l != null) l.primitiveDragStart(new PrimitiveDragEvent(geom, position));
	}
    protected void firePrimitiveDragged(double[] position) {
		final PrimitiveDragListener l=primitiveDragListener;
		if (l != null) l.primitiveDragged(new PrimitiveDragEvent(geom, position));
	}
	protected void firePrimitiveDragEnd(double[] position) {
		final PrimitiveDragListener l=primitiveDragListener;
		if (l != null) l.primitiveDragEnd(new PrimitiveDragEvent(geom, position));
	}
		   
    protected void firePointDragStart(double[] location) {
        final PointDragListener l=pointDragListener;
        if (l != null) l.pointDragStart(new PointDragEvent(pointSet, index, location));
    }
    protected void firePointDragged(double[] location) {
        final PointDragListener l=pointDragListener;
        if (l != null) l.pointDragged(new PointDragEvent(pointSet, index, location));
    }      
    protected void firePointDragEnd(double[] location) {
        final PointDragListener l=pointDragListener;
        if (l != null) l.pointDragEnd(new PointDragEvent(pointSet, index, location));
    }
    
	protected void fireLineDragStart(double[] translation, double[] position) {
	    final LineDragListener l=lineDragListener;
		if (l != null) l.lineDragStart(new LineDragEvent(lineSet, index, translation, position));
	}
    protected void fireLineDragged(double[] translation, double[] position) {
		final LineDragListener l=lineDragListener;
		if (l != null) l.lineDragged(new LineDragEvent(lineSet, index, translation,position));
	}
	protected void fireLineDragEnd(double[] translation, double[] position) {
		final LineDragListener l=lineDragListener;
		if (l != null) l.lineDragEnd(new LineDragEvent(lineSet, index, translation, position));
	}
		   
	protected void fireFaceDragStart(double[] translation, double[] position) {
		final FaceDragListener l=faceDragListener;
		if (l != null) l.faceDragStart(new FaceDragEvent(faceSet, index, translation,position));
	}
    protected void fireFaceDragged(double[] translation, double[] position) {
		final FaceDragListener l=faceDragListener;
		if (l != null) l.faceDragged(new FaceDragEvent(faceSet, index, translation, position));
	}
    protected void fireFaceDragEnd(double[] translation, double[] position) {
		final FaceDragListener l=faceDragListener;
		if (l != null) l.faceDragEnd(new FaceDragEvent(faceSet, index, translation, position));
	}
}
