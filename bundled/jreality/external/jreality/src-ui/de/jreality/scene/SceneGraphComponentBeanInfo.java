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


package de.jreality.scene;

import java.beans.BeanDescriptor;
import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.beans.SimpleBeanInfo;


public class SceneGraphComponentBeanInfo extends SimpleBeanInfo {

  private PropertyDescriptor[] pd;
  private BeanDescriptor bd;

  
  public SceneGraphComponentBeanInfo() throws IntrospectionException {
    bd = new BeanDescriptor(SceneGraphComponent.class);
    bd.setShortDescription("Basic building block of the jReality scene graph. "
        +"Can have another SceneGraphComponent instance as a child and "
        +"can contain other instances of SceneGraphNode.");
    
    PropertyDescriptor pdCCC = new PropertyDescriptor("children", bd.getBeanClass(), "getChildComponentCount", null);
    pdCCC.setDisplayName("#Children");
    pdCCC.setShortDescription("Number of child components");
    
    pd = new PropertyDescriptor[]{
        pdCCC,
        pd("visible", "Visible", "Flag to tell whether the node is visible"),
        pd("pickable", "Pickible", "Flag to tell whether the node is pickable"),
   };
  }
  
  
  @Override
  public BeanInfo[] getAdditionalBeanInfo() {
    try {
      return new BeanInfo[]{Introspector.getBeanInfo(SceneGraphNode.class)};
    } catch (Exception e) {
      return null;
    }
  }

  
  private PropertyDescriptor pd(String name, String... d) throws IntrospectionException {
    PropertyDescriptor p = new PropertyDescriptor(name, bd.getBeanClass());
    switch(d.length)
    {
      default: throw new IllegalArgumentException("too many args");
      case 2: p.setShortDescription(d[1]);
      case 1: p.setDisplayName(d[0]);
      case 0:
    }
    return p;
  }
  
  @Override
  public PropertyDescriptor[] getPropertyDescriptors() {
    return pd;
  }
  
  @Override
  public BeanDescriptor getBeanDescriptor() {
    return bd;
  }

}