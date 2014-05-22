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


package de.jreality.toolsystem;

import java.io.IOException;
import java.io.Serializable;

import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;

/**
 * @author weissman
 *
 **/
public class ToolEvent implements Serializable {
    
	private static final long serialVersionUID = -574219336588808514L;

	protected double axisEps = 0;

	InputSlot device;
    
    // these must be accessable from sublasses replace-methods
    protected AxisState axis;
    protected DoubleArray trafo;
    protected long time;

    private boolean consumed;

	private transient Object source;

	/**
	 * @deprecated Use ToolEvent(Object source, long when, InputSlot device, AxisState axis)
	 */
	public ToolEvent(Object source,  InputSlot device, AxisState axis) {
	    this(source, System.currentTimeMillis(), device, axis);
	}
    public ToolEvent(Object source, long when, InputSlot device, AxisState axis) {
        this(source, when, device, axis, null);
    }
    
	/**
	 * @deprecated Use ToolEvent(Object source, long when, InputSlot device, DoubleArray trafo)
	 */
	public ToolEvent(Object source,  InputSlot device, DoubleArray trafo) {
	    this(source, System.currentTimeMillis(), device, trafo);
	}
    public ToolEvent(Object source, long when, InputSlot device, DoubleArray trafo) {
        this(source, when, device, null, trafo);
    }
    
    public ToolEvent(Object source, long when, InputSlot device, AxisState axis, DoubleArray trafo) {
    	this.source=source;
    	time=when;
    	this.device=device;
    	this.axis=axis;
    	this.trafo=trafo;
    }

    public InputSlot getInputSlot() {
    	return device;
    }
    
	public AxisState getAxisState() {
		return axis;
	}

	public DoubleArray getTransformation() {
		return trafo;
	}
		
	public long getTimeStamp() {
		return time;
	}
  
  public String toString() {
    return "ToolEvent source="+getSource()+" device="+device+" "+axis+" trafo="+trafo;
  }
  
  public Object getSource() {
	return source;
}

/**
   * sets the 
   * @param replacement
   */
  protected void replaceWith(ToolEvent replacement) {
      this.axis = replacement.axis;
      this.trafo = replacement.trafo;
      //this.time = replacement.time;
  }
/**
 * 
 *  TODO improve this!
 */
  protected boolean canReplace(ToolEvent e) {
      return (device == e.device) 
          && (getSource() == e.getSource())
          && compareTransformation(trafo, e.trafo)
          && compareAxisStates(axis, e.axis);
  }

  protected boolean compareAxisStates(AxisState axis1, AxisState axis2) {
        if (axis1 == axis2) return true;
        if (axis1 == null || axis2 == null) return axis1 == axis2;
        // sign changed
        if ( (axis1.doubleValue() * axis2.doubleValue()) <= 0 ) return false;
        // one state changed
        if (   (axis1.isPressed() && !axis2.isPressed())
            || (!axis1.isPressed() && axis2.isPressed())
            || (axis1.isReleased() && !axis2.isReleased())
            || (!axis1.isReleased() && axis2.isReleased()) )
            return false;
        return (Math.abs(axis1.doubleValue() - axis2.doubleValue()) < axisEps);
    }
    
  protected boolean compareTransformation(DoubleArray trafo1, DoubleArray trafo2) {
        if (trafo1 == trafo2) return true;
        if (trafo1 == null || trafo1 == null) return trafo1 == trafo2;
        //return Rn.equals(trafo1.toDoubleArray(null), trafo2.toDoubleArray(null), 0.00000001);
        return false;
    }

    public void consume() {
      consumed=true;
    }
    
    public boolean isConsumed() {
      return consumed;
    }
    
    private void writeObject(java.io.ObjectOutputStream out) throws IOException {
        out.writeUTF(device.getName());
        out.writeObject(axis);
        if (trafo != null) out.writeObject(trafo.toDoubleArray(null));
        else out.writeObject(null);
        out.writeLong(time);
      }
    
      private void readObject(java.io.ObjectInputStream in)
        throws IOException, ClassNotFoundException {
    	  device = InputSlot.getDevice(in.readUTF());
    	  axis = (AxisState) in.readObject();
    	  double[] m = (double[]) in.readObject();
    	  if (m!=null) trafo=new DoubleArray(m);
    	  time=in.readLong();
    	  source = "REMOTE";
      }
}
