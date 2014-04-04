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


package de.jreality.toolsystem.config;

import java.beans.DefaultPersistenceDelegate;
import java.beans.PersistenceDelegate;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;

import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.VirtualDevice;
import de.jreality.util.LoggingSystem;


/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class VirtualDeviceConfig {
  
  static final PersistenceDelegate DELEGATE = new DefaultPersistenceDelegate(
      new String[]{"virtualDevice", "outSlot", "inSlots", "config"});

  private String virtualDevice;
  
  public Map getConfig() {
    return config;
  }
  public List getInSlots() {
    return inSlots;
  }
  public InputSlot getOutSlot() {
    return outSlot;
  }
  public String getVirtualDevice() {
    return virtualDevice;
  }
  
  private InputSlot outSlot;
  private List inSlots;
  private Map config;

  public VirtualDeviceConfig(String virtualDevice, InputSlot outSlot, List inSlots, Map config, String mapped) {
    this.virtualDevice = virtualDevice;
    this.outSlot = outSlot;
    this.inSlots = inSlots;
    this.config = config;
  }

  public VirtualDevice createDevice() throws InstantiationException {
    try {
      VirtualDevice ret = (VirtualDevice) Class.forName(virtualDevice).newInstance();
      ret.initialize(inSlots, outSlot, config);
      return ret;
    } catch (Throwable t){
      LoggingSystem.getLogger(this).log(Level.CONFIG, "cannot create virtual device", t);
      throw new InstantiationException("cannot create raw device:"+virtualDevice);
    }
  }
  
  public String toString() {
    return "VirtualDeviceConfig: "+(virtualDevice != null ? virtualDevice : "null") +" outSlot="+outSlot+ " inslots="+inSlots.toString()+" config="+config.toString();
  }
}
