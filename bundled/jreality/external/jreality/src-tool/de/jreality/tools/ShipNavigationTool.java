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

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.util.LoggingSystem;

/**
 * @author weissman
 *  
 */
public class ShipNavigationTool extends AbstractTool {

	private transient final InputSlot forwardBackward = InputSlot.getDevice("ForwardBackwardAxis");
	private transient final InputSlot leftRight = InputSlot.getDevice("LeftRightAxis");
	private transient final InputSlot jump = InputSlot.getDevice("JumpActivation");
	private transient final InputSlot rotateActivation = InputSlot.getDevice("ShipRotateActivation");
	private transient final InputSlot horizontalRotation = InputSlot.getDevice("HorizontalShipRotationAngleEvolution");
	private transient final InputSlot timer = InputSlot.getDevice("SystemTime");
	private transient final InputSlot run = InputSlot.getDevice("RunActivation");

	private transient final InputSlot gravityToggle = InputSlot.getDevice("GravityToggle");
	private transient final InputSlot groundToggle = InputSlot.getDevice("GroundToggle");

	private transient double[] velocity = {0,0,0};
	private transient boolean touchGround;

	private double gain = 4;
	private double runFactor=2;
	private double gravity = 9.81;
	private boolean gravitEnabled=true;
	private double jumpSpeed = 8;
	private boolean hasCenter = false;
	private double[] center={0,0,0,1};
	
	private transient boolean rotate=false;
	
	// if the "fall activation" (F-Key as default) is pressed, this is true...
	private transient boolean fall;
	
	
	private double minHeight;
	private PickDelegate pickDelegate;

	public ShipNavigationTool() {
		addCurrentSlot(forwardBackward);
		addCurrentSlot(leftRight);
		addCurrentSlot(rotateActivation);
		addCurrentSlot(jump);
		addCurrentSlot(gravityToggle);
		addCurrentSlot(groundToggle);
		addCurrentSlot(horizontalRotation);
	}

	public void perform(ToolContext tc) {

		if (tc.getSource() == gravityToggle) {
			if (tc.getAxisState(gravityToggle).isReleased()) return;
			setGravitEnabled(!isGravitEnabled());
		}

		if (tc.getSource() == groundToggle) {
			fall=tc.getAxisState(groundToggle).isPressed();
			if (fall) touchGround=false;
		}

		if (rotate) {
			if (!tc.getAxisState(rotateActivation).isPressed()) {
				removeCurrentSlot(horizontalRotation);
				rotate = false;
			}
		} else {
			if (tc.getAxisState(rotateActivation).isPressed()) {
				addCurrentSlot(horizontalRotation);
				rotate = true;
			}
		}
		if (tc.getSource() == rotateActivation) return;
		checkNeedTimer(tc);
		SceneGraphPath path = tc.getRootToLocal();
		SceneGraphComponent myComponent = path.getLastComponent();
		Matrix myMatrix = myComponent.getTransformation() != null ? new Matrix(myComponent.getTransformation()) : new Matrix();	
		
		
		if (rotate && tc.getSource() == horizontalRotation) {
			double rot = tc.getAxisState(horizontalRotation).doubleValue();
			MatrixBuilder.euclidean(myMatrix).rotateY(-rot);
			myMatrix.assignTo(myComponent);
			
			/*
			ToolEvent te = ((ToolContextImpl) tc).getEvent();
			System.out.println("rot="+rot+"\t dt="+tc.getAxisState(timer).intValue()+"\t vel="+(rot/(0.001*tc.getAxisState(timer).intValue()))+"\t when="+tc.getTime()+"\n\t"+te);
			*/
			
			return;
		}
		
		if (isGravitEnabled()) {
			if (tc.getSource() == jump && tc.getAxisState(jump).isPressed()) {
				velocity[1] = jumpSpeed;
			}
		} else velocity[1]=0;
		
		
		if (tc.getSource() != timer) return;
		// All the following happens only when perfrom was triggered with timer...
		
		// read velocity in x- and z- dir from forwardBackward- and leftRight-axes:
		AxisState axis;
		if ((axis = tc.getAxisState(forwardBackward)) != null) {
			velocity[2] = gain*axis.doubleValue();
		}
		if ((axis = tc.getAxisState(leftRight)) != null) {
			velocity[0] = -gain*axis.doubleValue();
		}
		if (tc.getAxisState(run) != null && tc.getAxisState(run).isPressed()) {
			velocity[0]*=runFactor;
			velocity[2]*=runFactor;
		}
		
		
		// only do something when we have non-zero velocity or we are falling...
		if (!(touchGround && velocity[0] == 0 && velocity[1] == 0 && velocity[2] == 0)) {

			double sec = 0.001* tc.getAxisState(timer).intValue(); // time since
			
			// dest is the point we would like to move to...
			double[] trans = new double[]{sec*velocity[0], sec*velocity[1], sec*velocity[2], 1};
			double[] dest = myMatrix.multiplyVector(trans);
			Pn.dehomogenize(dest,dest);
			
			if (myMatrix.containsNanOrInfinite()) System.out.println("NAN!!! 1");

			double[] upDir4=new double[4];
			if (!hasCenter) upDir4=new double[]{0, 1, 0, 0};
			else {
				Rn.subtract(upDir4, dest, center);
				upDir4[3]=0;
				Rn.normalize(upDir4, upDir4);
			}

			if (isGravitEnabled()) {
				// if fall is true, we ignore any pick hits and just fall through
				if (!fall) {
					// pick from the eye position down to the feet position:
					double[] pickStart= Rn.linearCombination(null, 1, dest, 1.7, upDir4);
					double[] hit = getHit(tc, pickStart, dest);
					// if we have hit something, make the pick point the new dist and update y-velocity to 0 and remember that we are touching the ground 
					if (hit != null) {
						dest = hit;
						velocity[1] = 0;
						touchGround = true;
					}
				}
				// fall=true: just fall until we reach minheight...
				double h = isCenter() ? Math.sqrt(dest[0]*dest[0]+dest[1]*dest[1]+dest[2]*dest[2]) : dest[1];
				// minheight reached: lift position to minheight and set y-velocity to zero.
				if (h<minHeight) {
					if (isCenter()) {
						dest=Rn.times(dest, minHeight/h, dest);
						dest[3]=1;
					} else {
						dest[1]=minHeight;
					}
					velocity[1] = 0;
					touchGround = true;
				} else {
					velocity[1] -= sec*gravity;
					touchGround = false;
				}
				
			}
			if (hasCenter)	{
				double[] rotateFrom4=Rn.subtract(null, Pn.dehomogenize(null,myMatrix.getColumn(3)), center);
				double[] rotateTo4=Rn.subtract(null,dest,center);
				Matrix rotation = MatrixBuilder.euclidean().rotateFromTo(rotateFrom4, rotateTo4).getMatrix();
				if (!rotation.containsNanOrInfinite()) myMatrix.multiplyOnLeft(rotation);
				else System.out.println("rotation NAN: from="+Arrays.toString(myMatrix.getColumn(3))+" to="+Arrays.toString(dest));
			}
			myMatrix.setColumn(3, dest);
			myMatrix.assignTo(myComponent);
		}
	}
	
