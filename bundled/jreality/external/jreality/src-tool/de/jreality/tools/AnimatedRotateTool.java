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

import de.jreality.math.MatrixBuilder;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;


/**
 * Instantly starts an animated rotation of a SceneGraphComponent after initialization of the tool system
 * when it is added to the components tools.<br>
 * The rotation angle and axis can be set using the corresponding methods.<br>
 * The animation stops with a right mouse click.
 */
public class AnimatedRotateTool  extends AbstractTool {
	
	private static InputSlot actSlot = InputSlot.getDevice("SystemTime");
	private static InputSlot deactSlot = InputSlot.getDevice("PrimarySelection");
  private static InputSlot pause = InputSlot.getDevice("RotationToggle");
	
	
	public AnimatedRotateTool() {
		addCurrentSlot(actSlot, "Need notification to perform once.");
    addCurrentSlot(pause);
    addCurrentSlot(deactSlot);
	}
	
	
	private double angle = 0.007;
	private double[] axis = new double[]{0, 1, 0};
	private AnimatorTask task = null;
  private SceneGraphComponent cmp = null;
  
  
	public void perform(ToolContext tc) {
		
    if (task == null) {  //first performance
      cmp = tc.getRootToToolComponent().getLastComponent();
      task = new AnimatorTask() {

        public boolean run(double time, double dt) {
          MatrixBuilder m = MatrixBuilder.euclidean(cmp.getTransformation());
          m.rotate(0.05*dt*angle, axis);
          m.assignTo(cmp);
          return true;
        }
      };
      removeCurrentSlot(actSlot);
      //task is scheduled in the following
    }
    
    //deactivation
		if (tc.getAxisState(deactSlot).isPressed()) {
			removeCurrentSlot(deactSlot);
      removeCurrentSlot(pause);
			AnimatorTool.getInstance(tc).deschedule(cmp);
			cmp.removeTool(this);
			return; 
		}
    
		//pause
    if (tc.getAxisState(pause).isReleased())
      AnimatorTool.getInstance(tc).schedule(cmp, task);
    if (tc.getAxisState(pause).isPressed())
      AnimatorTool.getInstance(tc).deschedule(cmp);
	}
	
	
	public double getAngle() {
		return angle;
	}
	
	public void setAngle(double angle) {
		this.angle = angle;
	}
	
	public double[] getAxis() {
		return axis;
	}
	
	public void setAxis(double[] axis) {
		this.axis = axis;
	}
	
	public void setAxis(double x, double y, double z) {
		this.axis = new double[]{x, y, z};
	}
	
	
	@Override
	public String getDescription() {
		
		return "Instantly starts an animated rotation of a SceneGraphComponent " +
				"after initialization of the tool system when it is added to the components tools. " +
				"The rotation angle and axis can be set using the corresponding methods. " +
				"The animation stops with a right mouse click.";
	}
	
}