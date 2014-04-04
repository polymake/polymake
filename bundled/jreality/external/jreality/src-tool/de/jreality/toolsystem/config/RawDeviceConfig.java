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
import java.util.Map;
import java.util.logging.Level;

import de.jreality.toolsystem.raw.RawDevice;
import de.jreality.util.LoggingSystem;


/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class RawDeviceConfig {

  public static final PersistenceDelegate DELEGATE = new DefaultPersistenceDelegate(
      new String[]{"rawDevice", "deviceID", "config"}
  );
  
  private final String deviceID;
  private final String rawDevice;
  private final Map<String, Object> config;
  
  public RawDeviceConfig(String type, String deviceID, Map<String, Object> config) {
    this.deviceID=deviceID;
    this.rawDevice = type;
    this.config = config;
  }
  
  public String getRawDevice() {
    return rawDevice;
  }
  
  public String getDeviceID() {
    return deviceID;
  }
  
  public Map<String, Object> getConfiguration() {
	  return config;
  }
  
  public String toString() {
    return deviceID + "["+rawDevice+"]";
  }
  
  public RawDevice createDevice() throws InstantiationException {
    try {
      RawDevice dev = (RawDevice) Class.forName(rawDevice).newInstance();
      return dev;
    } catch (Throwable t) {
      LoggingSystem.getLogger(this).log(Level.CONFIG, "cannot create raw device", t);
      throw new InstantiationException("cannot create raw device:"+rawDevice);
    }
  }
}
