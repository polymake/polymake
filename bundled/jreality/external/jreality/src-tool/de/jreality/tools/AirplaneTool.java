package de.jreality.tools;

import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;





/** @author gonska
 * Fly Tool that behaves like an airplane. 
 * Use on Node which schould be moved.
 * 
 * TODO:
 * - make mouse controling available
 * - make roll,up & down  
 *  	more lazy(an airplane can not react instantly) 
 * - what about controling the flight
 *      with respect to velocity (no velocity: no turning around?)
 */

public class AirplaneTool extends AbstractTool{
/// ------------------------------------ Slots ---------------------------------------------
	private transient final InputSlot forwardBackward = InputSlot.getDevice("ForwardBackwardAxis");//neigung
	private transient final InputSlot leftRight = InputSlot.getDevice("LeftRightAxis");// rollen
	private transient final InputSlot slower = InputSlot.getDevice("DecreaseSpeed"); // breack reverseThrust
	private transient final InputSlot faster = InputSlot.getDevice("IncreaseSpeed");// increase thrust
	/// TODO: need Mouse Position relativly to center 
	private transient final InputSlot timer = InputSlot.getDevice("SystemTime");
	private transient final InputSlot aBurner = InputSlot.getDevice("JumpActivation");// afterburner

//	/ -------------------------------------- public Fields -------------------------------------------
	private double thrustFactor=0.01;
	private boolean upDownInvert=false; 
	private double runFactor=2;
	private double rollSpeed= 1.0/500;
	private double rollUpDownSpeed= 1.0/2000;
	private double thrustHesitation=50; 

/// -------------------------------------- private Fields -------------------------------------------
	private double thrust =0;// destinated velocity in [0,1]  [-1,0 is breack & thrust-reverser]
	private transient double velocity = 0;
	private SceneGraphPath toolPath; 
	private SceneGraphComponent comp;
	private long timeStamp;
	private boolean afterburner=false;// thrust=runFactor* maxThrust
	private double rollLeftRight=0;// 1: left, 0: nothing -1:right
	private double rollUpDown=0;//  -1: down, 0: nothing 1:up
	private double changeThrust=0;// -1: slower, 0: nothing 1:faster
/// -------------------------------------- methods --------------------------------------------------
	public AirplaneTool() {
		addCurrentSlot(timer);
		addCurrentSlot(forwardBackward);
		addCurrentSlot(leftRight);
		addCurrentSlot(aBurner);
		addCurrentSlot(faster);
		addCurrentSlot(slower);
		
		setDescription("Fly like an airplane (no gravity)");
		setDescription(timer, "");
		setDescription(forwardBackward, "Nose up & down");
		setDescription(leftRight, "roll left & right");
		setDescription(aBurner, "afterburner(multiple Speed)");
		setDescription(slower, "decrease thrust");
		setDescription(faster, "increase thrust");
		
	}

	public void activate(ToolContext tc) {
		timeStamp=tc.getTime();
	}

	
	public void perform(ToolContext tc) {
		// prework:
		double dt=tc.getTime()-timeStamp; 
		toolPath=tc.getRootToToolComponent();
		comp=toolPath.getLastComponent();
		if(comp.getTools()==null)
			comp.setTransformation(new Transformation());
		/// afterburner switch:
		afterburner=tc.getAxisState(aBurner).isPressed();
		/// thrust:
		changeThrust=0;
		if (tc.getAxisState(slower).isPressed()) changeThrust-=1;			
		if (tc.getAxisState(faster).isPressed()) changeThrust+=1;			
		/// controling:
		rollUpDown=tc.getAxisState(forwardBackward).doubleValue();			
		rollLeftRight=tc.getAxisState(leftRight).doubleValue();
		if (upDownInvert) rollUpDown*=-1;
		/// calculate moving:
		thrust+= dt*(1.0/1000)*changeThrust;
		if(thrust<-1)thrust=-1;
		if(thrust>1)thrust=1;
		double trueThrust=thrust*thrustFactor;
		if(afterburner) trueThrust=runFactor*thrustFactor;
		
		velocity= velocity +(trueThrust-velocity)/thrustHesitation;
		if(velocity<=0){// no backwart moving
			thrust=Math.max(0, thrust);
			velocity=0;
		}
		
		double[] moveMatrix=
			MatrixBuilder.euclidean()
			.rotate(dt*rollSpeed*rollLeftRight,0,0,1)/// roll 
			.rotate(dt*rollUpDownSpeed*rollUpDown,1,0,0)/// up/Down
			.translate(0,0,-dt*velocity)
			.getArray();
		// set Matrix
		double[] newMatrix=Rn.times(null, comp.getTransformation().getMatrix(), moveMatrix);
		comp.getTransformation().setMatrix(newMatrix);
		// postwork:
		timeStamp+=dt;
	}
	/// --------------------------------------- getter setter --------------------------------------------

	public double getRollSpeed() {
		return rollSpeed*500;
	}

	public void setRollSpeed(double rollSpeed) {
		this.rollSpeed = rollSpeed/500;
	}

	public double getRollUpDownSpeed() {
		return rollUpDownSpeed*1000;
	}

	public void setRollUpDownSpeed(double rollUpDownSpeed) {
		this.rollUpDownSpeed = rollUpDownSpeed/1000;
	}

	public double getRunFactor() {
		return runFactor;
	}

	public void setRunFactor(double runFactor) {
		this.runFactor = runFactor;
	}

	public double getThrustFactor() {
		return thrustFactor;
	}

	public void setThrustFactor(double thrustFactor) {
		this.thrustFactor = thrustFactor;
	}

	public double getThrustHesitation() {
		return thrustHesitation;
	}

	public void setThrustHesitation(double thrustHesitation) {
		this.thrustHesitation = thrustHesitation;
	}

	public boolean isUpDownInvert() {
		return upDownInvert;
	}

	public void setUpDownInvert(boolean upDownInvert) {
		this.upDownInvert = upDownInvert;
	}
	
}
