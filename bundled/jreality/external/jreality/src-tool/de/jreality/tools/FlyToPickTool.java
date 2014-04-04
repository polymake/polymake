package de.jreality.tools;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;

/** Tool to fly to a picked target
 * 
 *   without "fixedYAxis" the avatar will Fly to the picked destination, while rotating around the target.
 *   The animation stops at a fixed Ratio of the way from start to picked destination.
 *   
 *   with "fixedYAxis" enabled, the avatar will only rotate around the vertical Axis by the target, changing hight and distance.
 *   The Avatar and Camera stay in the same allign except turning around the Y Axis. 
 *   
 *    By simulating a Scene with no specified up/Down-direktion one should use "fixedYAxis=false"
 *    Otherwise  "fixedYAxis=true" is strongly recommend
 * 
 * @author gonska
 */
public class FlyToPickTool  extends AbstractTool {

	static InputSlot timerSlot = InputSlot.getDevice("SystemTime");
	static InputSlot zoomActivateSlot = InputSlot.getDevice("JumpActivation");// F
	static InputSlot mouseAimer = InputSlot.getDevice("PointerNDC");// F
	static InputSlot reverseSlot = InputSlot.getDevice("Secondary");// F


	private double flightTime=1000;// time to fly (mil.sec) 
	private double goFactor=0.75;// goes % of the way, then stop 
	private double timeNow=0;
	private SceneGraphComponent arrowNode;
	private boolean attached=false;
	private boolean reverse=false;
	private boolean fixYAxis=true;

	// a "P" means "Point" 
	// a "V" means "Vector" 
	// a "W" means "in WorldCoords" 
	// a "T" means "relative to Target" 
	// a "A" means "in AvatarCoords"
	// a "S" means "in SceneCoords"
	// a number means the dimension
	private double[] mousePosFirst; 
	private double[] fromW4P;
	private double[] toW4P;
	private double[] axisW3V;
	private double angleW;
	double[] fromToW4V;
	double dist;
	private double[] startMatrixAvatar;
	private SceneGraphPath avaPath;
	private SceneGraphPath camPath;
	private SceneGraphComponent avaNode;
	
// for fixed Y Axis	
	double turnAngle;
	double[] turnAxis=new double []{0,1,0};
				
	double hightAngle;
	double[] hightAxis;


