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


package de.jreality.shader;

import java.awt.Color;
import java.beans.BeanInfo;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Method;

import junit.framework.TestCase;
import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;

public class SubEntityTest extends TestCase {

  public void testDefaultEntities() {
    
    Appearance a = new Appearance();
    EffectiveAppearance ea = EffectiveAppearance.create().create(a);

    assertFalse(AttributeEntityUtility.hasAttributeEntity(DefaultGeometryShader.class, "", a));
    assertTrue(AttributeEntityUtility.hasAttributeEntity(DefaultGeometryShader.class, "", ea));

    assertFalse(AttributeEntityUtility.hasAttributeEntity(PointShader.class, "", a));
    assertTrue(AttributeEntityUtility.hasAttributeEntity(PointShader.class, "", ea));
    assertFalse(AttributeEntityUtility.hasAttributeEntity(DefaultPointShader.class, "", a));
    assertTrue(AttributeEntityUtility.hasAttributeEntity(DefaultPointShader.class, "", ea));

    assertFalse(AttributeEntityUtility.hasAttributeEntity(LineShader.class, "", a));
    assertTrue(AttributeEntityUtility.hasAttributeEntity(LineShader.class, "", ea));
    assertFalse(AttributeEntityUtility.hasAttributeEntity(DefaultLineShader.class, "", a));
    assertTrue(AttributeEntityUtility.hasAttributeEntity(DefaultLineShader.class, "", ea));

    assertFalse(AttributeEntityUtility.hasAttributeEntity(PolygonShader.class, "", a));
    assertTrue(AttributeEntityUtility.hasAttributeEntity(PolygonShader.class, "", ea));
    assertFalse(AttributeEntityUtility.hasAttributeEntity(DefaultPolygonShader.class, "", a));
    assertTrue(AttributeEntityUtility.hasAttributeEntity(DefaultPolygonShader.class, "", ea));
    
    assertFalse(AttributeEntityUtility.hasAttributeEntity(Texture2D.class, "", a));
    assertFalse(AttributeEntityUtility.hasAttributeEntity(Texture2D.class, "", ea));

    assertFalse(AttributeEntityUtility.hasAttributeEntity(CubeMap.class, "", a));
    assertFalse(AttributeEntityUtility.hasAttributeEntity(CubeMap.class, "", ea));

    assertFalse(AttributeEntityUtility.hasAttributeEntity(TwoSidePolygonShader.class, "", a));
    assertFalse(AttributeEntityUtility.hasAttributeEntity(TwoSidePolygonShader.class, "", ea));
  
  }

  public void testDefaultGeometryShader() {
    Appearance a = new Appearance();
    Appearance a2 = new Appearance();
    
    DefaultGeometryShader gs = ShaderUtility.createDefaultGeometryShader(a, false);
    DefaultGeometryShader gs2 = ShaderUtility.createDefaultGeometryShader(a2, true);
    
    gs.setShowLines(Boolean.FALSE);
    gs.setShowFaces(Boolean.TRUE);
    
    DefaultPolygonShader ps = (DefaultPolygonShader) gs.getPolygonShader();
    
    ps.setAmbientCoefficient(new Double(1.2));
    ps.setSmoothShading(Boolean.FALSE);

    DefaultPointShader dps = (DefaultPointShader)gs.getPointShader();
    
    TwoSidePolygonShader tps = (TwoSidePolygonShader) dps.createPolygonShader("twoSide");
    
    DefaultPolygonShader back = (DefaultPolygonShader) tps.getBack();
    DefaultPolygonShader front = (DefaultPolygonShader) tps.getFront();
    
    back.setDiffuseColor(Color.BLACK);
    front.setDiffuseColor(Color.WHITE);
    
    DefaultPolygonShader dpols = (DefaultPolygonShader) gs.getPolygonShader();
    
    EffectiveAppearance ea = EffectiveAppearance.create().create(a);
    DefaultPolygonShader dpols2 = (DefaultPolygonShader) gs2.getPolygonShader();
    
    Color defCol = dpols2.getDiffuseColor();
    assertFalse(defCol == null);
    dpols2.setDiffuseColor(Color.black);
    defCol = dpols2.getDiffuseColor();
    assertFalse(defCol == null);
    dpols2.setDiffuseColor(null);
    assertFalse(defCol == null);
    defCol = dpols2.getDiffuseColor();
    
    System.out.println(a);
    dpols.setDiffuseColor(null);
    Object o = dpols.getDiffuseColor();
    System.out.println(o);
    assertTrue(o == null);
    
    System.out.println(a);
    
  }
  
  public void testDefaultGeometryShaderRead() {
    EffectiveAppearance ea = EffectiveAppearance.create();
    
    DefaultGeometryShader gs = (DefaultGeometryShader) AttributeEntityUtility.createAttributeEntity(DefaultGeometryShader.class, "", ea);
    
    DefaultPolygonShader ps = (DefaultPolygonShader) gs.getPolygonShader();
    DefaultLineShader ls = (DefaultLineShader) gs.getLineShader();
    DefaultPointShader pos = (DefaultPointShader) gs.getPointShader();
    
    System.out.println("\n***********");
    
    System.out.println(ps.getTexture2d());

    System.out.println("\n***********");

    
    System.out.println(gs);
  }
  
  public void testNonDefaultShaders() throws Exception {
	    System.out.println("\n***********");
	    System.out.println("\n***********");
	  
	  Appearance app = new Appearance();
	  Appearance app2 = new Appearance();
	  
	  DefaultPolygonShader def = (DefaultPolygonShader) ShaderUtility.createDefaultGeometryShader(app, true).getPolygonShader();
	  
	  EffectiveAppearance eapp = EffectiveAppearance.create().create(app);
	  
	  DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(app, true);
	  HatchPolygonShader hatch = (HatchPolygonShader) dgs.createPolygonShader("hatch");
	  
	  DefaultGeometryShader dgsEap = ShaderUtility.createDefaultGeometryShader(eapp);
	  PolygonShader ps = dgsEap.getPolygonShader();
	  assertTrue(hatch instanceof HatchPolygonShader);
	  System.out.println(ps);
	  assertTrue(ps instanceof HatchPolygonShader);

	  	System.out.println("\n***********");
	    System.out.println("\n***********");
	  
      BeanInfo bi = Introspector.getBeanInfo(DefaultPolygonShader.class, Introspector.IGNORE_ALL_BEANINFO);
	  for (PropertyDescriptor descriptor : bi.getPropertyDescriptors()) {
		  
		  System.out.println(descriptor.getName()+":");
		  // check defaults
		  Method r = descriptor.getReadMethod();
		  Object o1 = r.invoke(def, null);
		  Object o2 = r.invoke(hatch, null);
		  System.out.println("\t read: "+equals(o1, o2));
		  
		  // check writing
		  Method w = descriptor.getWriteMethod();
		  if (w==null) continue;
		  w.invoke(def, o1);
		  w.invoke(hatch, o2);
		  System.out.println("\t write: OK");
	  }
  }

	private boolean equals(Object o1, Object o2) {
		if (o1 == null) return o2 == null;
		if (o2 == null) return false;
		return o1.equals(o2);
	}
}
