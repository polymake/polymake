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


package de.jreality.scene.tool;

import java.io.Serializable;

/**
 * Represents a button or a double value for tools.
 */
public final class AxisState implements Serializable
{
    public static final AxisState PRESSED = new AxisState(Integer.MAX_VALUE);
    public static final AxisState ORIGIN  = new AxisState(0);
    private int state;
    
    private static final int MINUS_PRESSED = -Integer.MAX_VALUE;
    
    /**
     * double must be in the range [-1,1]
     * @param value
     */
    public AxisState(double value)
    {
    	if (value < -1 || value > 1) {
//    		throw new IllegalArgumentException("illegal axis state value");
    		value = value < 0?-1:1;
    	}
        state=(int)(value*Integer.MAX_VALUE);
    }
    public AxisState(int value)
    {
        state=value;
    }
    public int intValue()
    {
        return state;
    }
    public double doubleValue()
    {
        return state/(double)Integer.MAX_VALUE;
    }
    /**
     * returns true if the double value is 1 or -1.
     * @return
     */
    public boolean isPressed() {
    	return state==Integer.MAX_VALUE || state == MINUS_PRESSED;
    }
    public boolean isReleased() {
    	return state==0;
    }
    public String toString() {
      switch (state) {
        case 0:
          return "AxisState=ORIGIN";
        case Integer.MAX_VALUE:
          return "AxisState=PRESSED";
        case MINUS_PRESSED:
          return "AxisState=MINUS_PRESSED";
        default:
          return "AxisState="+state+" ["+((int)(doubleValue()*100))/100.+"]";
      }
    }
}