	public FlyToPickTool() {
		super(zoomActivateSlot);
		addCurrentSlot(timerSlot);
		arrowNode= new SceneGraphComponent();
		arrowNode.setGeometry(Primitives.arrow(0, 0, 1, 0, 0.2));
		setDescription("type jump: fly to picked Point ");
		setDescription(timerSlot, "");
		setDescription(zoomActivateSlot, 
				"type jump: fly to picked Point \n\r"+
		"hold jump: move mouse to set destinated to fly to \n\r");
		setDescription(reverseSlot, 
		"reverses the flight to zoom away");

	}
	@Override
	public void activate(ToolContext tc) {
		avaPath=tc.getAvatarPath();
		avaNode = avaPath.getLastComponent();
		camPath=tc.getViewer().getCameraPath();
		AnimatorTool.getInstance(tc).deschedule(avaNode);
		PickResult currentPick = tc.getCurrentPick();
		if ( currentPick!=null){
			fromW4P=Rn.matrixTimesVector(null, camPath.getMatrix(null), new double[]{0,0,0,1});
			toW4P=currentPick.getWorldCoordinates();
			// remember mousePos:
			double[] temp=tc.getTransformationMatrix(mouseAimer).toDoubleArray(null);
			mousePosFirst=new double[]{temp[3],temp[7]};
			// starting trafo
			Transformation cmpTrafo= avaNode.getTransformation();
			if (cmpTrafo==null) cmpTrafo=new Transformation();
			startMatrixAvatar=cmpTrafo.getMatrix();
			arrowNode.setVisible(true);
		}
		assureAttached(tc);
	}
	@Override
	public void deactivate(ToolContext tc) {
		// reverse
		reverse=tc.getAxisState(reverseSlot).isPressed();
		timeNow=0;
		AnimatorTask task= new AnimatorTask(){
			public boolean run(double time, double dt) {
				timeNow+=dt;
				if(timeNow>flightTime){
					avaNode.setTransformation(getNextTrafoOfN(1));
					return false;
				}
				avaNode.setTransformation(getNextTrafoOfN(timeNow/flightTime));
				return true;
			}
		};
		AnimatorTool.getInstance(tc).schedule(avaNode, task);		
		arrowNode.setVisible(false);
	}
	private void assureAttached(ToolContext tc) {
		if (!attached) tc.getViewer().getSceneRoot().addChild(arrowNode);
		attached = true;
		arrowNode.setVisible(true);
		arrowNode.setAppearance(new Appearance());
		arrowNode.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBES_DRAW, false);
	}

	private double[] toR3(double[] d){return new double[]{d[0],d[1],d[2]};}
	@Override
	public void perform(ToolContext tc) {
		///calculate mouseAiming
		double[] temp=tc.getTransformationMatrix(mouseAimer).toDoubleArray(null);
		double[] relMousePos=new double[]{mousePosFirst[0]-temp[3],mousePosFirst[1]-temp[7]};
		fromToW4V=Rn.subtract(null, fromW4P, toW4P); 
		double[]a=toR3(toW4P);
		double[]b=toR3(fromW4P);
		dist=Rn.euclideanDistance(a, b)*(1-goFactor);
		
		if(!fixYAxis){
			///rotation axis:
			axisW3V=new double[]{relMousePos[1],-relMousePos[0],0,0};
			axisW3V=Rn.matrixTimesVector(null, camPath.getMatrix(null), axisW3V);// "W" 
			axisW3V= new double[]{axisW3V[0],axisW3V[1],axisW3V[2]};// "3"
			Rn.normalize(axisW3V,axisW3V);
			///angle:
			angleW=Rn.euclideanNorm(relMousePos)*4;
			while(angleW>Math.PI) angleW-=Math.PI*2;
			while(angleW<-Math.PI) angleW+=Math.PI*2;
			/// show Destination-Point with arrow:
			
			MatrixBuilder.euclidean()
			.translate(toW4P).rotate(angleW, axisW3V)
			.rotateFromTo(new double[]{1,0,0}, fromToW4V)
			.scale(dist).assignTo(arrowNode);
		}
		else{
			turnAngle=-relMousePos[0]*4;

			hightAngle=relMousePos[1]*4;
			hightAxis=Rn.crossProduct(null,turnAxis, fromToW4V);
			Rn.normalize(hightAxis,hightAxis);
			
			MatrixBuilder.euclidean()
			.translate(toW4P)
			.rotate(turnAngle, turnAxis)
			.rotate(hightAngle, hightAxis)
			.rotateFromTo(new double[]{1,0,0}, fromToW4V)
			.scale(dist).assignTo(arrowNode);
		}
	}
	private Transformation getNextTrafoOfN(double s){
		/// make smoth motion:
		s=smothFlight(s);
		double[] toTargetMatrixW =MatrixBuilder.euclidean().translate(toW4P).getArray();// ohne s !
		/// camera move to unrotated Destination (mit s)
		double[] moveNearW4V;
		double[] translMatrixW;
		double[] combinedMatrix;
		if (!reverse){
			moveNearW4V=Rn.times(null, goFactor*s,Rn.subtract(null, toW4P, fromW4P));
		}
		else {
			moveNearW4V=Rn.times(null, (1-(1.0/(1-goFactor)))*s,Rn.subtract(null, toW4P, fromW4P));
		}
		moveNearW4V[3]=1;// to Point
		translMatrixW=MatrixBuilder.euclidean().translate(moveNearW4V).getArray();
		/// Fallunterscheidung:
		if(!fixYAxis){
			/// rotate around target 
			double[]rotateMatrixW=MatrixBuilder.euclidean().rotate(angleW*s, axisW3V).getArray();
			rotateMatrixW= Rn.conjugateByMatrix(null, rotateMatrixW, toTargetMatrixW);
			// combine
			combinedMatrix=Rn.times(null, rotateMatrixW,translMatrixW);
			combinedMatrix=Rn.times(null,combinedMatrix, startMatrixAvatar);
		}
		else{
			/// turn around target
			double[]rotateMatrixW=MatrixBuilder.euclidean().rotate(turnAngle*s, turnAxis).getArray();
			rotateMatrixW= Rn.conjugateByMatrix(null, rotateMatrixW, toTargetMatrixW);
			
			// move up/Down around target
			double alpha=Math.acos(fromToW4V[1]/Rn.euclideanNorm(toR3(fromToW4V)));
			double y=dist*(Math.cos(hightAngle*s+alpha)-Math.cos(alpha));
			double z=dist*(Math.sin(hightAngle*s+alpha)-Math.sin(alpha));
			double[] moveAroundMatrixT=MatrixBuilder.euclidean()
			.translate(0,y,z).getArray();
			combinedMatrix= MatrixBuilder.euclidean().getArray();
			combinedMatrix=Rn.times(null,combinedMatrix,rotateMatrixW);
			combinedMatrix=Rn.times(null,combinedMatrix,moveAroundMatrixT);
			combinedMatrix=Rn.times(null,combinedMatrix,translMatrixW);
			combinedMatrix=Rn.times(null,combinedMatrix, startMatrixAvatar);
		}
		return new Transformation(combinedMatrix);
	}
	private double smothFlight(double in){
		return -(Math.cos(in*Math.PI))/2+1.0/2;
	}
	// --------- getter & setter & description---------

	@Override
	public String getDescription() {
		return "type jump: fly to picked Point ";
	}
	@Override
	protected void setDescription(InputSlot slot, String description) {}
	public double getFlightTime() {
		return flightTime/1000;
	}
	public void setFlightTime(double flightTime) {
		this.flightTime = flightTime*1000;
	}
	public double getGoFactor() {
		return goFactor;
	}
	public void setGoFactor(double goFactor) {
		this.goFactor = goFactor;
	}
	public boolean isHoldYAxis() {
		return fixYAxis;
	}
	public void setHoldYAxis(boolean holdYAxis) {
		this.fixYAxis = holdYAxis;
	}
}