	protected double[] getHit(ToolContext tc, double[] pickStart, double[] dest) {
		if (pickDelegate != null && pickDelegate.isEnabled()) {
			return pickDelegate.getHit(tc, pickStart, dest);
		} else {
			double[] hit = null;
			List picks = Collections.EMPTY_LIST;
			try {
				picks = tc.getPickSystem().computePick(pickStart, dest);
			} catch (Exception e) {
				LoggingSystem.getLogger(this).warning("pick system error");
				return null;
			}
			if (!picks.isEmpty()) {
				PickResult pr = (PickResult) picks.get(0);
				hit = pr.getWorldCoordinates();
			}
			return hit;
		}
	}

	private transient boolean timerOnline;

	private void checkNeedTimer(ToolContext tc) {
		boolean needTimer = false;
		if (tc.getAxisState(forwardBackward) != null && !tc.getAxisState(forwardBackward).isReleased()) {
			needTimer = true;
		}
		if (tc.getAxisState(leftRight) != null && !tc.getAxisState(leftRight).isReleased()) {
			needTimer = true;
		}
		if (tc.getAxisState(jump) != null
				&& tc.getAxisState(jump).isPressed())
			needTimer = true;
		if (needTimer || !touchGround) {
			if (!timerOnline) {
				addCurrentSlot(timer);
				timerOnline = true;
			}
		} else {
			if (timerOnline) {
				removeCurrentSlot(timer);
				timerOnline = false;
			}
		}
	}

	public double getGain() {
		return gain;
	}

	public void setGain(double gain) {
		this.gain = gain;
	}

	public double getGravity() {
		return gravity;
	}

	public void setGravity(double gravity) {
		this.gravity = gravity;
	}

	public double getJumpSpeed() {
		return jumpSpeed;
	}

	public void setJumpSpeed(double jumpSpeed) {
		this.jumpSpeed = jumpSpeed;
	}

	public double getRunFactor() {
		return runFactor;
	}

	public void setRunFactor(double runFactor) {
		this.runFactor = runFactor;
	}

	public boolean isCenter() {
		return hasCenter;
	}
	public void setCenter(boolean center) {
		this.hasCenter = center;
	}
	public double[] getCenter(){
		return center;
	}
	public void setCenter(double[] center){
		if(center.length==4)
			Pn.dehomogenize(this.center,center);
		else{
			for(int i=0;i<3;i++)
				this.center[i]=center[i];
			this.center[3]=1;
		}		
	}

	public boolean isGravitEnabled() {
		return gravitEnabled;
	}

	public void setGravitEnabled(boolean gravitEnabled) {
		this.gravitEnabled = gravitEnabled;
	}

	public double getMinHeight() {
		return minHeight;
	}

	public void setMinHeight(double minHeight) {
		this.minHeight = minHeight;
	}
	
	public static interface PickDelegate {
		public double[] getHit(
				ToolContext tc,
				double[] pickStart,
				double[] dest
		);
		public boolean isEnabled();
	}

	public PickDelegate getPickDelegate() {
		return pickDelegate;
	}

	public void setPickDelegate(PickDelegate pickDelegate) {
		this.pickDelegate = pickDelegate;
	}
}