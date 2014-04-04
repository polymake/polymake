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


package de.jreality.reader;

import java.io.IOException;

import com.thoughtworks.xstream.XStream;

import de.jreality.io.JrScene;
import de.jreality.io.jrs.XStreamFactory;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.util.Input;
import de.jreality.util.SceneGraphUtility;

/**
 * A reader for the jReality proprietary XML format JRS.
 *
 */
public class ReaderJRS extends AbstractReader {

  JrScene read;
  
  public void setInput(Input input) throws IOException {
    super.setInput(input);
    Object stuff;
    XStream xstr = XStreamFactory.forVersion(0.2);
    try {
    	stuff = xstr.fromXML(input.getReader());
    } catch (Exception e) {
        xstr = XStreamFactory.forVersion(0.1);
        try {
            System.out.println("trying to read JRS v0.1");
        	stuff = xstr.fromXML(input.copy().getReader());
        	System.out.println("JRS v0.1 success.");
        } catch (Exception e2) {
        	throw new IOException("illegal JRS file");
        }
    }
    if (stuff instanceof JrScene) {
      read = (JrScene) stuff; 
      root = read.getSceneRoot();
    } else {
    	if (stuff instanceof SceneGraphComponent)
    		root = (SceneGraphComponent) stuff;
    	else {
    		root = new SceneGraphComponent();
    		root.setName("jrs");
    		SceneGraphUtility.addChildNode(root, (SceneGraphNode) stuff);
    	}
    }
  }
  
  public JrScene getScene() {
    return read;
  }
}
