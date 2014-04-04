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


package de.jreality.util;

import junit.framework.TestCase;
import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;


/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class AttributeEntityTest extends TestCase {

  public static void main(String[] args) {
    junit.textui.TestRunner.run(AttributeEntityTest.class);
  }
  
  public void testTexture2dSingleton() throws Exception {
    Appearance app1 = new Appearance();
    assertFalse(AttributeEntityUtility.hasAttributeEntity(Texture2D.class, "texture2d", app1));
    Texture2D tex = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "texture2d", app1, true);
    assertTrue(AttributeEntityUtility.hasAttributeEntity(Texture2D.class, "texture2d", app1));
    ImageData ai = ImageData.load(Input.getInput("textures/grid.jpeg"));
    tex.setImage(ai);
    tex.setApplyMode(Texture2D.GL_LINEAR_MIPMAP_LINEAR);
    System.out.println(tex);
    
    tex.setImage(null);
    
    System.out.println(tex);

    Texture2D t1 = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "texture2d", app1, true);
    System.out.println(t1);
    assertEquals(new Integer(Texture2D.GL_LINEAR_MIPMAP_LINEAR), t1.getApplyMode());

    
    EffectiveAppearance ea = EffectiveAppearance.create();
    assertFalse(AttributeEntityUtility.hasAttributeEntity(Texture2D.class, "texture2d", ea));
    ea = ea.create(app1);
    assertTrue(AttributeEntityUtility.hasAttributeEntity(Texture2D.class, "texture2d", ea));
    ea = ea.create(new Appearance());
    assertTrue(AttributeEntityUtility.hasAttributeEntity(Texture2D.class, "texture2d", ea));
    Texture2D t2 = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "texture2d", ea);
    System.out.println(t2);
    assertEquals(t2.getApplyMode(), new Integer(Texture2D.GL_LINEAR_MIPMAP_LINEAR));

  }
  
//  public void testDefaultShader() throws Exception {
//    System.out.println("\n");
//    Appearance a = new Appearance();
//    DefaultShaderInterface di = (DefaultShaderInterface) AttributeEntityFactory.createWriter(DefaultShaderInterface.class, "plygonShader", a);
//    //di.writeDefaults();
//    System.out.println(di);
//    System.out.println(a);
//    System.out.println("\n");
//    EffectiveAppearance ea = EffectiveAppearance.create();
//    ea = ea.create(a);
//    di = (DefaultShaderInterface) AttributeEntityFactory.createReader(DefaultShaderInterface.class, "plygonShader", ea);
//    System.out.println(di);
//    
//  }

}
