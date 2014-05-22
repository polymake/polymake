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
import de.jreality.math.FactoredMatrix;
import de.jreality.math.Rn;


/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class FactoredMatrixTest extends TestCase {

  public static void main(String[] args) {
    junit.textui.TestRunner.run(FactoredMatrixTest.class);
  }

  public void testConstructor() {
    OldTransformation t = new OldTransformation();
    FactoredMatrix fm = new FactoredMatrix();
    compareTrafos(t.getMatrix(), fm.getArray());
  }
  
  public void testTranslation() {
    OldTransformation t = new OldTransformation();
    FactoredMatrix fm = new FactoredMatrix();
    t.setTranslation(2,3,4);
    fm.setTranslation(2,3,4);
    compareTrafos(t.getMatrix(), fm.getArray());
  }
  
  public void testRotation() {
    OldTransformation t = new OldTransformation();
    FactoredMatrix fm = new FactoredMatrix();
    double rot = Math.random() * Math.PI * 2.;
    t.setRotation(rot, 1,1,1);
    fm.setRotation(rot, 1,1,1);
    compareTrafos(t.getMatrix(), fm.getArray());
  }
  
  public void testSequence() {
    OldTransformation t = new OldTransformation();
    FactoredMatrix fm = new FactoredMatrix();
    double rot = Math.random() * Math.PI * 2.;

    t.setRotation(rot, 1,1,1);
    t.setStretch(3);
    t.setStretch(1,1,4);
    t.setTranslation(2,8,8);

    fm.setRotation(rot, 1,1,1);
    fm.setStretch(3);
    fm.setStretch(1,1,4);
    fm.setTranslation(2,8,8);
    
    compareTrafos(t.getMatrix(), fm.getArray());
  }
  
  public static void compareTrafos(double[] d1, double[] d2) {
    System.out.println("d1:\n"+Rn.matrixToString(d1));
    System.out.println("d2:\n"+Rn.matrixToString(d2));
    for (int i = 0; i < 16; i++)
      assertEquals(d1[i], d2[i], 1e-11);
  }

}
