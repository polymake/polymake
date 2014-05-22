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

import java.awt.Color;
import java.util.NoSuchElementException;

import com.thoughtworks.xstream.converters.ConversionException;
import com.thoughtworks.xstream.converters.UnmarshallingContext;
import com.thoughtworks.xstream.io.HierarchicalStreamReader;
import com.thoughtworks.xstream.mapper.Mapper;

import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.tool.Tool;

/**
 * @author Steffen Weissmann
 */
class NodeReader extends SceneGraphVisitor {

  private HierarchicalStreamReader reader;
  private UnmarshallingContext context;
  private Mapper mapper;

  public void setUp(HierarchicalStreamReader reader, UnmarshallingContext context, Mapper mapper) {
    this.reader = reader;
    this.context = context;
    this.mapper = mapper;
  }

  public void visit(de.jreality.scene.Appearance a) {
    copyAttr(a);
  }

  public void visit(de.jreality.scene.Camera c) {
    copyAttr(c);
  }

  public void visit(de.jreality.scene.Cylinder c) {
    copyAttr(c);
  }

  public void visit(de.jreality.scene.DirectionalLight l) {
    copyAttr(l);
  }

  public void visit(de.jreality.scene.IndexedFaceSet i) {
    copyAttr(i);
  }

  public void visit(de.jreality.scene.IndexedLineSet ils) {
    copyAttr(ils);
  }

  public void visit(de.jreality.scene.PointSet p) {
    copyAttr(p);
  }

  public void visit(de.jreality.scene.SceneGraphComponent c) {
    copyAttr(c);
  }

  public void visit(de.jreality.scene.Sphere s) {
    copyAttr(s);
  }

  public void visit(de.jreality.scene.SpotLight l) {
    copyAttr(l);
  }

  public void visit(de.jreality.scene.ClippingPlane c) {
    copyAttr(c);
  }

  public void visit(de.jreality.scene.PointLight l) {
    copyAttr(l);
  }

  public void visit(de.jreality.scene.Transformation t) {
    copyAttr(t);
  }

  public void visit(de.jreality.scene.SceneGraphNode m) {
    throw new IllegalStateException(m.getClass() + " not handled by "
        + getClass().getName());
  }

  public void copyAttr(SceneGraphNode src) {
    String name = (String) read(String.class);
    src.setName(name);
  }

  public void copyAttr(SceneGraphComponent src) {
    copyAttr((SceneGraphNode) src);
    src.setVisible(readBool());
    src.setTransformation((Transformation) read(Transformation.class));
    src.setAppearance((Appearance) read(Appearance.class));
    src.setCamera((Camera)read(Camera.class));
    src.setLight((Light)read(Light.class));
    src.setGeometry((Geometry) read(Geometry.class));
    reader.moveDown();
    while (reader.hasMoreChildren()) {
      src.addChild((SceneGraphComponent) read(SceneGraphComponent.class));
    }
    reader.moveUp();
    reader.moveDown();
    while (reader.hasMoreChildren()) {
      Tool tool = (Tool) readUnknown();
      if (tool != null) src.addTool(tool);
    }
    reader.moveUp();
  }

  public void copyAttr(Appearance src) {
    copyAttr((SceneGraphNode) src);
    while (reader.hasMoreChildren()) {
      reader.moveDown();
      String aName = reader.getAttribute("name");
      Object val = readUnknown();
      src.setAttribute(aName, val);
      reader.moveUp();
    }
  }

  public void copyAttr(Transformation src) {
    copyAttr((SceneGraphNode) src);
    src.setMatrix((double[]) read(double[].class));
  }

  public void copyAttr(Light src) {
    copyAttr((SceneGraphNode) src);
    src.setColor(readColor());
    src.setIntensity(readDouble());
  }

  public void copyAttr(PointLight src) {
    copyAttr((Light) src);
    src.setFalloffA0(readDouble());
    src.setFalloffA1(readDouble());
    src.setFalloffA2(readDouble());
    src.setUseShadowMap(readBool());
    src.setShadowMapX(readInt());
    src.setShadowMapY(readInt());
    src.setShadowMap((String) read(String.class));
  }

  public void copyAttr(SpotLight src) {
    copyAttr((PointLight) src);
    src.setConeAngle(readDouble());
    src.setConeDeltaAngle(readDouble());
    src.setDistribution(readDouble());
  }

  public void copyAttr(Geometry src) {
    copyAttr((SceneGraphNode) src);
    reader.moveDown();
    while(reader.hasMoreChildren()) {
      reader.moveDown();
      String name = reader.getAttribute("name");
      Object val = readUnknown();
      reader.moveUp();
      src.setGeometryAttributes(name, val);
    }
    reader.moveUp();
  }

  public void copyAttr(PointSet src) {
    copyAttr((Geometry) src);
    src.setVertexCountAndAttributes((DataListSet) read(DataListSet.class));
  }

  public void copyAttr(IndexedLineSet src) {
    copyAttr((PointSet) src);
    src.setEdgeCountAndAttributes((DataListSet) read(DataListSet.class));
  }

  public void copyAttr(IndexedFaceSet src) {
    copyAttr((IndexedLineSet) src);
    src.setFaceCountAndAttributes((DataListSet) read(DataListSet.class));
  }

  public void copyAttr(Camera src) {
    copyAttr((SceneGraphNode) src);
    src.setEyeSeparation(readDouble());
    src.setFar(readDouble());
    src.setFieldOfView(readDouble());
    src.setFocus(readDouble());
    src.setNear(readDouble());
    src.setOnAxis(readBool());
    src.setOrientationMatrix((double[]) read(double[].class));
    src.setPerspective(readBool());
    src.setStereo(readBool());
  }

  private double readDouble() {
    return ((Double)read(Double.class)).doubleValue();
  }

  private int readInt() {
    return ((Integer)read(Integer.class)).intValue();
  }

  private boolean readBool() {
    return ((Boolean)read(Boolean.class)).booleanValue();
  }

  private Color readColor() {
    return (Color) read(Color.class);
  }

  private Object read(Class type) {
    Object ret = null;
    reader.moveDown();
    try {
      ret = context.convertAnother(null, type);
      reader.moveUp();
    } catch (ConversionException e) {
      if (e.getCause() instanceof NoSuchElementException) {
        reader.moveUp();
        return null;
      }
      else throw e;
    }
    return ret;
  }
  
  private Object readUnknown() {
    return XStreamFactory.readUnknown(reader, context, mapper);
  }

}
