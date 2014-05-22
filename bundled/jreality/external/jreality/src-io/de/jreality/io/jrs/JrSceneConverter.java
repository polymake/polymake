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


package de.jreality.io.jrs;

import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

import com.thoughtworks.xstream.converters.Converter;
import com.thoughtworks.xstream.converters.MarshallingContext;
import com.thoughtworks.xstream.converters.UnmarshallingContext;
import com.thoughtworks.xstream.io.HierarchicalStreamReader;
import com.thoughtworks.xstream.io.HierarchicalStreamWriter;
import com.thoughtworks.xstream.mapper.Mapper;

import de.jreality.io.JrScene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.util.LoggingSystem;

class JrSceneConverter implements Converter {

  Mapper mapper;
  
  public JrSceneConverter(Mapper mapper) {
    this.mapper = mapper;
  }

  public boolean canConvert(Class type) {
    return type == JrScene.class;
  }

  public void marshal(Object source, HierarchicalStreamWriter writer, MarshallingContext context) {
    JrScene scene = (JrScene) source;
    writer.startNode("sceneRoot");
    context.convertAnother(scene.getSceneRoot());
    writer.endNode();
    writer.startNode("scenePaths");
    for (Iterator it = scene.getScenePaths().entrySet().iterator(); it.hasNext(); ) {
      Map.Entry e = (Entry) it.next();
      writer.startNode("path");
      writer.addAttribute("name", (String) e.getKey());
      context.convertAnother(e.getValue());
      writer.endNode();
    }
    writer.endNode();
    writer.startNode("sceneAttributes");
    for (Iterator it = scene.getSceneAttributes().entrySet().iterator(); it.hasNext(); ) {
      Map.Entry e = (Entry) it.next();
      if (XStreamFactory.canWrite(e.getValue())) {
        writer.startNode("attribute");
        writer.addAttribute("name", (String) e.getKey());
        XStreamFactory.writeUnknown(e.getValue(), writer, context, mapper);
        writer.endNode();
      } else {
        LoggingSystem.getLogger(this).warning("cannot write scene attribute="+e.getKey()+" ["+e.getValue().getClass()+"] not supported.");
      }
    }
    writer.endNode();
  }

  public Object unmarshal(HierarchicalStreamReader reader, UnmarshallingContext context) {
    JrScene ret = new JrScene();
    
    reader.moveDown();
    SceneGraphComponent root = (SceneGraphComponent) context.convertAnother(null, SceneGraphComponent.class);
    reader.moveUp();
    
    ret.setSceneRoot(root);
    
    // paths
    reader.moveDown();
    while(reader.hasMoreChildren()) {
      reader.moveDown();
      String pathName = reader.getAttribute("name");
      SceneGraphPath path = (SceneGraphPath) context.convertAnother(null, SceneGraphPath.class);
      reader.moveUp();

      ret.addPath(pathName, path);
    }
    reader.moveUp();

    // attributes
    reader.moveDown();
    while(reader.hasMoreChildren()) {
      reader.moveDown();
      String attrName = reader.getAttribute("name");
      Object obj = XStreamFactory.readUnknown(reader, context, mapper);
      ret.addAttribute(attrName, obj);
    }
    reader.moveUp();
    
    return ret;
  }

}
