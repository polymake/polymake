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

import com.thoughtworks.xstream.converters.Converter;
import com.thoughtworks.xstream.converters.MarshallingContext;
import com.thoughtworks.xstream.converters.UnmarshallingContext;
import com.thoughtworks.xstream.io.HierarchicalStreamReader;
import com.thoughtworks.xstream.io.HierarchicalStreamWriter;
import com.thoughtworks.xstream.mapper.Mapper;

import de.jreality.scene.Scene;
import de.jreality.scene.SceneGraphNode;

class SceneGraphNodeConverter implements Converter {

  Mapper mapper;
  NodeWriter nodeWriter = new NodeWriter();
  NodeReader nodeReader = new NodeReader();
  
  public SceneGraphNodeConverter(Mapper mapper) {
    this.mapper = mapper;
  }

  public boolean canConvert(Class type) {
    return SceneGraphNode.class.isAssignableFrom(type);
  }

  public void marshal(Object source, HierarchicalStreamWriter writer, MarshallingContext context) {
    SceneGraphNode n = (SceneGraphNode) source;
    nodeWriter.setUp(writer, context, mapper);
    n.accept(nodeWriter);
  }

  public Object unmarshal(HierarchicalStreamReader reader, UnmarshallingContext context) {
    SceneGraphNode node = null;
    Class type = reader.getAttribute("type") == null ? context.getRequiredType() : mapper.realClass(reader.getAttribute("type"));
    if (!reader.hasMoreChildren()) return null;
    try {
      node = (SceneGraphNode) type.newInstance();
    } catch (Exception e) {
      return null;
    }
    nodeReader.setUp(reader, context, mapper);
    final SceneGraphNode fn = node;
    Scene.executeWriter(node, new Runnable() {
      public void run() {
        fn.accept(nodeReader);
      }
    });
    return node;
  }
  
}
