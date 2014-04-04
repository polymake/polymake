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

import java.io.ObjectStreamException;
import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

/**
 * Abstract input device, addressed via a logical name.
 */
public class InputSlot implements Serializable
{
    private static final Map<String, InputSlot> name2device = new HashMap<String,InputSlot>();
    
	public static final InputSlot POINTER_HIT = InputSlot.getDevice("PointerHit");
	public static final InputSlot POINTER_TRANSFORMATION = InputSlot.getDevice("PointerTransformation");
	public static final InputSlot SYSTEM_TIME = InputSlot.getDevice("SystemTime");
    // built-in button slots: only first three are guaranteed to exist
	public static final InputSlot LEFT_BUTTON = InputSlot.getDevice("PrimaryAction");
	public static final InputSlot MIDDLE_BUTTON = InputSlot.getDevice("PrimaryMenu");
	public static final InputSlot RIGHT_BUTTON = InputSlot.getDevice("PrimarySelection");
    // WARNING: following slots are NOT guaranteed to exist: for sure on desktop w/ shift modifier
	public static final InputSlot SHIFT_LEFT_BUTTON = InputSlot.getDevice("SecondaryAction");
	public static final InputSlot SHIFT_MIDDLE_BUTTON = InputSlot.getDevice("SecondaryMenu");
	public static final InputSlot SHIFT_RIGHT_BUTTON = InputSlot.getDevice("SecondarySelection");

	/**
	 * This inputslot can be used for customized tools. The standard key is CONTROL+mouseclick. 
	 * If you want to use ALT+mouseclick instead just uncomment the mapping AltMeta and 
	 * comment the Meta-mapping at the file toolconfig-mouse-keybord.xml 
	 * at de.jreality.toolsystem.config.chunks. 
	 * Be aware, that the most operating systems are using ALT+mouseclick for moving windows.
	 * In that case, the usage of the Operating system will overwrite your Tool.   
	 */
	public static final InputSlot META_LEFT_BUTTON = InputSlot.getDevice("TertiaryAction");
	public static final InputSlot META_MIDDLE_BUTTON = InputSlot.getDevice("TertiaryMenu");
	public static final InputSlot META_RIGHT_BUTTON = InputSlot.getDevice("TertiarySelection");
	
	private final int hash;
	private final String name;
    private InputSlot(String name)
    {
        this.name=name;
        hash = name.hashCode();
    }
    /**
     * Get the canonical device for the logical name. Devices with the
     * same name are meant to represent the same device and yield the
     * same instance.
     */
    public static InputSlot getDevice(String name)
    {
      synchronized (name2device) {
        Object old=name2device.get(name);
        if(old!=null) return (InputSlot)old;
        InputSlot dev=new InputSlot(name);
        name2device.put(name, dev);
        return dev;
      }
    }
    public String getName() {
      return name;
    }
    //TODO: something better here?
    public String toString()
    {
        return name;
    }
    
    Object readResolve() throws ObjectStreamException {
      return getDevice(getName());
    }

    @Override
    public int hashCode() {
    	return hash;
    }
    	
	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		InputSlot other = (InputSlot) obj;
		if (name == null) {
			if (other.name != null)
				return false;
		} else if (!name.equals(other.name))
			return false;
		return true;
	}
    
    
}
